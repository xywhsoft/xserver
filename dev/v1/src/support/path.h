#ifndef XS_SUPPORT_PATH_H
#define XS_SUPPORT_PATH_H

static inline char* XS_CopyText(const char* sText)
{
	if ( sText == NULL ) {
		return NULL;
	}
	
	return xrtCopyStr((char*)sText, 0);
}

static inline char* XS_NormalizePath(const char* sBaseDir, const char* sPath)
{
	if ( sPath == NULL || sPath[0] == '\0' ) {
		return NULL;
	}
	
	if ( xrtPathIsAbs((char*)sPath, 0) ) {
		return xrtCopyStr((char*)sPath, 0);
	}
	
	if ( sBaseDir == NULL || sBaseDir[0] == '\0' ) {
		return xrtCopyStr((char*)sPath, 0);
	}
	
	return xrtPathJoin(2, (char*)sBaseDir, (char*)sPath);
}

#endif
