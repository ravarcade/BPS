/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

/// some parts base on: https://codereview.stackexchange.com/questions/180058/a-vector-implementation

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
template <typename T, MemoryAllocator _allocator = Allocators::default, SIZE_T minReservedSize = 48>
class basic_array : public basic_array_base<T>
{
	typedef basic_array<T, _allocator, minReservedSize> _T;
	typedef basic_array_base<T> _B;


	void _Realocate(SIZE_T newSize)
	{
		T *oldBuf = _buf;
		SIZE_T oldReserved = _reserved;
		_reserved = newSize;
		_Allocate();

		if (oldBuf && _used)
			_Copy(_buf, oldBuf, _used);

		if (oldBuf)
			_allocator->deallocate(oldBuf);
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

	void _Allocate() { _buf = static_cast<T *>(_allocator->allocate(_reserved * sizeof(T))); }

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

	~basic_array() { if (_buf) _allocator->deallocate(_buf); }

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
			_allocator->deallocate(_buf);
			_buf = nullptr;
		}
	}

	void resize(SIZE_T newSize) { _Realocate(newSize); }

	template<typename Callback>
	void foreach(Callback &callback) 
	{
		for (auto f = begin(), e = end(); f != e; ++f)
			callback(*f);
	}
};

template <typename T, MemoryAllocator _allocator = Allocators::default, SIZE_T minReservedSize = 1>
class array
{
public:
	using value_type = T;
	using reference = T & ;
	using const_reference = const T&;
	using pointer = T * ;
	using const_pointer = const T*;
	using difference_type = ptrdiff_t;
	using size_type = U32;

	using iterator = T * ;
	using const_iterator = const T*;

	//	using reverse_iterator = std::reverse_iterator<iterator>;
	//	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	//	typedef array<T, _allocator, minReservedSize> _T;
	//	typedef basic_array_base<T> _B;


	array() noexcept {}

	explicit array(size_type n) 
	{
		_used = n;
		_reserved = _used;
		_allocate();
		_defaultConstruct<T>(_storage, _used);
	}

	array(const_iterator first, const_iterator last)
	{
		_used = (last - first) / sizeof(T);
		_reserved = _used;
		_allocate();
		_copyAll(_storage, first);
	}

	array(const array & src)
	{
		_used = src._used;
		_reserved = _used;
		_allocate();
		_copyAll(_storage, src._storage);
	}

	array(array && src) noexcept
	{
		swap(src);
	}

	~array()
	{
		if (_storage)
		{
			_deleteAll();
			_allocator->deallocate(_storage);
		}
	}

	array<T>& operator=(const array<T>& src)
	{
		if (src._used > _reserved)
		{
			swap(array<T>(src));
		}
		else
		{
			clear();
			_used = src._used;
			_moveAll(src._storage, _storage);			
		}
		return *this;
	}
	
	array<T>& operator=(array<T>&& src)
	{
		swap(src);
		return this;
	}

	reference operator[](size_type i) { return _storage[i]; }
	const_reference operator[](size_type i) const { return _storage[i]; }

	void push_back(const T& val) { emplace_back(val); }
	void push_back(T&& rval) { emplace_back(std::forward(rval)); }

	template <typename ... Args> reference emplace_back(Args&& ... args)
	{
		if (_used == _reserved)
		{
			_reserved += _reserved / 2 + minReservedSize;
			_reallocate();
		}
		new (_storage + _used)T(std::forward<Args>(args) ...);

		return _storage[_used++];
	}


	void swap(array && src)
	{
		std::swap(_used, src._used);
		std::swap(_reserved, src._reserved);
		std::swap(_storage, src._storage);
	}

	void clear()
	{
		if (_storage)
		{
			_deleteAll();
			_used = 0;
		}
	}

	void resize(size_type s)
	{
		// delete removed
		if (s < _used)
			_deleteRange<T>(_storage + s, _storage + _used);

		auto oldSize = _used;

		if (s > _reserved)
		{
			_reserved = s;
			_reallocate();
		}

		if (s > _used)
		{
			_defaultConstruct<T>(_storage + _used, s - _used);
		}

		_used = s;
	}

	void reserve(size_type s)
	{
		if (s == _reserved)
			return;

		if (s < _used) {
			_deleteRange<T>(_storage + s, _storage + _used);
			_used = s;
		}

		_reserved = s;
		_reallocate();
	}

	iterator begin() { return _storage; }
	iterator end() { return _storage + _used; }

 private:
	value_type * _storage = nullptr;
	size_type _used = 0;
	size_type _reserved = 0;

