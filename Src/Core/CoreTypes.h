/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
*
*/
struct RegisteredClasses
{
	static void Initialize();
	static void Finalize();
};

// ============================================================================


template <typename T, typename U>
struct basic_string_base
{
	T *_buf;
	U _reserved;
	U _used;
	basic_string_base() {}
	basic_string_base(U reserved, U used = 0, T* buf = nullptr) : _reserved(reserved), _used(used), _buf(buf) {}
	static SIZE_T length(const T *txt) { return strlen(txt); }
	static T tolower(T c) { return ::tolower(c); }
	static int ncmp(const T *string1, const T *string2, size_t count) { return strncmp(string1, string2, count); }
};

template <typename T, typename U>
struct simple_string : basic_string_base<T, U>
{
	typedef simple_string<T, U> _T;
	typedef basic_string_base<T, U> _B;

	simple_string() : _B(0), _alloc(nullptr) {}
	simple_string(IMemoryAllocator *alloc) : _B(0), _alloc(alloc) {}
	template<typename ST>
	simple_string(IMemoryAllocator *alloc, const ST *src, const ST *end = nullptr) : _alloc(alloc) { _used = end ? static_cast<U>(end - src) : static_cast<U>(basic_string_base<ST, U>::length(src)); _reserved = _used + 1; _Allocate(); _CopyText(_buf, src, _used); }
	template<typename ST, typename SU>
	simple_string(IMemoryAllocator *alloc, const basic_string_base<ST, SU> &src) : _B(static_cast<U>(src._used+1), static_cast<U>(src._used)), _alloc(alloc) { _Allocate(); _CopyText(_buf, src._buf, _used); }
	simple_string(IMemoryAllocator *alloc, const _T &src) : _B(static_cast<U>(src._used+1), static_cast<U>(src._used)), _alloc(alloc) { _Allocate(); _CopyText(_buf, _reserved, src._buf, _used); }
	~simple_string() { if (_buf && _alloc) { _alloc->deallocate(_buf); _buf = 0; _reserved = 0; _used = 0; } }

	void set_allocator(IMemoryAllocator *alloc) { if (!_alloc) _alloc = alloc; }

	template<typename V>
	simple_string& operator = (const basic_string_base<T, V>& src) { _used = 0; _Append(static_cast<U>(src._used), src._buf); return *this; }
	simple_string& operator = (const T* src) { _used = 0; U len = static_cast<U>(_B::length(src));  _Append(len, src); return *this; }
	simple_string& operator = (const _T& src) { _used = 0; _Append(src._used, src._buf); return *this; }
	
	T operator[](SIZE_T pos) const { return (pos < _used && pos >= 0) ? _buf[pos] : 0; };
	T& operator[](SIZE_T pos) { return (pos < _used && pos >= 0) ? _buf[pos] : _buf[0]; };

	bool operator == (const basic_string_base<T, U> &str) const 
	{
		if (str._buf == _buf)
			return true;
		if (str._used != _used)
			return false;
		return _B::ncmp(str._buf, _buf, _used) == 0;
	}

	SIZE_T size() const { return _used; }
	operator _B() { return *this; }

	const T * c_str() 
	{
		if (_used == _reserved)
		{
			static const char *zero = "\0";
			_Append(1, reinterpret_cast<const T*>(zero));
		}
		_buf[_used] = 0;

		return _buf;
	}

private:
	IMemoryAllocator *_alloc;
	void _Allocate() { _buf = static_cast<T *>(_alloc->allocate(_reserved * sizeof(T))); }

	void _Append(U len, const T *src)
	{
		T *oldBuf = nullptr;

		if (_reserved <= _used + len) // yes.... we will have alway atleast 1 char spared space.... for "\0" to make c-string
		{
			oldBuf = _buf;
			_reserved = _used + len + 1;
			_Allocate();

			if (oldBuf && _used)
				_CopyText(_buf, _reserved, oldBuf, _used);
		}

		if (len && src)
		{
			_CopyText(_buf + _used, _reserved - _used, src, len);
			_used += len;
		}

		// oldBuf is released here, because there is chance, that we copy part from source buffer
		if (oldBuf)
			_alloc->deallocate(oldBuf);
	}

	void _CopyText(char *dst, const char *src, SIZE_T srcSize) { memcpy_s(dst, srcSize, src, srcSize); }
	void _CopyText(wchar_t *dst, const wchar_t *src, SIZE_T srcSize) { memcpy_s(dst, srcSize * sizeof(wchar_t), src, srcSize * sizeof(wchar_t)); }
	void _CopyText(wchar_t *dst, const char *src, SIZE_T srcSize)
	{
		size_t convertedChars;
		mbstowcs_s(&convertedChars, dst, srcSize + 1, src, srcSize);
		assert(convertedChars != srcSize);
	}

	void _CopyText(char *dst, const wchar_t *src, SIZE_T srcSize)
	{
		// we want "pure ascii" chars (0x20 - 0x7e), so skip all other
		while (srcSize)
		{
			int c = *src;
			++src;
			if (c > 0x7e)
				c = '_';
			*dst = c;
			++dst;
			--srcSize;
		}
	}

