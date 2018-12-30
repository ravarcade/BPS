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

	inline U32 getHash(const TKey &key) // remove EMPTY & USED from posible hash values
	{
		hash_t h = hash<TKey>()(key);
		return h < RESERVED ? h + RESERVED : h;
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

	TVal & operator[] (const TKey &key)
	{
		if (used + 1 > treshold)
			rehash(size * 2);

		hash_t h = getHash(key);
		U32 i = h % size;
		U32 j = size;
		while (data[i].hash != EMPTY)
		{
			if (data[i].hash == h && data[i].key == key)
			{
				return data[i].val;
			}

			if (data[i].hash == USED_EMPTY && j == size)
				j = i;

			i = (i + 1) % size;
		}

		// go back if we have one used and empty
		if (j != size)
			i = j;

		// append
		++used;
		data[i].hash = h;
		data[i].key = key;

		return data[i].val;
	}


	bool exist(const TKey &key)
	{
		hash_t h = getHash(key);
		U32 i = h % size;

		while (data[i].hash != EMPTY)
		{
			if (data[i].hash == h && data[i].key == key)
			{
				return true;
			}

			i = (i + 1) % size;
		}

		return false;
	}

	void remove(const TKey &key)
	{
		hash_t h = getHash(key);
		U32 i = h % size;

		while (data[i].hash != EMPTY)
		{
			if (data[i].hash == h && data[i].key == key)
			{
				U32 j = (i + 1) % size;
				data[i].hash = data[j].hash == EMPTY ? EMPTY : USED_EMPTY; // we mark USED_EMPTY only if NEXT slot is not EMPTY... this way we will have more EMPTY slots
				--used;
				break;
			}

			i = (i + 1) % size;
		}
	}
};