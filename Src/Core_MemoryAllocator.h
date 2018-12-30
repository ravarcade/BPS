/**
 * DO NOT INCLUDE THIS FILE DIRECTLY. 
 * USE:
 *
 * #include "Core.h"
 *
 */

namespace Allocators {

	/// <summary>
	/// Standard interface for allocators.
	/// </summary>
	extern "C" {
		struct IMemoryAllocator
		{
			virtual ~IMemoryAllocator() {}
			virtual void *allocate(size_t bytes) = 0;
			virtual void deallocate(void *ptr) = 0;

			enum {
				default = 0,
				block = 1,
				ringbuffer = 2
			};
		};
	}

	typedef IMemoryAllocator *&MemoryAllocator;

	BAMS_EXPORT extern IMemoryAllocator *default;
	

	template<MemoryAllocator alloc = Allocators::default>
	class Ext {
	public:
		inline static void* allocate(size_t bytes) { return alloc->allocate(bytes); }
		inline static void deallocate(void* ptr) { alloc->deallocate(ptr); }
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

		inline static IMemoryAllocator* GetMemoryAllocator() { return alloc; }
	};

	class MemoryAllocatorImpl
	{
	private:
		IMemoryAllocator *alloc;

	public:
		MemoryAllocatorImpl(IMemoryAllocator *_alloc = Allocators::default) : alloc(_alloc) {}

		inline void* allocate(size_t bytes) { return alloc->allocate(bytes); }
		inline void deallocate(void* ptr) { alloc->deallocate(ptr); }
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
	};

	void GetMemoryAllocationStats(uint64_t *Max, uint64_t *Current, uint64_t *Counter);
	bool GetMemoryBlocks(void **current, size_t *size, size_t *counter, void **data);

	IMemoryAllocator *GetMemoryAllocator(uint32_t allocatorType = IMemoryAllocator::default, SIZE_T size = 0);
}

typedef Allocators::IMemoryAllocator IMemoryAllocator;
typedef Allocators::MemoryAllocator MemoryAllocator;
