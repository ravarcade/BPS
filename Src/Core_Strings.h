/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

/// <summary>
/// Basic string. It has only info about where in memory is string, length, 
/// </summary>
template <typename T, typename U>
struct basic_string_base
{
	T *_buf;
	U _reserved;
	U _used;
	basic_string_base() {}
	basic_string_base(U reserved, U used = 0, T* buf = nullptr) : _reserved(reserved), _used(used), _buf(buf) {}
	basic_string_base(U used, T *buf) : _buf(buf), _used(used), _reserved(used) {}
	basic_string_base(basic_string_base &src) : _buf(src._buf), _used(src._used), _reserved(src._reserved) {} // copy contructor

	inline const basic_string_base &ToBasicString() const { return *this; }

	SIZE_T size() const { return _used; }

	static SIZE_T length(const T *txt) { return strlen(txt); }
	static T tolower(T c) { return ::tolower(c); }
	static int ncmp(const T *string1, const T *string2, size_t count) { return strncmp(string1, string2, count); }
	static int nicmp(const T *string1, const T *string2, size_t count) { return strnicmp(string1, string2, count); }
};

DEFINE_HAS_METHOD(ToBasicString);
DEFINE_HAS_METHOD(GetValue);

// ============================================================================

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
	simple_string(IMemoryAllocator *alloc, const basic_string_base<ST, SU> &src) : _B(static_cast<U>(src._used + 1), static_cast<U>(src._used)), _alloc(alloc) { _Allocate(); _CopyText(_buf, src._buf, _used); }
	simple_string(IMemoryAllocator *alloc, const _T &src) : _B(static_cast<U>(src._used + 1), static_cast<U>(src._used)), _alloc(alloc) { _Allocate(); _CopyText(_buf, _reserved, src._buf, _used); }
	~simple_string() { if (_buf && _alloc) { _alloc->deallocate(_buf); _buf = 0; _reserved = 0; _used = 0; } }

	void set_allocator(IMemoryAllocator *alloc) { if (!_alloc) _alloc = alloc; }
	void resize(U32 s) 
	{ 
		if (s >= _reserved)
		{
			T *oldBuf = _buf;
			_reserved = s + 1;
			_Allocate();

			if (oldBuf && _used)
				_CopyText(_buf, _reserved, oldBuf, _used);

			// oldBuf is released here, because there is chance, that we copy part from source buffer
			if (oldBuf)
				_alloc->deallocate(oldBuf);
		}
		_used = s;
	}

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
		return _used ? _B::ncmp(str._buf, _buf, _used) == 0 : true;
	}

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

	const T * begin() const { return _buf; }
	const T * end() const { return _buf + _used; }

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
int  basic_string_base<wchar_t, U32>::ncmp(const wchar_t *string1, const wchar_t *string2, size_t count) { 
	return wcsncmp(string1, string2, count); 
}

template<>
int  basic_string_base<wchar_t, U16>::nicmp(const wchar_t *string1, const wchar_t *string2, size_t count) { return _wcsnicmp(string1, string2, count); }

template<>
int  basic_string_base<wchar_t, U32>::nicmp(const wchar_t *string1, const wchar_t *string2, size_t count) { return _wcsnicmp(string1, string2, count); }

// ============================================================================

template <typename T, typename U = U32, class Alloc = Allocators::Default, SIZE_T minReservedSize = 48>
class basic_string : public basic_string_base<T, U>
{
private:
	typedef basic_string<T, U, Alloc, minReservedSize> _T;
	typedef basic_string_base<T, U> _B;
	typedef MemoryAllocatorStatic<Alloc> _allocator;

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

	void _Allocate() { _buf = static_cast<T *>(_allocator::allocate(_reserved * sizeof(T))); }

public:
	// constructors

	// 1. Default for empty string. No memory allocation
	basic_string() : _B(0) {  }

	// 2. With c-string as param
	template<typename ST>
	basic_string(const ST *src, const ST *end = nullptr) {
		_used = end ? static_cast<U>(end - src) : static_cast<U>(basic_string_base<ST, U>::length(src));
		_reserved = max(_used, static_cast<U>(minReservedSize));
		_Allocate();
		_CopyText(_buf, src, _used);
	}

