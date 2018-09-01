/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class RawData : public ResoureImpl<RawData, 0x00010001, Allocators::Default>
{
public:
	U8 *Data;
	SIZE_T Size;

	RawData() : Data(nullptr), Size(0) {}	
	~RawData() { if (Data) deallocate(Data); }

	void Update(ResourceBase *res)
	{
		// memory is allocated with RawData MemoryAllocator, so we don't have to copy it.
		Data = static_cast<U8 *>(res->GetData());
		Size = res->GetSize();
	}

	void Release(ResourceBase *res)
	{
		res->ResourceLoad(nullptr, 0);
		res->GetMemoryAllocator()->deallocate(Data);
		Data = nullptr;
		Size = 0;
	}

	U8 *GetData() { return Data; }
	SIZE_T GetSize() { return Size; }
};
