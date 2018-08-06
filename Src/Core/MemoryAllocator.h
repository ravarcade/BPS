/**
 * DO NOT INCLUDE THIS FILE DIRECTLY. 
 * USE:
 *
 * #include "Core\Core.h"
 *
 */

 /// <summary>
 /// Standard interface for allocators.
 /// </summary>
struct IMemoryAllocator
{
	virtual void *allocate(size_t bytes) = 0;
	virtual void deallocate(void *ptr) = 0;
};

// ============================================================================ allocation & deallocacation functions ===
// Thay can be called only from alloctar classes. So we keep it in pivate class.

namespace MemmoryAllocatorsPrivate {

	/// <summary>
	/// Allocate memory aligned to requested number of bytes (but number must be power of 2).
	/// </summary>
	/// <param name="size">The size in bytes.</param>
	/// <param name="alignment">The alignment in bytes (power of 2).</param>
	/// <returns>The pointer to new aligned memory.</returns>
	inline void* _aligned_malloc(size_t size, size_t alignment)
	{
		--alignment;
		size_t extraDataSize = sizeof(void *);
		void* data = ::malloc(size + alignment + extraDataSize);
		if (data == nullptr)
			return nullptr;

		uintptr_t alignedData = reinterpret_cast<uintptr_t>(data) + extraDataSize + alignment;
		alignedData &= (uintptr_t(-1) - alignment);


		reinterpret_cast<void **>(alignedData)[-1] = data;
		return reinterpret_cast<void *>(alignedData);
	}

	/// <summary>
	/// Free allocated memory.
	/// </summary>
	/// <param name="ptr">The pointer to aligned memory.</param>
	inline void _aligned_free(void* ptr)
	{
		::free(reinterpret_cast<void **>(ptr)[-1]);
	}

	/// <summary>
	/// Allocate memory aligned to requested number of bytes (but number must be power of 2).
	/// </summary>
	/// <param name="size">The size in bytes.</param>
	/// <param name="alignment">The alignment in bytes (power of 2).</param>
	/// <param name="extraDataSize">Size of extrea spece before allocated memory. That space may be used to tag memory.</param>
	/// <returns>The pointer to new aligned memory.</returns>
	inline void* _aligned_malloc_plus(size_t size, size_t alignment, size_t extraDataSize)
	{
		--alignment;
		extraDataSize += sizeof(void *);
		void* data = ::malloc(size + alignment + extraDataSize);
		if (data == nullptr)
			return nullptr;

		uintptr_t alignedData = reinterpret_cast<uintptr_t>(data) + extraDataSize + alignment;
		alignedData &= (uintptr_t(-1) - alignment);

		reinterpret_cast<void **>(alignedData)[-1] = data;
		return reinterpret_cast<void *>(alignedData);
	}

	/// <summary>
	/// Free allocated memory.
	/// It is exacly same thing as _aligned_free. No differece. We want to keep "symmetry": 
	/// _aligned_malloc & _aligned_free => _aligned_malloc_plus & _aligned_free_plus
	/// </summary>
	/// <param name="ptr">The pointer to aligned memory.</param>
	inline void _aligned_free_plus(void* ptr)
	{
		::free(reinterpret_cast<void **>(ptr)[-1]);
	}

	/// <summary>
	/// Simplest memory allocator.
	/// </summary>
	struct Alligned16Bytes : public IMemoryAllocator
	{
		void* allocate(size_t bytes) { return MemmoryAllocatorsPrivate::_aligned_malloc(bytes, 16); }
		void deallocate(void* ptr) { MemmoryAllocatorsPrivate::_aligned_free(ptr); }
	};

	/// <summary>
	/// It have only some basic statistics.
	/// In release build we may want to keep tracking alloctions.
	/// </summary>
	struct StandardAllocator : public IMemoryAllocator
	{
		void* allocate(size_t bytes) { return MemmoryAllocatorsPrivate::_aligned_malloc(bytes, 4); }
		void deallocate(void* ptr) { MemmoryAllocatorsPrivate::_aligned_free(ptr); }
	};

