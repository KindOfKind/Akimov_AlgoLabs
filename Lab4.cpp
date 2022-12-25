#include <iostream>
#include "MemoryAllocator.h"



int main()
{
	{
		// TEST FOR FIXED-SIZE ALLOCATOR 64

		FixedSizeAllocator allocator;
		allocator.Init(64);
		int* p64[12];

		// The first Fixed-Size Allocator's page is filled
		for (int i = 0; i < 10; i++)
		{
			p64[i] = (int*)allocator.Alloc();
		}

		allocator.DumpStat();

		// The second Fixed-Size Allocator's page is created
		p64[10] = (int*)allocator.Alloc();

		allocator.DumpStat();

		p64[11] = (int*)allocator.Alloc();

		allocator.DumpStat();

		allocator.Free(p64[0]);
		allocator.Free(p64[1]);

		allocator.DumpStat();

		for (int i = 2; i < 12; i++)
		{
			allocator.Free(p64[i]);
		}

		allocator.DumpStat();

		allocator.Destroy();
	}
	

	{
		// TEST FOR COALESCE ALLOCATOR

		MemoryAllocator allocator;
		allocator.bDumpStatOnlyCoalesceAllocator = true;
		allocator.Init();
		int* p1024[12];

		allocator.DumpStat();

		for (int i = 0; i < 10; i++)
		{
			p1024[i] = (int*)allocator.Alloc(1024);
		}

		allocator.DumpStat();

		p1024[10] = (int*)allocator.Alloc(1024);

		allocator.DumpStat();

		// First 3 Coalesce Allocator's blocks are merged
		allocator.Free(p1024[0]);
		allocator.Free(p1024[2]);
		allocator.Free(p1024[1]);
		allocator.Free(p1024[3]);
		allocator.DumpStat();


		// All Coalesce Allocator's blocks are merged into one
		for (int i = 4; i < 11; i++)
		{
			allocator.Free(p1024[i]);
		}

		allocator.DumpStat();

		allocator.Destroy();
	}
	

	{
		// TEST FOR OS-BASED ALLOCATOR

		MemoryAllocator allocator;
		allocator.Init();

		int* p1 = (int*)allocator.Alloc(1048576 * 2);
		*p1 = 20;

		std::cout << "\nOS-based allocator:\n" << *p1 << std::endl;

		allocator.Free(p1);

		allocator.Destroy();
	}

	return 0;
}