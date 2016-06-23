﻿#include "Type.h"
#include "Interrupt.h"

#include "Heap.h"

#define MEMORY_ALIGN	4

/*
堆分配原理：
1，使用内存块，记录当前块已使用大小以及下一块地址，形成内存块链表
2，每个内存块剩余空间是下一块指针减去当前块指针再减去已用大小
3，初始化第一个内存块，已使用0
4，申请内存时，从第一块开始找空余空间大于等于目标大小的内存块，割下来作为链表新节点
5，释放内存时，找到所在块，然后当前块上一块的Next指针直接指向当前块的下一块地址，移除当前节点
*/

/******************************** MemoryBlock ********************************/
// 内存块。大小和下一块地址
typedef struct MemoryBlock_
{
	uint	Used;
	struct MemoryBlock_*	Next;
} MemoryBlock;

/******************************** Heap ********************************/
Heap::Heap(uint addr, uint size)
{
	// 地址对齐
	uint end	= addr + size;
	addr	= (addr + MEMORY_ALIGN - 1) & (~(MEMORY_ALIGN - 1));
	end		= end & (~(MEMORY_ALIGN - 1));

	Address	= addr;
	Size	= end - addr;

	// 第一块内存块
	auto mb	= (MemoryBlock*)addr;
	mb->Used = sizeof(MemoryBlock);
	mb->Next = (MemoryBlock*)(end - sizeof(MemoryBlock));
	mb->Next->Used = sizeof(MemoryBlock);
	mb->Next->Next = nullptr;

	_Used	= sizeof(MemoryBlock) << 1;
	_Count	= 0;
}

uint Heap::Used()	const { return _Used; }
uint Heap::Count()	const { return _Count; }

void* Heap::Alloc(uint size)
{
	// 要申请的内存大小需要对齐
	size = (size+MEMORY_ALIGN-1) & (~(MEMORY_ALIGN-1));

	void* ret	= nullptr;
	auto need	= size + sizeof(MemoryBlock);

	SmartIRQ irq;
	for(auto mcb=(MemoryBlock*)Address; mcb->Next!=nullptr; mcb=mcb->Next)
	{
		// 找到一块满足大小的内存块
		auto free	= (uint)mcb->Next - (uint)mcb - mcb->Used;
		if(free >= need)
		{
			// 割一块出来
			auto tmp	= (MemoryBlock*)((uint)mcb + mcb->Used);
			tmp->Next	= mcb->Next;
			tmp->Used	= need;
			mcb->Next	= tmp;

			ret	= (void*)((uint)(tmp+1));

			_Used	+= need;
			_Count++;

			break;
		}
	}

	return ret;
}

void Heap::Free(void* ptr)
{
	auto prev	= (MemoryBlock*)Address;
	auto mcb	= (MemoryBlock*)ptr - 1;

	SmartIRQ irq;
	for(auto find=prev->Next; find->Next!=nullptr; find=find->Next)
	{
		if(find == mcb)
		{
			_Used	-= (uint)find->Next - (uint)prev->Next;
			_Count--;

			prev->Next = find->Next;

			break;
		}
		prev = find;
	}
}