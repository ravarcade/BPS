/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
*
*/


/// <summary>
/// Queue template class. Thread safe.
/// For data Blocks allocator is default.
/// -------------------------------------
/// Example of queue entry class:
/// struct Ev {
///   Ev(IMemoryAllocator *alloc, const char *msg) :
///     message(alloc, msg) {} // allocator is passed to member initialization. Member will use allocator to store data.
///
///   SimpleString message;
/// };
/// </summary>
template<typename T, class Alloc = Allocators::Blocks>
struct queue {
private:
	struct Entry : T {
		template<class... Args>
		inline Entry(IMemoryAllocator *alloc, Args &&...args) :
			T(alloc, std::forward<Args>(args)...),
			next(nullptr)
		{}

		Entry *next;
	};

public:
	queue(SIZE_T defaultBlockSize = 16*1024) :
		alloc(MemoryAllocatorStatic<>::GetMemoryAllocator(), defaultBlockSize),
		first(nullptr),
		last(nullptr)
	{}

	template<class... Args>
	void push_back(Args &&... args)
	{
		std::lock_guard<std::mutex> lck(mutex);
		void *p = alloc.allocate(sizeof(Entry));
		new (p) Entry(&alloc, std::forward<Args>(args)...);
		auto e = reinterpret_cast<Entry *>(p);
		if (last)
			last->next = e;

		last = e;
		if (!first)
			first = e;
	}

	Entry *pop_front()
	{ 
		std::lock_guard<std::mutex> lck(mutex);
		Entry *ret = first;
		if (first)
			first = first->next;
		if (!first)
			last = nullptr;

		return ret;
	}

	void release(Entry *en)
	{
		en->~Entry();
		alloc.deallocate(en);
	}

private:
	Alloc alloc;
	Entry *first;
	Entry *last;
	std::mutex mutex;
};