	/// <summary>
	/// Container of statistic data about memory allocation.
	/// </summary>
	struct DebugStatistics
	{
		struct ExtraMemoryBlockInfo
		{
			U32 counter;
			size_t size;
			ExtraMemoryBlockInfo *prev;
			ExtraMemoryBlockInfo *next;
		};

		size_t MaxAllocatedMemory;
		size_t CurrentAllocatedMemory;
		size_t TotalAllocateCommands;
		U32 Counter;
		ExtraMemoryBlockInfo *Last;

		DebugStatistics() :
			MaxAllocatedMemory(0),
			CurrentAllocatedMemory(0),
			TotalAllocateCommands(0),
			Counter(0),
			Last(nullptr)
		{}

		bool list(void **current, size_t *size, size_t *counter, void **data)
		{
			if (current == nullptr)
				return false;

			ExtraMemoryBlockInfo * f;
			if (*current == nullptr)
			{
				f = Last;
			}
			else
			{
				f = reinterpret_cast<ExtraMemoryBlockInfo *>(*current);
				if (f)
					f = f->prev;
			}

			if (f == nullptr)
				return false;

			if (size)
				*size = f->size;
			if (counter)
				*counter = f->counter;
			if (data)
				*data = f + 1;
			*current = f;

			return true;
		}
	};

	extern DebugStatistics DebugStats;

	/// <summary>
	/// Helper template class used to convert "normal" memory allocator to "debug" version with memory allocation logging.
	/// </summary>
	template<class T>
	struct DebugAllocator : public T
	{
		template<class... Args>
		inline DebugAllocator(Args &&...args) : T(std::forward<Args>(args)...)
		{}

		void* allocate(size_t bytes)
		{
			++DebugStats.TotalAllocateCommands;
			DebugStats.CurrentAllocatedMemory += bytes;
			if (DebugStats.MaxAllocatedMemory < DebugStats.CurrentAllocatedMemory)
			{
				DebugStats.MaxAllocatedMemory = DebugStats.CurrentAllocatedMemory;
			}
			void *ptr = T::allocate(bytes + sizeof(DebugStatistics::ExtraMemoryBlockInfo));
			auto ext = reinterpret_cast<DebugStatistics::ExtraMemoryBlockInfo *>(ptr);
			ext->size = bytes;
			ext->counter = ++DebugStats.Counter;
			ext->next = nullptr;
			ext->prev = DebugStats.Last;
			if (DebugStats.Last)
				DebugStats.Last->next = ext;
			DebugStats.Last = ext;

			TRACE("++ [" << ext->counter << "]: allocate(" << ext->size << ")\n");
			return ext + 1;
		}

		void deallocate(void* ptr) {
			auto ext = reinterpret_cast<DebugStatistics::ExtraMemoryBlockInfo *>(ptr) - 1;
			DebugStats.CurrentAllocatedMemory -= ext->size;
			if (ext->next)
				ext->next->prev = ext->prev;
			if (ext->prev)
				ext->prev->next = ext->next;
			if (DebugStats.Last == ext)
				DebugStats.Last = ext->prev;

			TRACE("-- [" << ext->counter << "]: deallocate(" << ext->size << ")\n");
			T::deallocate(ext);
		}
	};

	/// <summary>
	/// Real block memory allocator.
	/// </summary>
	struct BlocksAllocator : public IMemoryAllocator
	{
		/// <summary>
		/// Initializes a new instance of the <see cref="Blocks"/> memory allocatr.
		/// As alsways. Construction of allocator not allocate any extra memory.
		/// </summary>
		/// <param name="alloc">The alloctor.</param>
		/// <param name="size">The minimum block size. Default is 16 kB</param>
		BlocksAllocator(IMemoryAllocator *alloc, SIZE_T size = 16 * 1024) :
			alloc(alloc),
			minBlockSize(size),
			current(nullptr),
			firstFree(nullptr),
			blockSizeStep(4096), // 4kB step
			maxFreeBlocks(5)
		{}

		~BlocksAllocator()
		{
			for (auto p = current; p; )
			{
				auto m = p;
				p = p->prev;
				alloc->deallocate(m);
			}

			for (auto p = firstFree; p; )
			{
				auto m = p;
				p = p->next;
				alloc->deallocate(m);
			}
		}

