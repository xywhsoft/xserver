


#define XXAPI	



#include <stdio.h>
#include <stdlib.h>



// xTemplate Engine Lexer 依赖的数据结构
#define MMU_USE_PAMM								// 动态指针数组
#define MMU_USE_SAMM								// 动态结构体数组
#define MMU_USE_MBMU								// 自增长缓冲区
#define MMU_USE_PSSTK								// 静态指针栈
#define MMU_USE_HASH32								// 32位哈希算法
#define MMU_USE_AVLHT32								// 基于 AVLTree 的哈希表（32bit哈希值）



#include "mmu.h"



#ifndef TRUE
	#define TRUE	1
#endif
#ifndef FALSE
	#define FALSE	0
#endif



// 最大支持参数数量
#define XTE_PARAM_MAXCOUNT		6



// Token 定义编号
#define XTE_TK_TEXT				0				// 文本内容
#define XTE_TK_COMMEN			1				// 注释				{! * }
#define XTE_TK_VAR				0x100			// 代入变量			{$ * : *}			参数：为 NULL 默认值
#define XTE_TK_NUM				0x101			// 代入数字变量		{% * : *}			参数：格式
#define XTE_TK_TIME				0x102			// 代入时间变量		{& * : *}			参数：格式
#define XTE_TK_BOOL				0x103			// 根据逻辑代入值		{? * : * : * }		参数：为真时的值和为假时的值
#define XTE_TK_ARR				0x104			// 代入数组			{* * : *}			参数：套用子模板
#define XTE_TK_PROC				0x105			// 代入函数或流程		{@ * : * ...}		参数：函数参数列表
#define XTE_TK_SUBTEMPLATE		0x106			// 代入子模板			{= * : * }			参数：子模板环境变量名
#define XTE_TK_SYMBOL			0xFFFF			// 预定义符号			{# * : * ...}		参数：语句参数列表

#define XTE_MODE_BLOCK			0xFFFE			// 特殊符号，表示进入数据块采集模式，以 {#end} 结尾



// Token 扩展编号
#define XTE_TK_INCLUDE			0x10000			// 引用外部文件
#define XTE_TK_DEFINE			0x10001			// 定义子模板
#define XTE_TK_SCRIPT			0x10002			// 脚本块
#define XTE_TK_IF				0x20000			// 判断语句
#define XTE_TK_ELSEIF			0x20001			// 判断语句
#define XTE_TK_ELSE				0x20002			// 判断语句
#define XTE_TK_FOR				0x30000			// 循环语句
#define XTE_TK_FOREACH			0x30001			// 迭代循环语句

#define XTE_TK_END				0xFFFFFF		// 语句结束

#define XTE_TK_USER				0x1000000		// 大于这个编号的，XTE模板后续更新不会使用，可以安全的用于扩展



#define XTE_IDTPE_DEFAULT		0				// 
#define XTE_IDTPE_BLOCK			1



// Ident Info 数据结构（用于定义标识符）
typedef struct {
	char* Ident;								// 标识符
	unsigned int TokenIndex;					// 对应的 Token 编号
	unsigned short Type;						// 0 = 单语句、1 = 独立语句块(以 {#end} 结尾)
	unsigned short Size;						// 标识符长度
	unsigned short MinParamCount;				// 最小参数数量
	unsigned short MaxParamCount;				// 最大参数数量
	unsigned int Hash;							// 标识符哈希值
} XTE_IdentInfo_Struct, *XTE_IdentInfo;

// Ident List 数据结构（标识符列表，可直接使用 SAMM_Object）
#define XTE_IdentList	SAMM_Object



// Token Item 数据结构
typedef struct {
	unsigned int Type;							// Token 定义编号
	char* Text;									// 关联文本
	size_t Size;								// 关联文本长度
	unsigned int ParamCount;					// 参数数量
	char* ParamText[XTE_PARAM_MAXCOUNT];		// 参数文本
	unsigned int ParamSize[XTE_PARAM_MAXCOUNT];	// 参数长度
	XTE_IdentInfo IdentInfo;					// 标识符语句对应的标识符信息结构体指针
	unsigned int RefLine;						// 语句在源文件中所在行
	unsigned int RefLinePos;					// 语句在源文件中所在行的位置
	unsigned int RefPos;						// 语句在源文件中所在的位置
	unsigned int RefSize;						// 语句在源文件中的长度
} XTE_TokenItem_Struct, *XTE_TokenItem;

