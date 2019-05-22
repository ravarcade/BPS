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
	RESID_RAWDATA,

	// resources used by rendering engine
	RESID_RENDERING_ENGINE_RESOURCES = 0x00020000,
	RESID_SHADER,
	RESID_SHADER_SRC,
	RESID_SHADER_BIN,
	RESID_SHADER_PROGRAM,

	RESID_MESH = RESID_RENDERING_ENGINE_RESOURCES + 0x201,
	RESID_MODEL,
	RESID_IMPORTMODEL,
	RESID_IMAGE,

	RESID_VERTEXDATA,
};

extern "C" {
	typedef void IResource;
	typedef void IResRawData;
	typedef void IResShader;
	typedef void IResMesh;
	typedef void IResImage;
	typedef void IResourceManager;

	typedef void IModule;
	typedef void IVertexDescription;

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
	BAMS_EXPORT void            IResource_AddRef(IResource *res);
	BAMS_EXPORT void            IResource_Release(IResource *res);
	BAMS_EXPORT uint32_t        IResource_GetRefCounter(IResource *res);
	BAMS_EXPORT UUID            IResource_GetUID(IResource *res);
	BAMS_EXPORT const char *    IResource_GetName(IResource *res);
	BAMS_EXPORT const wchar_t * IResource_GetPath(IResource *res);
	BAMS_EXPORT bool            IResource_IsLoaded(IResource *res);
	BAMS_EXPORT uint32_t        IResource_GetType(IResource *res);
	BAMS_EXPORT bool            IResource_IsLoadable(IResource *res);
	BAMS_EXPORT const char *    IResource_GetXML(IResource *res, uint32_t *pSize = nullptr);

	// ResourceManager
	BAMS_EXPORT IResourceManager * IResourceManager_Create();
	BAMS_EXPORT void               IResourceManager_Destroy(IResourceManager *);
	BAMS_EXPORT IResource *        IResourceManager_AddResource(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT IResource *        IResourceManager_AddResourceWithoutFile(IResourceManager *rm, const char *resName, uint32_t type = 0);
	BAMS_EXPORT void               IResourceManager_AddDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT void               IResourceManager_RootDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT IResource *        IResourceManager_FindByName(IResourceManager *rm, const char *name, uint32_t type = 0);
	BAMS_EXPORT void               IResourceManager_Filter(IResourceManager *rm, void (*callback)(IResource *, void *), void *localData, const char *pattern = nullptr, uint32_t typeId = RESID_UNKNOWN);
	BAMS_EXPORT IResource *        IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IResRawData *      IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IResRawData *      IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name);
	BAMS_EXPORT void               IResourceManager_LoadSync(IResourceManager *rm, IResource *res = nullptr);
	BAMS_EXPORT void               IResourceManager_RefreshResorceFileInfo(IResourceManager *rm, IResource *res);
	BAMS_EXPORT void               IResourceManager_StartDirectoryMonitor(IResourceManager *rm);
	BAMS_EXPORT void               IResourceManager_StopDirectoryMonitor(IResourceManager *rm);
	
	// ResRawData
	BAMS_EXPORT uint8_t * IResRawData_GetData(IResRawData *res);
	BAMS_EXPORT size_t    IResRawData_GetSize(IResRawData *res);

	// ResShader
	BAMS_EXPORT uint8_t *       IResShader_GetData(IResShader *res);
	BAMS_EXPORT size_t          IResShader_GetSize(IResShader *res);
	BAMS_EXPORT void            IResShader_AddProgram(IResShader *res, const wchar_t *fileName);
	BAMS_EXPORT const wchar_t * IResShader_GetSourceFilename(IResShader *res, int type);
	BAMS_EXPORT const wchar_t * IResShader_GetBinaryFilename(IResShader *res, int type);
	BAMS_EXPORT void            IResShader_Save(IResShader *res);
	BAMS_EXPORT uint32_t        IResShader_GetBinaryCount(IResShader *res);
	BAMS_EXPORT IResRawData *   IResShader_GetBinary(IResShader *res, uint32_t idx);

	// ResMesh
	BAMS_EXPORT void                 IResMesh_SetVertexDescription(IResMesh *res, IVertexDescription *_vd, uint32_t _meshHash, IResource *_meshSrc, U32 _meshIdx);
	BAMS_EXPORT IVertexDescription * IResMesh_GetVertexDescription(IResMesh * res, bool loadASAP = false);
	BAMS_EXPORT IResource *          IResMesh_GetMeshSrc(IResMesh * res);
	BAMS_EXPORT uint32_t             IResMesh_GetMeshIdx(IResMesh * res);
	BAMS_EXPORT uint32_t             IResMesh_GetMeshHash(IResMesh * res);
	BAMS_EXPORT void                 IResMesh_SetMeshIdx(IResMesh * res, uint32_t idx);

	// Image
	BAMS_EXPORT Image *   IResImage_GetImage(IResImage *res, bool loadASAP = false);
	BAMS_EXPORT void      IResImage_Updated(IResImage *res);
	BAMS_EXPORT uint8_t * IResImage_GetSrcData(IResImage *res);
	BAMS_EXPORT size_t    IResImage_GetSrcSize(IResImage *res);
	BAMS_EXPORT void      IResImage_ReleaseSrc(IResImage *res);


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
		void *data,
		uint32_t dateLen = 0);

	BAMS_EXPORT void IEngine_PostMsg(
		uint32_t msgId,
		uint32_t msgDst,
		uint32_t msgSrc,
		void *data,
		uint32_t dateLen = 0,
		uint32_t delay = 0);

	BAMS_EXPORT void IEngine_Update(float dt);

	BAMS_EXPORT Allocators::IMemoryAllocator *GetMemoryAllocator(uint32_t allocatorType = 0, SIZE_T size = 0);

	BAMS_EXPORT void DoTests();
};
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