		void* allocate(size_t bytes)
		{
			const int alignment = 4 - 1;

			// we need to store a point to block
			bytes += sizeof(void *) + alignment;
			bytes &= (uintptr_t(-1) - alignment);

			// allocNewBlock
			if (!current || current->free < bytes)
				allocNewBlock(bytes);

			BlockEntry **ret = reinterpret_cast<BlockEntry **>(current->mem);
			*ret = current;
			++ret;
			current->mem += bytes;
			current->free -= bytes;
			++current->counter;

			return ret;
		}

		void deallocate(void* ptr)
		{
			BlockEntry *block = reinterpret_cast<BlockEntry **>(ptr)[-1];
			--block->counter;
			if (block->counter == 0)
			{
				deallocateBlock(block);
			}
		}

		void stats()
		{
			U32 numBlocks = 0;
			U32 numFreeBlocks = 0;
			SIZE_T free = 0;
			SIZE_T usedMem = 0;
			SIZE_T unusedMem = 0;

			for (auto p = current; p; p = p->prev)
			{
				++numBlocks;
				unusedMem += p->free;
				usedMem += p->mem - reinterpret_cast<U8*>(p) - sizeof(BlockEntry);
			}

			for (auto p = firstFree; p; p = p->next)
			{
				++numBlocks;
				++numFreeBlocks;
				unusedMem += p->free;
			}

			if (current)
				free = current->free;

		}
	private:
		struct BlockEntry  // 5x4 = 20 bytes on 32 bit, 32 bytes on 64 bits (3x 8 +2x 4 = 32)
		{
			BlockEntry *next;
			BlockEntry *prev;
			U8 *mem;
			U32 counter;
			size_t free;
		};

		void allocNewBlock(size_t bytes)
		{
			const size_t reservedStandardMemoryAllocatorMemory = 64; // 64 bytes reserved for normal memory allocation
			bytes += sizeof(BlockEntry) + reservedStandardMemoryAllocatorMemory; // we need some extra memory to store block info & debug info.
			if (bytes < minBlockSize)
				bytes = minBlockSize;

			// Calc how big memory request will be.
			// We allocate memory in chunks
			bytes = static_cast<U32>((bytes + blockSizeStep - 1) / blockSizeStep) * blockSizeStep - reservedStandardMemoryAllocatorMemory;

			// try to find free block
			BlockEntry *newBlock = nullptr;
			for (auto p = firstFree; p; p = p->next)
			{
				if (p->free >= bytes)
				{
					newBlock = p;
					for (auto p = newBlock->next; p; p = p->next)
					{
						if (p->free >= bytes && p->free < newBlock->free)
						{
							newBlock = p;
						}
					}
					break;
				}
			}

			if (newBlock)
			{
				// remove from list of free blocks
				if (firstFree == newBlock)
					firstFree = newBlock->next;
				else
					newBlock->prev->next = newBlock->next;
			}
			else
			{
				newBlock = static_cast<BlockEntry*>(alloc->allocate(bytes));

				// put here allocation error.... or not.
			}


			if (current)
				current->next = newBlock;

			newBlock->next = nullptr;
			newBlock->prev = current;
			newBlock->counter = 0;
			newBlock->free = bytes - sizeof(BlockEntry);
			newBlock->mem = reinterpret_cast<U8*>(newBlock) + sizeof(BlockEntry);
			current = newBlock;
		}


		void deallocateBlock(BlockEntry *block)
		{
			// remove block from list of blocks
			if (block->prev)
				block->prev->next = block->next;
			if (block->next)
				block->next->prev = block->prev;

			// if it is current block, when set all for next block
			if (current == block)
				current = block->prev;

			// add block to list of free blocks
			if (firstFree)
				firstFree->prev = block;
			block->prev = nullptr;
			block->next = firstFree;
			firstFree = block;

			// here we decide to realy release some memory
			auto p = firstFree->next;
			U32 i = 0;
			for (; p && i <= maxFreeBlocks; ++i)
			{

				if (p && block->free > p->free)
					block = p;
				p = p->next;
			}

			// do we have too many unused blocks?
			if (i == maxFreeBlocks)
			{
				if (firstFree == block)
					firstFree = block->next;
				else
				{
					block->prev->next = block->next;
					block->next->prev = block->prev;
				}
				alloc->deallocate(block);
			}
		}

