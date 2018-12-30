/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
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
template<typename T>
struct queue {
private:
	IMemoryAllocator *alloc;
	struct Entry : T {
		template<class... Args>
		inline Entry(IMemoryAllocator *alloc, Args &&...args) :
			T(alloc, std::forward<Args>(args)...),
			next(nullptr)
		{}

		Entry *next;
	};

public:
	queue(SIZE_T size = 16*1024) :
		first(nullptr),
		last(nullptr),
		used(0),
		alloc(nullptr)
	{
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, size);
	}

	/// <summary>
	/// Add new entry to queue. It call object constructor with given params (and allocatora as first param).
	/// </summary>
	/// <param name="...args">The ...args.</param>
	/// <returns>Pointer to entry</returns>
	template<class... Args>
	Entry * push_back(Args &&... args)
	{
		++used;
		std::lock_guard<std::mutex> lck(mutex);
		void *p = alloc->allocate(sizeof(Entry));
		new (p) Entry(alloc, std::forward<Args>(args)...);
		auto e = reinterpret_cast<Entry *>(p);
		if (last)
			last->next = e;

		last = e;
		if (!first)
			first = e;

		return (Entry *)p;
	}

	/// <summary>
	/// Pops entry from front of queue.
	/// Waning: It will not removes it from memory. Rembember to call <see cref="release">release after</see> you end using it.
	/// </summary>
	/// <returns>Pointer to entry or nullptr if queue is empty.</returns>
	Entry *pop_front()
	{
		--used;
		std::lock_guard<std::mutex> lck(mutex);
		Entry *ret = first;
		if (first)
			first = first->next;
		if (!first)
			last = nullptr;

		return ret;
	}

	U32 size() { return used; }

	/// <summary>
	/// Releases entry. Removes it from queue memory.
	/// Rembember to call it after you end using it.
	/// </summary>
	/// <param name="en">The entry.</param>
	void release(Entry *entry)
	{
		entry->~Entry();
		alloc->deallocate(entry);
	}

private:
	Entry *first;
	Entry *last;
	U32 used;
	std::mutex mutex;
};