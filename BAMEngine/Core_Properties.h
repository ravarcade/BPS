
#include <thread>

struct Property
{
public:
	enum  {
		PT_EMPTY = 0,
		PT_I32,
		PT_U32,
		PT_I16,
		PT_U16,
		PT_I8,
		PT_U8,
		PT_F32, // mat4 = FP32 * 16, mat3 = FP32 * 9
		PT_CSTR,
		PT_ARRAY,
		PT_TEXTURE,
		PT_UNKNOWN = 0xffffffff
	};

	U32 type;
	U32 parent;
	U32 count;
	CSTR name;
	void *val;
	U32 array_size;
	U32 array_stride;
	
	// implementation
	U32 buffer_idx;
	U32 buffer_offset;
	U32 buffer_object_stride;

	Property()                                 : type(PT_EMPTY), parent(-1), count(0),   name(nullptr), val(nullptr), array_stride(0) {}
	Property(CSTR n, I32 *v, U32 cnt = 1)      : type(PT_I32),   parent(-1), count(cnt), name(n), val(v), array_stride(0) {}
	Property(CSTR n, F32 *v, U32 cnt = 1)      : type(PT_F32),   parent(-1), count(cnt), name(n), val(v), array_stride(0) {}
	Property(CSTR n, CSTR v)                   : type(PT_CSTR),  parent(-1), count(1),   name(n), val(const_cast<char *>(v)), array_stride(0) {}
	Property(CSTR n)                           : type(PT_EMPTY), parent(-1), count(0),   name(n), val(nullptr), array_stride(0) {}

	bool IsEmpty() const { return count == 0 || type == PT_EMPTY; };

	void  Set(U32 t, void *v, size_t s)
	{
		if (val && t == type)
		{
			memcpy_s(val, s, v, s);
		}
	}


	void Set(I32 v) { Set(PT_I32, &v, sizeof(v)); }
	void Set(F32 v) { Set(PT_F32, &v, sizeof(v)); }
	void Set(F32 *v, U32 cnt = 1) { Set(PT_F32, v, sizeof(F32)*cnt); }
	void Set(CSTR v) { val = const_cast<char *>(v); }

	void SetMem(void *v) { SIZE_T l = MemoryRequired(); if (!l) val = v; else memcpy_s(val, l, v, l); }
	SIZE_T MemoryRequired() const { return MemoryRequired(type, count); }
	static SIZE_T MemoryRequired(U32 type, U32 count) 
	{
		switch (type)
		{
		case Property::PT_I8:
		case Property::PT_U8:
			return count;
		case Property::PT_I16:
		case Property::PT_U16:
			return count * sizeof(U16);
		case Property::PT_I32:
		case Property::PT_U32:
		case Property::PT_F32:
			return count * sizeof(U32);
		case Property::PT_CSTR:
		case Property::PT_TEXTURE:
			return 0;

		case Property::PT_ARRAY:
			return count * sizeof(void *);
		}
		return 0;
	}
};

struct Properties
{
	U32 count;
	Property *properties;
	Properties() : count(0), properties(nullptr) {}

	Property *Find(const char *name)
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			if (strcmp(properties[i].name, name) == 0)
				return &properties[i];
		}
		return nullptr;
	}

	const Property *ConstFind(const char *name) const
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			if (strcmp(properties[i].name, name) == 0)
				return &properties[i];
		}
		return nullptr;
	}
	
	Property *begin() { return properties; }
	Property *end() { return properties + count; }
	const Property *begin() const { return properties; }
	const Property *end() const { return properties + count; }
};

class MProperties : public Properties
{
public:
	MProperties(U32 cnt = 0) : reserved(cnt), alloc(nullptr)
	{
		_setAlloc();
		if (cnt) {
			auto s = sizeof(Property)*reserved;
			properties = static_cast<Property*>(alloc->allocate(s));
			memset(properties, 0, s);
		}
	}

	MProperties(const Properties &src) : reserved(0)
	{
		_setAlloc();
		_cpy(src);
	}