	// 3. From basic_string_base
	template<typename ST, typename SU>
	basic_string(const basic_string_base<ST, SU> &src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used))
	{
		_Allocate();
		_CopyText(_buf, src._buf, _used);
	}

	// 4. Copy constructor
	basic_string(const _T &src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used)) { _Allocate(); _CopyText(_buf, _reserved, src._buf, _used); }

	// 5. Move constructor
	basic_string(_T &&src) : _B(static_cast<U>(src._reserved), static_cast<U>(src._used), src._buf) { src._used = 0; src._reserved = 0; src._buf = nullptr; }

	// 6. Reserve space constructor
	basic_string(U reserve) : _B(reserve) { _Allocate(); }

	~basic_string() { if (_buf) { _allocator::deallocate(_buf); _buf = 0; _reserved = 0; _used = 0; } }

	void resize(U32 s)
	{
		if (s >= _reserved)
		{
			T *oldBuf = _buf;
			_reserved = s + 1;
			_Allocate();

			if (oldBuf && _used)
				_CopyText(_buf, _reserved, oldBuf, _used);

			// oldBuf is released here, because there is chance, that we copy part from source buffer
			if (oldBuf)
				_allocator::deallocate(oldBuf);
		}
		_used = s;
	}

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

	// === string concatenation: operator +=
	template<typename V>
	basic_string & operator += (const basic_string_base<T, V>& src) { _Append(static_cast<U>(src._used), src._buf);  return *this; }
	basic_string & operator += (const T *src) { U len = static_cast<U>(_B::length(src)); _Append(len, src); return *this; }
	// 3. For all classes with ToBasicString
	template<typename V>
	typename std::enable_if_t<has_method_ToBasicString<V>::value, basic_string & >
		operator += (const V &src) { _Append(static_cast<U>(src.ToBasicString()._used), src.ToBasicString()._buf);  return *this; }

	// === opertor +
	template<typename V>
	basic_string operator + (const V& src)  const { basic_string tmp(*(static_cast<const _B *>(this))); tmp += src;	return tmp; }

	// ==== compare
	bool operator == (const basic_string_base<T, U> &str) const {
		if (str._buf == _buf)
			return true;
		if (str._used != _used)
			return false;

		return _B::ncmp(str._buf, _buf, _used) == 0;
	}

	bool startWith(const basic_string_base<T, U> &str, bool caseInsesitive = false) const {
		if (str._used >= _used)
			return false;
		return caseInsesitive ? _B::nicmp(str._buf, _buf, str._used) == 0 : _B::ncmp(str._buf, _buf, str._used) == 0;
	}

	basic_string_base<T, U> substr(I32 start, I32 len = 0x7fffffff)
	{
		I64 s = start >= 0 ? start : I64(_used) + I64(start);
		I64 l = _used;
		if (s < 0)
			s = 0;
		if (s > l)
			s = l;


		I64 e = (len < 0 ? l : s) + I64(len);
		if (e < s)
			e = s;
		if (e > l)
			e = l;

		basic_string_base<T, U> ret(U32(e-s), _buf+U32(s));
		return std::move(ret);
	}

	operator _B&() { return *this; }

	const T * c_str() {
		if (_used == _reserved)
		{
			static const char *zero = "\0";
			_Append(1, reinterpret_cast<const T*>(zero));
		}
		_buf[_used] = 0;

		return _buf;
	}


	void UTF8(const basic_string_base<char, U> &s)
	{
		if (!s.size() || std::is_same_v<T, char>)
		{ 
			_used = 0; 
		}
		else
		{
			int size_needed = MultiByteToWideChar(CP_UTF8, 0, s._buf, (int)s.size(), NULL, 0);
			_used = size_needed; // / sizeof(char) = 1;
			if (_used >= _reserved)
			{
				if (_buf)
					_allocator::deallocate(_buf);
				_reserved = size_needed + minReservedSize;
				_Allocate();					
			}

			MultiByteToWideChar(CP_UTF8, 0, s._buf, (int)s.size(), _buf, size_needed);
		}
	}

	void UTF8(const basic_string_base<wchar_t, U> &s)
	{
		if (!s.size() || std::is_same_v<T, wchar_t>)
		{
			_used = 0;
		}
		else
		{
			_used = WideCharToMultiByte(CP_UTF8, 0, s._buf, (int) s.size(), NULL, 0, NULL, NULL);
			if (_used >= _reserved)
			{
				if (_buf)
					_allocator::deallocate(_buf);
				_reserved = _used + minReservedSize;
				_Allocate();
			}

			WideCharToMultiByte(CP_UTF8, 0, s._buf, (int)s.size(), _buf, _used, NULL, NULL);
		}
	}


	template <typename V>
	bool wildcard(const basic_string_base<T, V>& pat, U pos = 0, V patPos = 0)
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

// ============================================================================

template <typename T, typename U = U32, class A = Allocators::Default, SIZE_T S = 48, SIZE_T S2 = 16>
class shared_string : public shared_base< basic_string<T, U, A, S>, A, S2>
{
public:
	typedef basic_string_base<T, U> _B;

	// constructors
	// 1. Default (empty string)
	shared_string() : shared_base(0) { CheckIfInitialized();  AddRef(); }

	// 2. copy constructor
	shared_string(const shared_string &src) : shared_base(src._idx) { AddRef(); }

	// 3. move costructor
	shared_string(shared_string &&src) : shared_base(src._idx) { src._idx = 0; }

	// 4. c-string constructor (for txt("hello") and for txt(str.begin(), str.end())
	template<typename ST> shared_string(const ST *src, const ST *end = nullptr) { MakeNewEntry(src, end); }