	void _CopyText(void *dst, SIZE_T dstSize, const void *src, SIZE_T srcSize)
	{
		memcpy_s(dst, dstSize * sizeof(T), src, srcSize * sizeof(T));
	}
};

template<>
SIZE_T basic_string_base<wchar_t, U16>::length(const wchar_t *txt) { return wcslen(txt); }
template<>
SIZE_T basic_string_base<wchar_t, U32>::length(const wchar_t *txt) { return wcslen(txt); }

template<>
wchar_t basic_string_base<wchar_t, U16>::tolower(wchar_t c) { return towlower(c); }
template<>
wchar_t basic_string_base<wchar_t, U32>::tolower(wchar_t c) { return towlower(c); }

template<>
int  basic_string_base<wchar_t, U16>::ncmp(const wchar_t *string1, const wchar_t *string2, size_t count) { return wcsncmp(string1, string2, count); }

template<>
int  basic_string_base<wchar_t, U32>::ncmp(const wchar_t *string1, const wchar_t *string2, size_t count) { return wcsncmp(string1, string2, count); }

template <typename T, typename U = U32, class Alloc = Allocators::Default, SIZE_T minReservedSize = 48>
class basic_string : public basic_string_base<T, U>
{
private:
	typedef basic_string<T, U, Alloc, minReservedSize> _T;
	typedef basic_string_base<T, U> _B;
	typedef MemoryAllocatorGlobal<Alloc> _allocator;

	void _CopyText(char *dst, const char *src, SIZE_T srcSize) { memcpy_s(dst, srcSize, src, srcSize); }
	void _CopyText(wchar_t *dst, const wchar_t *src, SIZE_T srcSize) { memcpy_s(dst, srcSize * sizeof(wchar_t), src, srcSize * sizeof(wchar_t)); }
	void _CopyText(wchar_t *dst, const char *src, SIZE_T srcSize)
	{
		size_t convertedChars;
		mbstowcs_s(&convertedChars, dst, srcSize + 1, src, srcSize);
		assert(convertedChars != srcSize);
	}

	void _CopyText(char *dst, const wchar_t *src, SIZE_T srcSize)
	{
		// we want to char "pure ascii" chars (0x20 - 0x7e)
		while (srcSize)
		{
			int c = *src;
			++src;
			if (c > 0x7e)
				c = '_';
			*dst = c;
			++dst;
			--srcSize;
		}
	}

	void _CopyText(void *dst, SIZE_T dstSize, const void *src, SIZE_T srcSize)
	{
		memcpy_s(dst, dstSize * sizeof(T), src, srcSize * sizeof(T));
	}

	void _Append(U len, const T *src)
	{
		T *oldBuf = nullptr;

		if (_reserved <= _used + len) // yes.... we will have alway atleast 1 char spared space.... for "\0" to make c-string
		{
			oldBuf = _buf;
			_reserved = _used + len + minReservedSize;
			_Allocate();

			if (oldBuf && _used)
				_CopyText(_buf, _reserved, oldBuf, _used);
		}

		if (len && src)
		{
			_CopyText(_buf + _used, _reserved - _used, src, len);
			_used += len;
		}

		// oldBuf is released here, because there is chance, that we copy part from source buffer
		if (oldBuf)
			_allocator::deallocate(oldBuf);
	}

	void _Allocate() { _buf = static_cast<T *>(_allocator::allocate(_reserved*sizeof(T))); }

public:
	basic_string() : _B(0) {  }
	template<typename ST>
	basic_string(const ST *src, const ST *end = nullptr) { 
		_used = end ? static_cast<U>(end - src) : static_cast<U>(basic_string_base<ST,U>::length(src));
		_reserved = max(_used, static_cast<U>(minReservedSize)); 
		_Allocate();  
		_CopyText(_buf, src, _used);
	}

