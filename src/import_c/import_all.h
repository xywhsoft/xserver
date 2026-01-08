


#include "import_xrt.h"
#include "import_libtcc.h"
#include "import_mongoose.h"
#include "import_sqlite.h"
#include "import_xdo.h"
//#include "import_md4c.h"
#include "import_other.h"



void ImportAll(TCCState* s)
{
	
	// 添加函数 - X Runtime
	ImportXRT(s);
	
	// 添加函数 - libtcc
	ImportLibTCC(s);
	
	// 添加函数 - mongoose
	ImportMongoose(s);
	
	// 添加函数 - SQLite
	ImportSQLite(s);
	
	// 添加函数 - XDO
	ImportXDO(s);
	
	// 添加函数 - MD4C
	//ImportMD4C(s);
	
	// 添加函数 - Other
	ImportOther(s);
	
	// 最后，把自己也添加进去，方便脚本环境里，创建同等的运行环境
	tcc_add_symbol(s, "ImportAll", ImportAll);
	
}