		BlockEntry *current;
		BlockEntry *firstFree;
		U32 maxFreeBlocks;
		size_t minBlockSize;
		size_t blockSizeStep;
		IMemoryAllocator *alloc;
	};

}

// ============================================================================ Allocators used in engine ===

namespace Allocators 
{

	 /// <summary>
	 /// Basic ring buffer. Size is limited to 2GB.
	 /// </summary>
	struct RingBuffer : public IMemoryAllocator
	{
		RingBuffer(IMemoryAllocator *alloc, SIZE_T size = 8192) : _alloc(alloc)
		{
			assert(size <= 0x7ffffffL); // 2 GB
			_buf_begin = reinterpret_cast<pointer>(_alloc->allocate(size));
			_buf_end = _buf_begin + size; _head = _tail = _buf_begin; 
			_last_assigned = nullptr; 
			_allocated_memory = 0;
			_memory_waiting_for_tail = 0;
			_counter = 0;
		};

		~RingBuffer() { _alloc->deallocate(_buf_begin); }

		void* allocate(size_t bytes) 
		{ 
			bytes = (bytes + _alignment_add) & _alignment_mask;

			// try to find space after _head before _buf_end
			pointer e = _tail > _head ? _tail : _buf_end;

			SIZE_T max = e - _head;
			if (_tail == _head && _allocated_memory > 0)
				max = 0;

			if (bytes > max && _tail < _head) // not enought space.... try at begin of buffer
			{
				// calc free space at begin of buffer
				max = _tail - _buf_begin;
				if (bytes <= max)
				{
					// now we have to move _head to begin of buffer and extend last allocated region
					if (_last_assigned)
					{
						if (*_last_assigned > 0)
						{
							_allocated_memory -= *_last_assigned;
							*_last_assigned = static_cast<size_type>(_buf_end - reinterpret_cast<pointer>(_last_assigned));
							_allocated_memory += *_last_assigned;
						}
						else
						{
							_memory_waiting_for_tail += *_last_assigned;
							*_last_assigned = -static_cast<size_type>(_buf_end - reinterpret_cast<pointer>(_last_assigned));
							_memory_waiting_for_tail -= *_last_assigned;
						}
					}

					_head = _buf_begin;
				}
			}

			// do we have needed free space?
			if (bytes > max)
				throw std::bad_alloc(); // no free space. PANIC!
			

			// we have needed free space after _head
			_last_assigned = reinterpret_cast<size_type *>(_head);
			*_last_assigned = static_cast<size_type>(bytes);
			auto ret = _head + sizeof(size_type);
			_head += bytes;
			if (_head == _buf_end)
				_head = _buf_begin;

			++_counter;
			_allocated_memory += static_cast<size_type>(bytes);
			return ret;
		}

		void deallocate(void* ptr) 
		{
			pointer p = reinterpret_cast<pointer>(ptr) - sizeof(size_type);
			size_type s = *reinterpret_cast<size_type *>(p);
			if (p != _tail) // no problem... we mark this block as "free" by setting negative size
			{
				_memory_waiting_for_tail += s;
				_allocated_memory -= s;
				*reinterpret_cast<size_type *>(p) = -s;
				return;
			}

			_allocated_memory -= s;
			p += s;
			for (;;)
			{
				if (p == _buf_end)
					p = _buf_begin;

				size_type s = *reinterpret_cast<size_type *>(p);
				if (s > 0)
					break;
				_memory_waiting_for_tail += s;
				if (p == _head)
				{
					_tail = _head = _buf_begin;
					_last_assigned = nullptr;
					return;
				}

				p -= s;
			}

			_tail = p;
		}

		void clear()
		{
			if (_tail == _head)
				_tail = _head = _buf_begin;
		}

		bool empty() { return _last_assigned == nullptr; }

		void *getTail() { return _tail + sizeof(size_type); }
		void *getHead() { return _head + sizeof(size_type); } // next possible allocated block
		SIZE_T getCapacity() { return _buf_end - _buf_begin; }

