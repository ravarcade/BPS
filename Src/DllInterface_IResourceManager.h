/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "DllInterface.h"
*
*/

enum {
	RESID_CORE_RESOURCES = 0x00010000,
	RESID_UNKNOWN,
	RESID_RAWDATA = RESID_UNKNOWN,

	// resources used by rendering engine
	RESID_RENDERING_ENGINE_RESOURCES = 0x00020000,
	RESID_SHADER,
	RESID_SHADER_PROGRAM,
	RESID_VERTEXDATA,
};

extern "C" {
	typedef void IResource;
	typedef void IRawData;
	typedef void IShader;
	typedef void IResourceManager;
	typedef void IModule;

	// Call Initialize befor do anything with resources, game engine, etc. Only some memory allocations.
	BAMS_EXPORT void Initialize();

	// Call Finalize as last thing to release all resources and memory
	BAMS_EXPORT void Finalize();

	BAMS_EXPORT IModule *GetModule(uint32_t moduleId);
//	BAMS_EXPORT void SendMsg();

	// MemoryAllocators
	BAMS_EXPORT void GetMemoryAllocationStats(uint64_t *Max, uint64_t *Current, uint64_t *Counter);
	BAMS_EXPORT bool GetMemoryBlocks(void **current, size_t *size, size_t *counter, void **data);

	// Resource
	BAMS_EXPORT void IResource_AddRef(IResource *res);
	BAMS_EXPORT void IResource_Release(IResource *res);
	BAMS_EXPORT uint32_t IResource_GetRefCounter(IResource *res);
	BAMS_EXPORT UUID IResource_GetUID(IResource *res);
	BAMS_EXPORT const char * IResource_GetName(IResource *res);
	BAMS_EXPORT const wchar_t * IResource_GetPath(IResource *res);
	BAMS_EXPORT bool IResource_IsLoaded(IResource *res);
	BAMS_EXPORT uint32_t IResource_GetType(IResource *res);
	BAMS_EXPORT bool IResource_IsLoadingNeeded(IResource *res);

	// ResourceManager
	BAMS_EXPORT IResourceManager *IResourceManager_Create();
	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *);
	BAMS_EXPORT IResource *IResourceManager_AddResource(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT void IResourceManager_AddDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT void IResourceManager_RootDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name, uint32_t type = 0);
	BAMS_EXPORT void IResourceManager_Filter(IResourceManager *rm, IResource **resList, uint32_t *resCount, const char *pattern);
	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name);
	BAMS_EXPORT void IResourceManager_LoadSync(IResourceManager *rm, IResource *res = nullptr);
	BAMS_EXPORT void IResourceManager_RefreshResorceFileInfo(IResourceManager *rm, IResource *res);
	BAMS_EXPORT void IResourceManager_StartDirectoryMonitor(IResourceManager *rm);
	BAMS_EXPORT void IResourceManager_StopDirectoryMonitor(IResourceManager *rm);

	// RawData
	BAMS_EXPORT unsigned char *IRawData_GetData(IRawData *res);
	BAMS_EXPORT size_t IRawData_GetSize(IRawData *res);

	// Shader
	BAMS_EXPORT unsigned char *IShader_GetData(IShader *res);
	BAMS_EXPORT size_t IShader_GetSize(IShader *res);
	BAMS_EXPORT void IShader_AddProgram(IShader *res, const wchar_t *fileName);
	BAMS_EXPORT const wchar_t * IShader_GetSourceFilename(IShader *res, int type);
	BAMS_EXPORT const wchar_t * IShader_GetBinaryFilename(IShader *res, int type);
	BAMS_EXPORT void IShader_Save(IShader *res);

	BAMS_EXPORT uint32_t IShader_GetBinaryCount(IShader *res);
	BAMS_EXPORT IRawData *IShader_GetBinary(IShader *res, uint32_t idx);

	// Module
	struct Message
	{
		U32 id;
		U32 targetModule;
		U32 source;
		void *data;
		U32 dataLen;
	};

	typedef void ExternalModule_Initialize(const void *moduleData);
	typedef void ExternalModule_Finalize(const void *moduleData);
	typedef void ExternalModule_Update(float dt, const void *moduleData);
	typedef void ExternalModule_SendMsg(BAMS::Message *msg, const void *moduleData);
	typedef void ExternalModule_Destroy(const void *moduleData);
	
	BAMS_EXPORT void IEngine_RegisterExternalModule(
		uint32_t moduleId, 
		ExternalModule_Initialize *moduleInitialize,
		ExternalModule_Finalize *moduleFinalize,
		ExternalModule_Update *moduleUpdate,
		ExternalModule_SendMsg *moduleSendMsg,
		ExternalModule_Destroy *moduleDestroy,
		const void *moduleData);

	BAMS_EXPORT void IEngine_SendMsg(
		uint32_t msgId,
		uint32_t msgDst,
		uint32_t msgSrc,
		const void *data,
		uint32_t dateLen);

	BAMS_EXPORT void IEngine_Update(float dt);

	BAMS_EXPORT BAMS::CORE::Allocators::IMemoryAllocator *GetMemoryAllocator(uint32_t allocatorType = 0, SIZE_T size = 0);

	// ================================================================================== Classes ===

	class ResourceSmartPtr
	{
	protected:
		struct ResourceSmartPtrData {
			IResource *ptr;
			uint32_t count;
			ResourceSmartPtrData(IResource *p, uint32_t c = 1) : ptr(p), count(c) {}
		} *val;

		void Release()
		{
			if (val && --val->count == 0)
			{
				IResource_Release(val->ptr);
			}
		}

	public:
		ResourceSmartPtr() { val = nullptr; }
		ResourceSmartPtr(IResource *p) { val = new ResourceSmartPtrData(p); }
		ResourceSmartPtr(const ResourceSmartPtr &s) : val(s.val) { ++val->count; }
		ResourceSmartPtr(ResourceSmartPtr &&s) : val(s.val) { s.val = nullptr; }
		~ResourceSmartPtr() {
			Release();
		}

		ResourceSmartPtr & operator = (const ResourceSmartPtr &s) { Release(); val = s.val; ++val->count; return *this; }
		ResourceSmartPtr & operator = (ResourceSmartPtr &&s) { val = s.val; s.val = nullptr; return *this; }

		inline IResource *Get() { return val ? val->ptr : nullptr; }
	};

	class CResourceManager;
	class CResource 
	{
	protected:
		ResourceSmartPtr _res;

		inline IResource *Get() { return _res.Get(); }
		friend CResourceManager;

	public:
		CResource() : _res(nullptr) {}
		CResource(IResource *r) : _res(r) { AddRef(); }
		CResource(const CResource &src) : _res(src._res) {}
		CResource(CResource &&src) : _res(std::move(src._res)) {}
		virtual ~CResource() {}

		CResource & operator = (const CResource &&src) { _res = std::move(src._res); return *this; }
		CResource & operator = (CResource &&src) { _res = std::move(src._res); return *this; }

		UUID GetUID() { return IResource_GetUID(Get()); }
		const char *GetName() { return IResource_GetName(Get()); }
		const wchar_t *GetPath() { return IResource_GetPath(Get()); }
		bool IsLoaded() { return IResource_IsLoaded(Get()); }
		uint32_t GetType() { return IResource_GetType(Get()); }

		void AddRef()  { IResource_AddRef(Get()); }
		void Release() { IResource_Release(Get()); }
		uint32_t GetRefCounter() { return IResource_GetRefCounter(Get()); }
	};

	class CRawData : public CResource
	{
	public:
		CRawData() {}
		CRawData(IRawData *r) : CResource(static_cast<IResource*>(r)) {}
		CRawData(const CRawData &src) : CResource(src) {}
		CRawData(CRawData &&src) : CResource(std::move(src)) {}
		virtual ~CRawData() {}

		CRawData & operator = (const CRawData &src) { _res = src._res; return *this; }
		CRawData & operator = (CRawData &&src) { _res = std::move(src._res); return *this; }

		unsigned char *GetData() { return IRawData_GetData(static_cast<IRawData*>(Get())); }
		size_t GetSize() { return IRawData_GetSize(static_cast<IRawData*>(Get())); }
	};

	class CShader : public CResource
	{
	public:
		CShader() {}
		CShader(IShader *r) : CResource(static_cast<IResource*>(r)) {}
		CShader(const CShader &src) : CResource(src) {}
		CShader(CShader &&src) : CResource(std::move(src)) {}
		virtual ~CShader() {}

		CShader & operator = (const CShader &src) { _res = src._res; return *this; }
		CShader & operator = (CShader &&src) { _res = std::move(src._res); return *this; }

		unsigned char *GetData() { return IShader_GetData(static_cast<IShader*>(Get())); }
		size_t GetSize() { return IShader_GetSize(static_cast<IShader*>(Get())); }

		void AddProgram(const wchar_t *fileName) { IShader_AddProgram(static_cast<IShader*>(Get()), fileName); }
		const wchar_t *GetSourceFilename(int type) { return IShader_GetSourceFilename(static_cast<IShader*>(Get()), type); }
		const wchar_t *GetBinaryFilename(int type) { return IShader_GetBinaryFilename(static_cast<IShader*>(Get()), type); }

		uint32_t GetBinaryCount() { return IShader_GetBinaryCount(static_cast<IShader*>(Get())); }
		CRawData GetBinary(uint32_t idx) { CRawData prg(IShader_GetBinary(static_cast<IShader*>(Get()), idx)); return  std::move(prg); }

		void Save() { IShader_Save(static_cast<IShader*>(Get())); }
	};

	class CResourceManager
	{
		IResourceManager *_rm;

	public:
		CResourceManager() { _rm = IResourceManager_Create(); }
		CResourceManager(IResourceManager *rm) : _rm(rm) {};
		~CResourceManager() { IResourceManager_Destroy(_rm); _rm = nullptr; }

		void AddResource(const wchar_t *path) 
		{ 
			IResourceManager_AddResource(_rm, path); 
		}
		void AddDir(const wchar_t *path) { IResourceManager_AddDir(_rm, path); }
		void RootDir(const wchar_t *path) { IResourceManager_RootDir(_rm, path); }

		void Filter(IResource **resList, uint32_t *resCount, const char *pattern) { IResourceManager_Filter(_rm, resList, resCount, pattern); }
		CResource FindByName(const char *name, uint32_t type = RESID_UNKNOWN) { CResource res(IResourceManager_FindByName(_rm, name, type)); return std::move(res); }
		CResource FindByUID(const UUID &uid) { CResource res(IResourceManager_FindByUID(_rm, uid)); return std::move(res); }

		CRawData GetRawDataByName(const char *name) { CRawData res(IResourceManager_GetRawDataByName(_rm, name)); return std::move(res); }
		CRawData GetRawDataByUID(const UUID &uid) { CRawData res(IResourceManager_GetRawDataByUID(_rm, uid)); return std::move(res); }

		CShader GetShaderByName(const char *name) { CShader res(IResourceManager_FindByName(_rm, name, RESID_SHADER)); return std::move(res); }
		void LoadSync() { IResourceManager_LoadSync(_rm, nullptr); }
		void LoadSync(CResource &res) { IResourceManager_LoadSync(_rm, res.Get()); }
		void RefreshResorceFileInfo(IResource *res) { IResourceManager_RefreshResorceFileInfo(_rm, res); }

		void StartDirectoryMonitor() { IResourceManager_StartDirectoryMonitor(_rm); }
		void StopDirectoryMonitor() { IResourceManager_StopDirectoryMonitor(_rm); }
	};

	

	class CEngine
	{
	public:
		static void RegisterExternalModule(
			uint32_t moduleId,
			ExternalModule_Initialize *moduleInitialize,
			ExternalModule_Finalize *moduleFinalize,
			ExternalModule_Update *moduleUpdate,
			ExternalModule_SendMsg *moduleSendMsg,
			ExternalModule_Destroy *moduleDestroy,
			const void *moduleData)
		{
			IEngine_RegisterExternalModule(
				moduleId,
				moduleInitialize,
				moduleFinalize,
				moduleUpdate,
				moduleSendMsg,
				moduleDestroy,
				moduleData);
		}

		static void SendMsg(
			uint32_t msgId,
			uint32_t msgDst,
			uint32_t msgSrc,
			const void *data,
			uint32_t dataLen = 0)
		{
			IEngine_SendMsg( msgId, msgDst, msgSrc, data, dataLen);
		}

		static void Update(float dt)
		{
			IEngine_Update(dt);
		}
	};

	BAMS_EXPORT void DoTests();
}

