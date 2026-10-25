/* Deterministic I/O failures for the packer; no vendor implementation is linked. */
#define XRT_MODULE_ALL
#include "lib/xrt.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#define XS_VFS_IMPLEMENTATION
#include "src/core/xs_vfs.h"
#define XS_PACK_IMPLEMENTATION
#include "src/core/xs_pack.h"
#include <assert.h>

bool g_XS_VfsDisabled = false;

static struct {
	uint64 size, final_size;
	size_t available, position;
	unsigned reads, closes, sizes;
	bool open_failure, size_failure, read_failure, eof_failure, close_failure, resize;
} file;

xfile xrtOpen(cstr path, uint32 flags)
{
	assert(path != NULL && flags == XFILE_READ);
	return file.open_failure ? NULL : (xfile)&file;
}

bool xrtFileSize(xfile handle, uint64* size)
{
	assert(handle == (xfile)&file);
	*size = file.sizes++ > 0 && file.resize ? file.final_size : file.size;
	return !file.size_failure;
}

bool xrtRead(xfile handle, ptr buffer, size_t request, size_t* done)
{
	assert(handle == (xfile)&file);
	file.reads++;
	*done = 0;
	if ( file.read_failure || (file.eof_failure && file.reads > 1) ) return false;
	*done = file.available - file.position;
	if ( *done > request ) *done = request;
	memset(buffer, 'x', *done);
	file.position += *done;
	return true;
}

bool xrtReadFull(xfile handle, ptr buffer, size_t request, size_t* done)
{
	return xrtRead(handle, buffer, request, done) && *done == request;
}

bool xrtClose(xfile handle)
{
	assert(handle == (xfile)&file);
	file.closes++;
	return !file.close_failure;
}

static struct {
	unsigned next, closes;
	uint64 size;
	bool open_failure, next_failure, close_failure;
} directory;

xdir xrtDirOpen(cstr path, uint32 flags)
{
	assert(path != NULL && flags == XDIR_STAT);
	return directory.open_failure ? NULL : (xdir)&directory;
}

xdirnext xrtDirNext(xdir handle, xdirentry* entry)
{
	assert(handle == (xdir)&directory);
	if ( directory.next++ == 0 ) {
		memset(entry, 0, sizeof(*entry));
		entry->Name = (xstrview){ "xs.json", 7 };
		entry->Info.Type = XFILE_TYPE_FILE;
		entry->Info.Size = directory.size;
		return XDIR_NEXT_ITEM;
	}
	return directory.next_failure ? XDIR_NEXT_ERROR : XDIR_NEXT_END;
}

bool xrtDirClose(xdir handle)
{
	assert(handle == (xdir)&directory);
	directory.closes++;
	return !directory.close_failure;
}

static void check_walk(void)
{
	XS_PackList list = {0};
	assert(XS_PackWalk(&list, "site", "", 0));
	assert(list.nCount == 1 && directory.closes == 1);
	XS_PackListUnit(&list);
	memset(&directory, 0, sizeof(directory));
	directory.next_failure = true;
	assert(!XS_PackWalk(&list, "site", "", 0));
	assert(strstr(list.sError, "cannot read directory") != NULL);
	assert(list.nCount == 1 && directory.closes == 1);
	XS_PackListUnit(&list);
	memset(&directory, 0, sizeof(directory));
	directory.close_failure = true;
	assert(!XS_PackWalk(&list, "site", "", 0));
	assert(strstr(list.sError, "cannot close directory") != NULL);
	assert(directory.closes == 1);
	XS_PackListUnit(&list);
	memset(&directory, 0, sizeof(directory));
	directory.open_failure = true;
	assert(!XS_PackWalk(&list, "site", "", 0));
	assert(list.nCount == 0 && directory.closes == 0);
	XS_PackListUnit(&list);
	memset(&directory, 0, sizeof(directory));
	directory.size = XS_VFS_MAX_ENTRY_ORIG_SIZE + 1u;
	assert(!XS_PackWalk(&list, "site", "", 0));
	assert(strstr(list.sError, "entry size over limit") != NULL);
	assert(list.nCount == 0 && directory.closes == 1);
	XS_PackListUnit(&list);
}

