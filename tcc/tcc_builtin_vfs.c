/*
 * Optional builtin resource VFS for TCC.
 *
 * Policy:
 *   1. Try the dynamic per-compile memory VFS first.
 *   2. Try the external filesystem.
 *   3. If a read-only open still fails, try the embedded resource table.
 *
 * The embedded table uses virtual names such as "include/stdio.h" and
 * "lib/libtcc1.a". Runtime lookup only accepts those normalized virtual names.
 */
#include "tcc_builtin_vfs.h"

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdatomic.h>

#ifdef _WIN32
# include <io.h>
# include <share.h>
# include <sys/stat.h>
# include "tcc_utf8_io.h"
#else
# include <unistd.h>
# include <sys/stat.h>
#endif

#include "LzmaDec.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif
#ifndef O_ACCMODE
# define O_ACCMODE 3
#endif

#define TCC_VFS_FD_BASE 0x40000000
#define TCC_VFS_MAX_OPEN 64
#define TCC_VFS_KEY_MAX 4096

typedef struct TCCVfsOpenFile {
    const unsigned char *data;
    unsigned char *owned_data;
    size_t size;
    size_t pos;
    int used;
} TCCVfsOpenFile;

typedef struct TCCVfsDynamicResource {
    char *name;
    unsigned char *data;
    size_t size;
} TCCVfsDynamicResource;

static TCCVfsOpenFile tcc_vfs_open_files[TCC_VFS_MAX_OPEN];
static TCCVfsDynamicResource *tcc_vfs_dynamic_resources;
static unsigned int tcc_vfs_dynamic_count;
static unsigned int tcc_vfs_dynamic_capacity;
static atomic_flag tcc_vfs_lock_flag = ATOMIC_FLAG_INIT;

static void tcc_vfs_lock(void)
{
    while (atomic_flag_test_and_set_explicit(&tcc_vfs_lock_flag, memory_order_acquire)) {
        /* 临界区只覆盖表操作与短拷贝，不调用外部回调。 */
    }
}

static void tcc_vfs_unlock(void)
{
    atomic_flag_clear_explicit(&tcc_vfs_lock_flag, memory_order_release);
}

static int tcc_vfs_is_virtual_slot(int fd)
{
    return fd >= TCC_VFS_FD_BASE
        && fd < TCC_VFS_FD_BASE + TCC_VFS_MAX_OPEN;
}

static int tcc_vfs_normalize_key(const char *path, char *out, size_t out_size)
{
    size_t i;
    size_t j;
    size_t pos;
    char segment[TCC_VFS_KEY_MAX];

    if (!out || !out_size) {
        errno = EINVAL;
        return 0;
    }
    if (!path)
        path = "";
    out[0] = '\0';
    pos = 0;
    i = 0;
    while (path[i]) {
        while (path[i] == '/' || path[i] == '\\')
            ++i;

        j = 0;
        while (path[i] && path[i] != '/' && path[i] != '\\') {
            unsigned char ch = (unsigned char)path[i++];
            if (ch >= 'A' && ch <= 'Z')
                ch = (unsigned char)(ch - 'A' + 'a');
            if (j + 1 >= sizeof segment)
                goto too_long;
            segment[j++] = (char)ch;
        }
        segment[j] = '\0';

        if (j == 0 || strcmp(segment, ".") == 0)
            continue;

        if (strcmp(segment, "..") == 0) {
            size_t last = pos;

            while (last > 0 && out[last - 1] != '/')
                --last;
            if (pos > 0 && strcmp(out + last, "..") != 0) {
                pos = last > 0 ? last - 1u : 0;
                out[pos] = '\0';
            } else {
                if (pos > 0 && pos + 1 >= out_size)
                    goto too_long;
                if (pos > 0)
                    out[pos++] = '/';
                if (pos + 2 >= out_size)
                    goto too_long;
                out[pos++] = '.';
                out[pos++] = '.';
                out[pos] = '\0';
            }
            continue;
        }

        if (pos > 0) {
            if (pos + 1 >= out_size)
                goto too_long;
            out[pos++] = '/';
        }
        for (j = 0; segment[j]; ++j) {
            if (pos + 1 >= out_size)
                goto too_long;
            out[pos++] = segment[j];
        }
        out[pos] = '\0';
    }
    return 1;

too_long:
    out[0] = '\0';
    errno = ENAMETOOLONG;
    return 0;
}

static const TCCBuiltinResource *tcc_vfs_lookup_exact(const char *key)
{
    unsigned int i;

    if (!key || key[0] == '\0')
        return NULL;

    for (i = 0; i < tcc_builtin_resources_count; ++i) {
        const TCCBuiltinResource *res = &tcc_builtin_resources[i];
        if (res->name && strcmp(res->name, key) == 0)
            return res;
    }
    return NULL;
}

