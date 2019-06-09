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
class TResRawData : public ResImp<TResRawData<DT>, DT>
{
public:
	U8 *Data;
	SIZE_T Size;

	TResRawData(ResBase *res) : Data(nullptr), Size(0) {}
	~TResRawData() { if (Data) deallocate(Data); }

	void Update(ResBase *res)
	{
		// memory is allocated with ResRawData MemoryAllocator, so we don't have to copy it.
		Data = static_cast<U8 *>(res->GetData());
		Size = res->GetSize();
	}

	void Release(ResBase *res)
	{
		res->ResourceLoad(nullptr);
		if (Data)
			res->GetMemoryAllocator()->deallocate(Data);
		Data = nullptr;
	}

	U8 *GetData() { return Data; }
	SIZE_T GetSize() { return Size; }
};

typedef TResRawData<RESID_RAWDATA> ResRawData;
