void ImportMMU(TCCState* s)
{
	// 添加函数 - MMU - Core
	tcc_add_symbol(s, "MMU_Init", MMU_Init);
	tcc_add_symbol(s, "MMU_Unit", MMU_Unit);
	tcc_add_symbol(s, "MMU_Thread_Init", MMU_Thread_Init);
	tcc_add_symbol(s, "MMU_Thread_Unit", MMU_Thread_Unit);
	
	// 添加函数 - MMU - PAMM
	tcc_add_symbol(s, "PAMM_Create", PAMM_Create);
	tcc_add_symbol(s, "PAMM_Destroy", PAMM_Destroy);
	tcc_add_symbol(s, "PAMM_Init", PAMM_Init);
	tcc_add_symbol(s, "PAMM_Unit", PAMM_Unit);
	tcc_add_symbol(s, "PAMM_Malloc", PAMM_Malloc);
	tcc_add_symbol(s, "PAMM_Insert", PAMM_Insert);
	tcc_add_symbol(s, "PAMM_Append", PAMM_Append);
	tcc_add_symbol(s, "PAMM_AddAlt", PAMM_AddAlt);
	tcc_add_symbol(s, "PAMM_Swap", PAMM_Swap);
	tcc_add_symbol(s, "PAMM_Remove", PAMM_Remove);
	tcc_add_symbol(s, "PAMM_GetVal", PAMM_GetVal);
	tcc_add_symbol(s, "PAMM_GetVal_Unsafe", PAMM_GetVal_Unsafe);
	tcc_add_symbol(s, "PAMM_SetVal", PAMM_SetVal);
	tcc_add_symbol(s, "PAMM_SetVal_Unsafe", PAMM_SetVal_Unsafe);
	tcc_add_symbol(s, "PAMM_Sort", PAMM_Sort);
	
	// 添加函数 - MMU - SAMM
	tcc_add_symbol(s, "SAMM_Create", SAMM_Create);
	tcc_add_symbol(s, "SAMM_Destroy", SAMM_Destroy);
	tcc_add_symbol(s, "SAMM_Init", SAMM_Init);
	tcc_add_symbol(s, "SAMM_Unit", SAMM_Unit);
	tcc_add_symbol(s, "SAMM_Malloc", SAMM_Malloc);
	tcc_add_symbol(s, "SAMM_Insert", SAMM_Insert);
	tcc_add_symbol(s, "SAMM_Append", SAMM_Append);
	tcc_add_symbol(s, "SAMM_Swap", SAMM_Swap);
	tcc_add_symbol(s, "SAMM_Remove", SAMM_Remove);
	tcc_add_symbol(s, "SAMM_GetPtr", SAMM_GetPtr);
	tcc_add_symbol(s, "SAMM_GetPtr_Unsafe", SAMM_GetPtr_Unsafe);
	tcc_add_symbol(s, "SAMM_Sort", SAMM_Sort);
	
	// 添加函数 - MMU - MBMU
	tcc_add_symbol(s, "MBMU_Create", MBMU_Create);
	tcc_add_symbol(s, "MBMU_Destroy", MBMU_Destroy);
	tcc_add_symbol(s, "MBMU_Init", MBMU_Init);
	tcc_add_symbol(s, "MBMU_Unit", MBMU_Unit);
	tcc_add_symbol(s, "MBMU_Malloc", MBMU_Malloc);
	tcc_add_symbol(s, "MBMU_Insert", MBMU_Insert);
	tcc_add_symbol(s, "MBMU_Append", MBMU_Append);
	
	// 添加函数 - MMU - BSMM
	tcc_add_symbol(s, "BSMM_Create", BSMM_Create);
	tcc_add_symbol(s, "BSMM_Destroy", BSMM_Destroy);
	tcc_add_symbol(s, "BSMM_Init", BSMM_Init);
	tcc_add_symbol(s, "BSMM_Unit", BSMM_Unit);
	tcc_add_symbol(s, "BSMM_Alloc", BSMM_Alloc);
	tcc_add_symbol(s, "BSMM_Free", BSMM_Free);
	
	// 添加函数 - MMU - MMU256
	tcc_add_symbol(s, "MMU256_Create", MMU256_Create);
	tcc_add_symbol(s, "MMU256_Alloc", MMU256_Alloc);
	tcc_add_symbol(s, "MMU256_FreeIdx", MMU256_FreeIdx);
	tcc_add_symbol(s, "MMU256_Free", MMU256_Free);
	tcc_add_symbol(s, "MMU256_GC", MMU256_GC);
	
	// 添加函数 - MMU - MM256
	tcc_add_symbol(s, "MM256_Create", MM256_Create);
	tcc_add_symbol(s, "MM256_Destroy", MM256_Destroy);
	tcc_add_symbol(s, "MM256_Init", MM256_Init);
	tcc_add_symbol(s, "MM256_Unit", MM256_Unit);
	tcc_add_symbol(s, "MM256_Alloc", MM256_Alloc);
	tcc_add_symbol(s, "MM256_Free", MM256_Free);
	tcc_add_symbol(s, "MM256_GC", MM256_GC);
	
	// 添加函数 - MMU - SSSTK
	tcc_add_symbol(s, "SSSTK_Create", SSSTK_Create);
	tcc_add_symbol(s, "SSSTK_Push", SSSTK_Push);
	tcc_add_symbol(s, "SSSTK_PushData", SSSTK_PushData);
	tcc_add_symbol(s, "SSSTK_Pop", SSSTK_Pop);
	tcc_add_symbol(s, "SSSTK_Top", SSSTK_Top);
	tcc_add_symbol(s, "SSSTK_GetPos", SSSTK_GetPos);
	tcc_add_symbol(s, "SSSTK_GetPos_Unsafe", SSSTK_GetPos_Unsafe);
	
	// 添加函数 - MMU - PSSTK
	tcc_add_symbol(s, "PSSTK_Create", PSSTK_Create);
	tcc_add_symbol(s, "PSSTK_Push", PSSTK_Push);
	tcc_add_symbol(s, "PSSTK_Pop", PSSTK_Pop);
	tcc_add_symbol(s, "PSSTK_Top", PSSTK_Top);
	tcc_add_symbol(s, "PSSTK_GetPos", PSSTK_GetPos);
	tcc_add_symbol(s, "PSSTK_GetPos_Unsafe", PSSTK_GetPos_Unsafe);
	
	// 添加函数 - MMU - SDSTK
	tcc_add_symbol(s, "SDSTK_Create", SDSTK_Create);
	tcc_add_symbol(s, "SDSTK_Destroy", SDSTK_Destroy);
	tcc_add_symbol(s, "SDSTK_Init", SDSTK_Init);
	tcc_add_symbol(s, "SDSTK_Unit", SDSTK_Unit);
	tcc_add_symbol(s, "SDSTK_Push", SDSTK_Push);
	tcc_add_symbol(s, "SDSTK_PushData", SDSTK_PushData);
	tcc_add_symbol(s, "SDSTK_Pop", SDSTK_Pop);
	tcc_add_symbol(s, "SDSTK_Top", SDSTK_Top);
	tcc_add_symbol(s, "SDSTK_GetPos", SDSTK_GetPos);
	tcc_add_symbol(s, "SDSTK_GetPos_Unsafe", SDSTK_GetPos_Unsafe);
	
	// 添加函数 - MMU - PDSTK
	tcc_add_symbol(s, "PDSTK_Create", PDSTK_Create);
	tcc_add_symbol(s, "PDSTK_Destroy", PDSTK_Destroy);
	tcc_add_symbol(s, "PDSTK_Init", PDSTK_Init);
	tcc_add_symbol(s, "PDSTK_Unit", PDSTK_Unit);
	tcc_add_symbol(s, "PDSTK_Push", PDSTK_Push);
	tcc_add_symbol(s, "PDSTK_Pop", PDSTK_Pop);
	tcc_add_symbol(s, "PDSTK_Top", PDSTK_Top);
	tcc_add_symbol(s, "PDSTK_GetPos", PDSTK_GetPos);
	tcc_add_symbol(s, "PDSTK_GetPos_Unsafe", PDSTK_GetPos_Unsafe);
	
	// 添加函数 - MMU - LLIST Base
	tcc_add_symbol(s, "LLB_InsertPrev", LLB_InsertPrev);
	tcc_add_symbol(s, "LLB_InsertNext", LLB_InsertNext);
	tcc_add_symbol(s, "LLB_Remove", LLB_Remove);
	
	// 添加函数 - MMU - LLIST
	tcc_add_symbol(s, "LList_Create", LList_Create);
	tcc_add_symbol(s, "LList_Destroy", LList_Destroy);
	tcc_add_symbol(s, "LList_Init", LList_Init);
	tcc_add_symbol(s, "LList_Unit", LList_Unit);
	tcc_add_symbol(s, "LList_InsertPrev", LList_InsertPrev);
	tcc_add_symbol(s, "LList_InsertNext", LList_InsertNext);
	tcc_add_symbol(s, "LList_Remove", LList_Remove);
	
	// 添加函数 - MMU - AVLTree Base
	tcc_add_symbol(s, "AVLTB_Insert", AVLTB_Insert);
	tcc_add_symbol(s, "AVLTB_Remove", AVLTB_Remove);
	tcc_add_symbol(s, "AVLTB_Search", AVLTB_Search);
	
	// 添加函数 - MMU - AVLTree
	tcc_add_symbol(s, "AVLTree_Create", AVLTree_Create);
	tcc_add_symbol(s, "AVLTree_Destroy", AVLTree_Destroy);
	tcc_add_symbol(s, "AVLTree_Init", AVLTree_Init);
	tcc_add_symbol(s, "AVLTree_Unit", AVLTree_Unit);
	tcc_add_symbol(s, "AVLTree_Insert", AVLTree_Insert);
	tcc_add_symbol(s, "AVLTree_Remove", AVLTree_Remove);
	tcc_add_symbol(s, "AVLTree_Search", AVLTree_Search);
	
	// 添加函数 - MMU - MP256
	tcc_add_symbol(s, "MP256_Create", MP256_Create);
	tcc_add_symbol(s, "MP256_Destroy", MP256_Destroy);
	tcc_add_symbol(s, "MP256_Init", MP256_Init);
	tcc_add_symbol(s, "MP256_Unit", MP256_Unit);
	tcc_add_symbol(s, "MP256_Alloc", MP256_Alloc);
	tcc_add_symbol(s, "MP256_Free", MP256_Free);
	tcc_add_symbol(s, "MP256_GC", MP256_GC);
	
	// 添加函数 - MMU - AVLHT32
	tcc_add_symbol(s, "AVLHT32_Create", AVLHT32_Create);
	tcc_add_symbol(s, "AVLHT32_Destroy", AVLHT32_Destroy);
	tcc_add_symbol(s, "AVLHT32_Init", AVLHT32_Init);
	tcc_add_symbol(s, "AVLHT32_Unit", AVLHT32_Unit);
	tcc_add_symbol(s, "AVLHT32_Set", AVLHT32_Set);
	tcc_add_symbol(s, "AVLHT32_SetPtr", AVLHT32_SetPtr);
	tcc_add_symbol(s, "AVLHT32_Get", AVLHT32_Get);
	tcc_add_symbol(s, "AVLHT32_GetPtr", AVLHT32_GetPtr);
	tcc_add_symbol(s, "AVLHT32_Remove", AVLHT32_Remove);
	tcc_add_symbol(s, "AVLHT32_Exists", AVLHT32_Exists);
	tcc_add_symbol(s, "AVLHT32_Count", AVLHT32_Count);
	tcc_add_symbol(s, "AVLHT32_Walk", AVLHT32_Walk);
	
	// 添加函数 - MMU - AVLHT64
	tcc_add_symbol(s, "AVLHT64_Create", AVLHT64_Create);
	tcc_add_symbol(s, "AVLHT64_Destroy", AVLHT64_Destroy);
	tcc_add_symbol(s, "AVLHT64_Init", AVLHT64_Init);
	tcc_add_symbol(s, "AVLHT64_Unit", AVLHT64_Unit);
	tcc_add_symbol(s, "AVLHT64_Set", AVLHT64_Set);
	tcc_add_symbol(s, "AVLHT64_SetPtr", AVLHT64_SetPtr);
	tcc_add_symbol(s, "AVLHT64_Get", AVLHT64_Get);
	tcc_add_symbol(s, "AVLHT64_GetPtr", AVLHT64_GetPtr);
	tcc_add_symbol(s, "AVLHT64_Remove", AVLHT64_Remove);
	tcc_add_symbol(s, "AVLHT64_Exists", AVLHT64_Exists);
	tcc_add_symbol(s, "AVLHT64_Count", AVLHT64_Count);
	tcc_add_symbol(s, "AVLHT64_Walk", AVLHT64_Walk);
	
}