static const TCCBuiltinResource *tcc_vfs_lookup_resource(const char *path)
{
    char norm[TCC_VFS_KEY_MAX];

    if (!tcc_vfs_normalize_key(path, norm, sizeof norm))
        return NULL;
    return tcc_vfs_lookup_exact(norm);
}

static char *tcc_vfs_copy_key(const char *key)
{
    size_t len;
    char *copy;

    if (!key)
        return NULL;
    len = strlen(key);
    copy = (char *)malloc(len + 1);
    if (!copy)
        return NULL;
    memcpy(copy, key, len + 1);
    return copy;
}

static int tcc_vfs_find_dynamic_key(const char *key)
{
    unsigned int i;

    if (!key || key[0] == '\0')
        return -1;
    for (i = 0; i < tcc_vfs_dynamic_count; ++i) {
        if (tcc_vfs_dynamic_resources[i].name &&
            strcmp(tcc_vfs_dynamic_resources[i].name, key) == 0)
            return (int)i;
    }
    return -1;
}

/* 动态资源只允许复制快照离开锁，调用方绝不借用可被替换/清空的表指针。 */
static int tcc_vfs_copy_dynamic(const char *path, unsigned char **out_data, size_t *out_size)
{
    char norm[TCC_VFS_KEY_MAX];
    int index;
    unsigned char *copy = NULL;

    if (!out_data || !out_size) {
        errno = EINVAL;
        return -1;
    }
    *out_data = NULL;
    *out_size = 0;

    if (!tcc_vfs_normalize_key(path, norm, sizeof norm))
        return -1;
    tcc_vfs_lock();
    index = tcc_vfs_find_dynamic_key(norm);
    if (index >= 0 && tcc_vfs_dynamic_resources[index].size > 0) {
        copy = (unsigned char *)malloc(tcc_vfs_dynamic_resources[index].size);
        if (!copy) {
            tcc_vfs_unlock();
            errno = ENOMEM;
            return -1;
        }
        memcpy(copy, tcc_vfs_dynamic_resources[index].data,
               tcc_vfs_dynamic_resources[index].size);
    }
    if (index >= 0) {
        *out_data = copy;
        *out_size = tcc_vfs_dynamic_resources[index].size;
    }
    tcc_vfs_unlock();
    return index >= 0 ? 1 : 0;
}

static void tcc_vfs_free_dynamic_resource(TCCVfsDynamicResource *res)
{
    if (!res)
        return;
    free(res->name);
    free(res->data);
    memset(res, 0, sizeof *res);
}

TCC_VFS_API void tcc_vfs_clear_dynamic(void)
{
    unsigned int i;
    TCCVfsDynamicResource *resources;
    unsigned int count;

	tcc_vfs_lock();
	resources = tcc_vfs_dynamic_resources;
	count = tcc_vfs_dynamic_count;
	tcc_vfs_dynamic_resources = NULL;
	tcc_vfs_dynamic_count = 0;
	tcc_vfs_dynamic_capacity = 0;
	tcc_vfs_unlock();
	for (i = 0; i < count; ++i)
		tcc_vfs_free_dynamic_resource(&resources[i]);
	free(resources);
}

