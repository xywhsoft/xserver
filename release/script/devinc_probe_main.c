/*
 * dev_inc / dev_lib 门控探针：验证 host 级额外目录配置生效。
 * dev_inc 指向 release/devsdk/inc（含 dev_extra.h）。
 */
#include <xsbase.h>
#include <dev_extra.h>
#include <stdio.h>

void ServiceInit(XS_HostInfo* pHost)
{
	(void)pHost;
	printf("[devinc] probe %s (value=%d)\n",
		XS_DEV_INC_VALUE == 4242 ? "ok" : "FAIL", XS_DEV_INC_VALUE);
}

void ServiceUnit(XS_HostInfo* pHost)
{
	(void)pHost;
}
