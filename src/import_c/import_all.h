


#include "import_core.h"
#include "import_mongoose.h"
#include "import_mmu.h"
#include "import_libtcc.h"
#include "import_template.h"
#include "import_json.h"
#include "import_sqlite.h"
//#include "import_md4c.h"
#include "import_other.h"



void ImportAll(TCCState* s)
{
	
	// 添加函数 - xCore
	ImportCore(s);
	
	// 添加函数 - mongoose
	ImportMongoose(s);
	
	// 添加函数 - mmu
	ImportMMU(s);
	
	// 添加函数 - libtcc
	ImportLibTCC(s);
	
	// 添加函数 - xTemplate
	ImportTemplate(s);
	
	// 添加函数 - JSON
	ImportJSON(s);
	
	// 添加函数 - SQLite
	ImportSQLite(s);
	
	// 添加函数 - MD4C
	//ImportMD4C(s);
	
	// 添加函数 - Other
	ImportOther(s);
	
}