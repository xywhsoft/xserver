


// 解析 JSON 文件到 xte Value
PSSTK_Object varStack;
XTE_Value varRoot;
XTE_Value varCur;
json_sax_ret_t xte_private_ParseJSON_Proc(json_sax_parser_t *parser)
{
	json_string_t *jkey = &parser->array[parser->index];
	// 新结构数据入栈
	if ( ((jkey->type == JSON_ARRAY) || (jkey->type == JSON_OBJECT)) && (parser->value.vcmd == JSON_SAX_START) ) {
		if ( jkey->type == JSON_ARRAY ) {
			if ( varRoot ) {
				if ( varCur->MainType == XTE_DT_ARRAY ) {
					XTE_Value arrNew = xteValueCreateArray();
					xteArrayAppendValue(varCur, arrNew, TRUE);
					PSSTK_Push(varStack, arrNew);
					varCur = arrNew;
				} else if ( varCur->MainType == XTE_DT_TABLE ) {
					XTE_Value arrNew = xteValueCreateArray();
					char* sKey = malloc(jkey->len + 1);
					memcpy(sKey, jkey->str, jkey->len);
					sKey[jkey->len] = 0;
					xteTableSetValue(varCur, sKey, jkey->len, arrNew, TRUE);
					PSSTK_Push(varStack, arrNew);
					varCur = arrNew;
				}
			} else {
				varRoot = xteValueCreateArray();
				PSSTK_Push(varStack, varRoot);
				varCur = varRoot;
			}
		} else if ( jkey->type == JSON_OBJECT ) {
			if ( varRoot ) {
				if ( varCur->MainType == XTE_DT_ARRAY ) {
					XTE_Value tblNew = xteValueCreateTable();
					xteArrayAppendValue(varCur, tblNew, TRUE);
					PSSTK_Push(varStack, tblNew);
					varCur = tblNew;
				} else if ( varCur->MainType == XTE_DT_TABLE ) {
					XTE_Value tblNew = xteValueCreateTable();
					char* sKey = malloc(jkey->len + 1);
					memcpy(sKey, jkey->str, jkey->len);
					sKey[jkey->len] = 0;
					xteTableSetValue(varCur, sKey, jkey->len, tblNew, TRUE);
					PSSTK_Push(varStack, tblNew);
					varCur = tblNew;
				}
			} else {
				varRoot = xteValueCreateTable();
				PSSTK_Push(varStack, varRoot);
				varCur = varRoot;
			}
		}
    }
	if ( jkey->type == JSON_NULL ) {
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendNull(varCur);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetNull(varCur, sKey, jkey->len);
			}
        } else {
			varRoot = xteValueCreateNull();
        }
	} else if ( jkey->type == JSON_BOOL ) {
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendBool(varCur, parser->value.vnum.vbool);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetBool(varCur, sKey, jkey->len, parser->value.vnum.vbool);
			}
        } else {
			varRoot = xteValueCreateBool(parser->value.vnum.vbool);
        }
	} else if ( jkey->type == JSON_INT ) {
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendInt(varCur, parser->value.vnum.vint);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetInt(varCur, sKey, jkey->len, parser->value.vnum.vint);
			}
        } else {
			varRoot = xteValueCreateInt(parser->value.vnum.vint);
        }
	} else if ( jkey->type == JSON_HEX ) {
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendInt(varCur, parser->value.vnum.vhex);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetInt(varCur, sKey, jkey->len, parser->value.vnum.vhex);
			}
        } else {
			varRoot = xteValueCreateInt(parser->value.vnum.vhex);
        }
	} else if ( jkey->type == JSON_LINT ) {
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendInt(varCur, parser->value.vnum.vlint);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetInt(varCur, sKey, jkey->len, parser->value.vnum.vlint);
			}
        } else {
			varRoot = xteValueCreateInt(parser->value.vnum.vlint);
        }
	} else if ( jkey->type == JSON_LHEX ) {
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendInt(varCur, parser->value.vnum.vlhex);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetInt(varCur, sKey, jkey->len, parser->value.vnum.vlhex);
			}
        } else {
			varRoot = xteValueCreateInt(parser->value.vnum.vlhex);
        }
	} else if ( jkey->type == JSON_DOUBLE ) {
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendFloat(varCur, parser->value.vnum.vdbl);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetFloat(varCur, sKey, jkey->len, parser->value.vnum.vdbl);
			}
        } else {
			varRoot = xteValueCreateFloat(parser->value.vnum.vdbl);
        }
	} else if ( jkey->type == JSON_STRING ) {
		char* sText = malloc(parser->value.vstr.len + 1);
		memcpy(sText, parser->value.vstr.str, parser->value.vstr.len);
		sText[parser->value.vstr.len] = 0;
        if ( varRoot ) {
			if ( varCur->MainType == XTE_DT_ARRAY ) {
				xteArrayAppendText(varCur, sText, TRUE);
			} else if ( varCur->MainType == XTE_DT_TABLE ) {
				char* sKey = malloc(jkey->len + 1);
				memcpy(sKey, jkey->str, jkey->len);
				sKey[jkey->len] = 0;
				xteTableSetText(varCur, sKey, jkey->len, sText, TRUE);
			}
        } else {
			varRoot = xteValueCreateText(sText, TRUE);
        }
	} else if ( jkey->type == JSON_ARRAY ) {
	} else if ( jkey->type == JSON_OBJECT ) {
	} else {
		printf("Unknown data type\n");
	}
    if ( ((jkey->type == JSON_ARRAY) || (jkey->type == JSON_OBJECT)) && (parser->value.vcmd == JSON_SAX_FINISH) ) {
		if ( varStack > 0) {
			PSSTK_Pop(varStack);
			varCur = PSSTK_Top(varStack);
		}
	}
    return JSON_SAX_PARSE_CONTINUE;
}
XTE_Value xteParseJSON_File(char* sFile)
{
	varStack = PSSTK_Create(256);
	varRoot = NULL;
	varCur = NULL;
	int iRet = json_sax_parse_file(sFile, xte_private_ParseJSON_Proc);
	if ( iRet < 0 ) {
		return xteValueCreateNull();
	}
	PSSTK_Destroy(varStack);
	return varRoot;
}
XTE_Value xteParseJSON(char* sText, size_t iSize)
{
	varStack = PSSTK_Create(256);
	varRoot = NULL;
	varCur = NULL;
	if ( iSize == 0 ) {
		iSize = strlen(sText);
	}
	int iRet = json_sax_parse_str(sText, iSize, xte_private_ParseJSON_Proc);
	if ( iRet < 0 ) {
		return xteValueCreateNull();
	}
	PSSTK_Destroy(varStack);
	return varRoot;
}




