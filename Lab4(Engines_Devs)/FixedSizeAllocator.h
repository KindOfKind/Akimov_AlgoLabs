#pragma once

namespace FSA
{
	struct PageHeader
	{
		PageHeader* next;
		void* firstBlock;
		int firstFreeBlockIdx;	// -1 if no block were freed with Free method
		int blocksInit;
	};
}


class FixedSizeAllocator
{
private:

	FSA::PageHeader* _firstPage;
	size_t _blockSize;

#ifdef _DEBUG
	bool _isInit;
	bool _isDestoyed;
	int _freeBlocksNum;
#endif

public:

	FixedSizeAllocator();

	~FixedSizeAllocator();

	void Init(size_t blockSize);
	void Destroy();

	void* Alloc();
	bool Free(void* p);

#ifdef _DEBUG

	void DumpStat() const;
	void DumpBlocks() const;

#endif

private:

	FSA::PageHeader* AllocNewPage();
	FSA::PageHeader* GetLastPage();
	inline void* GetMemoryBlockPayloadPointer(FSA::PageHeader* pageHeader, int blockIdx) const;

};
