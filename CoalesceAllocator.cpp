#include <Windows.h>
#include <iostream>
#include <cassert>
#include "CoalesceAllocator.h"
#include "Constants.h"

using Coalesce::MemoryBlockHeader;
using Coalesce::PageHeader;


CoalesceAllocator::CoalesceAllocator()
{
#ifdef _DEBUG
	_isInit = false;
	_isDestoyed = false;
	_freeBlocksNum = BLOCKS_PER_PAGE;
#endif
}

CoalesceAllocator::~CoalesceAllocator()
{
#ifdef _DEBUG
	assert((_isDestoyed, "Allocator Destroy function call is required before calling destructor."));
#endif
}


void CoalesceAllocator::Init()
{
#ifdef _DEBUG
	_isInit = true;
	_isDestoyed = false;
#endif

	_firstPage = AllocNewPage(COALESCE_PAGE_SIZE);
}


PageHeader* CoalesceAllocator::AllocNewPage(size_t size)
{
	PageHeader* newPage = (PageHeader*)(VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	if (newPage)
	{
		newPage->next = nullptr;
		newPage->firstBlock = newPage->firstFreeBlock = (MemoryBlockHeader*)((byte*)(newPage)+sizeof(PageHeader));

		newPage->firstBlock->next = nullptr;
		newPage->firstBlock->prev = nullptr;
		newPage->firstBlock->nextFree = nullptr;
		newPage->firstBlock->prevFree = nullptr;
		newPage->firstBlock->bIsUsed = false;
		newPage->firstBlock->size = size - sizeof(PageHeader) - sizeof(MemoryBlockHeader);

		newPage->next = _firstPage;
		_firstPage = newPage;

#ifdef _DEBUG
		_freeBlocksNum += 1;
#endif
	}

	return newPage;
}


void CoalesceAllocator::Destroy()
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Destroy function."));
#endif
	
	// VirtualFree for all pages
	PageHeader* pageHeader = _firstPage;
	if (pageHeader)
	{
		PageHeader* nextPageHeader = _firstPage->next;
		while (nextPageHeader)
		{
			VirtualFree((void*)pageHeader, 0, MEM_RELEASE);
			pageHeader = nextPageHeader;
			nextPageHeader = pageHeader->next;
		}
		VirtualFree((void*)pageHeader, 0, MEM_RELEASE);
	}

#ifdef _DEBUG
	_isInit = false;
	_isDestoyed = true;
#endif
}


void* CoalesceAllocator::Alloc(size_t size)
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Alloc function."));
#endif

	// Searching for the first-fit free memory block on any page.
	PageHeader* pageHeader = _firstPage;

	while (pageHeader)
	{
		MemoryBlockHeader* freeBlock = pageHeader->firstFreeBlock;
		while (freeBlock)
		{
			if (freeBlock->bIsUsed == true)
			{
				freeBlock = freeBlock->next;
				continue;
			}

			if (freeBlock->size >= size)
			{
				MemoryBlockHeader* splitBlock = SplitMemoryBlock(freeBlock, size);

				if (pageHeader->firstFreeBlock == freeBlock)
					pageHeader->firstFreeBlock = splitBlock;

				return GetMemoryBlockPayloadPointer(freeBlock);
			}

			freeBlock = freeBlock->next;
		}

		pageHeader = pageHeader->next;
	}

	//If no memory block was found
	pageHeader = AllocNewPage(COALESCE_PAGE_SIZE);
	SplitMemoryBlock(pageHeader->firstBlock, size);

	return GetMemoryBlockPayloadPointer(pageHeader->firstBlock);
}


MemoryBlockHeader* CoalesceAllocator::SplitMemoryBlock(MemoryBlockHeader* blockH, size_t size)
{
	MemoryBlockHeader* firstBlockH = blockH;

	MemoryBlockHeader* secondBlockH = (MemoryBlockHeader*)((byte*)firstBlockH + size + sizeof(MemoryBlockHeader));

	secondBlockH->next = firstBlockH->next;
	secondBlockH->prev = firstBlockH;
	secondBlockH->nextFree = firstBlockH->nextFree;
	secondBlockH->prevFree = firstBlockH->prevFree;
	secondBlockH->size = firstBlockH->size - size;
	secondBlockH->bIsUsed = false;

	firstBlockH->next = secondBlockH;
	firstBlockH->nextFree = nullptr;
	firstBlockH->prevFree = nullptr;
	firstBlockH->size = size;
	firstBlockH->bIsUsed = true;

	if (secondBlockH->prev)
		secondBlockH->prev->next = secondBlockH;
	if (secondBlockH->next)
		secondBlockH->next->prev = secondBlockH;

	if (secondBlockH->prevFree)
		secondBlockH->prevFree->nextFree = secondBlockH;
	if (secondBlockH->nextFree)
		secondBlockH->nextFree->prevFree = secondBlockH;

	return secondBlockH;
}


