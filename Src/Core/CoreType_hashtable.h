/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
*
*/


/// <summary>
/// Hash table template class.
/// </summary>
template<typename K, typename T, class Hash = hash<K>, U32 block = 256, class Alloc = Allocators::Default>
class hashtable : public MemoryAllocatorStatic<Alloc>
{
private:
	typedef U32 HASH;
	HASH *hashes;
	T* values;
	SIZE_T size;

public:
	hashtable(SIZE_T _size = 256) :
		size(_size)
	{
		hashes = static_cast<HASH *>(allocate(size * sizeof(HASH)));
		memset(hashes, 0, size * sizeof(HASH));
		values = static_cast<T *>(allocate(size * sizeof(T*)));
		memset(values, 0, size * sizeof(T*)); // why i clear "datas"?
	}

	~hashtable()
	{
		deallocate(hashes);
		deallocate(values);
	}

	void append(const K& key, T *value)
	{
		HASH h = Hash()(key);
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