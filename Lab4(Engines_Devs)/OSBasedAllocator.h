#pragma once

class OSBasedAllocator
{
private:


public:

	OSBasedAllocator() {};

	~OSBasedAllocator() {};

	virtual void* Alloc(size_t size) { return nullptr; };
	virtual void Free(void* p) {};
};