	template<typename ST, typename SU>
	basic_string(const basic_string_base<ST, SU> &src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used))
	{
		_Allocate();
		_CopyText(_buf, src._buf, _used);
	}

	basic_string(const _T &src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used)) { _Allocate(); _CopyText(_buf, _reserved, src._buf, _used); }
	basic_string(_T &&src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used), src._buf) { src._used = 0; src._reserved = 0; src._buf = nullptr; }
	~basic_string() { if (_buf) { _allocator::deallocate(_buf); _buf = 0; _reserved = 0; _used = 0; } }

	template<typename V>
	basic_string& operator = (const basic_string_base<T, V>& src) { _used = 0; _Append(static_cast<U>(src._used), src._buf); return *this; }
	basic_string& operator = (const T* src) { _used = 0; U len = static_cast<U>(_B::length(src));  _Append(len, src); return *this; }
	basic_string& operator = (const _T& src) { _used = 0; _Append(src._used, src._buf); return *this; }
	basic_string& operator = (_T&& src) {
		if (_buf)
			_allocator::deallocate(_buf);

		_used = src._used;
		_reserved = src._reserved;
		_buf = src._buf;
		src._used = 0; src._reserved = 0; src._buf = nullptr;

		return *this;
	}

	T operator[](SIZE_T pos) const { return (pos < _used && pos >= 0) ? _buf[pos] : 0; };
	T& operator[](SIZE_T pos) { return (pos < _used && pos >= 0) ? _buf[pos] : _buf[0]; };

	template<typename V>
	basic_string & operator += (const basic_string_base<T, V>& src) { _Append(static_cast<U>(src._used), src._buf);  return *this; }
	basic_string & operator += (const T *src) { U len = static_cast<U>(_B::length(src)); _Append(len, src); return *this; }

	template<typename V>
	basic_string operator + (const basic_string_base<T, V>& src)  const { basic_string tmp(*(static_cast<const _B *>(this))); tmp += src;	return tmp; }
	basic_string operator + (const T *src) const { basic_string tmp(*(static_cast<const _B *>(this))); tmp += src;	return tmp; }

	bool operator == (const basic_string_base<T, U> &str) const {
		if (str._buf == _buf)
			return true;
		if (str._used != _used)
			return false;
		return _B::ncmp(str._buf, _buf, _used) == 0;
	}

	SIZE_T size() const { return _used; }

	operator _B() { return *this; }

	const T * c_str() { 
		if (_used == _reserved)
		{
			static const char *zero = "\0";
			_Append(1, reinterpret_cast<const T*>(zero));
		}
		_buf[_used] = 0;

		return _buf; 
	}

	template <typename V>
	bool wildcard(const basic_string_base<T, V>& pat, U pos = 0, V patPos = 0)
	{
		for (V i=patPos; i<pat._used;)
		{
			T c = pat._buf[i];
			++i;
			switch (c) {
			case '?':
				if (pos <_used)
					return false; // we have more chars in pattern than in haystack

				++pos;
				break;

			case '*': {
				if (i == pat._used)
					return true; // last char in pattern is '*'

				for (U j = pos + 1; j < _used; ++j)
					if (wildcard(pat, j, i ))
						return true;
				return false;
			}

			default:
				if (c != _buf[pos])
					return false;
				++pos;
			}
		}

		return pos != _used;
	}

	template <typename V>
	bool iwildcard(const basic_string_base<T, V>& pat, U pos = 0, V patPos = 0)
	{
		for (V i = patPos; i<pat._used;)
		{
			T c = pat._buf[i];
			++i;
			switch (c) {
			case '?':
				if (pos <_used)
					return false; // we have more chars in pattern than in haystack

				++pos;
				break;

			case '*': {
				if (i == pat._used)
					return true; // last char in pattern is '*'

				for (U j = pos + 1; j < _used; ++j)
					if (wildcard(pat, j, i))
						return true;
				return false;
			}

			default:
				if (_B::tolower(c) != _B::tolower(_buf[pos]))
					return false;
				++pos;
			}
		}

		return pos != _used;
	}

};

template <typename T>
struct basic_array_base
{
	T *_buf;
	SIZE_T _reserved;
	SIZE_T _used;
	basic_array_base() : _reserved(0), _used(0), _buf(nullptr) {}
	basic_array_base(SIZE_T reserved, SIZE_T used = 0, T* buf = nullptr) : _reserved(reserved), _used(used), _buf(buf) {}
};

/***
 * Use it for simple data type or simple struct (without own memory allocation). So, if in stuct you have String object, when it will crash.
 */
template <typename T, class Alloc = Allocators::Default, SIZE_T minReservedSize = 48>
class basic_array : public basic_array_base<T>
{
	typedef basic_array<T, Alloc, minReservedSize> _T;
	typedef basic_array_base<T> _B;
	typedef MemoryAllocatorGlobal<Alloc> _allocator;

	void _Realocate(SIZE_T newSize)
	{
		T *oldBuf = _buf;
		SIZE_T oldReserved = _reserved;
		_reserved = newSize;
		_Allocate();

		if (oldBuf && _used)
			_Copy(_buf, oldBuf, _used);

		if (oldBuf)
			_allocator::deallocate(oldBuf);
	}

	void _Copy(T *dst, const T *src, SIZE_T len)
	{
		size_t s = len * sizeof(T);
		memcpy_s(dst, s, src, s);
	}

	void _Append(SIZE_T len, const T *src)
	{
		if (_reserved < _used + len)
			_Realocate(_used + len + minReservedSize);
			
		if (len && src)
		{
			_Copy(_buf + _used, src, len);
			_used += len;
		}
	}

	void _Allocate() { _buf = static_cast<T *>(_allocator::allocate(_reserved * sizeof(T))); }

	void _InitializeEntry(T * begin, T * end)
	{
		while (begin < end)
		{
			new (begin) T();
			++begin;
		}
	}

public:
	basic_array() {}
	basic_array(SIZE_T s) : _B(minReservedSize, s) { if (_used > _reserved) _reserved = _used + minReservedSize; _Allocate(); _InitializeEntry(_buf, _buf + _used); }
	basic_array(const _B &src) : _B(src._reserved, src._used) { _Allocate(); _Copy(_buf, src._buf, src._used); }
	basic_array(std::initializer_list<T> list) : _B(list.size()) { _Allocate();	_Append(list.size(), list.begin());	}
	basic_array(_T &&src) : _B(src._reserved, src._used, src._buf) { src._reserved = 0; src._used = 0; src._buf = nullptr; }

