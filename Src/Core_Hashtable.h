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
template<typename TKey, typename TVal>
class hashtable
{
	typedef U32 hash_t;
private:
	U32 size;
	U32 used;
	U32 treshold;
	float tresholdLevel;
	IMemoryAllocator *alloc;

	struct TEntry{
		hash_t hash;
		TKey key;
		TVal val;
	};

	TEntry *data;
	enum {
		EMPTY = 0,
		USED_EMPTY = 1,
		RESERVED = 2
	};

	void rehash(U32 newSize)
	{
		auto oldData = data;
		auto f = oldData;
		auto e = oldData + size;

		size = newSize;
		SIZE_T s = size * sizeof(data[0]);
		treshold = (SIZE_T)(size * tresholdLevel);
		data = reinterpret_cast<TEntry*>(alloc->allocate(s));
		memset(data, 0, s); // we clear values, becasue there is info, if slot is used or not.

		while (f != e)
		{
			if (f->hash >= RESERVED)
			{
				U32 i = f->hash % size;
				while (data[i].hash != EMPTY)
				{
					i = (i + 1) % size;
				}

				data[i].hash = f->hash;
				data[i].key = f->key;
				data[i].val = f->val;
			}
			++f;
		}
		alloc->deallocate(oldData);
	}

public:
	hashtable(U32 _size = 256, float _tresholdLevel = 0.7f, int allocId = 0) :
		size(_size),
		used(0),
		tresholdLevel(_tresholdLevel),
		treshold(static_cast<U32>(_tresholdLevel * _size))
	{
		alloc = Allocators::GetMemoryAllocator(allocId);
		SIZE_T s = size * sizeof(data[0]);
		data = reinterpret_cast<TEntry*>(alloc->allocate(s));
		memset(data, 0, s); // we clear values, becasue we store inside info, if slot is used or not.
	}

	~hashtable()
	{
		alloc->deallocate(data);
	}

	void clear() { memset(data, 0, size * sizeof(data[0])); }

	TVal & operator[] (const TKey &key)
	{
		hash_t h = getHash(key);
		auto pVal = find(key, h);
		if (pVal)
			return *pVal;

		return *add(key, h);
	}

	inline U32 getHash(const TKey &key) // remove EMPTY & USED from posible hash values
	{
		hash_t h = hash<TKey>()(key);
		return h < RESERVED ? h + RESERVED : h;
	}

	TVal *add(const TKey &key) { return add(key, getHash(key)); }
	TVal *add(const TKey &key, hash_t h)
	{
		++used;
		if (used > treshold)
			rehash(size * 2);

		auto pD = data + (h % size);
		auto e = data + size;
		for (;;)
		{
			if (pD->hash < RESERVED) // == EMPTY or == USED_EMPTY
			{
				pD->hash = h;
				pD->key = key;
				return &pD->val;
			}
			if (++pD == e)
				pD = data;
		}
	}

	TVal *find(const TKey &key) { return find(key, getHash(key)); }
	TVal *find(const TKey &key, hash_t h)
	{
		U32 i = h % size;

		while (data[i].hash != EMPTY)
		{
			if (data[i].hash == h && cmp<TKey>()(data[i].key, key))
			{
				return &data[i].val;
			}

			i = (i + 1) % size;
		}

		return nullptr;
	}

	void remove(const TKey &key)
	{
		hash_t h = getHash(key);
		U32 i = h % size;

		while (data[i].hash != EMPTY)
		{
			if (data[i].hash == h && cmp<TKey>()(data[i].key, key))
			{
				U32 j = (i + 1) % size;
				data[i].hash = data[j].hash == EMPTY ? EMPTY : USED_EMPTY; // we mark USED_EMPTY only if NEXT slot is not EMPTY... this way we will have more EMPTY slots
				--used;
				break;
			}

			i = (i + 1) % size;
		}
	}
	
	template<typename Callback>
	void foreach(Callback &f) 
	{
		for (uint32_t i = 0; i < size; ++i)
		{
			if (data[i].hash >= RESERVED)
				f(data[i].val);
		}
	}
};


template <typename T>
struct CStringStructHastable
{
	IMemoryAllocator *alloc;
	BAMS::CORE::hashtable<const char *, T *> ht;

	CStringStructHastable()
	{
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, 4096);
	}

	~CStringStructHastable()
	{
		delete alloc;
	}

	void clear()
	{
		ht.clear();                                                             // clear hashtable data
		delete alloc;                                                           // delete allocated memory for strings (all at once)
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, 4096);  // create new allocator for strings (with empty buffer)
	}

	T * find(const char *s) { auto pv = ht.find(s); return pv ? *pv : nullptr; }

	T * add(const char *s)
	{
		auto h = ht.getHash(s);
		auto pV = ht.find(s, h);
		if (!pV)
		{
			SIZE_T l = strlen(s) + 1;
			char *newS = static_cast<char *>(alloc->allocate(l));
			T *pT = static_cast<T *>(alloc->allocate(sizeof(T)));
			memcpy_s(newS, l, s, l);
			s = newS;
			pV = ht.add(s, h);
			*pV = pT;
		}
		return *pV;
	}

	template<typename Callback>
	void foreach(Callback &f) { ht.foreach(f); }
};

template <typename T>
class CStringHastable
{
	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == false>::type
		deleteObjects()
	{
		ht.foreach([&](T *o) {
			o->~T();
		});
	}

	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == true>::type
		deleteObjects()
	{
		// Trivially destructible objects can be reused without using the destructor.
	}

public:
	IMemoryAllocator *alloc;
	BAMS::CORE::hashtable<const char *, T *> ht;

	CStringHastable()
	{
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, 4096);
	}

	~CStringHastable()
	{
		deleteObjects<T>();
		delete alloc;
	}

	void clear()
	{
		deleteObjects<T>();                                                     // call destructors of objects if needed
		ht.clear();                                                             // clear hashtable data
		delete alloc;                                                           // delete allocated memory for strings (all at once)
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, 4096);  // create new allocator for strings (with empty buffer)
	}

	T * find(const char *s) { auto p = ht.find(s); return p ? *p : nullptr; }
	
	template<class... Args>
	T * add(const char *s, Args &&...args)
	{
		auto h = ht.getHash(s);
		auto pV = ht.find(s, h);
		if (!pV)
		{
			SIZE_T l = strlen(s) + 1;
			char *newS = static_cast<char *>(alloc->allocate(l));
			memcpy_s(newS, l, s, l);
			s = newS;
			pV = ht.add(s, h);
			*pV = new (alloc->allocate(sizeof(T))) T(std::forward<Args>(args)...);
		}
		return *pV;
	}

	T & operator[] (const char *s)
	{
		return *add(s);
	}

	template<typename Callback>
	void foreach(Callback &f) { ht.foreach(f); }
};