	void _allocate() { _storage = static_cast<T *>(_allocator->allocate(_reserved * sizeof(T))); }
	void _reallocate() 
	{
		T* new_storage = static_cast<T *>(_allocator->allocate(_reserved * sizeof(T)));
		if (_storage)
		{
			_moveAll(new_storage, _storage);
			_realocateDelte<T>();
		}
		
		_storage = new_storage;
	}
	template<typename X>
	typename std::enable_if<std::is_trivially_move_constructible<X>::value == false>::type
		_realocateDelte() {  _deleteAll(); _allocator->deallocate(_storage); }

	template<typename X>
	typename std::enable_if<std::is_trivially_move_constructible<X>::value == true>::type
		_realocateDelte() { _allocator->deallocate(_storage); }


	template<typename X>
	typename std::enable_if<std::is_trivially_constructible<X>::value == false>::type
		_defaultConstruct(T *dst, size_type num) { for (size_type i = 0; i < num; ++i) new(_storage + i)T();  }

	template<typename X>
	typename std::enable_if<std::is_trivially_constructible<X>::value == true>::type
		_defaultConstruct(T *dst, size_type num) { memset(dst, 0, sizeof(T)*num); }


	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == false>::type
		_deleteRange(T* f, T* l) { while (f != l) { auto tmp = f; ++f; tmp->~T(); } }
	
	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == true>::type
		_deleteRange(T* f, T* l) {} // nothing to do


	template<typename X>
	typename std::enable_if<std::is_trivially_copy_constructible<X>::value == false>::type
		_copyRange(T* dst, const_iterator f, const_iterator e) { for (; f != e; ++f, ++dst) new(dst)T(*f); }

	template<typename X>
	typename std::enable_if<std::is_trivially_copy_constructible<X>::value == true>::type
		_copyRange(T* dst, const_iterator f, const_iterator e) {
		size_t diff = reinterpret_cast<difference_type>(e) - reinterpret_cast<difference_type>(f);
		memcpy_s(dst, diff, f, diff);
	}


	template<typename X>
	typename std::enable_if<std::is_trivially_move_constructible<X>::value == false>::type
		_moveRange(T* dst, const_iterator f, const_iterator e) { 
		for (; f != e; ++f, ++dst) 
			new(dst)T(*f); 
	}

	template<typename X>
	typename std::enable_if<std::is_trivially_move_constructible<X>::value == true>::type
		_moveRange(T* dst, const_iterator f, const_iterator e) { 
		size_t diff = reinterpret_cast<difference_type>(e) - reinterpret_cast<difference_type>(f);
		memcpy_s(dst, diff, f, diff); 
	}


	void _deleteAll() { _deleteRange<T>(_storage, _storage + _used); }
	void _moveAll(T* dst, const_iterator f) { _moveRange<T>(dst, f, f + _used); }
	void _copyAll(T* dst, const_iterator f) { _copyRange<T>(dst, f, f + _used); }

	
/*

	// ======================================================================== "not trivial" ===
	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == false>::type
		deleteObjects()
	{
		foreach([&](T *o) {
			o->~T();
		});
	}

	// ======================================================================== "trivial" ===
	template<typename X>
	typename std::enable_if<std::is_trivially_destructible<X>::value == true>::type
		deleteObjects()
	{
		// Trivially destructible objects can be reused without using the destructor.
	}


	void _Realocate(SIZE_T newSize)
	{
		T *oldBuf = _buf;
		SIZE_T oldReserved = _reserved;
		_reserved = newSize;
		_Allocate();

		if (oldBuf && _used)
			_Copy(_buf, oldBuf, _used);

		if (oldBuf)
			_allocator->deallocate(oldBuf);
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

	void _Allocate() { _buf = static_cast<T *>(_allocator->allocate(_reserved * sizeof(T))); }

public:
	array() { }
	array(SIZE_T s) : _B(s, s) { _Allocate(); for (SIZE_T i = 0; i < _used; ++i) new (_buf + i) T(); }
	array(const _B &src) : _B(src._reserved, src._used) { _Allocate(); _Construct(_buf, src._buf, src._used); }
	array(std::initializer_list<T> list) : _B(list.size()) { _Allocate(); _Append(list.size(), list.begin()); }
	array(_T &&src) : _B(src._reserved, src._used, src._buf) { src._reserved = 0; src._used = 0; src._buf = nullptr; }

	~array() { if (_buf) _allocator->deallocate(_buf); }

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

	array & operator += (const _T &src) { _Append(src._used, src._buf); return *this; }
	array & operator = (const _T& src) { clear(); _Append(src._used, src._buf); return *this; }
	array & operator = (_T&& src) {
		clear();

		if (_buf)
			_allocator->deallocate(oldBuf);

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
	*/
};
