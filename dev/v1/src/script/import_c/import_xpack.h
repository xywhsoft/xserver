#ifndef XS_SCRIPT_IMPORT_C_IMPORT_XPACK_H
#define XS_SCRIPT_IMPORT_C_IMPORT_XPACK_H



static inline void ImportXPack(TCCState* s)
{
	if ( s == NULL ) {
		return;
	}

	tcc_add_symbol(s, "xpkOpen", xpkOpen);
	tcc_add_symbol(s, "xpkClose", xpkClose);
	tcc_add_symbol(s, "xpkSave", xpkSave);
	tcc_add_symbol(s, "xpkBuild", xpkBuild);
	tcc_add_symbol(s, "xpkGetPackType", xpkGetPackType);
	tcc_add_symbol(s, "xpkSetPackType", xpkSetPackType);
	tcc_add_symbol(s, "xpkGetDefaultComp", xpkGetDefaultComp);
	tcc_add_symbol(s, "xpkSetDefaultComp", xpkSetDefaultComp);
	tcc_add_symbol(s, "xpkGetMetaComp", xpkGetMetaComp);
	tcc_add_symbol(s, "xpkSetMetaComp", xpkSetMetaComp);
	tcc_add_symbol(s, "xpkGetInfoComp", xpkGetInfoComp);
	tcc_add_symbol(s, "xpkSetInfoComp", xpkSetInfoComp);
	tcc_add_symbol(s, "xpkGetInfoExtSize", xpkGetInfoExtSize);
	tcc_add_symbol(s, "xpkSetInfoExtSize", xpkSetInfoExtSize);
	tcc_add_symbol(s, "xpkGetVolumeSize", xpkGetVolumeSize);
	tcc_add_symbol(s, "xpkSetVolumeSize", xpkSetVolumeSize);
	tcc_add_symbol(s, "xpkGetSolidMode", xpkGetSolidMode);
	tcc_add_symbol(s, "xpkSetSolidMode", xpkSetSolidMode);
	tcc_add_symbol(s, "xpkMetaGet", xpkMetaGet);
	tcc_add_symbol(s, "xpkMetaSet", xpkMetaSet);
	tcc_add_symbol(s, "xpkMetaClear", xpkMetaClear);
	tcc_add_symbol(s, "xpkCount", xpkCount);
	tcc_add_symbol(s, "xpkGetInfo", xpkGetInfo);
	tcc_add_symbol(s, "xpkGetInfoExt", xpkGetInfoExt);
	tcc_add_symbol(s, "xpkSetInfoExt", xpkSetInfoExt);
	tcc_add_symbol(s, "xpkAddFile", xpkAddFile);
	tcc_add_symbol(s, "xpkAddData", xpkAddData);
	tcc_add_symbol(s, "xpkReadToFile", xpkReadToFile);
	tcc_add_symbol(s, "xpkReadToMemory", xpkReadToMemory);
	tcc_add_symbol(s, "xpkUpdateFile", xpkUpdateFile);
	tcc_add_symbol(s, "xpkUpdateData", xpkUpdateData);
	tcc_add_symbol(s, "xpkRemove", xpkRemove);
	tcc_add_symbol(s, "xpkSetFlag", xpkSetFlag);
	tcc_add_symbol(s, "xpkIndexFind", xpkIndexFind);
	tcc_add_symbol(s, "xpkIndexGetInfo", xpkIndexGetInfo);
	tcc_add_symbol(s, "xpkIndexAddFile", xpkIndexAddFile);
	tcc_add_symbol(s, "xpkIndexAddData", xpkIndexAddData);
	tcc_add_symbol(s, "xpkIndexReadToFile", xpkIndexReadToFile);
	tcc_add_symbol(s, "xpkIndexReadToMemory", xpkIndexReadToMemory);
	tcc_add_symbol(s, "xpkIndexUpdateFile", xpkIndexUpdateFile);
	tcc_add_symbol(s, "xpkIndexUpdateData", xpkIndexUpdateData);
	tcc_add_symbol(s, "xpkIndexRemove", xpkIndexRemove);
	tcc_add_symbol(s, "xpkIndexSetFlag", xpkIndexSetFlag);
	tcc_add_symbol(s, "xpkPathExists", xpkPathExists);
	tcc_add_symbol(s, "xpkPathGetInfo", xpkPathGetInfo);
	tcc_add_symbol(s, "xpkPathAddFile", xpkPathAddFile);
	tcc_add_symbol(s, "xpkPathAddData", xpkPathAddData);
	tcc_add_symbol(s, "xpkPathReadToFile", xpkPathReadToFile);
	tcc_add_symbol(s, "xpkPathReadToMemory", xpkPathReadToMemory);
	tcc_add_symbol(s, "xpkPathUpdateFile", xpkPathUpdateFile);
	tcc_add_symbol(s, "xpkPathUpdateData", xpkPathUpdateData);
	tcc_add_symbol(s, "xpkPathRename", xpkPathRename);
	tcc_add_symbol(s, "xpkPathRemove", xpkPathRemove);
	tcc_add_symbol(s, "xpkPathSetAttr", xpkPathSetAttr);
	tcc_add_symbol(s, "xpkEach", xpkEach);
	tcc_add_symbol(s, "xpkEachMatch", xpkEachMatch);
	tcc_add_symbol(s, "xpkVerify", xpkVerify);
	tcc_add_symbol(s, "xpkVerifyAll", xpkVerifyAll);
	tcc_add_symbol(s, "xpkStatGet", xpkStatGet);
	tcc_add_symbol(s, "xpkFree", xpkFree);
	tcc_add_symbol(s, "xpkHash32", xpkHash32);
	tcc_add_symbol(s, "xpkLastError", xpkLastError);
	tcc_add_symbol(s, "xpkLastErrorMessage", xpkLastErrorMessage);
}

#endif