	// c++ rule of 5
	MProperties(const MProperties &src) : reserved(0)
	{
		_setAlloc();
		_cpy(src);
	}

	MProperties(MProperties &&src) noexcept
	{
		memcpy_s(this, sizeof(MProperties), &src, sizeof(MProperties));
		src.properties = nullptr;
		src.alloc = nullptr;
	}

	~MProperties()
	{
		if (properties && alloc)
			alloc->deallocate(properties);
	}

	MProperties &operator = (const MProperties &src)
	{
		_cpy(src);
		return *this;
	}

	MProperties &operator = (MProperties &&src) noexcept
	{
		if (properties)
			alloc->deallocate(properties);

		memcpy_s(this, sizeof(MProperties), &src, sizeof(MProperties));
		src.properties = nullptr;
		return *this;
	}

	template<class... Args>
	Property *add(Args &&...args)
	{
		if (reserved <= (count+1))
		{
			reserved += reserved / 2 + 8;
			_resize();
		}

		auto ret = new (&properties[count]) Property(std::forward<Args>(args)...);
		++count;
		return ret;
	}

	void remove(U32 idx)
	{
		for (U32 i = 0; i < count; ++i)
		{
			if (properties[idx].parent > idx)
				properties[idx].parent -= 1;

			if (properties[i].parent == idx)
				remove(i);
		}

		if (idx+1 < count)
		{
			auto s = sizeof(Property)*(count - idx - 1);
			memcpy_s(&properties[idx], s, &properties[idx+1], s);
		}

		--count;
	}

	Property & operator [] (U32 i)
	{
		assert(i < count);
		return properties[i];
	}


	void clear() { count = 0; }
	U32 size() { return count; }

	Property * begin() { return properties; }
	Property * end() { return &properties[count]; }

	const Property * begin() const { return properties; }
	const Property * end() const { return &properties[count]; }

private:
	IMemoryAllocator *alloc;
	U32 reserved;

	void _resize()
	{
		Property *old = properties;
		if (!alloc) 
			_setAlloc();
		properties = static_cast<Property*>(alloc->allocate(sizeof(Property)*reserved));
		if (old && count)
			memcpy_s(properties, sizeof(Property)*reserved, old, sizeof(Property)*count);

		if (old)
			alloc->deallocate(old);
	}

	void _setAlloc()
	{
		alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::default);
	}

	void _cpy(const Properties &src)
	{
		if (src.count > reserved)
		{
			reserved = src.count;
			_resize();
		}
		count = src.count;
		if (count)
		{
			auto s = sizeof(Property)*reserved;
			memcpy_s(properties, s, src.properties, s);
		}
	}
};

class sProperties : public Properties
{
public:
	sProperties() : _alloc(nullptr), _reserved(0) { 
		count = 0; 
		properties = nullptr; 
	}

	sProperties(const Properties &src) : _alloc(nullptr), _reserved(0) { 
		_assign(src);  
	}

	sProperties(const sProperties &src) : _alloc(nullptr), _reserved(0) {
		_assign(src);
	}

	sProperties(sProperties &&src)
	{
		_alloc = src._alloc;
		_reserved = src._reserved;
		properties = src.properties;
		count = src.count;
		src._alloc = nullptr;
		src.properties = nullptr;
		src.count = 0;
	}

	~sProperties() { clear(); }

	sProperties & operator = (const Properties &src) { _assign(src); return *this; }

	sProperties & operator = (sProperties &&src)
	{
		clear();
		_alloc = src._alloc;
		properties = src.properties;
		_reserved = src._reserved;
		count = src.count;
		src._alloc = nullptr;
		src.properties = nullptr;
		src._reserved = 0;
		src.count = 0;
	}

	void _assign(const Properties &src)
	{
		clear();
		for (U32 i = 0; i < src.count; ++i)
			add(src.properties[i]);
	}

	void clear()
	{
		if (_alloc)
		{
			properties = nullptr;
			count = 0;
			_reserved = 0;
			delete _alloc; // releas all memory
			_alloc = nullptr;
		}
	}

