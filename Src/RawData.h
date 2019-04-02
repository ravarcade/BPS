/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

/// <summary>
/// Default resource class needed to get raw (not processed) data with given reource type id.
/// </summary>
template <U32 DT>
class TRawData : public ResoureImpl<TRawData<DT>, DT, Allocators::default>
{
public:
	U8 *Data;
	SIZE_T Size;

	TRawData(ResourceBase *res) : Data(nullptr), Size(0) {}
	~TRawData() { if (Data) deallocate(Data); }

	void Update(ResourceBase *res)
	{
		// memory is allocated with RawData MemoryAllocator, so we don't have to copy it.
		Data = static_cast<U8 *>(res->GetData());
		Size = res->GetSize();
	}

	void Release(ResourceBase *res)
	{
		res->ResourceLoad(nullptr);
		if (Data)
			res->GetMemoryAllocator()->deallocate(Data);
		Data = nullptr;
	}

	U8 *GetData() { return Data; }
	SIZE_T GetSize() { return Size; }
};

typedef TRawData<RESID_RAWDATA> RawData;
