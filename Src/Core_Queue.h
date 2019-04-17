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

	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == false>::type
		deleteObjects()
	{
		for (; first; first = first->next)
			first->~Entry(); // destructor is called, but memory is not released, so "next" will be valid
	}

	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == true>::type
		deleteObjects()
	{
		// Trivially destructible objects can be reused without using the destructor.
	}

public:
	queue(SIZE_T size = 16*1024) :
		first(nullptr),
		last(nullptr),
		used(0),
		alloc(nullptr),
		queueSize(size)
	{
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, size);
	}

	~queue()
	{
		deleteObjects<T>();
		used = 0;
		last = nullptr;
		delete alloc;
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

		e->next = nullptr;

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
		std::lock_guard<std::mutex> lck(mutex);
		Entry *ret = first;
		if (first) {
			--used;
			first = first->next;
		}
		if (!first)
			last = nullptr;

		return ret;
	}

	template<typename TSelector, typename TCallback >
	void filter(TSelector &test, TCallback &process)
	{
		if (!first && !used) // do not waste time on empty queue
			return;

		Entry *fFirst = nullptr;
		{	// we have segment here, because we want to release lck
			std::lock_guard<std::mutex> lck(mutex);
			Entry *p = first;
			Entry *fLast = nullptr;
			first = nullptr;
			last = nullptr;
			while (p)
			{
				if (test(p))
				{
					if (!fFirst)
						fFirst = p;
					if (fLast)
						fLast->next = p;
					fLast = p;
					--used;
				}
				else {
					if (!first)
						first = p;
					if (last)
						last->next = p;
					last = p;
				}
				auto np = p->next;
				p->next = nullptr;
				p = np;
			}
		}
		// We removed all "filtered" entrys from queue/list 
		// ... and we have all that entrys in separate queue
		for (Entry *ent = fFirst; ent; ent = ent->next)
		{
			process(ent);
			release(ent);			
		}
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

	void reset()
	{
		deleteObjects<T>();
		used = 0;
		last = nullptr;
		first = nullptr;
		delete alloc;
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, queueSize);
	}

private:
	Entry *first;
	Entry *last;
	U32 used;
	std::mutex mutex;
	SIZE_T queueSize;
};