TCC_VFS_API int tcc_vfs_mount_memory(const char *path, const void *data, size_t size)
{
    char norm[TCC_VFS_KEY_MAX];
    char *name_copy;
    unsigned char *data_copy;
    TCCVfsDynamicResource *new_items;
    unsigned int new_capacity;
    size_t allocation_count;
    int index;

    if (!tcc_vfs_normalize_key(path, norm, sizeof norm))
        return 0;
    if (norm[0] == '\0' || (size > 0 && !data)) {
        errno = EINVAL;
        return 0;
    }

    name_copy = tcc_vfs_copy_key(norm);
    if (!name_copy) {
        errno = ENOMEM;
        return 0;
    }

    data_copy = NULL;
    if (size > 0) {
        data_copy = (unsigned char *)malloc(size);
        if (!data_copy) {
            free(name_copy);
            errno = ENOMEM;
            return 0;
        }
        memcpy(data_copy, data, size);
    }

    tcc_vfs_lock();
    index = tcc_vfs_find_dynamic_key(norm);
    if (index >= 0) {
        TCCVfsDynamicResource *res = &tcc_vfs_dynamic_resources[index];
        free(res->name);
        free(res->data);
        res->name = name_copy;
        res->data = data_copy;
        res->size = size;
        tcc_vfs_unlock();
        return 1;
    }

    if (tcc_vfs_dynamic_count == tcc_vfs_dynamic_capacity) {
        if (tcc_vfs_dynamic_capacity > UINT_MAX / 2u) {
            tcc_vfs_unlock();
            free(name_copy);
            free(data_copy);
            errno = EOVERFLOW;
            return 0;
        }
        new_capacity = tcc_vfs_dynamic_capacity == 0 ? 8u : tcc_vfs_dynamic_capacity * 2u;
        allocation_count = (size_t)new_capacity;
        if (allocation_count > SIZE_MAX / sizeof(TCCVfsDynamicResource)) {
            tcc_vfs_unlock();
            free(name_copy);
            free(data_copy);
            errno = EOVERFLOW;
            return 0;
        }
        new_items = (TCCVfsDynamicResource *)realloc(
            tcc_vfs_dynamic_resources,
            allocation_count * sizeof(TCCVfsDynamicResource));
        if (!new_items) {
            tcc_vfs_unlock();
            free(name_copy);
            free(data_copy);
            errno = ENOMEM;
            return 0;
        }
        memset(new_items + tcc_vfs_dynamic_capacity, 0,
               (new_capacity - tcc_vfs_dynamic_capacity) * sizeof(TCCVfsDynamicResource));
        tcc_vfs_dynamic_resources = new_items;
        tcc_vfs_dynamic_capacity = new_capacity;
    }

    tcc_vfs_dynamic_resources[tcc_vfs_dynamic_count].name = name_copy;
    tcc_vfs_dynamic_resources[tcc_vfs_dynamic_count].data = data_copy;
    tcc_vfs_dynamic_resources[tcc_vfs_dynamic_count].size = size;
    tcc_vfs_dynamic_count += 1;
    tcc_vfs_unlock();
    return 1;
}

TCC_VFS_API int tcc_vfs_unmount(const char *path)
{
    char norm[TCC_VFS_KEY_MAX];
    TCCVfsDynamicResource removed;
    int index;

    memset(&removed, 0, sizeof removed);
    if (!tcc_vfs_normalize_key(path, norm, sizeof norm))
        return 0;
    if (norm[0] == '\0') {
        errno = EINVAL;
        return 0;
    }
    tcc_vfs_lock();
    index = tcc_vfs_find_dynamic_key(norm);
    if (index >= 0) {
        unsigned int last = tcc_vfs_dynamic_count - 1u;

        removed = tcc_vfs_dynamic_resources[index];
        if ((unsigned int)index != last)
            tcc_vfs_dynamic_resources[index] = tcc_vfs_dynamic_resources[last];
        memset(&tcc_vfs_dynamic_resources[last], 0,
               sizeof tcc_vfs_dynamic_resources[last]);
        tcc_vfs_dynamic_count = last;
    }
    tcc_vfs_unlock();
    if (index < 0) {
        errno = ENOENT;
        return 0;
    }
    tcc_vfs_free_dynamic_resource(&removed);
    return 1;
}

static int tcc_vfs_materialize_resource(
    const TCCBuiltinResource *res,
    const unsigned char **out_data,
    unsigned char **out_owned_data,
    size_t *out_size);

static int tcc_vfs_alloc_materialized_fd(const unsigned char *data, unsigned char *owned_data, size_t size)
{
    int i;

    tcc_vfs_lock();
    for (i = 0; i < TCC_VFS_MAX_OPEN; ++i) {
        TCCVfsOpenFile *vf = &tcc_vfs_open_files[i];
        if (!vf->used) {
            vf->data = data;
            vf->owned_data = owned_data;
            vf->size = size;
            vf->pos = 0;
            vf->used = 1;
            tcc_vfs_unlock();
            return TCC_VFS_FD_BASE + i;
        }
    }

    tcc_vfs_unlock();
    free(owned_data);
    errno = EMFILE;
    return -1;
}

static int tcc_vfs_alloc_fd(const TCCBuiltinResource *res)
{
    const unsigned char *data;
    unsigned char *owned_data;
    size_t size;

    if (!tcc_vfs_materialize_resource(res, &data, &owned_data, &size))
        return -1;

    return tcc_vfs_alloc_materialized_fd(data, owned_data, size);
}

static int tcc_vfs_is_readonly_open(int flags)
{
    return (flags & O_ACCMODE) == O_RDONLY;
}

static void *tcc_vfs_lzma_alloc(ISzAllocPtr alloc, size_t size)
{
    (void)alloc;
    if (size == 0)
        return NULL;
    return malloc(size);
}

