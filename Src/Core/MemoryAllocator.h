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
	void* data = ::malloc(size + alignment + sizeof(void*));
	if (data == nullptr)
		return nullptr;

	uintptr_t alignedData = reinterpret_cast<uintptr_t>(data) + sizeof(void*) + alignment;
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

struct IMemoryAllocator
{
	virtual void *allocate(size_t bytes) = 0;
	virtual void deallocate(void *ptr) = 0;
};

namespace Allocators 
{
	struct Alligned16Bytes : public IMemoryAllocator
	{
		void* allocate(size_t bytes) { return _aligned_malloc(bytes, 16); }
		void deallocate(void* ptr) { _aligned_free(ptr); }
	};

	typedef Alligned16Bytes Default;
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
			p->~T();
			deallocate(p);
		}
	}

//	IMemoryAllocator * operator() { return &_allocator; }
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
	inline static void make_delete(T *p)
	{
		if (p)
		{
			p->~T();
			deallocate(p);
		}
	}

	//	IMemoryAllocator * operator() { return &_allocator; }
};

template<typename T>
MemoryAllocator<T> MemoryAllocatorGlobal<T>::_memoryAllocator;

/*
class MemoryAllocatorUni
{
private:
	IMemoryAllocator *_pAllocator;

public:
	MemoryAllocatorUni(IMemoryAllocator *pAllocator) : _pAllocator(pAllocator) {}

	inline void* allocate(size_t bytes)
	{
		return _pAllocator->allocate(bytes);
	}

	inline void deallocate(void* ptr)
	{
		_pAllocator->deallocate(ptr);
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
		if (p) {
			p->~T();
			deallocate(p);
		}
	}
};
/*
namespace Allocators {


	struct Alligned16Bytes
	{
		inline void* allocate(size_t bytes)
		{
			return _aligned_malloc(bytes, 16);
		}

		inline void deallocate(void* ptr)
		{
			_aligned_free(ptr);
		}
	};

	typedef Alligned16Bytes Default;

	struct Stack
	{
		inline void* allocate(size_t bytes)
		{
			return _aligned_malloc(bytes, 16);
		}

		inline void deallocate(void* ptr)
		{
			_aligned_free(ptr);
		}
	};

	struct Blocks
	{
		inline void* allocate(size_t bytes)
		{
			return _aligned_malloc(bytes, 16);
		}

		inline void deallocate(void* ptr)
		{
			_aligned_free(ptr);
		}
	};

	struct Linear
	{
		inline void* allocate(size_t bytes)
		{
			return _aligned_malloc(bytes, 16);
		}

		inline void deallocate(void* ptr)
		{
			_aligned_free(ptr);
		}
	};

}


/// <summary>
/// Simple memory allocator template class. Should be derived to provide selected memory allocator. ATL style.
/// </summary>
template<class T = Allocators::Default>
class MemoryAllocatorImpl : private T
{

public:
	inline void* allocate(size_t bytes)
	{
		return T::allocate(bytes);
	}

	inline void deallocate(void* ptr)
	{
		T::deallocate(ptr);
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
		if (p) {
			p->~T();
			deallocate(p);
		}
	}
};

/// <summary>
/// Simple memory allocator.
/// </summary>
template<class T = Allocators::Default>
class MemoryAllocator
{
private:
	static MemoryAllocatorImpl<T> _alloc;

public:
	inline static void* allocate(size_t bytes) { return _alloc.allocate(bytes); }
	inline static void deallocate(void *ptr) { _alloc.deallocate(ptr); }
};

template<class T>
MemoryAllocatorImpl<T> MemoryAllocator<T>::_alloc;


/**
 * We dont use STL types. So we don't need STL allocator.
 *

/// <summary>
/// STL style allocator. Can be used with vector or string. Second template arg can be used to customize memory allocation pattern.
/// </summary>
template <typename T, class Alloc = Allocators::Default>
class stl_allocator : public MemoryAllocatorImpl<Alloc>
{
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	stl_allocator() {}
	~stl_allocator() {}

	template <class U> struct rebind { typedef stl_allocator<U> other; };
	template <class U> stl_allocator(const stl_allocator<U>&) {}

	pointer address(reference x) const { return &x; }
	const_pointer address(const_reference x) const { return &x; }
	size_type max_size() const throw() { return size_t(-1) / sizeof(value_type); }

	pointer allocate(size_type n, const void* = 0)
	{
		return static_cast<pointer>(MemoryAllocatorImpl<Alloc>::allocate(n * sizeof(T)));
	}

	void deallocate(pointer p, size_type n)
	{
		MemoryAllocatorImpl<Alloc>::deallocate(p);
	}

	void construct(pointer p, const T& val)
	{
		new(static_cast<void*>(p)) T(val);
	}

	void construct(pointer p)
	{
		new(static_cast<void*>(p)) T();
	}

	void destroy(pointer p)
	{
		p->~T();
	}
};

*/