// Token List 数据结构
typedef struct {
	int Success;								// 解析是否成功
	int ErrorCode;								// 错误代码（0=成功）
	const char* ErrorDesc;						// 错误描述
	unsigned int ErrorLine;						// 错误行号
	unsigned int ErrorLinePos;					// 错误行位置
	unsigned int ErrorPos;						// 错误位置
	unsigned int ErrorRefLine;					// 出错参考行
	unsigned int ErrorRefLinePos;				// 出错参考行位置
	unsigned int ErrorRefPos;					// 错误参考位置
	SAMM_Struct Tokens;							// Token 列表
} XTE_TokenList_Struct, *XTE_TokenList;



// xTemplate Engine Lite 数据结构
typedef struct {
	int Success;								// 解析是否成功
	int ErrorCode;								// 错误代码（0=成功）
	const char* ErrorDesc;						// 错误描述
	unsigned int ErrorLine;						// 错误行号
	unsigned int ErrorLinePos;					// 错误行位置
	unsigned int ErrorPos;						// 错误位置
	unsigned int ErrorRefLine;					// 出错参考行
	unsigned int ErrorRefLinePos;				// 出错参考行位置
	unsigned int ErrorRefPos;					// 错误参考位置
	SAMM_Object Tokens;							// Token 列表
	PAMM_Struct Actions;						// 编译后的动作列表
	AVLHT32_Struct SubTemplates;				// 子模板列表（哈希表）
} XTE_LiteStruct, *XTE_LiteObject;



// 数据类型 - 主类型
#define XTE_DT_NULL			0				// 空值
#define XTE_DT_BOOL			1				// 逻辑
#define XTE_DT_INT			2				// 整数（int64）
#define XTE_DT_FLOAT		3				// 浮点数（double）
#define XTE_DT_TEXT			4				// 文本
#define XTE_DT_ARRAY		100				// 数组
#define XTE_DT_TABLE		101				// 表
#define XTE_DT_FUNC			200				// 函数
#define XTE_DT_CUSTOM		255				// 自定义变量



// 函数类型回调
typedef struct XTE_ValueStruct XTE_ValueStruct, *XTE_Value;
typedef XTE_Value (*XTE_FUNC)(XTE_Value varENV, XTE_Value varParam);

// 数据单元
struct XTE_ValueStruct {
	unsigned char MainType;
	unsigned char SubType;
	unsigned short RefCount;
	union {
		int vBool;
		long long vInt;
		double vFloat;
		char* vText;
		PAMM_Object vArray;
		AVLHT32_Object vTable;
		XTE_FUNC vFunc;
		void* vCustom;
	};
};



// 创建关键字列表（失败返回 NULL）
XXAPI XTE_IdentList xteCreateIdentList();

// 销毁关键字列表
XXAPI void xteDestroyIdentList(XTE_IdentList objList);

// 添加一个关键字到列表
XXAPI int xteAddIdentToList(XTE_IdentList objList, char* sID, unsigned int iSize, unsigned int iIndex, unsigned int iType, unsigned int iMinParamCount, unsigned int iMaxParamCount);



// 释放 XTE_TokenList
XXAPI void xteLexerFree(XTE_TokenList arrToken);

// 解析模板文件为 Token 列表
XXAPI XTE_TokenList xteLexer(char* sText, size_t iSize, XTE_IdentList objIdentList, char* sBracket);



// 将 XTE_TokenList 转换为 XTE_LiteObject（XTE_TokenList将被释放）
XXAPI XTE_LiteObject xteLiteParseFromTokenList(XTE_TokenList objToks);

// 解析返回语法列表
XXAPI XTE_LiteObject xteLiteParse(char* sText, size_t iSize, char* sBracket);

// 释放 XTE_LiteObject 对象
XXAPI void xteLiteParseFree(XTE_LiteObject objLite);



// 引用计数操作
XXAPI void xteValueRef(XTE_Value objVal);
XXAPI void xteValueUnref(XTE_Value objVal);
#define xteValueFree	xteValueUnref