// 将 xte Value 序列化为字符串
void xte_private_Stringify_Table(json_sax_print_hd handle, XTE_Value varVal, json_string_t* sKey);
void xte_private_Stringify_Array(json_sax_print_hd handle, XTE_Value varVal, json_string_t* sKey)
{
	json_sax_print_array(handle, sKey, JSON_SAX_START);
	for ( int i = 0; i < varVal->vArray->Count; i++ ) {
		XTE_Value objItem = xteArrayGetValue(varVal, i);
		if ( objItem->MainType == XTE_DT_NULL ) {
			json_sax_print_null(handle, NULL);
		} else if ( objItem->MainType == XTE_DT_BOOL ) {
			json_sax_print_bool(handle, NULL, objItem->vBool);
		} else if ( objItem->MainType == XTE_DT_INT ) {
			json_sax_print_lint(handle, NULL, objItem->vInt);
		} else if ( objItem->MainType == XTE_DT_FLOAT ) {
			json_sax_print_double(handle, NULL, objItem->vFloat);
		} else if ( objItem->MainType == XTE_DT_TEXT ) {
			json_string_t jstr = {0};
			jstr.str = objItem->vText;
			json_string_info_update(&jstr);
			json_sax_print_string(handle, NULL, &jstr);
		} else if ( objItem->MainType == XTE_DT_ARRAY ) {
			xte_private_Stringify_Array(handle, objItem, NULL);
		} else if ( objItem->MainType == XTE_DT_TABLE ) {
			xte_private_Stringify_Table(handle, objItem, NULL);
		}
	}
	json_sax_print_array(handle, NULL, JSON_SAX_FINISH);
}
int xte_private_Stringify_Table_Proc(Hash32_Key* pKey, XTE_Value* ppVal, json_sax_print_hd handle)
{
	XTE_Value objItem = *ppVal;
	json_string_t jkey = {0};
	jkey.str = pKey->Key;
	jkey.len = pKey->KeyLen;
	if ( objItem->MainType == XTE_DT_NULL ) {
		json_sax_print_null(handle, &jkey);
	} else if ( objItem->MainType == XTE_DT_BOOL ) {
		json_sax_print_bool(handle, &jkey, objItem->vBool);
	} else if ( objItem->MainType == XTE_DT_INT ) {
		json_sax_print_lint(handle, &jkey, objItem->vInt);
	} else if ( objItem->MainType == XTE_DT_FLOAT ) {
		json_sax_print_double(handle, &jkey, objItem->vFloat);
	} else if ( objItem->MainType == XTE_DT_TEXT ) {
		json_string_t jstr = {0};
		jstr.str = objItem->vText;
		json_string_info_update(&jstr);
		json_sax_print_string(handle, &jkey, &jstr);
	} else if ( objItem->MainType == XTE_DT_ARRAY ) {
		xte_private_Stringify_Array(handle, objItem, &jkey);
	} else if ( objItem->MainType == XTE_DT_TABLE ) {
		xte_private_Stringify_Table(handle, objItem, &jkey);
	}
	return FALSE;
}
void xte_private_Stringify_Table(json_sax_print_hd handle, XTE_Value varVal, json_string_t* sKey)
{
	json_sax_print_object(handle, sKey, JSON_SAX_START);
	AVLHT32_Walk(varVal->vTable, (void*)xte_private_Stringify_Table_Proc, (void*)handle);
	json_sax_print_object(handle, NULL, JSON_SAX_FINISH);
}
char* xteStringifyJSON(XTE_Value varVal, int bFormat, size_t* pRetSize)
{
	// 初始化 SAX 字符串输出
	json_print_choice_t choice;
	memset(&choice, 0, sizeof(choice));
	choice.format_flag = bFormat;
    choice.item_total = 32;
    choice.ptr = NULL;
    json_sax_print_hd handle = json_sax_print_start(&choice);
    // 遍历根节点
    if ( varVal->MainType == XTE_DT_NULL ) {
		json_sax_print_null(handle, NULL);
    } else if ( varVal->MainType == XTE_DT_BOOL ) {
		json_sax_print_bool(handle, NULL, varVal->vBool);
    } else if ( varVal->MainType == XTE_DT_INT ) {
		json_sax_print_lint(handle, NULL, varVal->vInt);
    } else if ( varVal->MainType == XTE_DT_FLOAT ) {
		json_sax_print_double(handle, NULL, varVal->vFloat);
    } else if ( varVal->MainType == XTE_DT_TEXT ) {
		json_string_t jstr = {0};
		jstr.str = varVal->vText;
		json_string_info_update(&jstr);
		json_sax_print_string(handle, NULL, &jstr);
    } else if ( varVal->MainType == XTE_DT_ARRAY ) {
		xte_private_Stringify_Array(handle, varVal, NULL);
    } else if ( varVal->MainType == XTE_DT_TABLE ) {
		xte_private_Stringify_Table(handle, varVal, NULL);
    }
    // 返回结果
	return json_sax_print_finish(handle, pRetSize, NULL);
}
int xteStringifyJSON_File(char* sFile, XTE_Value varVal, int bFormat)
{
	// 初始化 SAX 字符串输出
	json_print_choice_t choice;
	memset(&choice, 0, sizeof(choice));
	choice.format_flag = bFormat;
	choice.item_total = 32;
	choice.path = sFile;
	choice.ptr = NULL;
    json_sax_print_hd handle = json_sax_print_start(&choice);
    // 遍历根节点
    if ( varVal->MainType == XTE_DT_NULL ) {
		json_sax_print_null(handle, NULL);
    } else if ( varVal->MainType == XTE_DT_BOOL ) {
		json_sax_print_bool(handle, NULL, varVal->vBool);
    } else if ( varVal->MainType == XTE_DT_INT ) {
		json_sax_print_lint(handle, NULL, varVal->vInt);
    } else if ( varVal->MainType == XTE_DT_FLOAT ) {
		json_sax_print_double(handle, NULL, varVal->vFloat);
    } else if ( varVal->MainType == XTE_DT_TEXT ) {
		json_string_t jstr = {0};
		jstr.str = varVal->vText;
		json_string_info_update(&jstr);
		json_sax_print_string(handle, NULL, &jstr);
    } else if ( varVal->MainType == XTE_DT_ARRAY ) {
		xte_private_Stringify_Array(handle, varVal, NULL);
    } else if ( varVal->MainType == XTE_DT_TABLE ) {
		xte_private_Stringify_Table(handle, varVal, NULL);
    }
    // 返回结果
	char* sRet = json_sax_print_finish(handle, NULL, NULL);
	if ( strcmp(sRet, "ok") == 0 ) {
		return TRUE;
	} else {
		return FALSE;
	}
}