// ================================================================================== CResource ===

class CResourceManager;
class CResource
{
protected:
	ResourceSmartPtr _res;

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
	bool IsLoadable() { return IResource_IsLoadable(Get()); }

	void AddRef() { IResource_AddRef(Get()); }
	void Release() { IResource_Release(Get()); }
	uint32_t GetRefCounter() { return IResource_GetRefCounter(Get()); }
	inline IResource *Get() { return _res.Get(); }
	const char *GetXML(uint32_t *pSize = nullptr) { return IResource_GetXML(Get(), pSize); }
};

// ================================================================================== Help macro ===

#define CRESOURCEEXT(x, resTypeId) \
		C##x() {} \
		C##x(I##x *r) : CResource(static_cast<IResource *>(r)) {} \
		C##x(const C##x &src) : CResource(src) {} \
		C##x(C##x &&src) : CResource(std::move(src)) {} \
		virtual ~C##x() {} \
		C##x & operator = (const C##x &src) { _res = src._res; return *this; } \
		C##x & operator = (C##x &&src) { _res = std::move(src._res); return *this; } \
		static uint32_t GetType() { return resTypeId; }

	// ================================================================================== CResRawData ===

class CResRawData : public CResource
{
public:
	CRESOURCEEXT(ResRawData, RESID_RAWDATA)

		unsigned char *GetData() { return IResRawData_GetData(static_cast<IResRawData*>(Get())); }
	size_t GetSize() { return IResRawData_GetSize(static_cast<IResRawData*>(Get())); }
};

// ================================================================================== CResShader ===

class CResShader : public CResource
{
public:
	CRESOURCEEXT(ResShader, RESID_SHADER)

		unsigned char *GetData() { return IResShader_GetData(static_cast<IResShader*>(Get())); }
	size_t GetSize() { return IResShader_GetSize(static_cast<IResShader*>(Get())); }

	void AddProgram(const wchar_t *fileName) { IResShader_AddProgram(static_cast<IResShader*>(Get()), fileName); }
	const wchar_t *GetSourceFilename(int type) { return IResShader_GetSourceFilename(static_cast<IResShader*>(Get()), type); }
	const wchar_t *GetBinaryFilename(int type) { return IResShader_GetBinaryFilename(static_cast<IResShader*>(Get()), type); }