// 创建 Value
XXAPI XTE_Value xteValueCreateNull();
XXAPI XTE_Value xteValueCreateBool(int bVal);
XXAPI XTE_Value xteValueCreateInt(long long iVal);
XXAPI XTE_Value xteValueCreateFloat(double fVal);
XXAPI XTE_Value xteValueCreateText(char* sVal, int bColloc);
XXAPI XTE_Value xteValueCreateArray();
XXAPI XTE_Value xteValueCreateTable();
XXAPI XTE_Value xteValueCreateFunc(XTE_FUNC pFunc);
XXAPI XTE_Value xteValueCreateCustom(void* pVal);

// Value 读数据（最低转换度）
XXAPI int xteValueGetBool(XTE_Value objVal);
XXAPI long long xteValueGetInt(XTE_Value objVal);
XXAPI double xteValueGetFloat(XTE_Value objVal);
XXAPI char* xteValueGetText(XTE_Value objVal);
XXAPI PAMM_Object xteValueGetArray(XTE_Value objVal);
XXAPI AVLHT32_Object xteValueGetTable(XTE_Value objVal);
XXAPI XTE_FUNC xteValueGetFunc(XTE_Value objVal);
XXAPI void* xteValueGetCustom(XTE_Value objVal);

// Table 读 Key 操作
XXAPI XTE_Value xteTableGetValue(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI int xteTableGetBool(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI long long xteTableGetInt(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI double xteTableGetFloat(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI char* xteTableGetText(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI PAMM_Object xteTableGetArray(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI AVLHT32_Object xteTableGetTable(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI XTE_FUNC xteTableGetFunc(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI void* xteTableGetCustom(XTE_Value tblVal, char* sKey, unsigned int iSize);

// Table 写 Key 操作
XXAPI int xteTableSetValue(XTE_Value tblVal, char* sKey, unsigned int iSize, XTE_Value objNewVal, int bColloc);
XXAPI int xteTableSetNull(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI int xteTableSetBool(XTE_Value tblVal, char* sKey, unsigned int iSize, int bVal);
XXAPI int xteTableSetInt(XTE_Value tblVal, char* sKey, unsigned int iSize, long long iVal);
XXAPI int xteTableSetFloat(XTE_Value tblVal, char* sKey, unsigned int iSize, double fVal);
XXAPI int xteTableSetText(XTE_Value tblVal, char* sKey, unsigned int iSize, char* sVal, int bColloc);
XXAPI int xteTableSetArray(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI int xteTableSetTable(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI int xteTableSetFunc(XTE_Value tblVal, char* sKey, unsigned int iSize, XTE_FUNC pFunc);
XXAPI int xteTableSetCustom(XTE_Value tblVal, char* sKey, unsigned int iSize, void* pVal);

// 表操作
XXAPI int xteTableExists(XTE_Value tblVal, char* sKey, unsigned int iSize);
XXAPI int xteTableItemCount(XTE_Value tblVal);
XXAPI int xteTableClear(XTE_Value tblVal);

// Array 读操作（索引从 0 开始）
XXAPI XTE_Value xteArrayGetValue(XTE_Value arrVal, unsigned int idx);
XXAPI int xteArrayGetBool(XTE_Value arrVal, unsigned int idx);
XXAPI long long xteArrayGetInt(XTE_Value arrVal, unsigned int idx);
XXAPI double xteArrayGetFloat(XTE_Value arrVal, unsigned int idx);
XXAPI char* xteArrayGetText(XTE_Value arrVal, unsigned int idx);
XXAPI PAMM_Object xteArrayGetArray(XTE_Value arrVal, unsigned int idx);
XXAPI AVLHT32_Object xteArrayGetTable(XTE_Value arrVal, unsigned int idx);
XXAPI XTE_FUNC xteArrayGetFunc(XTE_Value arrVal, unsigned int idx);
XXAPI void* xteArrayGetCustom(XTE_Value arrVal, unsigned int idx);

// Array 追加操作
XXAPI int xteArrayAppendValue(XTE_Value arrVal, XTE_Value objNewVal, int bColloc);
XXAPI int xteArrayAppendNull(XTE_Value arrVal);
XXAPI int xteArrayAppendBool(XTE_Value arrVal, int bVal);
XXAPI int xteArrayAppendInt(XTE_Value arrVal, long long iVal);
XXAPI int xteArrayAppendFloat(XTE_Value arrVal, double fVal);
XXAPI int xteArrayAppendText(XTE_Value arrVal, char* sVal, int bColloc);
XXAPI int xteArrayAppendArray(XTE_Value arrVal);
XXAPI int xteArrayAppendTable(XTE_Value arrVal);
XXAPI int xteArrayAppendFunc(XTE_Value arrVal, XTE_FUNC pFunc);
XXAPI int xteArrayAppendCustom(XTE_Value arrVal, void* pVal);

// Array 插入操作
XXAPI int xteArrayInsertValue(XTE_Value arrVal, unsigned int idx, XTE_Value objNewVal, int bColloc);
XXAPI int xteArrayInsertNull(XTE_Value arrVal, unsigned int idx);
XXAPI int xteArrayInsertBool(XTE_Value arrVal, unsigned int idx, int bVal);
XXAPI int xteArrayInsertInt(XTE_Value arrVal, unsigned int idx, long long iVal);
XXAPI int xteArrayInsertFloat(XTE_Value arrVal, unsigned int idx, double fVal);
XXAPI int xteArrayInsertText(XTE_Value arrVal, unsigned int idx, char* sVal, int bColloc);
XXAPI int xteArrayInsertArray(XTE_Value arrVal, unsigned int idx);
XXAPI int xteArrayInsertTable(XTE_Value arrVal, unsigned int idx);
XXAPI int xteArrayInsertFunc(XTE_Value arrVal, unsigned int idx, XTE_FUNC pFunc);
XXAPI int xteArrayInsertCustom(XTE_Value arrVal, unsigned int idx, void* pVal);

// Array 写操作（元素必须已存在）
XXAPI int xteArraySetValue(XTE_Value arrVal, unsigned int idx, XTE_Value objNewVal, int bColloc);
XXAPI int xteArraySetNull(XTE_Value arrVal, unsigned int idx);
XXAPI int xteArraySetBool(XTE_Value arrVal, unsigned int idx, int bVal);
XXAPI int xteArraySetInt(XTE_Value arrVal, unsigned int idx, long long iVal);
XXAPI int xteArraySetFloat(XTE_Value arrVal, unsigned int idx, double fVal);
XXAPI int xteArraySetText(XTE_Value arrVal, unsigned int idx, char* sVal, int bColloc);
XXAPI int xteArraySetArray(XTE_Value arrVal, unsigned int idx);
XXAPI int xteArraySetTable(XTE_Value arrVal, unsigned int idx);
XXAPI int xteArraySetFunc(XTE_Value arrVal, unsigned int idx, XTE_FUNC pFunc);
XXAPI int xteArraySetCustom(XTE_Value arrVal, unsigned int idx, void* pVal);

// 数组操作
XXAPI int xteArraySwap(XTE_Value arrVal, unsigned int idx1, unsigned int idx2);
XXAPI int xteArrayRemove(XTE_Value arrVal, unsigned int idx, unsigned int iCount);
XXAPI int xteArrayItemCount(XTE_Value arrVal);
XXAPI int xteArrayClear(XTE_Value arrVal);

// Call 函数
XXAPI XTE_Value xteValueCall(XTE_Value varFunc, XTE_Value varENV, XTE_Value varParam);
XXAPI XTE_Value xteTableItemCall(XTE_Value tblVal, char* sKey, unsigned int iSize, XTE_Value varENV, XTE_Value varParam);
XXAPI XTE_Value xteArrayItemCall(XTE_Value arrVal, unsigned int idx, XTE_Value varENV, XTE_Value varParam);

// 类型操作
XXAPI int xteValueType(XTE_Value varVal);
XXAPI int xteTableItemType(XTE_Value varVal, char* sKey, unsigned int iSize);
XXAPI int xteArrayItemType(XTE_Value varVal, unsigned int idx);



// 根据 XTE_LiteObject 模板对象生成文档
XXAPI char* xteLiteMakeActions(PAMM_Object arrAction, XTE_LiteObject objTemplate, XTE_Value tblVal, XTE_Value tblRoot, XTE_Value tblENV, AVLHT32_Object tblInclude, size_t* pRetSize);
XXAPI char* xteLiteMake(XTE_LiteObject objTemplate, XTE_Value tblVal, XTE_Value tblENV, AVLHT32_Object tblInclude, size_t* pRetSize);


