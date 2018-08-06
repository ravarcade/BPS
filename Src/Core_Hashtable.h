/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/


/// <summary>
/// Hash table template class.
/// </summary>
template<typename T, U32 block = 256, class Alloc = Allocators::Default>
class hashtable : public MemoryAllocatorStatic<Alloc>
{
private:
	typedef U32 hash_t;
	typedef const T *val_t;
	typedef typename T::key_t key_t;

	val_t*data;
	SIZE_T size;
	SIZE_T used;
	SIZE_T treshold;
	float tresholdLevel;

	const uintptr_t EMPTY = 0;
	const uintptr_t USED = 1;
	const uintptr_t mask = uintptr_t(-2);

public:
	hashtable(SIZE_T _size = 256, float _tresholdLevel = 0.7f) :
		used(0),
		size(_size),
		tresholdLevel(_tresholdLevel),
		treshold(SIZE_T(_size * _tresholdLevel))

	{
		allocArray(size, data);
		memset(data, 0, size * sizeof(data[0])); // we clear values, becasue we store inside info, if slot is used or not.
	}

	~hashtable()
	{
		deallocate(data);
	}

	void append(val_t value)
	{
		++used;
		if (used >= treshold)
		{
			rehash(size * 2);
		}

		U32 i = value->getHash() % size;
		while (reinterpret_cast<uintptr_t>(data[i]) & mask) // if data[i] == 0 || data[i] == 1 slot is empty
		{
			i = (i+1) % size;			
		}

		data[i] = value;
	}

	val_t find(key_t *key)
	{
		hash_t h = key->getHash();
		U32 i = h % size;

		while (!data[i])
		{
			auto pVal = data[i];
			auto adr = reinterpret_cast<uintptr_t>(data[i]);
			if ((adr & mask) != EMPTY &&
				pVal->getHash() == h && 
				*(pVal->getKey()) == *key)
			{
				return data[i];
			}

			i = (i+1) % size;
		}

		return nullptr;
	}

	val_t & operator[] (key_t *key)
	{
		hash_t h = key->getHash();
		U32 i = h % size;

		while (!data[i])
		{
			auto pVal = data[i];
			auto adr = reinterpret_cast<uintptr_t>(data[i]);
			if ((adr & mask) != EMPTY && pVal->getHash() == h && *(pVal->getKey()) == *key)
			{
				return data[i];
			}

			i = (i + 1) % size;
		}

		// append
		++used;
		if (used >= treshold)
		{
			rehash(size * 2);
		}

		U32 i = key->getHash() % size;
		while (reinterpret_cast<uintptr_t>(data[i]) & mask) // if data[i] == 0 || data[i] == 1 slot is empty
		{
			i = (i + 1) % size;
		}
		data[i] = (val_t)key;

		return data[i];
	}

	void remove(key_t *key)
	{
		hash_t h = key->getHash();
		U32 i = h % size;

		while (data[i])
		{
			auto pVal = data[i];
			if (reinterpret_cast<uintptr_t>(pVal) != USED &&
				pVal->getHash() == h && 
				(*(pVal->getKey())) == (*key))
			{
				U32 j = (i + 1) % size;
				auto p = reinterpret_cast<uintptr_t *>(data);
				p[i] = p[j] == EMPTY ? EMPTY : USED;
				--used;
				break;
			}

			i = (i + 1) % size;
		}
	}

	void rehash(SIZE_T newSize)
	{
		auto oldData = data;
		auto f = oldData;
		auto e = oldData + size;
		
		size = newSize;
		treshold = (SIZE_T)(size * tresholdLevel);
		allocArray(size, data);
		memset(data, 0, size * sizeof(data[0])); // we clear values, becasue there is info, if slot is used or not.

		while (f != e)
		{
			uintptr_t memAdr = reinterpret_cast<uintptr_t>(*f);
			if (memAdr & mask)
			{
				hash_t h = (*f)->getHash();
				U32 i = h % size;
				while (!(reinterpret_cast<uintptr_t>(data[i]) & mask)) // if data[i] == 0 || data[i] == 1 slot is empty
				{
					i = (i + 1) % size;
				}

				data[i] = *f;

			}
			++f;
		}
		deallocate(oldData);
	}
};


/*



template<class... Args>
void insert(const K &key, Args &&...args)
{
	void *p = alloc.allocate(sizeof(Entry));
	new (p) Entry(key, &alloc, std::forward<Args>(args)...);
	auto e = reinterpret_cast<Entry *>(p);
	auto h = e->hash = Hash()(key);
	U32 rootIdx = h & rootTableMask;
	if (rootTable[rootIdx])
		e->next = rootTable[rootIdx];
	rootTable[rootIdx] = e;
}

void remove(const K &key)
{
	U32 h = Hash()(key);
	U32 rootIdx = h & rootTableMask;
	for (Entry **prev = &rootTable[rootIdx]; Entry *f = *prev; prev = &f->next)
	{
		if (f->hash == h &&
			f->key == key)
		{
			*prev = f->next;

			f->~Entry();
			alloc.deallocate(f);
			break;
		}
	}
}

T *find(const K &key)
{
	U32 h = Hash()(key);
	U32 rootIdx = h & rootTableMask;
	for (Entry *f = rootTable[rootIdx]; f; f = f->next)
	{
		if (f->hash == h &&
			f->key == key)
		{
			return &f->value;
		}
	}
	return nullptr;
}

void resizeRootTable(U32 newRootTableSize)
{
	Entry **oldRootTable = rootTable;
	SIZE_T oldRootTableSize = rootTableSize;
	SIZE_T rootTableMemorySize = sizeof(Entry *) * rootTableSize;
	rootTable = reinterpret_cast<Entry **>(alloc.allocate(rootTableMemorySize));
	memset(_rootTable, 0, rootTableMemorySize);
	rootTableSize = newRootTableSize;
	rootTableMask = static_cast<U32>(newRootTableSize - 1);

	// Move all entrys from oldRootTable to new
	// We do not copy real data. We only change _next member and put all in new _rootTable.
	// We also don't calc hash value... they are already stored.
	auto ort = oldRootTable;
	while (oldRootTableSize)
	{
		auto f = *oldRootTable;
		++oldRootTable;
		--oldRootTableSize;
		while (f)
		{
			auto e = f;
			f = f->next;

			auto h = e->_hash;
			U32 rootIdx = h & rootTableMask;
			if (rootTable[rootIdx])
				e->_next = rootTable[rootIdx];
			rootTable[rootIdx] = e;
		}
	}
	alloc.deallocate(ort);
}

private:
	Alloc alloc;
	Entry **rootTable;
	U32 rootTableMask;
	SIZE_T rootTableSize;

};

*/