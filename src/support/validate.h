#ifndef XS_SUPPORT_VALIDATE_H
#define XS_SUPPORT_VALIDATE_H

typedef struct {
	const char* sKey;
	int iType;
	bool bRequired;
} XS_FieldRule;

static inline bool XS_FindFieldRule(const XS_FieldRule* arrRule, size_t iRuleCount, const char* sKey, size_t iKeyLen)
{
	size_t i;
	
	for ( i = 0; i < iRuleCount; i++ ) {
		if ( arrRule[i].sKey == NULL ) {
			continue;
		}
		if ( strlen(arrRule[i].sKey) == iKeyLen && strncmp(arrRule[i].sKey, sKey, iKeyLen) == 0 ) {
			return TRUE;
		}
	}
	
	return FALSE;
}

#endif