	uint32_t GetBinaryCount() { return IResShader_GetBinaryCount(static_cast<IResShader*>(Get())); }
	CResRawData GetBinary(uint32_t idx) { CResRawData prg(IResShader_GetBinary(static_cast<IResShader*>(Get()), idx)); return  std::move(prg); }

	void Save() { IResShader_Save(static_cast<IResShader*>(Get())); }
};

// ================================================================================== CResMesh ===

class CResMesh : public CResource
{
public:
	CRESOURCEEXT(ResMesh, RESID_MESH)

		void SetVertexDescription(IVertexDescription *vd, uint32_t meshHash, CResource &meshRes, uint32_t meshIdx) { IResMesh_SetVertexDescription(static_cast<IResMesh*>(Get()), vd, meshHash, meshRes.Get(), meshIdx); }
	void SetVertexDescription(IVertexDescription *vd, uint32_t meshHash, IResource *meshRes, uint32_t meshIdx) { IResMesh_SetVertexDescription(static_cast<IResMesh*>(Get()), vd, meshHash, meshRes, meshIdx); }
	IVertexDescription *GetVertexDescription(bool loadASAP = false) { return IResMesh_GetVertexDescription(static_cast<IResMesh*>(Get()), loadASAP); }
	IResource *GetMeshSrc() { return IResMesh_GetMeshSrc(static_cast<IResMesh*>(Get())); }
	uint32_t GetMeshIdx() { return IResMesh_GetMeshIdx(static_cast<IResMesh*>(Get())); }
	uint32_t GetMeshHash() { return IResMesh_GetMeshHash(static_cast<IResMesh*>(Get())); }
	void SetMeshIdx(uint32_t idx) { IResMesh_SetMeshIdx(static_cast<IResMesh*>(Get()), idx); }
};


// ================================================================================== CResImage ===

class CResImage : public CResource
{
public:
	CRESOURCEEXT(ResImage, RESID_IMAGE)

	Image *GetImage(bool loadASAP = false) { return IResImage_GetImage(static_cast<IResImage*>(Get()), loadASAP); }
	void Updated() { IResImage_Updated(static_cast<IResImage*>(Get())); }
	uint8_t *GetSrcData() { return IResImage_GetSrcData(Get()); }
	size_t GetSrcSize() { return IResImage_GetSrcSize(Get()); }
	void ReleaseSrc() { return IResImage_ReleaseSrc(Get()); }
};

// ================================================================================== CResourceManager ===

class CResourceManager
{
	IResourceManager *_rm;

public:
	CResourceManager() { _rm = IResourceManager_Create(); }
	CResourceManager(IResourceManager *rm) : _rm(rm) {};
	~CResourceManager() { IResourceManager_Destroy(_rm); _rm = nullptr; }

	IResource * AddResource(const wchar_t *path) { return IResourceManager_AddResource(_rm, path); }
	IResource * AddResource(const char *resName, uint32_t type = 0) { return IResourceManager_AddResourceWithoutFile(_rm, resName, type); }

	void AddDir(const wchar_t *path) { IResourceManager_AddDir(_rm, path); }
	void RootDir(const wchar_t *path) { IResourceManager_RootDir(_rm, path); }

	void Filter(void(*callback)(IResource *, void *), void *localData, const char *pattern = nullptr, uint32_t typeId = RESID_UNKNOWN) { IResourceManager_Filter(_rm, callback, localData, pattern, typeId); }
	IResource * Find(const char *name, uint32_t type = RESID_UNKNOWN) { return IResourceManager_FindByName(_rm, name, type); }
	IResource * Find(const UUID &uid) { return IResourceManager_FindByUID(_rm, uid); }

	CResRawData GetRawData(const char *name) { CResRawData res(IResourceManager_GetRawDataByName(_rm, name)); return std::move(res); }
	CResRawData GetRawData(const UUID &uid) { CResRawData res(IResourceManager_GetRawDataByUID(_rm, uid)); return std::move(res); }

	CResShader GetShaderByName(const char *name) { CResShader res(IResourceManager_FindByName(_rm, name, RESID_SHADER)); return std::move(res); }

	template<typename T>
	T Get(const char *name) { T res(IResourceManager_FindByName(_rm, name, T::GetType())); return std::move(res); }

