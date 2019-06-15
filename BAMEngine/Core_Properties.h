

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
		if (val && t != type)
		{
			memcpy_s(val, s, v, s);
		}
	}


	void Set(I32 v) { Set(PT_I32, &v, sizeof(v)); }
	void Set(F32 v) { Set(PT_F32, &v, sizeof(v)); }
	void Set(F32 *v, U32 cnt = 1) { Set(PT_F32, v, sizeof(F32)*cnt); }
	void Set(CSTR v) { val = const_cast<char *>(v); }
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

	MProperties(const Properties &src)
	{
		_setAlloc();
		_cpy(src);
	}

	// c++ rule of 5
	MProperties(const MProperties &src)
	{
		_setAlloc();
		_cpy(src);
	}

	MProperties(MProperties &&src) noexcept
	{
		memcpy_s(this, sizeof(MProperties), &src, sizeof(MProperties));
		src.properties = nullptr;
	}

	~MProperties()
	{
		if (properties)
			alloc->deallocate(properties);
	}

	MProperties &operator = (const MProperties &src)
	{
		if (&src != this)
		{
			if (properties && reserved >= src.count)
			{
				count = src.count;
				auto s = sizeof(Property) * count;
				memcpy_s(properties, s, src.properties, s);
			}
			else
			{
				if (properties)
					alloc->deallocate(properties);

				_cpy(src);
			}
		}

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
			if (reserved == 0)
				reserved = 8;
			_resize(reserved * 2);
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
private:
	IMemoryAllocator *alloc;
	U32 reserved;

	void _resize(U32 res)
	{
		reserved = res;
		Property *old = properties;
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
		reserved = count = src.count;
		if (count)
		{
			auto s = sizeof(Property)*reserved;
			properties = static_cast<Property*>(alloc->allocate(s));
			memcpy_s(properties, s, src.properties, s);
		}
		else {
			properties = nullptr;
		}
	}
};