	// 5. copy constructors for classes with ToBasicString
	template<typename V,
			 typename = std::enable_if_t<has_method_ToBasicString<V>::value> 
	>
	shared_string(const V &src) { MakeNewEntry(src.ToBasicString()); }

	// 6. reserve memory for string
	shared_string(U reserve) { MakeNewEntry(reserve); }

	~shared_string() { Release(); }

	template<typename V>
	shared_string& operator = (const basic_string_base<T, V>& src) { NeedUniq(); GetValue() = src; return *this; }
	shared_string& operator = (const T* src) { NeedUniq(); GetValue() = src; return *this; }
	shared_string& operator = (const shared_string& src) { Release(); _idx = src._idx; AddRef(); return *this; }
	shared_string& operator = (shared_string&& src) { Release(); _idx = src._idx; src._idx = 0; return *this; }

	T operator[](SIZE_T pos) const { return GetValue()[pos]; }
	T& operator[](SIZE_T pos) { NeedUniq(); return GetValue()[pos]; }

	// === string concatenations: operator +=
	// 1. For all classes with GetValue (derived from shared_base)
	template<typename V>
	typename std::enable_if_t<has_method_GetValue<V>::value, shared_string & >
		operator += (const V &src) { NeedUniq(); GetValue() += src.GetValue(); return *this; }

	// 2. For all classes without GetValue but with ToBasicString
	template<typename V>
	typename std::enable_if_t<has_method_ToBasicString<V>::value && !has_method_GetValue<V>::value, shared_string & >
		operator += (const V &src) { NeedUniq(); GetValue() += src.ToBasicString(); return *this; }
	// 3. For just c-string 
	shared_string & operator += (const T *src) { NeedUniq(); GetValue() += src; return *this; }


	// === operator +
	template<typename V>
	shared_string operator + (const V &src)  const { 
		auto vv = ToBasicString();
		shared_string tmp(vv);
		tmp += src; 
		return tmp; 
	}

	// === compare
	bool operator == (const basic_string_base<T, U> &str) const { return GetValue() == str; }
	bool operator == (const shared_string &str) const { return _idx == str._idx ? true : GetValue() == str.GetValue(); } // no need to compare same strings
	bool startWith(const basic_string_base<T, U> &str, bool caseInsesitive = false) const { return GetValue().startWidt(str); }
	bool startWith(const shared_string &str, bool caseInsesitive = false) const { return GetValue().startWith(str.GetValue()); }

	SIZE_T size() const { return GetValue().size(); }
	void resize(U32 s) { GetValue().resize(s); }

	operator basic_string_base<T, U>&() { return GetValue(); }            // It is safe. Dont have to "uniq", because basic_string_base is used only as arg (read only).
	basic_string_base<T, U> &ToBasicString() const { return GetValue(); } // It is safe. Dont have to "uniq", because basic_string_base is used only as arg (read only).

	const T * c_str() const { return GetValue().c_str(); }

	const T * begin() const { return GetValue()._buf; }
	const T * end() { auto &v = GetValue();  return v._buf + v._used; }
	basic_string_base<T, U> substr(I32 start, I32 len = 0x7fffffff) const { return GetValue().substr(start, len); }

	bool wildcard(const shared_string& pat, U pos = 0, U patPos = 0) const { return GetValue().wildcard(pat.ToBasicString(), pos, patPos); }

	template <typename V>
	bool wildcard(const basic_string_base<T, V>& pat, U pos = 0, V patPos = 0) const { return GetValue().wildcard(pat, pos, patPos); }

	template <typename V>
	bool iwildcard(const basic_string_base<T, V>& pat, U pos = 0, V patPos = 0) const { return GetValue().iwildcard(pat, pos, patPos); }

	void UTF8(const basic_string_base<wchar_t, U> &s) { NeedUniq(); GetValue().UTF8(s); }
	void UTF8(const basic_string_base<char, U> &s)    { NeedUniq(); GetValue().UTF8(s); }

	void Dump() const
	{
		TRACEW(L"" << _idx << " (" << _pool[_idx].refCounter << ") : [" << GetValue().size() << " ]" << "\n");
	}
};


// ============================================================================

typedef shared_string<char> STR;
typedef shared_string<char, U16> ShortSTR;
typedef shared_string<wchar_t> WSTR;
typedef shared_string<wchar_t, U32, Allocators::Default, 250> PathSTR;

typedef simple_string<wchar_t, U32> WSimpleString;
typedef simple_string<char, U32> SimpleString;

typedef basic_string_base<wchar_t, U32> WString;
typedef basic_string_base<char, U32> String;

/**
 * STR & WSTR should be used as standard strings. It store: 1x pointer, 2x 32-bit for length of string and length of buffer.
 * PathSTR = WSTR with default minimal length = 250 chars
 * ShortSTR = STR with max length = 65535.
 *
 * SimpleString & WSimpleString - first arg of contructor is allocator. We have control how memory is allocated "per instance".
 * We can copy content of string with different memory allocator.
 */