bool CoalesceAllocator::Free(void* p)
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Free function."));
#endif

	PageHeader* pageHeader = _firstPage;

	while (pageHeader)
	{
		if ((p >= pageHeader->firstBlock) && (p < (void*)((byte*)(pageHeader) + COALESCE_PAGE_SIZE)))
		{
			MemoryBlockHeader* blockToFree = (MemoryBlockHeader*)((byte*)p - sizeof(MemoryBlockHeader));
			blockToFree->nextFree = pageHeader->firstFreeBlock;
			blockToFree->prevFree = nullptr;
			pageHeader->firstFreeBlock = blockToFree;
			blockToFree->bIsUsed = false;

			MergeAdjacentMemoryBlocks(pageHeader, blockToFree);

#ifdef _DEBUG
			_freeBlocksNum += 1;
#endif

			return true;
		}

		pageHeader = pageHeader->next;
	}

	return false;
}


void CoalesceAllocator::MergeAdjacentMemoryBlocks(PageHeader* pageH, MemoryBlockHeader* blockH)
{
	if (blockH->prev && !(blockH->prev->bIsUsed))
	{
		// Страшный пережиток прошлого, но для отчётности оставил. Считайте, что нашли пасхалку :)

		//if (pageH->firstFreeBlock->nextFree)
		//{
		//	// Searching for a block in free-list which is followed by blockH->prev to correctly change free-list.
		//	// It would be easier if we had doubly linked free-list. I should probably redo it this way...
		//	MemoryBlockHeader* blockIter = pageH->firstFreeBlock;

		//	while (blockH->prev && blockIter->nextFree && blockIter->nextFree != blockH->prev)
		//		blockIter = blockIter->nextFree;

		//	//(blockH->prev is now the head and can't be next to any other block). It's a measure against loops in free-list.
		//	if (blockIter == blockH->prev->nextFree)
		//		blockIter->nextFree = nullptr;
		//	else
		//		blockIter->nextFree = blockH->prev->nextFree;
		//}

		//if (pageH->firstFreeBlock == blockH)
		//	pageH->firstFreeBlock = blockH->prev;

		if (blockH->next)
			blockH->next->prev = blockH->prev;

		if (pageH->firstFreeBlock == blockH)
			pageH->firstFreeBlock = blockH->prev;

		if (blockH->prevFree)
			blockH->prevFree->nextFree = blockH->nextFree;
		if (blockH->nextFree)
			blockH->nextFree->prevFree = blockH->prevFree;

		blockH->prev->next = blockH->next;
		blockH->prev->size += blockH->size;

		blockH = blockH->prev;
	}

	if (blockH->next && !(blockH->next->bIsUsed))
	{

		if (blockH->next->next)
			blockH->next->next->prev = blockH;

		if (blockH->next->prevFree)
			blockH->next->prevFree->nextFree = blockH->next->nextFree;
		if (blockH->next->nextFree)
			blockH->next->nextFree->prevFree = blockH->next->prevFree;

		blockH->size += blockH->next->size;
		blockH->next = blockH->next->next;
	}
}


inline void* CoalesceAllocator::GetMemoryBlockPayloadPointer(MemoryBlockHeader* blockH) const
{
	return ((byte*)(blockH) + sizeof(MemoryBlockHeader));
}



#ifdef _DEBUG

void CoalesceAllocator::DumpStat() const
{
	assert((_isInit, "Allocator initialization is required before calling DumpStat function."));

	std::cout << std::endl << "------\n" << "CoalesceAllocator:\n";

	int pagesNum = 0;
	int allocatedBlocks = 0;
	int freeBlocks = 0;
	int freeBytes = 0;
	PageHeader* pageHeader = _firstPage;

	std::cout << "\nFree blocks list:\n";
	while (pageHeader)
	{
		pagesNum += 1;

		MemoryBlockHeader* blockH = pageHeader->firstBlock;
		while (blockH)
		{
			if (blockH->bIsUsed)
			{
				allocatedBlocks += 1;
			}
			else
			{
				freeBlocks += 1;
				freeBytes += blockH->size;

				std::cout << GetMemoryBlockPayloadPointer(blockH) << " : " << blockH->size << " bytes;\n";
			}

			blockH = blockH->next;
		}

		pageHeader = pageHeader->next;
	}

	std::cout << std::endl << "Pages number: " << pagesNum << std::endl;
	std::cout << std::endl << "Blocks allocated: " << allocatedBlocks << "; free blocks: " << freeBlocks << std::endl;
	std::cout << std::endl << "FreeMemory: " << freeBytes << " bytes." << std::endl;

	std::cout << "------\n";
}


void CoalesceAllocator::DumpBlocks() const
{
	assert((_isInit, "Allocator initialization is required before calling DumpBlocks function."));


	std::cout << std::endl << "------\n" << "CoalesceAllocator blocks:\n";

	int pagesNum = 0;
	PageHeader* pageHeader = _firstPage;
	while (pageHeader)
	{
		pagesNum += 1;

		MemoryBlockHeader* blockH = pageHeader->firstBlock;
		while (blockH)
		{
			if (blockH->bIsUsed)
			{
				std::cout << GetMemoryBlockPayloadPointer(blockH) << " : " << blockH->size << " bytes : Allocated;\n";
			}
			else
			{
				std::cout << GetMemoryBlockPayloadPointer(blockH) << " : " << blockH->size << " bytes : Free;\n";
			}

			blockH = blockH->next;
		}

		pageHeader = pageHeader->next;
	}

	std::cout << "------\n";
}

#endif