		SIZE_T getFree()
		{
			pointer e = _tail > _head ? _tail : _buf_end;

			SIZE_T max = e - _head;
			if (_tail < _head)
			{
				// calc free space at begin of buffer
				SIZE_T max2 = _tail - _buf_begin;
				if (max2 > max)
					max = max2;
			}

			return max;
		}
	private:
		typedef U8 * pointer;
		typedef I32 size_type;  // signed, because we use negative size to mark block "free"
		pointer _buf_begin, _buf_end, _head, _tail;
		size_type *_last_assigned;
		const uintptr_t alignment = 4;
		const uintptr_t _alignment_add = sizeof(size_type) + alignment - 1;
		const uintptr_t _alignment_mask = uintptr_t(0) - alignment ;
		size_type _allocated_memory;
		size_type _memory_waiting_for_tail;
		U32 _counter;
		IMemoryAllocator *_alloc;
	};

	
	typedef MemmoryAllocatorsPrivate::DebugAllocator<MemmoryAllocatorsPrivate::StandardAllocator> Default;
	typedef MemmoryAllocatorsPrivate::BlocksAllocator Blocks;

	typedef MemmoryAllocatorsPrivate::Alligned16Bytes Stack;
	typedef MemmoryAllocatorsPrivate::Alligned16Bytes Frame;
}

// ============================================================================ Helper classes ===

/// <summary>
/// We can have different memory allocator for different objects types.
/// One allocator / one object.
/// Two objects of same type will have two different allocators (but allocator are same type).
/// Usage: 
/// class nameOfObjectType : public MemoryAllocator<Allocators::Blocks>
/// class nameOfObjectType : public MemoryAllocator<Allocators::Default>
/// </summary>
template<typename T = Allocators::Default>
class MemoryAllocator 
{
private:
	T _allocator;

public:
	inline void* allocate(size_t bytes)
	{
		return _allocator.allocate(bytes);
	}

	inline void deallocate(void* ptr)
	{
		_allocator.deallocate(ptr);
	}

	template <class U>
	inline void allocArray(size_t arraySize, U*& result)
	{
		result = static_cast<U*>(allocate(sizeof(U) * arraySize));
	}

	template<class U, class... Args>
	inline U* make_new(Args &&...args)
	{
		return new (allocate(sizeof(U))) U(std::forward<Args>(args)...);
	}


	template <class T>
	inline void make_delete(void *p)
	{
		if (p) 
		{
			reinterpret_cast<T*>(p)->~T();
			deallocate(p);
		}
	}

	operator IMemoryAllocator*() { return &_allocator; }
};



/// <summary>
/// All object of give type will use same allocator.
/// So, two objects of same type will call same instance of allocator.
/// Usage: 
/// class nameOfObjectType : public MemoryAllocatorStatic<Allocators::Blocks>
/// class nameOfObjectType : public MemoryAllocatorStatic<Allocators::Default>
/// </summary>
template<typename T = Allocators::Default>
class MemoryAllocatorStatic
{
private:
	static MemoryAllocator<T> _memoryAllocator;

public:
	typedef T Allocator;

	inline static void* allocate(size_t bytes)
	{
		return _memoryAllocator.allocate(bytes);
	}

	inline static void deallocate(void* ptr)
	{
		_memoryAllocator.deallocate(ptr);
	}

	template <typename U>
	inline static U allocate(size_t bytes)
	{
		return static_cast<U>(_memoryAllocator.allocate(bytes));
	}

	template <class U>
	inline static void allocArray(size_t arraySize, U*& result)
	{
		result = static_cast<U*>(allocate(sizeof(U) * arraySize));
	}

	template<class U, class... Args>
	inline static U* make_new(Args &&...args)
	{
		return new (allocate(sizeof(U))) U(std::forward<Args>(args)...);
	}


	template <class T>
	inline static void make_delete(void *p)
	{
		if (p)
		{
			reinterpret_cast<T*>(p)->~T();
			deallocate(p);
		}
	}

	inline static IMemoryAllocator* GetMemoryAllocator() { return _memoryAllocator; }
};

template<typename T>
MemoryAllocator<T> MemoryAllocatorStatic<T>::_memoryAllocator;
