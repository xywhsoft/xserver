#ifndef XS_RUNTIME_BYTE_BUDGET_H
#define XS_RUNTIME_BYTE_BUDGET_H

/* A reservation survives until its owning allocation is released. */
typedef struct XS_ByteBudget {
	xatomic64 Used;
	uint64 Limit;
} XS_ByteBudget;

static inline void XS_ByteBudgetInit(XS_ByteBudget* pBudget, uint64 iLimit)
{
	xrtAtomic64Init(&pBudget->Used, 0);
	pBudget->Limit = iLimit;
}

static inline bool XS_ByteBudgetReserve(XS_ByteBudget* pBudget, uint64 iBytes)
{
	uint64 iUsed = xrtAtomic64Load(&pBudget->Used, XMEMORY_ACQUIRE);
	for ( ;; ) {
		if ( iUsed > pBudget->Limit || iBytes > pBudget->Limit - iUsed ) return false;
		if ( xrtAtomic64CompareExchange(&pBudget->Used, &iUsed, iUsed + iBytes,
			XMEMORY_ACQ_REL, XMEMORY_ACQUIRE) ) return true;
	}
}

static inline bool XS_ByteBudgetRelease(XS_ByteBudget* pBudget, uint64 iBytes)
{
	uint64 iUsed = xrtAtomic64Load(&pBudget->Used, XMEMORY_ACQUIRE);
	for ( ;; ) {
		if ( iBytes > iUsed ) return false;
		if ( xrtAtomic64CompareExchange(&pBudget->Used, &iUsed, iUsed - iBytes,
			XMEMORY_ACQ_REL, XMEMORY_ACQUIRE) ) return true;
	}
}

#endif
