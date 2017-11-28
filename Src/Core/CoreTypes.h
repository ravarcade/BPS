/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
*
*/


template <typename T, typename U>
struct basic_string_base
{
	T *_buf;
	U _reserved;
	U _used;
	basic_string_base() {}
	basic_string_base(U reserved, U used = 0, T* buf = nullptr) : _reserved(reserved), _used(used), _buf(buf) {}
	basic_string_base(const basic_string_base<T, U> &s) : _reserved(s._reserved), _used(s._used) {}
};

typedef basic_string_base<char, U16> basic_string_base_U16;
typedef basic_string_base<char, U32> basic_string_base_U32;
typedef basic_string_base<wchar_t, U16> basic_string_base_WU16;
typedef basic_string_base<wchar_t, U32> basic_string_base_WU32;

template <typename T, typename U = U32, class Alloc = Allocators::Default, SIZE_T minReservedSize = 48>
class basic_string : public basic_string_base<T, U>
{
private:
	typedef basic_string<T, U, Alloc, minReservedSize> _T;
	typedef basic_string_base<T, U> _B;
	typedef MemoryAllocator<Alloc> _allocator;

	void _Reserve(U newSize)
	{
		T* old = _buf;
		_reserved = max(newSize, minReservedSize);
		_Allocate();

		if (old)
		{
			if (_used)
				memcpy_s(_buf, _reserved, old, _used);

			_allocator::deallocate(old);
		}
	}

	void _Append(U len, const T *src)
	{
		T *oldBuf = nullptr;

		if (_reserved <= _used + len) // yes.... we will have alway atleast 1 byte spared space.... for "\0" to make c-string
		{
			oldBuf = _buf;
			_reserved = _used + len + minReservedSize;
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

	void _Allocate() { _buf = static_cast<T *>(_allocator::allocate(_reserved)); }

public:
	basic_string() : _B(minReservedSize) { _Allocate(); }
	basic_string(const T *src, const T *end = nullptr) { _used = end ? end - src : static_cast<U>(strlen(src)); _reserved = max(_used, static_cast<U>(minReservedSize)); _Allocate();  memcpy_s(_buf, _reserved, src, _used); }
	template<typename V>
	basic_string(const basic_string_base<T, V> &src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used)) { _Allocate(); memcpy_s(_buf, _reserved, src._buf, _used); }

	//	basic_string(const basic_string_base_U16 &src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used)) { _Allocate(); memcpy_s(_buf, _reserved, src._buf, _used); }
	//	basic_string(const basic_string_base_U32 &src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used)) { _Allocate(); memcpy_s(_buf, _reserved, src._buf, _used); }

	basic_string(_T &&src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used), src._buf) { src._used = 0; src._reserved = 0; src._buf = nullptr; }
	~basic_string() { if (_buf) _allocator::deallocate(_buf); }


	basic_string& operator=(const T* src) { _used = 0; U len = strlen(src);  _Append(len, src); return *this; }
	template<typename V>
	basic_string& operator=(const basic_string_base<T, V>& src) { _used = 0; _Append(static_cast<U>(src._used), src._buf); return *this; }
	//	basic_string& operator=(const basic_string_base_U16& src) { _used = 0; _Append(src._used, src._buf); return *this; }
	//	basic_string& operator=(const basic_string_base_U32& src) { _used = 0; _Append(src._used, src._buf); return *this; }
	basic_string& operator=(const _T& src) { _used = 0; _Append(src._used, src._buf); return *this; }

	T operator[](SIZE_T pos) const { return (pos < _used && pos >= 0) ? _buf[pos] : 0; };
	T& operator[](SIZE_T pos) { return (pos < _used && pos >= 0) ? _buf[pos] : _buf[0]; };

	basic_string & operator += (const T *src) { U len = static_cast<U>(strlen(src)); _Append(len, src); return *this; }
	template<typename V>
	basic_string & operator += (const basic_string_base<T, V>& src) { _Append(static_cast<U>(src._used), src._buf);  return *this; }
	//	basic_string & operator += (const basic_string_base_U16 &src) { _Append(static_cast<U>(src._used), src._buf);  return *this; }
	//	basic_string & operator += (const basic_string_base_U32 &src) { _Append(static_cast<U>(src._used), src._buf);  return *this; }

	basic_string operator + (const T *src) const { basic_string tmp(*(static_cast<const _B *>(this))); tmp += src;	return tmp; }
	template<typename V>
	basic_string operator + (const basic_string_base<T, V>& src)  const { basic_string tmp(*(static_cast<const _B *>(this))); tmp += src;	return tmp; }
	//	basic_string operator + (const basic_string_base_U16 &src)  const { basic_string tmp(*(static_cast<const _B *>(this))); tmp += src;	return tmp; }
	//	basic_string operator + (const basic_string_base_U32 &src)  const { basic_string tmp(*(static_cast<const _B *>(this))); tmp += src;	return tmp; }

	bool operator == (const basic_string_base<T, U> &str) const {
		if (str._buf == _buf)
			return true;
		if (str._used != _used)
			return false;
		return strncmp(str._buf, _buf, _used) == 0;
	}

	SIZE_T size() const { return _used; }

	operator _B() { return *this; }

	operator const T *() { 
		if (_used == _reserved)
			_Append(1, "");
		_buf[_used] = 0;

		return _buf; 
	}
};

typedef basic_string<char> STR;
typedef basic_string<char, U16> ShortSTR;
typedef basic_string<char, U16, BAMS::CORE::Allocators::Default, 250> PathSTR;

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