	~basic_array() { if (_buf) _allocator::deallocate(_buf); }

	T operator[](SIZE_T pos) const { return (pos < _used && pos >= 0) ? _buf[pos] : 0; };
	T& operator[](SIZE_T pos) { return (pos < _used && pos >= 0) ? _buf[pos] : _buf[0]; };

	SIZE_T size() const { return _used; }
	SIZE_T capacity() const { return _reserved; }

	T *begin() { return _buf; }
	T *end() { return _buf + _used; }

	void push_back(const T &v) {
		if (size() == capacity())
			_Realocate(_used + minReservedSize);

		_buf[_used] = v;
		++_used;
	}

	void push_back(T && v) { 
		if (size() == capacity())
			_Realocate(_used + minReservedSize);

		_buf[_used] = std::move(v);
		++_used;
	}

	T *add_empty()
	{
		if (size() == capacity())
			_Realocate(_used + minReservedSize);
		return _buf + _used++;
	}


	basic_array & operator += (const _T &src) { _Append(src._used, src._buf); return *this; }
	basic_array & operator = (const _T& src) { _used = 0; _Append(src._used, src._buf); return *this; }

	bool operator == (const _B &src) const {
		if (src._buf == _buf)
			return true;
		if (src._used != _used)
			return false;
		return memcmp(src._buf, _buf, _used) == 0;
	}

	void clear() { _used = 0; }

	void reset()
	{
		_used = 0;
		_reserved = 0;
		if (_buf)
		{
			_allocator::deallocate(_buf);
			_buf = nullptr;
		}
	}
};

template <typename T, class Alloc = Allocators::Default, SIZE_T minReservedSize = 48>
class object_array : public basic_array_base<T>
{
	typedef object_array<T, Alloc, minReservedSize> _T;
	typedef basic_array_base<T> _B;
	typedef MemoryAllocatorGlobal<Alloc> _allocator;

	void _Realocate(SIZE_T newSize)
	{
		T *oldBuf = _buf;
		SIZE_T oldReserved = _reserved;
		_reserved = newSize;
		_Allocate();

		if (oldBuf && _used)
			_Copy(_buf, oldBuf, _used);

		if (oldBuf)
			_allocator::deallocate(oldBuf);
	}

	void _Construct(T *dst, const T *src, SIZE_T len)
	{
		while (len)
		{
			new (*dst) T(*src);
			++dst;
			++src;
			--len;
		}
	}

	void _Copy(T *dst, const T *src, SIZE_T len)
	{
		size_t s = len * sizeof(T);
		memcpy_s(dst, s, src, s);
	}

	void _Append(SIZE_T len, const T *src)
	{
		if (_reserved < _used + len)
			_Realocate(_used + len + minReservedSize);

		if (len && src)
		{
			_Construct(_buf + _used, src, len);
			_used += len;
		}
	}

	void _Allocate() { _buf = static_cast<T *>(_allocator::allocate(_reserved * sizeof(T))); }

public:
	object_array() { }
	object_array(SIZE_T s) : _B(s,s) { _Allocate(); for (SIZE_T i = 0; i < _used; ++i) new (_buf + i) T(); }
	object_array(const _B &src) : _B(src._reserved, src._used) { _Allocate(); _Construct(_buf, src._buf, src._used); }
	object_array(std::initializer_list<T> list) : _B(list.size()) { _Allocate(); _Append(list.size(), list.begin()); }
	object_array(_T &&src) : _B(src._reserved, src._used, src._buf) { src._reserved = 0; src._used = 0; src._buf = nullptr; }

	~object_array() { if (_buf) _allocator::deallocate(_buf); }

	T operator[](SIZE_T pos) const { return (pos < _used && pos >= 0) ? _buf[pos] : 0; };
	T& operator[](SIZE_T pos) { return (pos < _used && pos >= 0) ? _buf[pos] : _buf[0]; };

	SIZE_T size() const { return _used; }
	SIZE_T capacity() const { return _reserved; }

	T *begin() { return _buf; }
	T *end() { return _buf + _used; }

	void push_back(const T &v) {
		if (size() == capacity())
			_Realocate(_used + minReservedSize);

		// we must call constructor for new object
		new (_buf + _used) T(v);
		++_used;
	}

	void push_back(T && v) {
		if (size() == capacity())
			_Realocate(_used + minReservedSize);

		new (_buf + _used) T(std::move(v));
		++_used;
	}

	object_array & operator += (const _T &src) { _Append(src._used, src._buf); return *this; }
	object_array & operator = (const _T& src) { clear(); _Append(src._used, src._buf); return *this; }
	object_array & operator = (_T&& src) {
		clear();

		if (_buf)
			_allocator::deallocate(oldBuf);

		_reserved = src._reserved;
		_used = src._used;
		_buf = src._buf;

		src._reserved = 0; src._used = 0; src._buf = nullptr;

		return *this; 
	}

