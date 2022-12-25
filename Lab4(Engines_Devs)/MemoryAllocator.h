#pragma once

#include "CoalesceAllocator.h"
#include "FixedSizeAllocator.h"
#include "OSBasedAllocator.h"
#include <map>


class MemoryAllocator
{
public:

	bool bDumpStatOnlyCoalesceAllocator;

private:

#ifdef _DEBUG
	bool _isInit;
	bool _isDestoyed;

	int _freeBlocksNum;
	int _allocationsNumber;
#endif

	FixedSizeAllocator fsa16, fsa32, fsa64, fsa128, fsa256, fsa512;

	CoalesceAllocator coalesceAllocator;

	std::map<void*, int> osMemBlocks;	// (void*) Payload pointer, (int) size

	bool bIsInit;

public:

	MemoryAllocator();

	~MemoryAllocator();

	virtual void Init();
	virtual void Destroy();
	virtual void* Alloc(size_t size);
	virtual void Free(void* p);

#ifdef _DEBUG

	virtual void DumpStat() const;
	virtual void DumpBlocks() const;

#endif

private:

	FixedSizeAllocator* SelectFixedSizeAllocator(size_t size);

};
