void ImportCore(TCCState* s)
{
	// 添加函数 - Core
	tcc_add_symbol(s, "xrtInit", xrtInit);
	tcc_add_symbol(s, "xrtUnit", xrtUnit);
	
	// 添加函数 - Sup
	tcc_add_symbol(s, "memmem", memmem);
	tcc_add_symbol(s, "u16len", u16len);
	tcc_add_symbol(s, "u32len", u32len);
	
	// 添加函数 - Base
	tcc_add_symbol(s, "xrtMalloc", xrtMalloc);
	tcc_add_symbol(s, "xrtCalloc", xrtCalloc);
	tcc_add_symbol(s, "xrtRealloc", xrtRealloc);
	tcc_add_symbol(s, "xrtFree", xrtFree);
	tcc_add_symbol(s, "xrtTempMemory", xrtTempMemory);
	tcc_add_symbol(s, "xrtSetError", xrtSetError);
	tcc_add_symbol(s, "xrtSetErrorU16", xrtSetErrorU16);
	tcc_add_symbol(s, "xrtSetErrorU32", xrtSetErrorU32);
	tcc_add_symbol(s, "xrtClearError", xrtClearError);
	
	// 添加函数 - OS
	tcc_add_symbol(s, "xrtRun", xrtRun);
	tcc_add_symbol(s, "xrtRunW", xrtRunW);
	tcc_add_symbol(s, "xrtStart", xrtStart);
	tcc_add_symbol(s, "xrtStartW", xrtStartW);
	tcc_add_symbol(s, "xrtChain", xrtChain);
	tcc_add_symbol(s, "xrtChainW", xrtChainW);
	
	// 添加函数 - Math
	tcc_add_symbol(s, "xrtRand32", xrtRand32);
	tcc_add_symbol(s, "xrtSetRandSeed32", xrtSetRandSeed32);
	tcc_add_symbol(s, "xrtRandRange", xrtRandRange);
	
	// 添加函数 - Charset
	tcc_add_symbol(s, "xrtUTF8to16", xrtUTF8to16);
	tcc_add_symbol(s, "xrtUTF8to32", xrtUTF8to32);
	tcc_add_symbol(s, "xrtUTF16to8", xrtUTF16to8);
	tcc_add_symbol(s, "xrtUTF16to32", xrtUTF16to32);
	tcc_add_symbol(s, "xrtUTF32to8", xrtUTF32to8);
	tcc_add_symbol(s, "xrtUTF32to16", xrtUTF32to16);
	tcc_add_symbol(s, "xrtUTF16LEtoBE", xrtUTF16LEtoBE);
	tcc_add_symbol(s, "xrtUTF32LEtoBE", xrtUTF32LEtoBE);
	tcc_add_symbol(s, "xrtConvCharset", xrtConvCharset);
	tcc_add_symbol(s, "xrtIsUTF8", xrtIsUTF8);
	tcc_add_symbol(s, "xrtDetectCharset", xrtDetectCharset);
	tcc_add_symbol(s, "xrtGetCharSize", xrtGetCharSize);
	
	// 添加函数 - String
	tcc_add_symbol(s, "xrtCopyStr", xrtCopyStr);
	tcc_add_symbol(s, "xrtCopyStrW", xrtCopyStrW);
	tcc_add_symbol(s, "xrtCopyStrU16", xrtCopyStrU16);
	tcc_add_symbol(s, "xrtCopyStrU32", xrtCopyStrU32);
	tcc_add_symbol(s, "xrtLCase", xrtLCase);
	tcc_add_symbol(s, "xrtLCaseW", xrtLCaseW);
	tcc_add_symbol(s, "xrtUCase", xrtUCase);
	tcc_add_symbol(s, "xrtUCaseW", xrtUCaseW);
	tcc_add_symbol(s, "xrtFindStr", xrtFindStr);
	tcc_add_symbol(s, "xrtInStr", xrtInStr);
	tcc_add_symbol(s, "xrtFindStrW", xrtFindStrW);
	tcc_add_symbol(s, "xrtInStrW", xrtInStrW);
	tcc_add_symbol(s, "xrtCheckStr", xrtCheckStr);
	tcc_add_symbol(s, "xrtCheckStrW", xrtCheckStrW);
	tcc_add_symbol(s, "xrtLTrim", xrtLTrim);
	tcc_add_symbol(s, "xrtRTrim", xrtRTrim);
	tcc_add_symbol(s, "xrtTrim", xrtTrim);
	tcc_add_symbol(s, "xrtLTrimW", xrtLTrimW);
	tcc_add_symbol(s, "xrtRTrimW", xrtRTrimW);
	tcc_add_symbol(s, "xrtTrimW", xrtTrimW);
	tcc_add_symbol(s, "xrtFilterStr", xrtFilterStr);
	tcc_add_symbol(s, "xrtFilterStrW", xrtFilterStrW);
	tcc_add_symbol(s, "xrtFormat", xrtFormat);
	tcc_add_symbol(s, "xrtFormatW", xrtFormatW);
	tcc_add_symbol(s, "xrtReplace", xrtReplace);
	tcc_add_symbol(s, "xrtReplaceW", xrtReplaceW);
	tcc_add_symbol(s, "xrtSplit", xrtSplit);
	tcc_add_symbol(s, "xrtSplitW", xrtSplitW);
	tcc_add_symbol(s, "xrtRandStr", xrtRandStr);
	tcc_add_symbol(s, "xrtRandStrW", xrtRandStrW);
	tcc_add_symbol(s, "xrtHexEncode", xrtHexEncode);
	tcc_add_symbol(s, "xrtHexEncodeW", xrtHexEncodeW);
	tcc_add_symbol(s, "xrtHexDecode", xrtHexDecode);
	tcc_add_symbol(s, "xrtHexDecodeW", xrtHexDecodeW);
	tcc_add_symbol(s, "xrtBase64Encode", xrtBase64Encode);
	tcc_add_symbol(s, "xrtBase64EncodeW", xrtBase64EncodeW);
	tcc_add_symbol(s, "xrtBase64Decode", xrtBase64Decode);
	tcc_add_symbol(s, "xrtBase64DecodeW", xrtBase64DecodeW);
	
	// 添加函数 - Time
	tcc_add_symbol(s, "xrtIsLeapYear", xrtIsLeapYear);
	tcc_add_symbol(s, "xrtDaysInMonth", xrtDaysInMonth);
	tcc_add_symbol(s, "xrtDaysInYear", xrtDaysInYear);
	tcc_add_symbol(s, "xrtTimeSerial", xrtTimeSerial);
	tcc_add_symbol(s, "xrtDateSerial", xrtDateSerial);
	tcc_add_symbol(s, "xrtDateTimeSerial", xrtDateTimeSerial);
	tcc_add_symbol(s, "xrtSecond", xrtSecond);
	tcc_add_symbol(s, "xrtMinute", xrtMinute);
	tcc_add_symbol(s, "xrtHour", xrtHour);
	tcc_add_symbol(s, "xrtDay", xrtDay);
	tcc_add_symbol(s, "xrtMonth", xrtMonth);
	tcc_add_symbol(s, "xrtYear", xrtYear);
	tcc_add_symbol(s, "xrtWeekday", xrtWeekday);
	tcc_add_symbol(s, "xrtDayOfYear", xrtDayOfYear);
	tcc_add_symbol(s, "xrtDecodeSerial", xrtDecodeSerial);
	tcc_add_symbol(s, "xrtNow", xrtNow);
	tcc_add_symbol(s, "xrtDate", xrtDate);
	tcc_add_symbol(s, "xrtTime", xrtTime);
	tcc_add_symbol(s, "xrtNowStr", xrtNowStr);
	tcc_add_symbol(s, "xrtNowStrW", xrtNowStrW);
	tcc_add_symbol(s, "xrtDateStr", xrtDateStr);
	tcc_add_symbol(s, "xrtDateStrW", xrtDateStrW);
	tcc_add_symbol(s, "xrtTimeStr", xrtTimeStr);
	tcc_add_symbol(s, "xrtTimeStrW", xrtTimeStrW);
	tcc_add_symbol(s, "xrtTimeToStr", xrtTimeToStr);
	tcc_add_symbol(s, "xrtTimeToStrW", xrtTimeToStrW);
	tcc_add_symbol(s, "xrtDateAdd", xrtDateAdd);
	tcc_add_symbol(s, "xrtDateDiff", xrtDateDiff);
	
	// 添加函数 - Path
	tcc_add_symbol(s, "xrtPathGetNameExt", xrtPathGetNameExt);
	tcc_add_symbol(s, "xrtPathGetNameExtW", xrtPathGetNameExtW);
	tcc_add_symbol(s, "xrtPathGetName", xrtPathGetName);
	tcc_add_symbol(s, "xrtPathGetNameW", xrtPathGetNameW);
	tcc_add_symbol(s, "xrtPathGetExt", xrtPathGetExt);
	tcc_add_symbol(s, "xrtPathGetExtW", xrtPathGetExtW);
	tcc_add_symbol(s, "xrtPathGetDir", xrtPathGetDir);
	tcc_add_symbol(s, "xrtPathGetDirW", xrtPathGetDirW);
	tcc_add_symbol(s, "xrtPathIsAbs", xrtPathIsAbs);
	tcc_add_symbol(s, "xrtPathIsAbsW", xrtPathIsAbsW);
	tcc_add_symbol(s, "xrtPathRandom", xrtPathRandom);
	tcc_add_symbol(s, "xrtPathRandomW", xrtPathRandomW);
	tcc_add_symbol(s, "xrtPathJoin", xrtPathJoin);
	tcc_add_symbol(s, "xrtPathJoinW", xrtPathJoinW);
	
	// 添加函数 - File
	tcc_add_symbol(s, "xrtOpen", xrtOpen);
	tcc_add_symbol(s, "xrtOpenW", xrtOpenW);
	tcc_add_symbol(s, "xrtClose", xrtClose);
	tcc_add_symbol(s, "xrtSeek", xrtSeek);
	tcc_add_symbol(s, "xrtTell", xrtTell);
	tcc_add_symbol(s, "xrtGetEOF", xrtGetEOF);
	tcc_add_symbol(s, "xrtEOF", xrtEOF);
	tcc_add_symbol(s, "xrtSetEOF", xrtSetEOF);
	tcc_add_symbol(s, "xrtRead", xrtRead);
	tcc_add_symbol(s, "xrtReadW", xrtReadW);
	tcc_add_symbol(s, "xrtWrite", xrtWrite);
	tcc_add_symbol(s, "xrtWriteW", xrtWriteW);
	tcc_add_symbol(s, "xrtGet", xrtGet);
	tcc_add_symbol(s, "xrtPut", xrtPut);
	tcc_add_symbol(s, "xrtFileAppend", xrtFileAppend);
	tcc_add_symbol(s, "xrtFileAppendW", xrtFileAppendW);
	tcc_add_symbol(s, "xrtFileWriteAll", xrtFileWriteAll);
	tcc_add_symbol(s, "xrtFileWriteAllW", xrtFileWriteAllW);
	tcc_add_symbol(s, "xrtFileReadAll", xrtFileReadAll);
	tcc_add_symbol(s, "xrtFileReadAllW", xrtFileReadAllW);
	tcc_add_symbol(s, "xrtFilePutAll", xrtFilePutAll);
	tcc_add_symbol(s, "xrtFilePutAllW", xrtFilePutAllW);
	tcc_add_symbol(s, "xrtFileGetAll", xrtFileGetAll);
	tcc_add_symbol(s, "xrtFileGetAllW", xrtFileGetAllW);
	tcc_add_symbol(s, "xrtPathExists", xrtPathExists);
	tcc_add_symbol(s, "xrtPathExistsW", xrtPathExistsW);
	tcc_add_symbol(s, "xrtFileExists", xrtFileExists);
	tcc_add_symbol(s, "xrtFileExistsW", xrtFileExistsW);
	tcc_add_symbol(s, "xrtDirExists", xrtDirExists);
	tcc_add_symbol(s, "xrtDirExistsW", xrtDirExistsW);
	tcc_add_symbol(s, "xrtFileGetSize", xrtFileGetSize);
	tcc_add_symbol(s, "xrtFileGetSizeW", xrtFileGetSizeW);
	tcc_add_symbol(s, "xrtFileSetSize", xrtFileSetSize);
	tcc_add_symbol(s, "xrtFileSetSizeW", xrtFileSetSizeW);
	tcc_add_symbol(s, "xrtFileGetAttr", xrtFileGetAttr);
	tcc_add_symbol(s, "xrtFileGetAttrW", xrtFileGetAttrW);
	tcc_add_symbol(s, "xrtFileSetAttr", xrtFileSetAttr);
	tcc_add_symbol(s, "xrtFileSetAttrW", xrtFileSetAttrW);
	tcc_add_symbol(s, "xrtFileGetAccessTime", xrtFileGetAccessTime);
	tcc_add_symbol(s, "xrtFileGetAccessTimeW", xrtFileGetAccessTimeW);
	tcc_add_symbol(s, "xrtFileGetChangeTime", xrtFileGetChangeTime);
	tcc_add_symbol(s, "xrtFileGetChangeTimeW", xrtFileGetChangeTimeW);
	tcc_add_symbol(s, "xrtFileCopy", xrtFileCopy);
	tcc_add_symbol(s, "xrtFileCopyW", xrtFileCopyW);
	tcc_add_symbol(s, "xrtFileMove", xrtFileMove);
	tcc_add_symbol(s, "xrtFileMoveW", xrtFileMoveW);
	tcc_add_symbol(s, "xrtFileDelete", xrtFileDelete);
	tcc_add_symbol(s, "xrtFileDeleteW", xrtFileDeleteW);
	tcc_add_symbol(s, "xrtDirScan", xrtDirScan);
	tcc_add_symbol(s, "xrtDirScanW", xrtDirScanW);
	tcc_add_symbol(s, "xrtDirCreate", xrtDirCreate);
	tcc_add_symbol(s, "xrtDirCreateW", xrtDirCreateW);
	tcc_add_symbol(s, "xrtDirCreateAll", xrtDirCreateAll);
	tcc_add_symbol(s, "xrtDirCreateAllW", xrtDirCreateAllW);
	tcc_add_symbol(s, "xrtDirCopy", xrtDirCopy);
	tcc_add_symbol(s, "xrtDirCopyW", xrtDirCopyW);
	tcc_add_symbol(s, "xrtDirMove", xrtDirMove);
	tcc_add_symbol(s, "xrtDirMoveW", xrtDirMoveW);
	tcc_add_symbol(s, "xrtDirDelete", xrtDirDelete);
	tcc_add_symbol(s, "xrtDirDeleteW", xrtDirDeleteW);
	
	// 添加函数 - Hash
	tcc_add_symbol(s, "xrtHash32_WithSeed", xrtHash32_WithSeed);
	tcc_add_symbol(s, "xrtHash32", xrtHash32);
	tcc_add_symbol(s, "xrtHash64_WithSeed", xrtHash64_WithSeed);
	tcc_add_symbol(s, "xrtHash64", xrtHash64);
	tcc_add_symbol(s, "xrtHash64_Micro_WithSeed", xrtHash64_Micro_WithSeed);
	tcc_add_symbol(s, "xrtHash64_Micro", xrtHash64_Micro);
	tcc_add_symbol(s, "xrtHash64_Nano_WithSeed", xrtHash64_Nano_WithSeed);
	tcc_add_symbol(s, "xrtHash64_Nano", xrtHash64_Nano);
	
}