	bool operator == (const _B &src) const {
		if (src._buf == _buf)
			return true;
		if (src._used != _used)
			return false;
		for (SIZE_T i = 0; i < _used; ++i)
			if (src._buf[i] != _buf[i])
				return false;

		return true;
	}

	void clear() { 
		// call destructor on every object stored in array
		for (auto *p : *this)
			p->~T();

		_used = 0; 
	}
};

/// <summary>
/// Extension class template for shared objects. Like shared_string.
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
		if (src.refCounter > 1)
		{
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

	inline T & GetValue() const { return _pool[_idx].value; }

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
};


template <typename T, class A, SIZE_T S>
typename shared_base<T, A, S>::Pool shared_base<T, A, S>::_pool;

template <typename T, class A, SIZE_T S>
U32 shared_base<T, A, S>::_firstFreeEntry = 0;

template <typename T, class A, SIZE_T S>
U32 shared_base<T, A, S>::_used = 0;

template <typename T, typename U = U32, class A = Allocators::Default, SIZE_T S = 48, SIZE_T S2 = 16>
class shared_string : public shared_base< basic_string<T, U, A, S>, A, S2>
{
public:
	shared_string() : shared_base(0) { CheckIfInitialized();  AddRef(); }
	shared_string(const shared_string &src) : shared_base(src._idx) { AddRef(); }
	shared_string(shared_string &&src) : shared_base(src._idx) { src._idx = 0; }
	template<typename ST>
	shared_string(const ST *src, const ST *end = nullptr) { MakeNewEntry(src, end); }
	template<typename ST, typename SU>
	shared_string(const basic_string_base<ST, SU> &src) { MakeNewEntry(src); }
	template<typename ST, typename SU>
	shared_string(const basic_string_base<ST, SU> *src) { MakeNewEntry(src); }
	~shared_string() { Release(); }

	template<typename V>
	shared_string& operator = (const basic_string_base<T, V>& src)       { NeedUniq(); GetValue() = src; return *this; }
	shared_string& operator = (const T* src)                             { NeedUniq(); GetValue() = src; return *this; }
	shared_string& operator = (const shared_string& src)                 { Release(); _idx = src._idx; AddRef(); return *this; }
	shared_string& operator = (shared_string&& src)                      { Release(); _idx = src._idx; src._idx = 0; return *this; }

	T operator[](SIZE_T pos) const                                       { return GetValue()[pos]; }
	T& operator[](SIZE_T pos)                                            { NeedUniq(); return GetValue()[pos]; }

	template<typename V>
	shared_string & operator += (const basic_string_base<T, V>& src)     { NeedUniq(); GetValue() += src; return *this; }
	shared_string & operator += (const T *src)                           { NeedUniq(); GetValue() += src; return *this; }
	shared_string & operator += (const shared_string &src)               { NeedUniq(); GetValue() += src.GetValue(); return *this; }

	template<typename V>
	shared_string operator + (const basic_string_base<T, V>& src)  const { shared_string tmp(GetValue()); tmp += src; return tmp; }
	shared_string operator + (const T *src) const                        { shared_string tmp(GetValue()); tmp += src; return tmp; }
	shared_string operator + (const shared_string &src) const            { shared_string tmp(GetValue()); tmp += src; return tmp; }

	bool operator == (const basic_string_base<T, U> &str) const          { return GetValue() == str; }
	bool operator == (const shared_string &str) const                    { return GetValue() == str.GetValue(); }

	SIZE_T size() const                                                  { return GetValue().size(); }

	operator const basic_string_base<T, U> ()                            { return GetValue(); } // It is safe. Dont have to "uniq", because basic_string_base is used only as arg (read only).
	const basic_string_base<T, U> ToBasicString() const                  { return GetValue(); } // It is safe. Dont have to "uniq", because basic_string_base is used only as arg (read only).

	const T * c_str()                                                    { return GetValue().c_str(); }

	const T * begin()                                                    { return GetValue()._buf; }
	const T * end()                                                      { auto &v = GetValue();  return v._buf + v._used; }

	bool wildcard(const shared_string& pat, U pos = 0, U patPos = 0) const { return GetValue().wildcard(pat.ToBasicString(), pos, patPos); }

	template <typename V>
	bool wildcard(const basic_string_base<T, V>& pat, U pos = 0, V patPos = 0) const { return GetValue().wildcard(pat, pos, patPos); }

	template <typename V>
	bool iwildcard(const basic_string_base<T, V>& pat, U pos = 0, V patPos = 0) const { return GetValue().iwildcard(pat, pos, patPos); }
};


// ============================================================================


typedef shared_string<char> STR;
typedef shared_string<char, U16> ShortSTR;
typedef shared_string<wchar_t> WSTR;
typedef shared_string<wchar_t, U32, Allocators::Default, 250> PathSTR;
typedef simple_string<wchar_t, U32> WSimpleString;
typedef simple_string<char, U32> SimpleString;

// ============================================================================

/**
 * Basic ringbuffer. No thread safe. No resize when bad_alloc exeption.
 * It uses RingBuffer memory allocator.
 */
