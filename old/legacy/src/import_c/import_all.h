


#include "import_xrt.h"
#include "import_libtcc.h"
#include "import_xrt_net.h"
#include "import_sqlite.h"
#include "import_xdo.h"
#include "import_xtp.h"
#include "import_other.h"



void ImportAll(TCCState* s)
{
	
	// 添加函数 - X Runtime
	ImportXRT(s);
	
	// 添加函数 - libtcc
	ImportLibTCC(s);
	
	// 添加函数 - xrt Network API
	ImportXrtNet(s);
	
	// 添加函数 - SQLite
	ImportSQLite(s);
	
	// 添加函数 - XDO
	ImportXDO(s);
	
	// 添加函数 - Other
	ImportOther(s);
	
	// 添加函数 - XTP
	ImportXTP(s);
	
	// 最后，把自己也添加进去，方便脚本环境里，创建同等的运行环境
	tcc_add_symbol(s, "ImportAll", ImportAll);
	
}
