#ifndef XS_SUPPORT_LOG_H
#define XS_SUPPORT_LOG_H

typedef enum {
	XS_LOG_DEBUG = 0,
	XS_LOG_INFO = 1,
	XS_LOG_WARN = 2,
	XS_LOG_ERROR = 3
} XS_LogLevel;

static inline const char* XS_LogLevelName(XS_LogLevel iLevel)
{
	switch ( iLevel ) {
		case XS_LOG_DEBUG: return "debug";
		case XS_LOG_INFO: return "info";
		case XS_LOG_WARN: return "warn";
		case XS_LOG_ERROR: return "error";
		default: return "log";
	}
}

static inline void XS_Log(XS_LogLevel iLevel, const char* sFormat, ...)
{
	va_list args;
	
	printf("[xs:%s] ", XS_LogLevelName(iLevel));
	va_start(args, sFormat);
	vprintf(sFormat, args);
	va_end(args);
	printf("\n");
}

#define XS_LogDebug(...) XS_Log(XS_LOG_DEBUG, __VA_ARGS__)
#define XS_LogInfo(...) XS_Log(XS_LOG_INFO, __VA_ARGS__)
#define XS_LogWarn(...) XS_Log(XS_LOG_WARN, __VA_ARGS__)
#define XS_LogError(...) XS_Log(XS_LOG_ERROR, __VA_ARGS__)

#endif