template <typename T, class Alloc = Allocators::Default>
class base_ringbuffer : MemoryAllocatorGlobal<Alloc>
{
public:
	typedef MemoryAllocatorGlobal<Alloc> _allocator;

	base_ringbuffer() = delete;

	base_ringbuffer(SIZE_T cap = 8192) : _counter(0) 
	{ 
		_ring_buffer_alloc = make_new<Allocators::RingBuffer>(GetMemoryAllocator(), cap); 
	}

	virtual ~base_ringbuffer() 
	{ 
		make_delete<Allocators::RingBuffer>(_ring_buffer_alloc);
	}

	/// It not only adds new entry to ring buffer. I works also as contructor.
	template<class... Args>
	void push_back(Args &&... args)
	{
		void *p = _ring_buffer_alloc->allocate(sizeof(T));
		new (p) T(_ring_buffer_alloc, std::forward<Args>(args)...);
		++_counter;
	}

	void pop()
	{
		assert(_counter > 0);
		T *p = front();
		p->~T();
		_ring_buffer_alloc->deallocate(p);
		--_counter;
	}

	T* front()
	{
		assert(_counter > 0);
		return reinterpret_cast<T*>(_ring_buffer_alloc->getTail());
	}

	bool empty() { return _counter == 0; }
	SIZE_T size() { return _counter; }
	SIZE_T free() { return _ring_buffer_alloc->getFree(); }

	void resize(SIZE_T newCapacity)
	{
		_resize(newCapacity);
	}

	void clear()
	{
		_ring_buffer_alloc->clear();
		_counter = 0;
	}

private:
	SIZE_T _counter = 0;
	Allocators::RingBuffer *_ring_buffer_alloc;

	void _resize(SIZE_T newCapacity)
	{
		Allocators::RingBuffer *new_ring_buffer_alloc = make_new<Allocators::RingBuffer>(GetMemoryAllocator(), newCapacity);
		for (SIZE_T i = _counter; i > 0; --i)
		{
			T* old = reinterpret_cast<T*>(_ring_buffer_alloc->getTail());

			auto p = new_ring_buffer_alloc->allocate(sizeof(T));
			new (p) T(new_ring_buffer_alloc, *old);
			old->~T();
			_ring_buffer_alloc->deallocate(old);
		}
		make_delete<Allocators::RingBuffer>(_ring_buffer_alloc);
		_ring_buffer_alloc = new_ring_buffer_alloc;
	}
};

template <class T, class Alloc>
using base_queue = base_ringbuffer<T, Alloc>;

// ============================================================================

/**
 * Threasafe version of ringbuffer designed for many-creators -> one-or-many-consumer
 */
 // ============================================================================

 /**
 * It uses RingBuffer memory allocator.
 */
template <typename T, class Alloc = Allocators::Default>
class base_ringbuffer_threadsafe : MemoryAllocatorGlobal<Alloc>
{
public:
	typedef MemoryAllocatorGlobal<Alloc> _allocator;

	base_ringbuffer_threadsafe() = delete;

	base_ringbuffer_threadsafe(SIZE_T cap = 8192) : _counter(0)
	{
		_ring_buffer_alloc = make_new<Allocators::RingBuffer>(GetMemoryAllocator(), cap);
	}

	virtual ~base_ringbuffer_threadsafe()
	{
		make_delete<Allocators::RingBuffer>(_ring_buffer_alloc);
	}

	/// It not only adds new entry to ring buffer. I works also as contructor.
	template<class... Args>
	void push_back(Args &&... args)
	{
		std::lock_guard<std::mutex> lck(_write_mutex);
		try {
			void *p = _ring_buffer_alloc->allocate(sizeof(T));
			new (p) T(_ring_buffer_alloc, std::forward<Args>(args)...);
		}
		catch (std::bad_alloc &)
		{
			// resize
			std::lock_guard<std::mutex> lck(_read_mutex);
			_resize(_ring_buffer_alloc->getCapacity() * 2);

			// try again
			void *p = _ring_buffer_alloc->allocate(sizeof(T));
			new (p) T(_ring_buffer_alloc, std::forward<Args>(args)...);
		}
		++_counter;
	}

	void pop()
	{
		lock();
		pop_and_unlock();
	}

	T* front()
	{
		assert(_counter > 0);
		return reinterpret_cast<T*>(_ring_buffer_alloc->getTail());
	}

	void lock() { _read_mutex.lock(); }
	void unlock() { _read_mutex.unlock(); }

	T* lock_and_front()
	{
		lock();
		return front();
	}

	void pop_and_unlock()
	{
		std::lock_guard<std::mutex> lck(_write_mutex);
		assert(_counter > 0);
		T *p = front();
		p->~T();
		_ring_buffer_alloc->deallocate(p);
		--_counter;
		_read_mutex.unlock();
	}

	bool empty() { return _counter == 0; }
	SIZE_T size() { return _counter; }
	SIZE_T free() { return _ring_buffer_alloc->getFree(); }

