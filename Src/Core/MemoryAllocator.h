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
	alignedData &= (uintptr_t(-1) - alignment);;


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

			_aligned_free(ptr); 
		}
	};

	struct Alligned16Bytes : public IMemoryAllocator
	{
		void* allocate(size_t bytes) { return _aligned_malloc(bytes, 16); }
		void deallocate(void* ptr) { _aligned_free(ptr); }
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

	IMemoryAllocator* GetMemoryAllocator() { return _memoryAllocator; }
};

template<typename T>
MemoryAllocator<T> MemoryAllocatorGlobal<T>::_memoryAllocator;