template<typename T, uint32_t moduleId>
class IExternalModule
{
protected:
	static void _Initialize(const void *p) { reinterpret_cast<T*>(const_cast<void *>(p))->Initialize(); }
	static void _Finalize(const void *p) { reinterpret_cast<T*>(const_cast<void *>(p))->Finalize(); }
	static void _Update(float dt, const void *p) { reinterpret_cast<T*>(const_cast<void *>(p))->Update(dt); }
	static void _SendMsg(Message* msg, const void *p) { reinterpret_cast<T*>(const_cast<void *>(p))->SendMsg(msg); }
	static void _Destroy(const void *p) { reinterpret_cast<T*>(const_cast<void *>(p))->Destroy(); }

public:
	IExternalModule() {}

	void Register()
	{
		IEngine_RegisterExternalModule(moduleId, _Initialize, _Finalize, _Update, _SendMsg, _Destroy, this);
	}
};


// ============================================================================

enum { // msgDst
	RENDERING_ENGINE = 0x201 // VK
};

enum { // msgId
	// to RENDERING_ENGINE
	CREATE_WINDOW = 0x20001,
	CLOSE_WINDOW  = 0x20002,
	ADD_MODEL     = 0x20003, // object name + model name + shader program
	ADD_SHADER    = 0x20004,
	RELOAD_SHADER = 0x20005,
};

struct PCREATE_WINDOW {
	uint32_t wnd;
	uint32_t w, h;
	uint32_t x, y;
};

struct PCLOSE_WINDOW {
	uint32_t wnd;
};

struct PADD_MODEL {
	uint32_t wnd;
	const char *name;    // uniq name
	const char *mesh;	 // resource name
	const char *shader;  // resource name
};

struct PADD_SHADER {
	uint32_t wnd;
	const char *shader;
};

typedef struct PADD_SHADER PRELOAD_SHADER;

