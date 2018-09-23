/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

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
	typedef MemoryAllocatorStatic<Alloc> _allocator;

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
	basic_array(std::initializer_list<T> list) : _B(list.size()) { _Allocate();	_Append(list.size(), list.begin()); }
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

	void resize(SIZE_T newSize) { _Realocate(newSize); }
};

template <typename T, class Alloc = Allocators::Default, SIZE_T minReservedSize = 48>
class object_array : public basic_array_base<T>
{
	typedef object_array<T, Alloc, minReservedSize> _T;
	typedef basic_array_base<T> _B;
	typedef MemoryAllocatorStatic<Alloc> _allocator;

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
	object_array(SIZE_T s) : _B(s, s) { _Allocate(); for (SIZE_T i = 0; i < _used; ++i) new (_buf + i) T(); }
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
