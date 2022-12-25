#pragma once

namespace Coalesce
{
	// Structure that describes one block of memory, allocated by coaleace allocator.
	struct MemoryBlockHeader
	{
		MemoryBlockHeader* next;	// Next and previous blocks in a page.
		MemoryBlockHeader* prev;

		MemoryBlockHeader* nextFree;	// Next and previous blocks in a page.
		MemoryBlockHeader* prevFree;	// Next and previous blocks in a page.

		size_t size;
		bool bIsUsed;
	};

	struct PageHeader
	{
		PageHeader* next;
		MemoryBlockHeader* firstBlock;
		MemoryBlockHeader* firstFreeBlock;
	};
}


class CoalesceAllocator
{
private:

	Coalesce::PageHeader* _firstPage;

#ifdef _DEBUG
	bool _isInit;
	bool _isDestoyed;
	int _freeBlocksNum;
#endif

public:

	CoalesceAllocator();

	~CoalesceAllocator();

	void Init();
	void Destroy();
	virtual void* Alloc(size_t size);
	bool Free(void* p);

#ifdef _DEBUG

	void DumpStat() const;
	void DumpBlocks() const;

#endif

private:

	Coalesce::PageHeader* AllocNewPage(size_t size);
	Coalesce::MemoryBlockHeader* SplitMemoryBlock(Coalesce::MemoryBlockHeader* blockH, size_t size);
	void MergeAdjacentMemoryBlocks(Coalesce::PageHeader* pageH, Coalesce::MemoryBlockHeader* blockH);
	inline void* GetMemoryBlockPayloadPointer(Coalesce::MemoryBlockHeader* blockH) const;
};