	void resize(SIZE_T newCapacity)
	{
		std::lock_guard<std::mutex> rlck(_read_mutex);
		std::lock_guard<std::mutex> wlck(_write_mutex);
		_resize(newCapacity);
	}

	void clear()
	{
		std::lock_guard<std::mutex> rlck(_read_mutex);
		std::lock_guard<std::mutex> wlck(_write_mutex);
		_ring_buffer_alloc->clear();
		_counter = 0;
	}

private:
	SIZE_T _counter = 0;
	Allocators::RingBuffer *_ring_buffer_alloc;
	std::mutex _write_mutex;
	std::mutex _read_mutex;

	void _resize(SIZE_T newCapacity)
	{
		Allocators::RingBuffer *new_ring_buffer_alloc = make_new<Allocators::RingBuffer>(GetMemoryAllocator(), newCapacity);
		for (SIZE_T i = _counter; i > 0; --i)
		{
			T* old = reinterpret_cast<T*>(_ring_buffer_alloc->getTail());

			auto p = new_ring_buffer_alloc->allocate(sizeof(T));
			new (p) T(new_ring_buffer_alloc, *old);
			old->~T();
			_ring_buffer_alloc->deallocate(old);
		}
		make_delete<Allocators::RingBuffer>(_ring_buffer_alloc);
		_ring_buffer_alloc = new_ring_buffer_alloc;
	}
};

template <class T, class Alloc = Allocators::Default>
using base_queue_threadsafe = base_ringbuffer_threadsafe<T, Alloc>;

/*
template <typename T, typename U = U32, class Alloc = Allocators::Default, SIZE_T minReservedSize = 48>
class shared_string
{
private:
	typedef shared_string<T, U, Alloc, minReservedSize > _T;
	typedef basic_string<T, U, Alloc, minReservedSize > _B;
	struct Entry
	{
		_B value;
		U32 refCounter;
	};
	typedef basic_array<Entry, Alloc> PoolOfStrings;

	static PoolOfStrings _pool;	
	static U32 _freeEntry;

	/// <summary>
	/// Get empty entry in <see cref="_pool"/>.
	/// </summary>
	/// <remarks>
	/// <para>If we have empty, unused item in <see cref="_pool"/>, it will return that item.</para>
	/// <para>Otherwise, it will create new entry in <see cref="_pool"/>.</para>
	/// </remarks>
	/// <param name="args">Arguments passed to value constructor.</param>
	/// <returns>Index in <see cref="_pool"/>.</returns>
	template<class... Args>
	static U32 make_new_entry(Args &&... args)
	{
		U32 idx = _freeEntry;
		Entry *pEntry;
		if (_freeEntry)
		{
			pEntry = &_pool[idx];
			_freeEntry = pEntry->refCounter;
		}
		else
		{
			idx = static_cast<U32>(_pool.size());
			pEntry = _pool.add_empty();
		}
		pEntry->refCounter = 1;
		new (&pEntry->value) _B(std::forward<Args>(args)...);

		return idx;
	}

	/// only real data is _idx
	U32 _idx;
	inline void AddRef() { ++_pool[_idx].refCounter; }
	inline U32 Release() { return --_pool[_idx].refCounter; }
	
	/// <summary>Called, when member function need unique value (not shared with others).</summary>
	inline void NeedUniq()
	{ 
		auto &src = _pool[_idx];
		if (src.refCounter > 1)
		{
			--src.refCounter;
			_idx = make_new_entry(src.value);
		}
	}
public:
	shared_string() : _idx(0) { AddRef(); }

	template<typename ST>
	shared_string(const ST *src, const ST *end = nullptr) { _idx = make_new_entry(src, end); }

	template<typename ST, typename SU>
	shared_string(const basic_string_base<ST, SU> &src) { _idx = make_new_entry(src); }

	shared_string(const _T &src) : _idx(src._idx) { AddRef(); } 
	shared_string(_T &&src) : _idx(src._idx) { }
	~shared_string() {
		if (!Release())
		{
			auto entry = _pool[_idx];
			// We will release memory used by string, but entry stays in _pool. It is now empty.
			reinterpret_cast<_B*>(&entry.value)->~_B();

			// We will use refCounter as index to previous empty.
			entry.refCounter = _freeEntry;
			_freeEntry = _idx;
		}
	}
};

/// <summary>
/// The pool used to store all entries (data).
/// </summary>
template <typename T, typename U, class Alloc, SIZE_T minReservedSize>
typename shared_string<T, U, Alloc, minReservedSize>::PoolOfStrings shared_string<T, U, Alloc, minReservedSize>::_pool;

/// <summary>
/// Index of first free entry in <see cref="_pool">.
/// </summary>
template <typename T, typename U, class Alloc, SIZE_T minReservedSize>
U32 shared_string<T, U, Alloc, minReservedSize>::_freeEntry = 0;

*/

