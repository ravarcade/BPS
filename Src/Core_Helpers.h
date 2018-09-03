/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

/// <summary>
/// Extension class template for shared objects. Like shared_string.
/// Something lika shared pointer.
/// Goal:
/// - minimize memory usage. In instance there is only one index to array. Only one 32-bit unsigned int.
/// - minimize memory allocation. Assigment or copy construction will only increase refCounter. Object will be copie when it is moddified.
/// </summary>
template <typename T, class A = Allocators::Default, SIZE_T S = 48>
class shared_base
{
private:
	struct Entry
	{
		T value;
		U32 refCounter;
		U32 nextFree;
	};

	typedef basic_array<Entry, A, S> Pool;
	static Pool _pool;
	static U32 _firstFreeEntry;
	static U32 _used;

protected:
	U32 _idx;

	shared_base() : _idx(0) {}
	shared_base(U32 idx) : _idx(idx) {}

	inline void CheckIfInitialized()
	{
#ifdef _DEBUG
		if (!_pool.size())
		{
			TRACE("Critical Error: shared_base not initialized befor first use.");
			Entry *pEntry = _pool.add_empty();
			pEntry->refCounter = 2;
			new (&pEntry->value) T();
		}
#endif
	}

	template<class... Args>
	void MakeNewEntry(Args &&... args)
	{
		U32 idx = _firstFreeEntry;
		Entry *pEntry;
		if (idx)
		{
			pEntry = &_pool[idx];
			_firstFreeEntry = pEntry->nextFree;
		}
		else
		{
			CheckIfInitialized();
			idx = static_cast<U32>(_pool.size());
			pEntry = _pool.add_empty();
		}
		++_used;
		pEntry->refCounter = 1;
		new (&pEntry->value) T(std::forward<Args>(args)...);

		_idx = idx;
	}

	inline void NeedUniq()
	{
		auto &src = _pool[_idx];
		if (src.refCounter > 1 || _idx == 0)
		{
			if (_idx)
				--src.refCounter;
			MakeNewEntry(src.value);
		}
	}

	inline void AddRef() { if (_idx) ++_pool[_idx].refCounter; }
	inline void Release() {
		if (_idx)
		{
			auto ref = --_pool[_idx].refCounter;

			if (!ref)
			{
				auto &entry = _pool[_idx];

				// We will release memory used by string, but entry stays in _pool. It is now empty.
				reinterpret_cast<T*>(&entry.value)->~T();

				// We will use refCounter as index to previous empty.
				entry.nextFree = _firstFreeEntry;
				_firstFreeEntry = _idx;
				--_used;
			}
		}
	}

	

public:
	static void Initialize()
	{
		Entry *pEntry = _pool.add_empty();
		pEntry->refCounter = 2;
		new (&pEntry->value) T();
	}

	static void Finalize()
	{
		if (_used == 0)
		{
			auto &entry = _pool[0];

			// We will release memory of last used shared string (default empty string).
			reinterpret_cast<T*>(&entry.value)->~T();

			// We will release memory used to store entries;
			_pool.reset();

			// No free entries
			_firstFreeEntry = 0;
		}
		else
		{
			TRACE("Critical Error: shared_base finalized, but not all shared entries are release.\n");
		}
	}

	inline T & GetValue() const { return _pool[_idx].value; }
};


template <typename T, class A, SIZE_T S>
typename shared_base<T, A, S>::Pool shared_base<T, A, S>::_pool;

template <typename T, class A, SIZE_T S>
U32 shared_base<T, A, S>::_firstFreeEntry = 0;

template <typename T, class A, SIZE_T S>
U32 shared_base<T, A, S>::_used = 0;




#define DEFINE_HAS_METHOD(x)                                                                       \
template <typename T>                                                                              \
struct has_method_##x                                                                              \
{                                                                                                  \
	template <class, class> class checker;                                                         \
																								   \
	template <typename C>                                                                          \
	static std::true_type test(checker<C, decltype(&C::x)> *);                                     \
																								   \
	template <typename C>                                                                          \
	static std::false_type test(...);                                                              \
																								   \
	typedef decltype(test<T>(nullptr)) type;                                                       \
	enum { value = std::is_same<std::true_type, decltype(test<T>(nullptr))>::value };              \
};