	Property *add(const char *name, const char *value)
	{
		auto  p = add(name, Property::PT_CSTR, 1);
		p->val = const_cast<char *>(storeString(value));
		return p;
	}

	Property *add(const char *name, F32 *value, U32 count = 1)
	{
		auto  p = add(name, Property::PT_F32, count);
		p->SetMem(value);
		return p;
	}

	Property *add(const char *name, U32 *value, U32 count = 1)
	{
		auto  p = add(name, Property::PT_U32, count);
		p->SetMem(value);
		return p;
	}

	Property *add(const char *name, I32 *value, U32 count = 1)
	{
		auto  p = add(name, Property::PT_I32, count);
		p->SetMem(value);
		return p;
	}

	Property *add(const char *name, U32 type, U32 _count)
	{
		if (_reserved == count)
		{
			_reserved += _reserved / 2 + 1;
			_reallocate();
		}
		auto p = new (properties + count)Property();
		++count;

		p->name = storeString(name);
		p->type = type;
		p->count = _count;
		p->val = getMem<U8*>(p->MemoryRequired());

		return p;
	}

	Property *add(const Property &p)
	{
		auto ret = add(p.name, p.type, p.count);
		_copyValue(*ret, p);
		return ret;
	}

	void set(const Property &p)
	{
		Property *dst = Find(p.name);
		if (!dst)
		{
			add(p);
		}
		else
		{
			if (p.type == dst->type && p.count == dst->count)
			{
				dst->SetMem(p.val);
			}
			else
			{
				SIZE_T oldMemReq = dst->MemoryRequired();
				SIZE_T newMemReq = p.MemoryRequired();
				dst->type = p.type;
				dst->count = p.count;
				if (oldMemReq < newMemReq || (oldMemReq == 0 && newMemReq != 0))
				{
					dst->val = getMem<U8*>(p.MemoryRequired());
				}
				dst->SetMem(p.val);

			}
		}
	}

	void set(const char *name, void *val)
	{
		auto p = Find(name);
		if (p)
		{
			p->SetMem(val);
		}
	}

	void _reallocate()
	{
		auto new_properties = getMem<Property>(_reserved);
		if (count) 
		{
			auto l = count * sizeof(Property);
			memcpy_s(new_properties, l, properties, l);
			_alloc->deallocate(properties);
		}
		properties = new_properties;
	}

	void _copyValue(Property &dst, const Property &src)
	{
		switch (src.type)
		{
		case Property::PT_I8:
		case Property::PT_U8:
		case Property::PT_I16:
		case Property::PT_U16:
		case Property::PT_I32:
		case Property::PT_U32:
		case Property::PT_F32:
		{
			auto l = src.MemoryRequired();
			memcpy_s(dst.val, l, src.val, l);
		}
		break;
		case Property::PT_CSTR: dst.val = const_cast<char *>(storeString(reinterpret_cast<CSTR>(src.val))); break;
		case Property::PT_TEXTURE: dst.val = src.val; break;
		}
	}

	const char *storeString(const char *str)
	{
		size_t l = strlen(str) + 1;
		auto ret = getMem<char>(l);
		memcpy_s(ret, l, str, l);
		return ret;
	}

	const char *storeString(const String &str)
	{
		auto ret = getMem<char>(str._used+1);
		memcpy_s(ret, str._used, str._buf, str._used);
		ret[str._used] = 0;
		return ret;
	}

	Property *begin() { return properties; }
	Property *end() { return properties + count; }

	const Property *begin() const { return properties; }
	const Property *end() const { return properties + count; }

private:
	IMemoryAllocator *_alloc;
	U32 _reserved;

	template<typename T>
	T *getMem(SIZE_T size)
	{
		if (!size)
			return nullptr;

		if (!_alloc)
			_alloc = Allocators::GetMemoryAllocator(IMemoryAllocator::block, 4096);

		return reinterpret_cast<T *>(_alloc->allocate(size * sizeof(T)));
	}
};