/**
* === WASTE LAND ===
*
* PART OF CODE NOT USED ANY MORE.
*

typedef std::basic_string<char, std::char_traits<char>, stl_allocator<char>  > STR;
typedef std::basic_string<char, std::char_traits<char>, stl_allocator<char, NAMESPACE_CORE_USING::Allocators::Stack>  > TSTR; // temporary used string .... on stack


struct SimpleString_base
{
char *_buf;
U32 _reserved;
U32 _used;

SimpleString_base() {}
SimpleString_base(U32 reserved, U32 used = 0, char *buf = nullptr) : _reserved(reserved), _used(used), _buf(buf) {}
SimpleString_base(const SimpleString_base &src) : _reserved(src._reserved), _used(src._used), _buf(nullptr) {}
};

template<class Alloc = Allocators::Default>
class  SimpleString : public SimpleString_base
{
private:
typedef MemoryAllocator<Alloc> _allocator;

U32 _defaultSize;

void _Append(U32 len, const char *src)
{
char *oldBuf = nullptr;

if (_reserved < _used + len)
{
oldBuf = _buf;
_reserved = _used + len + _defaultSize;
_Allocate();

if (oldBuf && _used)
memcpy_s(_buf, _reserved, oldBuf, _used);
}

if (len && src)
{
memcpy_s(_buf + _used, _reserved - _used, src, len);
_used += len;
}

// oldBuf is released here, because there is chance, that we copy part from source buffer
if (oldBuf)
_allocator::deallocate(oldBuf);
}

void _Allocate() { _buf = static_cast<char *>(_allocator::allocate(_reserved)); }

public:
SimpleString(U32 size = 48) : _defaultSize(size) {}
SimpleString(const char *src) { _used = static_cast<U32>(strlen(src)); _reserved = max(_used, _defaultSize); _Allocate();  memcpy_s(_buf, _reserved, src, _used); }
SimpleString(const SimpleString_base &src) : SimpleString_base(src) { _Allocate();  memcpy_s(_buf, _reserved, src._buf, _used); }
SimpleString(SimpleString &&src) : SimpleString_base(src) { _buf = src._buf; src._used = 0; src._reserved = 0; src._buf = nullptr; }
~SimpleString() { if (_buf) _allocator::deallocate(_buf); }

SimpleString& operator=(const char* src) { _used = 0; U32 len = strlen(src);  _Append(len, src); return *this; }
SimpleString& operator=(const SimpleString_base& src) { _used = 0; _Append(src._used, src._buf); return *this; }
SimpleString& operator=(const SimpleString& src) { _used = 0; _Append(src._used, src._buf); return *this; }

#ifdef _DEBUG
char operator[](U32 pos) const { return (pos < _used && pos >= 0) ? _buf[pos] : 0; };
char& operator[](U32 pos) { return (pos < _used && pos >= 0) ? _buf[pos] : _buf[0]; };
#else
char operator[](U32 pos) const { return _buf[pos]; };
char& operator[](U32 pos) { return _buf[pos]; };
#endif

SimpleString & operator += (const char *src) { U32 len = static_cast<U32>(strlen(src)); _Append(len, src); return *this; }
SimpleString & operator += (const SimpleString_base &src) { _Append(src._used, src._buf);  return *this; }
//		SimpleString & operator += (const SimpleString &src) { _Append(src._used), src._buf);  return *this; }

SimpleString operator + (const char *src) const { return SimpleString(*this) += src; }
SimpleString operator + (const SimpleString_base &src)  const { return SimpleString(*this) += src; }
//		SimpleString operator + (const SimpleString &src)  const { return SimpleString(*this) += src; }

SIZE_T size() const { return _used; }

operator SimpleString_base() { return *this; }

};

template<typename T>
class relative_ptr {
private:
size_t offset;

public:
relative_ptr() = default;
relative_ptr(void *m) { offset = (size_t)(m)-(size_t)(this); }

relative_ptr<T> & operator = (relative_ptr<T> &) = delete;
relative_ptr<T> & operator = (const T m) { offset = (size_t)(m)-(size_t)(this); return *this; }
operator T() { return (T)(size_t(this) + offset); }
};


template<typename T, SIZE_T MIN_EXTRA_MEMORY = 512, SIZE_T MEMORY_BLOCK_SIZE = 1024>
struct BAMS_EXPORT ExtraMemory
{
	template<typename U>
	class BAMS_EXPORT relative_ptr {
	private:
		size_t offset;

	public:
		relative_ptr() = default;
		relative_ptr(void *m) { offset = (size_t)(m)-(size_t)(this); }

		relative_ptr<U> & operator = (relative_ptr<U> &) = delete;
		relative_ptr<U> & operator = (const U m) { offset = (size_t)(m)-(size_t)(this); return *this; }
		operator U() { return (U)(size_t(this) + offset); }
	};

	static const SIZE_T REQ_SIZE;

	static T * Create()
	{
		return new (T::allocate(REQ_SIZE))T();
	}


};

template<typename T, SIZE_T MIN_EXTRA_MEMORY, SIZE_T MEMORY_BLOCK_SIZE>
const SIZE_T ExtraMemory<T, MIN_EXTRA_MEMORY, MEMORY_BLOCK_SIZE>::REQ_SIZE = ((sizeof(T) + MIN_EXTRA_MEMORY + MEMORY_BLOCK_SIZE - 1) / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE;
*/