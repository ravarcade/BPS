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
template <U32 DT, bool isLoadable = true>
class TResRawData : public ResImp<TResRawData<DT, isLoadable>, DT, isLoadable>
{
private:	
	ResBase *_res;

public:
	TResRawData(ResBase *res) : _res(res) {}
	~TResRawData() { if (_res) _res->ReleaseMemory(); }

	void Update(ResBase *res)
	{
		// check if we still are with same resource
		if (_res == nullptr)
			_res = res;
		assert(res == _res);
	}

	void Release(ResBase *res)
	{
		// just check if we still are with same resource
		assert(res == _res);
		if (_res) 
		{
			_res->ReleaseMemory();
			_res = nullptr;
		}
	}

	U8 *GetData() { return _res ? static_cast<U8 *>(_res->GetData()) : nullptr; }
	SIZE_T GetSize() { return _res ? _res->GetSize() : 0; }
};

typedef TResRawData<RESID_RAWDATA> ResRawData;
