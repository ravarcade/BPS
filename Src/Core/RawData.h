/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
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
		// memory is allocated with RawData MemoryAllocator, so we don't have tp copy it
		Data = static_cast<U8 *>(res->GetData());
		Size = res->GetSize();
	}

	U8 *GetData() { return Data; }
	SIZE_T GetSize() { return Size; }
};