// 输出 xte Value 的结构和值（调试用）
int TableItemProc(Hash32_Key* pKey, XTE_Value* ppVal, int iLevel);
void xtePrintValue(XTE_Value objVal, int iLevel, int iMode, int iKey, char* sKey)
{
	for ( int i = 0; i < iLevel; i++ ) {
		printf("    ");
	}
	if ( iMode == 1 ) {
		// 输出数组元素
		if ( objVal->MainType == XTE_DT_NULL ) {
			printf("[%d] (null)\n", iKey);
		} else if ( objVal->MainType == XTE_DT_BOOL ) {
			printf("[%d] %s (bool)\n", iKey, xteValueGetText(objVal));
		} else if ( objVal->MainType == XTE_DT_INT ) {
			printf("[%d] %lld (int)\n", iKey, xteValueGetInt(objVal));
		} else if ( objVal->MainType == XTE_DT_FLOAT ) {
			printf("[%d] %f (float)\n", iKey, xteValueGetFloat(objVal));
		} else if ( objVal->MainType == XTE_DT_TEXT ) {
			printf("[%d] %s (text)\n", iKey, xteValueGetText(objVal));
		} else if ( objVal->MainType == XTE_DT_ARRAY ) {
			printf("[%d] (array)\n", iKey);
		} else if ( objVal->MainType == XTE_DT_TABLE ) {
			printf("[%d] (table)\n", iKey);
		} else {
			printf("Unknown data type\n");
		}
	} else if ( iMode == 2 ) {
		// 输出表元素
		if ( objVal->MainType == XTE_DT_NULL ) {
			printf("[%s] (null)\n", sKey);
		} else if ( objVal->MainType == XTE_DT_BOOL ) {
			printf("[%s] %s (bool)\n", sKey, xteValueGetText(objVal));
		} else if ( objVal->MainType == XTE_DT_INT ) {
			printf("[%s] %lld (int)\n", sKey, xteValueGetInt(objVal));
		} else if ( objVal->MainType == XTE_DT_FLOAT ) {
			printf("[%s] %f (float)\n", sKey, xteValueGetFloat(objVal));
		} else if ( objVal->MainType == XTE_DT_TEXT ) {
			printf("[%s] %s (text)\n", sKey, xteValueGetText(objVal));
		} else if ( objVal->MainType == XTE_DT_ARRAY ) {
			printf("[%s] (array)\n", sKey);
		} else if ( objVal->MainType == XTE_DT_TABLE ) {
			printf("[%s] (table)\n", sKey);
		} else {
			printf("Unknown data type\n");
		}
	} else {
		if ( objVal->MainType == XTE_DT_NULL ) {
			printf("(null)\n");
		} else if ( objVal->MainType == XTE_DT_BOOL ) {
			printf("%s (bool)\n", xteValueGetText(objVal));
		} else if ( objVal->MainType == XTE_DT_INT ) {
			printf("%lld (int)\n", xteValueGetInt(objVal));
		} else if ( objVal->MainType == XTE_DT_FLOAT ) {
			printf("%f (float)\n", xteValueGetFloat(objVal));
		} else if ( objVal->MainType == XTE_DT_TEXT ) {
			printf("%s (text)\n", xteValueGetText(objVal));
		} else if ( objVal->MainType == XTE_DT_ARRAY ) {
			printf("(array: count[%d])\n", xteArrayItemCount(objVal));
		} else if ( objVal->MainType == XTE_DT_TABLE ) {
			printf("(table)\n");
		} else {
			printf("Unknown data type\n");
		}
	}
	if ( objVal->MainType == XTE_DT_ARRAY ) {
		for ( int i = 0; i < objVal->vArray->Count; i++ ) {
			XTE_Value objItem = xteArrayGetValue(objVal, i);
			xtePrintValue(objItem, iLevel+1, 1, i, NULL);
		}
	} else if ( objVal->MainType == XTE_DT_TABLE ) {
		AVLHT32_Walk(objVal->vTable, (void*)TableItemProc, (void*)(intptr_t)(iLevel+1));
	}
}
int TableItemProc(Hash32_Key* pKey, XTE_Value* ppVal, int iLevel)
{
	xtePrintValue(*ppVal, iLevel, 2, 0, pKey->Key);
	return FALSE;
}


