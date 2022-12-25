#include "MemoryAllocator.h"
#include "Constants.h"
#include <cassert>
#include <Windows.h>

MemoryAllocator::MemoryAllocator()
{
#ifdef _DEBUG
	_isInit = false;
	_isDestoyed = false;

	_freeBlocksNum = BLOCKS_PER_PAGE;
	_allocationsNumber = 0;

	bDumpStatOnlyCoalesceAllocator = false;
#endif
}

MemoryAllocator::~MemoryAllocator()
{
#ifdef _DEBUG
	assert((_isDestoyed, "Allocator Destroy function call is required before calling destructor."));
#endif
}

void MemoryAllocator::Init()
{
#ifdef _DEBUG
	_isInit = true;
	_isDestoyed = false;
#endif

	fsa16.Init(16);
	fsa32.Init(32);
	fsa64.Init(64);
	fsa128.Init(128);
	fsa256.Init(256);
	fsa512.Init(512);
	coalesceAllocator.Init();
}

void MemoryAllocator::Destroy()
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Destroy function."));
#endif

	fsa16.Destroy();
	fsa32.Destroy();
	fsa64.Destroy();
	fsa128.Destroy();
	fsa256.Destroy();
	fsa512.Destroy();
	coalesceAllocator.Destroy();

	// OS-based
	for (auto i : osMemBlocks)
	{
		VirtualFree(i.first, 0, MEM_RELEASE);
	}

#ifdef _DEBUG
	_isInit = false;
	_isDestoyed = true;
#endif
}

void* MemoryAllocator::Alloc(size_t size)
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Alloc function."));
#endif

	if (size <= 512)
	{
		return SelectFixedSizeAllocator(size)->Alloc();
	}
	if (size <= COALESCE_PAGE_SIZE)
	{
		return coalesceAllocator.Alloc(size);
	}
	
	// OS-based
	void* p = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#ifdef _DEBUG
	assert((p, "OS-based allocation error."));
	_freeBlocksNum -= 1;
#endif

	osMemBlocks.emplace(p, size);

	return p;
}

void MemoryAllocator::Free(void* p)
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Free function."));
#endif

	if (fsa16.Free(p))
		return;
	if (fsa32.Free(p))
		return;
	if (fsa64.Free(p))
		return;
	if (fsa128.Free(p))
		return;
	if (fsa256.Free(p))
		return;
	if (fsa512.Free(p))
		return;

	if (coalesceAllocator.Free(p))
		return;

	// OS-based
	osMemBlocks.erase(p);

	bool success = VirtualFree(p, 0, MEM_RELEASE);

#ifdef _DEBUG
	if (!success)
		assert((p, "OS-based Free call error."));

	_freeBlocksNum += 1;
#endif
}


#ifdef _DEBUG

void MemoryAllocator::DumpStat() const
{
	assert((_isInit, "Allocator initialization is required before calling DumpStat function."));

	if (bDumpStatOnlyCoalesceAllocator == false)
	{
		fsa16.DumpStat();
		fsa32.DumpStat();
		fsa64.DumpStat();
		fsa128.DumpStat();
		fsa256.DumpStat();
		fsa512.DumpStat();
	}

	coalesceAllocator.DumpStat();
}


void MemoryAllocator::DumpBlocks() const
{
	assert((_isInit, "Allocator initialization is required before calling DumpBlocks function."));

	fsa16.DumpBlocks();
	fsa32.DumpBlocks();
	fsa64.DumpBlocks();
	fsa128.DumpBlocks();
	fsa256.DumpBlocks();
	fsa512.DumpBlocks();

	coalesceAllocator.DumpBlocks();
}

#endif


FixedSizeAllocator* MemoryAllocator::SelectFixedSizeAllocator(size_t size)
{
	if (size <= 16)
		return &fsa16;
	if (size <= 32)
		return &fsa32;
	if (size <= 64)
		return &fsa64;
	if (size <= 128)
		return &fsa128;
	if (size <= 256)
		return &fsa256;
	if (size <= 512)
		return &fsa512;

	return nullptr;
}
