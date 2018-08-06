/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/


template<typename T, class Alloc = Allocators::Blocks>
struct list {
private:
	struct entry : T {
		template<class... Args>
		inline entry(IMemoryAllocator *alloc, Args &&...args) :
			T(alloc, std::forward<Args>(args)...),
			next(nullptr)
		{}
		entry *next;
		entry *prev;
	};

public:
	struct iterator {
	private:
		entry *e = nullptr;
		iterator(entry *_e) : e(_e) {}

	friend list;
	public:
		iterator() = default;
		entry& operator*() { return *e; }
		entry* operator->() { return e; }
		iterator operator ++() { e = e->next; return *this; }
		iterator operator --() { e = e->prev; return *this; }
		iterator operator ++(int) { iterator r = *this;  e = e->next; return r; }
		iterator operator --(int) { iterator r = *this;  e = e->prev; return r; }
		bool operator==(iterator b) const { return e == b.e; }
		bool operator!=(iterator b) const { return e != b.e; }
	};

	list(SIZE_T defaultBlockSize = 16 * 1024) :
		alloc(MemoryAllocatorStatic<>::GetMemoryAllocator(), defaultBlockSize),
		first(nullptr),
		last(nullptr),
		size(0)
	{}

	~list()
	{
		clear();
	}

	void clear()
	{
		auto f = first;
		while (f)
		{
			auto en = f;
			f = f->next;
			en->~entry();
			alloc.deallocate(en);
		}

		size = 0;
		first = last = nullptr;
	}

	template<class... Args>
	T *push_back(Args &&... args)
	{
		void *p = alloc.allocate(sizeof(entry));
		new (p) entry(&alloc, std::forward<Args>(args)...);
		auto e = reinterpret_cast<entry *>(p);
		if (last)
			last->next = e;
		e->prev = last;
		last = e;
		if (!first)
			first = e;

		return e;
	}


	template<class... Args>
	iterator insert(iterator &pos, Args &&... args) // before
	{
		void *p = alloc.allocate(sizeof(entry));
		new (p) entry(&alloc, std::forward<Args>(args)...);
		auto e = reinterpret_cast<entry *>(p);
		if (pos.e == nullptr) // end() interator
		{
			push_back(std::forward<Args>(args)...);
			return;
		}
		e->prev = pos->prev;
		pos->prev = e;
		e->next = pos;
		if (e->prev)
			e->prev->next = e;
		if (pos == first)
			first = e;
		if (pos == last)
			last = e;

		return pos;
	}

	iterator begin() { return first; }
	iterator end() { return nullptr; }

	entry *pop_front()
	{
		std::lock_guard<std::mutex> lck(mutex);
		entry *ret = first;
		if (first)
			first = first->next;
		if (!first)
			last = nullptr;

		return ret;
	}

	void erase(entry *en)
	{
		if (en->next)
			en->next->prev = en->prev;
		if (en->prev)
			en->prev->next = en->next;
		if (en == first)
			first = en->next;
		if (en == last)
			last = en->prev;

		en->~entry();
		alloc.deallocate(en);
	}

	iterator erase(iterator &i)
	{
		iterator ret = i->next;
		erase(&*i);
		return ret;
	}

private:
	Alloc alloc;
	entry *first;
	entry *last;
	SIZE_T size;
};