	template<typename T>
	T Get(const UUID &uid) { T res(IResourceManager_FindByUID(_rm, uid)); return std::move(res); }

	void LoadSync() { IResourceManager_LoadSync(_rm, nullptr); }
	void LoadSync(CResource &res) { IResourceManager_LoadSync(_rm, res.Get()); }
	void RefreshResorceFileInfo(IResource *res) { IResourceManager_RefreshResorceFileInfo(_rm, res); }

	void StartDirectoryMonitor() { IResourceManager_StartDirectoryMonitor(_rm); }
	void StopDirectoryMonitor() { IResourceManager_StopDirectoryMonitor(_rm); }
};

// ================================================================================== CEngine ===

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
		uint32_t msgDst = 0,
		uint32_t msgSrc = 0,
		void *data = nullptr,
		uint32_t dataLen = 0)
	{
		IEngine_SendMsg(msgId, msgDst, msgSrc, data, dataLen);
	}

	static void PostMsg(
		uint32_t msgId,
		uint32_t msgDst,
		uint32_t msgSrc,
		void *data,
		uint32_t dataLen = 0,
		uint32_t delay = 0)
	{
		IEngine_PostMsg(msgId, msgDst, msgSrc, data, dataLen, delay);
	}

	static void Update(float dt)
	{
		IEngine_Update(dt);
	}
};

// ================================================================================== IExternalModule ===

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
	RENDERING_ENGINE = 0x201, // VK
	IMPORT_MODULE    = 0x401 // 
};

enum { // msgId
	// to everyone (general notifications)
	RESOURCE_ADD_FILE        = 0x10001,
	RESOURCE_MANIFEST_PARSED = 0x10003,

	// to RENDERING_ENGINE
	CREATE_WINDOW         = 0x20001,
	CLOSE_WINDOW          = 0x20002,
	ADD_MESH              = 0x20003, // mesh_name_in_shader + mesh_name_as_resource + shader_program
	ADD_SHADER            = 0x20004,
	RELOAD_SHADER         = 0x20005,
	GET_SHADER_PARAMS     = 0x20006,
	GET_OBJECT_PARAMS     = 0x20007,
	UPDATE_DRAW_COMMANDS  = 0x20008,
	SET_CAMERA            = 0x20009,
	ADD_TEXTURE           = 0x2000a,

	// to IMPORT_MODULE (or everyone?)
	IDENTIFY_RESOURCE        = 0x40001,
	IMPORTMODULE_UPDATE      = 0x40002,
	IMPORTMODULE_LOADMESH    = 0x40003,
	IMPORTMODULE_LOADIMAGE   = 0X40004,
	IMPORTMODULE_UPDATEIMAGE = 0X40005,
};

struct PCREATE_WINDOW {
	uint32_t wnd;
	uint32_t w, h;
	uint32_t x, y;
};

struct PCLOSE_WINDOW {
	uint32_t wnd;
};

struct PADD_MESH {
	uint32_t wnd;
	const char *name;    // uniq name
	const char *mesh;	 // resource name
	const char *shader;  // resource name
	Properties **pProperties;
	uint32_t *pId;
};

struct PADD_SHADER {
	uint32_t wnd;
	const char *shader;
};

struct SHADER_PARAM_ENTRY {
	uint32_t parent;
	uint32_t type;
	uint32_t count;
	const char *name;
};

struct PGET_SHADER_PARAMS {
	uint32_t wnd;
	const char *name;
	Properties **pProperties;
};

typedef struct PADD_SHADER PRELOAD_SHADER;
struct PGET_OBJECT_PARAMS {
	uint32_t wnd;
	const char *name;
	uint32_t objectIdx;
	Properties **pProperties;
};

struct PIDETIFY_RESOURCE {
	const wchar_t *filename;
	uint32_t *pType;
	void *res;
};

struct PUPDATE_DRAW_COMMANDS {
	uint32_t wnd;
};

struct PSET_CAMERA {
	uint32_t wnd;
	float eye[3];
	float lookAt[3];
	float up[3];
	float fov;
	float zNear;
	float zFar;
};

struct PADD_TEXTURE {
	uint32_t wnd;
	void *propVal;
	const char *textureResourceName;
};