static void tcc_vfs_lzma_free(ISzAllocPtr alloc, void *address)
{
    (void)alloc;
    free(address);
}

static const ISzAlloc tcc_vfs_lzma_allocator = {
    tcc_vfs_lzma_alloc,
    tcc_vfs_lzma_free
};

static int tcc_vfs_decode_lzma(
    const unsigned char *src,
    size_t src_size,
    unsigned char *dst,
    size_t dst_size)
{
    SizeT in_size;
    SizeT out_size;
    ELzmaStatus status;
    SRes result;

    if (src_size < LZMA_PROPS_SIZE)
        return 0;

    in_size = (SizeT)(src_size - LZMA_PROPS_SIZE);
    out_size = (SizeT)dst_size;
    result = LzmaDecode(
        (Byte *)dst,
        &out_size,
        (const Byte *)src + LZMA_PROPS_SIZE,
        &in_size,
        (const Byte *)src,
        LZMA_PROPS_SIZE,
        LZMA_FINISH_END,
        &status,
        &tcc_vfs_lzma_allocator);

    return result == SZ_OK
        && out_size == (SizeT)dst_size
        && in_size == (SizeT)(src_size - LZMA_PROPS_SIZE);
}

static int tcc_vfs_materialize_resource(
    const TCCBuiltinResource *res,
    const unsigned char **out_data,
    unsigned char **out_owned_data,
    size_t *out_size)
{
    unsigned char *data;

    if (!out_data || !out_owned_data || !out_size)
        return 0;
    *out_data = NULL;
    *out_owned_data = NULL;
    *out_size = 0;

    if (!res || !res->data) {
        errno = ENOENT;
        return 0;
    }

    if (res->method == TCC_BUILTIN_RESOURCE_STORE) {
        *out_data = res->data;
        *out_size = res->size;
        return 1;
    }

    if (res->method != TCC_BUILTIN_RESOURCE_LZMA) {
        errno = EINVAL;
        return 0;
    }

    data = (unsigned char *)malloc(res->size ? res->size : 1u);
    if (!data) {
        errno = ENOMEM;
        return 0;
    }
    if (!tcc_vfs_decode_lzma(res->data, res->packed_size, data, res->size)) {
        free(data);
        errno = EINVAL;
        return 0;
    }

    *out_data = data;
    *out_owned_data = data;
    *out_size = res->size;
    return 1;
}

#ifdef _WIN32
static int tcc_vfs_real_open(const char *path, int flags, int mode)
{
    return tcc_utf8_open(path, flags, mode);
}

static FILE *tcc_vfs_real_fopen(const char *path, const char *mode)
{
    return tcc_utf8_fopen(path, mode);
}
#else
static int tcc_vfs_real_open(const char *path, int flags, int mode)
{
    return open(path, flags, mode);
}

static FILE *tcc_vfs_real_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}
#endif

static int tcc_vfs_is_readonly_fopen(const char *mode)
{
    return mode && mode[0] == 'r' && strchr(mode, '+') == NULL;
}

static FILE *tcc_vfs_fopen_bytes(const unsigned char *data, size_t size, unsigned char *owned_data)
{
    FILE *file;

    file = tmpfile();
    if (!file) {
        if (owned_data)
            free(owned_data);
        return NULL;
    }

    if (size && fwrite(data, 1, size, file) != size) {
        if (owned_data)
            free(owned_data);
        fclose(file);
        return NULL;
    }
    if (owned_data)
        free(owned_data);
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    return file;
}

static FILE *tcc_vfs_fopen_resource(const TCCBuiltinResource *res)
{
    const unsigned char *data;
    unsigned char *owned_data;
    size_t size;

    if (!tcc_vfs_materialize_resource(res, &data, &owned_data, &size))
        return NULL;
    return tcc_vfs_fopen_bytes(data, size, owned_data);
}

TCC_VFS_API int tcc_vfs_open(const char *path, int flags, ...)
{
    int fd, mode = 0;
    const TCCBuiltinResource *res;
    unsigned char *dynamic_data = NULL;
    size_t dynamic_size = 0;
    int dynamic_status;
    va_list ap;

    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }

    if (tcc_vfs_is_readonly_open(flags)) {
        dynamic_status = tcc_vfs_copy_dynamic(path, &dynamic_data, &dynamic_size);
        if (dynamic_status > 0)
            return tcc_vfs_alloc_materialized_fd(
                dynamic_data, dynamic_data, dynamic_size);
        if (dynamic_status < 0)
            return -1;
    }

    fd = tcc_vfs_real_open(path, flags, mode);
    if (fd >= 0 || !tcc_vfs_is_readonly_open(flags))
        return fd;

    res = tcc_vfs_lookup_resource(path);
    if (!res)
        return -1;
    return tcc_vfs_alloc_fd(res);
}

