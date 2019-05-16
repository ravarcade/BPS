//
//using namespace ::BAMS::CORE;
//
//class VertexData : public ResoureImpl<VertexData, RESID_VERTEXDATA, Allocators::default>
//{
//public:
//	U8 *Data;
//	SIZE_T Size;
//
//	VertexData(ResourceBase *res) : Data(nullptr), Size(0) {}
//	~VertexData() { if (Data) deallocate(Data); }
//
//	void Update(ResourceBase *res)
//	{
//		// memory is allocated with RawData MemoryAllocator, so we don't have to copy it.
//		Data = static_cast<U8 *>(res->GetData());
//		Size = res->GetSize();
//	}
//
//	void Release(ResourceBase *res)
//	{
//		res->ResourceLoad(nullptr, 0);
//		res->GetMemoryAllocator()->deallocate(Data);
//		Data = nullptr;
//		Size = 0;
//	}
//
//	U8 *GetData() { return Data; }
//	SIZE_T GetSize() { return Size; }
//};