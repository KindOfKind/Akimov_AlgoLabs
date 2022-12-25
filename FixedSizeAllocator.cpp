#include <Windows.h>
#include <iostream>
#include <cassert>
#include "FixedSizeAllocator.h"
#include "Constants.h"

using namespace FSA;


FixedSizeAllocator::FixedSizeAllocator()
{
#ifdef _DEBUG
	_isInit = false;
	_isDestoyed = false;
	_freeBlocksNum = BLOCKS_PER_PAGE;
#endif

	_blockSize = 0;
}

FixedSizeAllocator::~FixedSizeAllocator()
{
#ifdef _DEBUG
	assert((_isDestoyed, "Allocator Destroy function call is required before calling destructor."));
#endif
}


void FixedSizeAllocator::Init(size_t blockSize)
{
#ifdef _DEBUG
	_isInit = true;
	_isDestoyed = false;
#endif

	_blockSize = blockSize;
	AllocNewPage();
}


PageHeader* FixedSizeAllocator::AllocNewPage()
{
	PageHeader* newPage = (PageHeader*)(VirtualAlloc(NULL, _blockSize * BLOCKS_PER_PAGE + sizeof(PageHeader),
																	MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	if (newPage)
	{
		newPage->next = nullptr;
		newPage->firstBlock = ((byte*)(newPage)+sizeof(PageHeader));
		newPage->firstFreeBlockIdx = -1;
		newPage->blocksInit = 0;

		newPage->next = _firstPage;
		_firstPage = newPage;
		
		//// ToDo modify this by initializing blocks when allocating
		//for (int i = 0; i < BLOCKS_PER_PAGE - 1; i++)
		//{
		//	// Set the payload of a free block to it's following free block index
		//	int* blockPayload = (int*)GetMemoryBlockPayloadPointer(newPage, i);
		//	*blockPayload = i + 1;
		//}
		//*(int*)GetMemoryBlockPayloadPointer(newPage, BLOCKS_PER_PAGE - 1) = -1;	// The last block contains index -1

#ifdef _DEBUG
		_freeBlocksNum += BLOCKS_PER_PAGE;
#endif
	}

	return newPage;
}


void FixedSizeAllocator::Destroy()
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


void* FixedSizeAllocator::Alloc()
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Alloc function."));
	_freeBlocksNum -= 1;
#endif

	PageHeader* pageHeader = _firstPage;
	int freeBlockIdx = -1;

	while (pageHeader)
	{
		if (pageHeader->blocksInit < BLOCKS_PER_PAGE)
		{
			pageHeader->blocksInit += 1;
			return GetMemoryBlockPayloadPointer(pageHeader, pageHeader->blocksInit - 1);
		}

		if (pageHeader->firstFreeBlockIdx > -1)
		{
			int freeBlockIdx = pageHeader->firstFreeBlockIdx;
			// The second block from free-list becomes the first one (or it's set to -1 if there are no other free blocks)
			pageHeader->firstFreeBlockIdx = *(int*)GetMemoryBlockPayloadPointer(pageHeader, pageHeader->firstFreeBlockIdx);
			return GetMemoryBlockPayloadPointer(pageHeader, freeBlockIdx);
		}

		pageHeader = pageHeader->next;
	}

	pageHeader = AllocNewPage();
	pageHeader->blocksInit += 1;
	return GetMemoryBlockPayloadPointer(pageHeader, 0);
}


bool FixedSizeAllocator::Free(void* p)
{
#ifdef _DEBUG
	assert((_isInit, "Allocator initialization is required before calling Free function."));
#endif

	PageHeader* pageHeader = _firstPage;

	while (pageHeader)
	{
		if ((p >= pageHeader->firstBlock) && (p < (void*)((byte*)(pageHeader->firstBlock) + _blockSize * BLOCKS_PER_PAGE)))
		{
			*(int*)p = pageHeader->firstFreeBlockIdx;
			pageHeader->firstFreeBlockIdx = (int)((byte*)p - (byte*)(pageHeader->firstBlock)) / _blockSize;

#ifdef _DEBUG
			_freeBlocksNum += 1;
#endif

			return true;
		}

		pageHeader = pageHeader->next;
	}

	return false;
}


PageHeader* FixedSizeAllocator::GetLastPage()
{
	if (!_firstPage)
		return nullptr;
	else
	{
		PageHeader* tmp = _firstPage;
		while (tmp->next) tmp = tmp->next;
		return tmp;
	}
}


inline void* FixedSizeAllocator::GetMemoryBlockPayloadPointer(PageHeader* pageHeader, int blockIdx) const
{
	return ((byte*)(pageHeader->firstBlock) + (blockIdx * _blockSize));
}



#ifdef _DEBUG

void FixedSizeAllocator::DumpStat() const
{
	assert((_isInit, "Allocator initialization is required before calling DumpStat function."));

	std::cout << std::endl << "------\n" << "FixedSizeAllocator for " << _blockSize << " bytes blocks:\n";
	
	int pagesNum = 0;
	int allocatedBlocks = 0;
	int freeBlocks = 0;
	PageHeader* pageHeader = _firstPage;

	std::cout << "\nFree blocks list:\n";
	while (pageHeader)
	{
		pagesNum += 1;
		allocatedBlocks += pageHeader->blocksInit;

		int freeBlockIdx = pageHeader->firstFreeBlockIdx;
		while (freeBlockIdx != -1)
		{
			std::cout << GetMemoryBlockPayloadPointer(pageHeader, freeBlockIdx) << std::endl;
			freeBlockIdx = *(int*)GetMemoryBlockPayloadPointer(pageHeader, freeBlockIdx);

			freeBlocks += 1;
			allocatedBlocks -= 1;
		}
		pageHeader = pageHeader->next;
	}

	std::cout << std::endl << "Pages number: " << pagesNum << std::endl;
	std::cout << std::endl << "Blocks allocated: " << allocatedBlocks << "; free blocks: " << freeBlocks << std::endl;
	std::cout << "------\n";
}


void FixedSizeAllocator::DumpBlocks() const
{
	assert((_isInit, "Allocator initialization is required before calling DumpBlocks function."));

	std::cout << std::endl << "------\n" << "FixedSizeAllocator for " << _blockSize << " bytes blocks:\n";

	PageHeader* pageHeader = _firstPage;

	std::cout << "\nFree blocks list:\n";
	while (pageHeader)
	{
		int freeBlockIdx = pageHeader->firstFreeBlockIdx;
		while (freeBlockIdx != -1)
		{
			std::cout << GetMemoryBlockPayloadPointer(pageHeader, freeBlockIdx) << std::endl;
			freeBlockIdx = *(int*)GetMemoryBlockPayloadPointer(pageHeader, freeBlockIdx);
		}
		pageHeader = pageHeader->next;
	}
	std::cout << "------\n";
}

#endif