TCC_VFS_API int tcc_vfs_close(int fd)
{
    unsigned char *owned_data = NULL;

    if (tcc_vfs_is_virtual_slot(fd)) {
        TCCVfsOpenFile *vf;

        tcc_vfs_lock();
        vf = &tcc_vfs_open_files[fd - TCC_VFS_FD_BASE];
        if (!vf->used) {
            tcc_vfs_unlock();
            errno = EBADF;
            return -1;
        }
        owned_data = vf->owned_data;
        memset(vf, 0, sizeof *vf);
        tcc_vfs_unlock();
        free(owned_data);
        return 0;
    }
#ifdef _WIN32
    return _close(fd);
#else
    return close(fd);
#endif
}

TCC_VFS_API int tcc_vfs_read(int fd, void *buf, size_t count)
{
    if (count > 0 && !buf) {
        errno = EFAULT;
        return -1;
    }
    if (count > (size_t)INT_MAX)
        count = (size_t)INT_MAX;
    if (tcc_vfs_is_virtual_slot(fd)) {
        TCCVfsOpenFile *vf;
        size_t n = 0;

        tcc_vfs_lock();
        vf = &tcc_vfs_open_files[fd - TCC_VFS_FD_BASE];
        if (!vf->used) {
            tcc_vfs_unlock();
            errno = EBADF;
            return -1;
        }
        if (vf->pos < vf->size) {
            n = vf->size - vf->pos;
            if (n > count)
                n = count;
        }
        if (n) {
            memcpy(buf, vf->data + vf->pos, n);
            vf->pos += n;
        }
        tcc_vfs_unlock();
        return (int)n;
    }
#ifdef _WIN32
    return _read(fd, buf, (unsigned int)count);
#else
    return (int)read(fd, buf, count);
#endif
}

TCC_VFS_API long tcc_vfs_lseek(int fd, long offset, int whence)
{
    if (tcc_vfs_is_virtual_slot(fd)) {
        TCCVfsOpenFile *vf;
        size_t base;
        size_t pos;
        size_t magnitude;

        tcc_vfs_lock();
        vf = &tcc_vfs_open_files[fd - TCC_VFS_FD_BASE];
        if (!vf->used) {
            tcc_vfs_unlock();
            errno = EBADF;
            return -1;
        }
        switch (whence) {
        case SEEK_SET:
            base = 0;
            break;
        case SEEK_CUR:
            base = vf->pos;
            break;
        case SEEK_END:
            base = vf->size;
            break;
        default:
            tcc_vfs_unlock();
            errno = EINVAL;
            return -1;
        }
        if (offset < 0) {
            magnitude = (size_t)(-(offset + 1L)) + 1u;
            if (magnitude > base) {
                tcc_vfs_unlock();
                errno = EINVAL;
                return -1;
            }
            pos = base - magnitude;
        } else {
            if (base > (size_t)LONG_MAX - (size_t)offset) {
                tcc_vfs_unlock();
                errno = EOVERFLOW;
                return -1;
            }
            pos = base + (size_t)offset;
        }
        if (pos > (size_t)LONG_MAX) {
            tcc_vfs_unlock();
            errno = EOVERFLOW;
            return -1;
        }
        vf->pos = pos;
        tcc_vfs_unlock();
        return (long)pos;
    }
#ifdef _WIN32
    return _lseek(fd, offset, whence);
#else
    return (long)lseek(fd, offset, whence);
#endif
}

TCC_VFS_API FILE *tcc_vfs_fopen(const char *path, const char *mode)
{
    FILE *file;
    const TCCBuiltinResource *res;
    unsigned char *dynamic_data = NULL;
    size_t dynamic_size = 0;
    int dynamic_status;

    if (tcc_vfs_is_readonly_fopen(mode)) {
        dynamic_status = tcc_vfs_copy_dynamic(path, &dynamic_data, &dynamic_size);
        if (dynamic_status > 0)
            return tcc_vfs_fopen_bytes(dynamic_data, dynamic_size, dynamic_data);
        if (dynamic_status < 0)
            return NULL;
    }

    file = tcc_vfs_real_fopen(path, mode);
    if (file || !tcc_vfs_is_readonly_fopen(mode))
        return file;

    res = tcc_vfs_lookup_resource(path);
    return tcc_vfs_fopen_resource(res);
}

TCC_VFS_API int tcc_vfs_fclose(FILE *file)
{
    return fclose(file);
}
