#ifndef XS_CORE_LIMITS_H
#define XS_CORE_LIMITS_H

/* Host policy; override at compile time when a deployment needs larger inputs. */
#ifndef XS_CONFIG_BYTES_LIMIT
#define XS_CONFIG_BYTES_LIMIT (16u * 1024u * 1024u)
#endif
#ifndef XS_SCRIPT_BYTES_LIMIT
#define XS_SCRIPT_BYTES_LIMIT (16u * 1024u * 1024u)
#endif
#ifndef XS_CONFIG_STRING_LIMIT
#define XS_CONFIG_STRING_LIMIT 65535u
#endif
#ifndef XS_PATH_BYTES_LIMIT
#define XS_PATH_BYTES_LIMIT 4095u
#endif
#ifndef XS_CONFIG_SERVERS_LIMIT
#define XS_CONFIG_SERVERS_LIMIT 4096u
#endif
#ifndef XS_CONFIG_HOSTS_LIMIT
#define XS_CONFIG_HOSTS_LIMIT 4096u
#endif

#endif
