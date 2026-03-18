#ifndef XS_CORE_ERROR_H
#define XS_CORE_ERROR_H

typedef struct {
	int WarnCount;
	int ErrorCount;
} XS_ErrorState;

static XS_ErrorState g_objXsErrorState = {0};

static inline void XS_ResetErrors()
{
	g_objXsErrorState.WarnCount = 0;
	g_objXsErrorState.ErrorCount = 0;
}

static inline void XS_ReportWarn(const char* sFormat, ...)
{
	va_list args;
	
	g_objXsErrorState.WarnCount++;
	printf("[xs:warn] ");
	va_start(args, sFormat);
	vprintf(sFormat, args);
	va_end(args);
	printf("\n");
	fflush(stdout);
}

static inline void XS_ReportError(const char* sFormat, ...)
{
	va_list args;
	
	g_objXsErrorState.ErrorCount++;
	printf("[xs:error] ");
	va_start(args, sFormat);
	vprintf(sFormat, args);
	va_end(args);
	printf("\n");
	fflush(stdout);
}

static inline bool XS_HasErrors()
{
	return g_objXsErrorState.ErrorCount > 0;
}

#endif
