/**
 * DO NOT INCLUDE THIS FILE DIRECTLY. 
 * USE:
 *
 * #include "Core\Core.h"
 *
 */

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
/// </summary>
/// <param name="ptr">The pointer to aligned memory.</param>
inline void _aligned_free_plus(void* ptr)
{
	::free(reinterpret_cast<void **>(ptr)[-1]);
}

struct IMemoryAllocator
{
	virtual void *allocate(size_t bytes) = 0;
	virtual void deallocate(void *ptr) = 0;
};

namespace Allocators 
{
	struct Standard : public IMemoryAllocator
	{
		static size_t MaxAllocatedMemory;
		static size_t CurrentAllocatedMemory;
		static size_t TotalAllocateCommands;
		struct ExtraMemoryBlockInfo
		{
			size_t size;
		};

		void* allocate(size_t bytes) {
			++TotalAllocateCommands;
			CurrentAllocatedMemory += bytes;
			if (MaxAllocatedMemory < CurrentAllocatedMemory)
			{
				MaxAllocatedMemory = CurrentAllocatedMemory;
			}

			void *ptr = _aligned_malloc_plus(bytes, 4, sizeof(ExtraMemoryBlockInfo));
			ExtraMemoryBlockInfo *ext = reinterpret_cast<ExtraMemoryBlockInfo *>(reinterpret_cast<uintptr_t>(ptr) - sizeof(ExtraMemoryBlockInfo) - sizeof(void *));
			ext->size = bytes;
			return ptr;
		}

		void deallocate(void* ptr) {
			ExtraMemoryBlockInfo *ext = reinterpret_cast<ExtraMemoryBlockInfo *>(reinterpret_cast<uintptr_t>(ptr) - sizeof(ExtraMemoryBlockInfo) - sizeof(void *));
			CurrentAllocatedMemory -= ext->size;
			_aligned_free(ptr);
		}
	};

	struct Debug : public IMemoryAllocator
	{
		static size_t MaxAllocatedMemory;
		static size_t CurrentAllocatedMemory;
		static size_t TotalAllocateCommands;
		static U32 Counter;

		struct ExtraMemoryBlockInfo
		{
			U32 counter;
			size_t size;
			ExtraMemoryBlockInfo *prev;
			ExtraMemoryBlockInfo *next;
		};

		static ExtraMemoryBlockInfo *Last;

		void* allocate(size_t bytes) { 
			++TotalAllocateCommands; 
			CurrentAllocatedMemory += bytes; 
			if (MaxAllocatedMemory < CurrentAllocatedMemory)
			{
				MaxAllocatedMemory = CurrentAllocatedMemory;
			}
			
			void *ptr = _aligned_malloc_plus(bytes, 4, sizeof(ExtraMemoryBlockInfo));
			ExtraMemoryBlockInfo *ext = reinterpret_cast<ExtraMemoryBlockInfo *>(reinterpret_cast<uintptr_t>(ptr) - sizeof(ExtraMemoryBlockInfo) - sizeof(void *));
			ext->size = bytes;
			ext->counter = ++Counter;
			ext->next = nullptr;
			ext->prev = Last;
			if (Last)
				Last->next = ext;
			Last = ext;

			TRACE ("++ [" << ext->counter <<  "]: allocate(" << ext->size << ")\n");
			return ptr;
		}

		void deallocate(void* ptr) { 
			ExtraMemoryBlockInfo *ext = reinterpret_cast<ExtraMemoryBlockInfo *>(reinterpret_cast<uintptr_t>(ptr) - sizeof(ExtraMemoryBlockInfo) - sizeof(void *));
			CurrentAllocatedMemory -= ext->size;
			if (ext->next)
				ext->next->prev = ext->prev;
			if (ext->prev)
				ext->prev->next = ext->next;
			if (Last == ext)
				Last = ext->prev;

			TRACE("-- [" << ext->counter << "]: deallocate(" << ext->size << ")\n");
			_aligned_free(ptr);
		}

		static bool list(void **current, size_t *size, size_t *counter, void **data)
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

	struct Alligned16Bytes : public IMemoryAllocator
	{
		void* allocate(size_t bytes) { return _aligned_malloc(bytes, 16); }
		void deallocate(void* ptr) { _aligned_free(ptr); }
	};

	/**
	 * Buffer size is limited to 2GB
	 */
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

	

	typedef Debug Default;

//	typedef Alligned16Bytes Default;
	typedef Alligned16Bytes Stack;
	typedef Alligned16Bytes Frame;
	typedef Alligned16Bytes Blocks;
}

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



template<typename T = Allocators::Default>
class MemoryAllocatorGlobal
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
MemoryAllocator<T> MemoryAllocatorGlobal<T>::_memoryAllocator;