static void read_fails(uint64 limit)
{
	size_t size = 123;
	assert(XS_PackReadWhole("input", limit, &size) == NULL);
	assert(size == 0 && file.closes == (file.open_failure ? 0u : 1u));
}

static void check_read(void)
{
	size_t size = 123;
	uint8_t* data;
	file.size = file.available = 3;
	data = XS_PackReadWhole("input", 3, &size);
	assert(data != NULL && size == 3 && memcmp(data, "xxx", 4) == 0);
	assert(file.closes == 1);
	free(data);
	memset(&file, 0, sizeof(file));
	data = XS_PackReadWhole("input", 0, &size);
	assert(data != NULL && size == 0 && data[0] == 0 && file.closes == 1);
	free(data);
	memset(&file, 0, sizeof(file));
	file.size = XS_VFS_MAX_ENTRY_ORIG_SIZE + 1u;
	read_fails(XS_VFS_MAX_ENTRY_ORIG_SIZE);
	assert(file.reads == 0);
	memset(&file, 0, sizeof(file));
	file.size = UINT64_C(1) << 32;
	read_fails(XS_VFS_MAX_ENTRY_ORIG_SIZE);
	assert(file.reads == 0);
	memset(&file, 0, sizeof(file));
	file.size = SIZE_MAX;
	read_fails(UINT64_MAX);
	assert(file.reads == 0);
	memset(&file, 0, sizeof(file));
	file.open_failure = true;
	read_fails(3);
	memset(&file, 0, sizeof(file));
	file.size_failure = true;
	read_fails(3);
	memset(&file, 0, sizeof(file));
	file.size = 3;
	file.available = 2;
	read_fails(3);
	memset(&file, 0, sizeof(file));
	file.size = 3;
	file.available = 4;
	read_fails(3);
	memset(&file, 0, sizeof(file));
	file.size = file.available = 3;
	file.read_failure = true;
	read_fails(3);
	memset(&file, 0, sizeof(file));
	file.size = file.available = 3;
	file.eof_failure = true;
	read_fails(3);
	memset(&file, 0, sizeof(file));
	file.size = file.available = 3;
	file.resize = true;
	file.final_size = 4;
	read_fails(3);
	memset(&file, 0, sizeof(file));
	file.close_failure = true;
	read_fails(3);
}

static void check_paths(void)
{
	static const char* valid[] = {
		"a", "a/b.txt", ".hidden", "a/.well-known/key", "a/.../b", "a/..suffix"
	};
	static const char* invalid[] = {
		"", "/a", "a/", "a//b", ".", "..", "./a", "../a", "a/./b", "a/../b", "a\\b"
	};
	const char bounded[] = {'a', '/', 'b'};
	const char embedded_nul[] = {'a', 0, 'b'};
	char long_path[XS_VFS_MAX_PATH_BYTES + 1u];
	for ( size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++ )
		assert(XS_VfsArchivePathValid(valid[i], strlen(valid[i])));
	for ( size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++ )
		assert(!XS_VfsArchivePathValid(invalid[i], strlen(invalid[i])));
	assert(!XS_VfsArchivePathValid(NULL, 0));
	assert(!XS_VfsArchivePathValid(NULL, 1));
	assert(XS_VfsArchivePathValid(bounded, sizeof(bounded)));
	assert(!XS_VfsArchivePathValid(embedded_nul, sizeof(embedded_nul)));
	memset(long_path, 'a', sizeof(long_path));
	assert(XS_VfsArchivePathValid(long_path, XS_VFS_MAX_PATH_BYTES));
	assert(!XS_VfsArchivePathValid(long_path, sizeof(long_path)));
}

int main(void)
{
	check_walk();
	check_read();
	check_paths();
	puts("PASS native pack I/O and canonical path boundaries");
	return 0;
}
