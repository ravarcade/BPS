/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "DllInterface.h"
*
*/

enum {
	RESID_FORBIDEN = 0,

	RESID_CORE_RESOURCES = 0x00010000,
	RESID_UNKNOWN,
	RESID_RAWDATA,

	// resources used by rendering engine
	RESID_RENDERING_ENGINE_RESOURCES = 0x00020000,
	RESID_SHADER,
	RESID_SHADER_SRC,
	RESID_SHADER_BIN,

	RESID_MESH = RESID_RENDERING_ENGINE_RESOURCES + 0x201,
	RESID_MODEL,
	RESID_IMPORTMODEL,
	RESID_IMAGE,
	RESID_DRAWMODEL,

	RESID_VERTEXDATA,


	// Property GUI types
	PTGUI_COLOR = 1,
	PTGUI_DRAG = 2,
};

// ============================================================================

//typedef void IResource;
//typedef void IResRawData;
//typedef void IResShader;
//typedef void IResMesh;
//typedef void IResModel;
//typedef void IResImage;
//typedef void IResourceManager;

//typedef void IModule;

struct IResource { /* empty */ };
struct IResRawData : public IResource { /* empty */ };
struct IResShader : public IResource { /* empty */ };
struct IResMesh : public IResource { /* empty */ };
struct IResModel : public IResource { /* empty */ };
struct IResImage : public IResource { /* empty */ };
struct IResourceManager { /* empty */ };

struct IModule { /* empty */ };


enum { // msgDst
	RENDERING_ENGINE = 0x201, // VK
	IMPORT_MODULE = 0x401 // 
};

enum { // msgId
	// to everyone (general notifications)
	RESOURCE_ADD_FILE = 0x10001,
	RESOURCE_MANIFEST_PARSED = 0x10003,

	// to RENDERING_ENGINE
	CREATE_WINDOW = 0x20001,
	CLOSE_WINDOW = 0x20002,
	ADD_MESH = 0x20003, // mesh_name_in_shader + mesh_name_as_resource + shader_program
	ADD_SHADER = 0x20004,
	RELOAD_SHADER = 0x20005,
	GET_SHADER_PARAMS = 0x20006,
	GET_MESH_PARAMS = 0x20007,
	UPDATE_DRAW_COMMANDS = 0x20008,
	SET_CAMERA = 0x20009,
	ADD_TEXTURE = 0x2000a,
	ADD_MODEL = 0x2000b,
	UPDATE_TEXTURE = 0x2000c,
	SET_MODEL_MATRIX = 0x2000d,

	// to GUI
	SHOW_PROPERTIES = 0x30001,

	// to IMPORT_MODULE (or everyone?)
	IDENTIFY_RESOURCE = 0x40001,
	IMPORTMODULE_UPDATE = 0x40002,
	IMPORTMODULE_LOADMESH = 0x40003,
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

typedef uint32_t HandleMesh;
typedef uint32_t HandleModel;

struct PADD_MESH {
	uint32_t wnd;
	IResource *resMesh;
	IResource *resShader;
	HandleMesh *pHandleMesh;
};

struct PADD_SHADER {
	uint32_t wnd;
	IResource *resShader;
};
//
//struct SHADER_PARAM_ENTRY {
//	uint32_t parent;
//	uint32_t type;
//	uint32_t count;
//	const char *name;
//};

struct PGET_SHADER_PARAMS {
	uint32_t wnd;
	IResource *resShader;
	Properties **pProperties;
};

typedef struct PADD_SHADER PRELOAD_SHADER;
struct PGET_MESH_PARAMS {
	uint32_t wnd;
	HandleMesh hMesh;
	Properties **pProperties;
};

struct PIDETIFY_RESOURCE {
	const wchar_t *filename;
	uint32_t *pType;
	IResource *res;
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
	IResource *resTexture;
};

struct PUPDATE_TEXTURE {
	uint32_t wnd;
	IResource *resTexture;
};

struct PADD_MODEL {
	uint32_t wnd;
	IResource * resModel;
	HandleModel *pHandleModel;
};

struct PSHOW_PROPERTIES 
{
	uint32_t wnd;
	HandleMesh hMesh;
	const char *propertiesName;
};

struct PSET_MODEL_MATRIX
{
	uint32_t wnd;
	HandleMesh hModel;
	const float *T;
	const char *propName;
};

extern "C" {

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

	// ResourceManager
	BAMS_EXPORT IResourceManager * IResourceManager_Create();
	BAMS_EXPORT void               IResourceManager_Destroy(IResourceManager *);
	BAMS_EXPORT IResource *        IResourceManager_CreateResource(IResourceManager *rm, const char *resName, uint32_t type);
	BAMS_EXPORT IResource *        IResourceManager_FindOrCreate(IResourceManager *rm, const char *resName, uint32_t type = RESID_FORBIDEN);
	BAMS_EXPORT IResource *        IResourceManager_FindOrCreateByFilename(IResourceManager *rm, const wchar_t *path, uint32_t type = RESID_FORBIDEN);
	BAMS_EXPORT IResource *        IResourceManager_FindOrCreateByUID(IResourceManager *rm, const UUID &uid, uint32_t type = RESID_FORBIDEN);

	BAMS_EXPORT IResource *        IResourceManager_FindExisting(IResourceManager *rm, const char *resName, uint32_t type);
	BAMS_EXPORT IResource *        IResourceManager_FindExistingByFilename(IResourceManager *rm, const wchar_t *path, uint32_t type);
	BAMS_EXPORT IResource *        IResourceManager_FindExistingByUID(IResourceManager *rm, const UUID &uid, uint32_t type);

	BAMS_EXPORT void               IResourceManager_AddDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT void               IResourceManager_RootDir(IResourceManager *rm, const wchar_t *path);

	BAMS_EXPORT void               IResourceManager_Filter(
		IResourceManager *rm,
		void(*callback)(IResource *, void *),
		void * localData,
		const char * namePattern = nullptr,
		const wchar_t * filenamePattern = nullptr,
		const char * filenamePatternUTF8 = nullptr,
		const wchar_t * rootPath = nullptr,
		uint32_t typeId = RESID_UNKNOWN,
		bool caseInsesitive = false);

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
	BAMS_EXPORT void                      IResMesh_SetVertexDescription(IResMesh *res, VertexDescription *_vd, uint32_t _meshHash, IResource *_meshSrc, U32 _meshIdx, const Properties *_meshProperties);
	BAMS_EXPORT const VertexDescription * IResMesh_GetVertexDescription(IResMesh * res, bool loadASAP = false);
	BAMS_EXPORT const Properties *        IResMesh_GetMeshProperties(IResMesh * res, bool loadASAP = false);
	BAMS_EXPORT IResource *               IResMesh_GetMeshSrc(IResMesh * res);
	BAMS_EXPORT uint32_t                  IResMesh_GetMeshIdx(IResMesh * res);
	BAMS_EXPORT uint32_t                  IResMesh_GetMeshHash(IResMesh * res);
	BAMS_EXPORT void                      IResMesh_SetMeshIdx(IResMesh * res, uint32_t idx);

	// ResModel
	BAMS_EXPORT void                 IResModel_AddMesh(IResModel *res, const char *meshResName, const char * shaderProgramName, const float *m = nullptr);
	BAMS_EXPORT void                 IResModel_AddMeshRes(IResModel *res, IResource *mesh, IResource * shader, const float *m = nullptr);
	BAMS_EXPORT void                 IResModel_GetMesh(IResModel *res, uint32_t idx, const char **pMesh, const char ** pShader, const float **pM, const Properties **pProperties);
	BAMS_EXPORT void                 IResModel_GetMeshRes(IResModel *res, uint32_t idx, IResource **pMesh, IResource ** pShader, const float **pM, const Properties **pProperties);
	BAMS_EXPORT uint32_t             IResModel_GetMeshCount(IResModel *res);
	BAMS_EXPORT void                 IResModel_SetMeshProperties(IResModel *res, uint32_t idx, const Properties *properties);

	// Image
	BAMS_EXPORT Image *   IResImage_GetImage(IResImage *res, bool loadASAP = false);
//	BAMS_EXPORT void      IResImage_Updated(IResImage *res);
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
	template<typename T>
	T *Get() { return reinterpret_cast<T*>(_res.Get()); }
};

// ================================================================================== Help macro ===

#define CRESOURCEEXT2(x, y, resTypeId) \
		C##x() {} \
		C##x(IResource *r): CResource(r) {} \
		C##x(I##y *r) : CResource(reinterpret_cast<IResource *>(r)) {} \
		C##x(const C##x &src) : CResource(src) {} \
		C##x(C##x &&src) : CResource(std::move(src)) {} \
		virtual ~C##x() {} \
		C##x & operator = (const C##x &src) { _res = src._res; return *this; } \
		C##x & operator = (C##x &&src) { _res = std::move(src._res); return *this; } \
		static uint32_t GetType() { return resTypeId; } \
		I##y *Self() { return Get<I##y>(); }

#define CRESOURCEEXT(x, resTypeId) CRESOURCEEXT2(x, x, resTypeId)

// ================================================================================== CResRawData ===

class CResRawData : public CResource
{
public:
	CRESOURCEEXT(ResRawData, RESID_RAWDATA);

	unsigned char *GetData() { return IResRawData_GetData(Self()); }
	size_t GetSize() { return IResRawData_GetSize(Self()); }
};

// ================================================================================== CResShader ===

class CResShader : public CResource
{
public:
	CRESOURCEEXT(ResShader, RESID_SHADER);

	unsigned char *GetData() { return IResShader_GetData( Self() ); }
	size_t GetSize() { return IResShader_GetSize( Self() ); }

	void AddProgram(const wchar_t *fileName) { IResShader_AddProgram( Self(), fileName); }
	const wchar_t *GetSourceFilename(int type) { return IResShader_GetSourceFilename( Self(), type); }
	const wchar_t *GetBinaryFilename(int type) { return IResShader_GetBinaryFilename( Self(), type); }

	uint32_t GetBinaryCount() { return IResShader_GetBinaryCount( Self()); }
	CResRawData GetBinary(uint32_t idx) { CResRawData prg(IResShader_GetBinary( Self(), idx)); return  std::move(prg); }

	void Save() { IResShader_Save( Self()); }
};

// ================================================================================== CResMesh ===

class CResMesh : public CResource
{
public:
	CRESOURCEEXT(ResMesh, RESID_MESH);

	void SetVertexDescription(VertexDescription *vd, uint32_t meshHash, CResource &meshRes, uint32_t meshIdx, const Properties *meshProperties) { IResMesh_SetVertexDescription(Self(), vd, meshHash, meshRes.Get(), meshIdx, meshProperties); }
	void SetVertexDescription(VertexDescription *vd, uint32_t meshHash, IResource *meshRes, uint32_t meshIdx, const Properties *meshProperties) { IResMesh_SetVertexDescription(Self(), vd, meshHash, meshRes, meshIdx, meshProperties); }
	const VertexDescription *GetVertexDescription(bool loadASAP = false) { return IResMesh_GetVertexDescription(Self(), loadASAP); }
	const Properties *GetMeshProperties(bool loadASAP = false) { return IResMesh_GetMeshProperties(Self(), loadASAP); }
	IResource *GetMeshSrc() { return IResMesh_GetMeshSrc(Self()); }
	uint32_t GetMeshIdx() { return IResMesh_GetMeshIdx(Self()); }
	uint32_t GetMeshHash() { return IResMesh_GetMeshHash(Self()); }
	void SetMeshIdx(uint32_t idx) { IResMesh_SetMeshIdx(Self(), idx); }
};

// ================================================================================== CResModel ===

class CResModel : public CResource
{
public:
	CRESOURCEEXT(ResModel, RESID_MODEL);

	void AddMesh(const char * meshResName, const char *shaderProgramName, const float * m = nullptr) { IResModel_AddMesh(Self(), meshResName, shaderProgramName, m); }
	void AddMesh(IResource * mesh, IResource *shader, const float * m = nullptr) { IResModel_AddMeshRes(Self(), mesh, shader, m); }
	void GetMesh(uint32_t idx, const char **pMesh, const char **pShader = nullptr, const float **pM = nullptr, const Properties **pProperties = nullptr) { IResModel_GetMesh(Self(), idx, pMesh, pShader, pM, pProperties); }
	void GetMesh(uint32_t idx, IResource **pMesh, IResource **pShader = nullptr, const float **pM = nullptr, const Properties **pProperties = nullptr) { IResModel_GetMeshRes(Self(), idx, pMesh, pShader, pM, pProperties); }
	uint32_t GetMeshCount() { return IResModel_GetMeshCount(Self()); }
	void SetMeshProperties(uint32_t idx, const Properties *properties) { IResModel_SetMeshProperties(Self(), idx, properties); }
	void ModifyMeshProperties(uint32_t idx, const Properties *properties) 
	{ 
		const Properties *orgProperties;
		IResModel_GetMeshRes(Self(), idx, nullptr, nullptr, nullptr, &orgProperties);
		if (orgProperties)
		{
			sProperties newProperties(*orgProperties);
			for (auto &p : *properties)
			{
				newProperties.set(p);
			}
			IResModel_SetMeshProperties(Self(), idx, &newProperties);
		}
	}

	void SetMeshProperty(uint32_t idx, const char *name, uint32_t type, uint32_t count, void *val)
	{
		Property p;
		p.name = name;
		p.type = type;
		p.count = count;
		p.val = val;

		const Properties *orgProperties;
		IResModel_GetMeshRes(Self(), idx, nullptr, nullptr, nullptr, &orgProperties);
		if (orgProperties)
		{
			sProperties newProperties(*orgProperties);
			newProperties.set(p);
			IResModel_SetMeshProperties(Self(), idx, &newProperties);
		}
	}

	void SetMeshProperty(uint32_t idx, const char *name, void *val)
	{
		const Properties *orgProperties;
		IResModel_GetMeshRes(Self(), idx, nullptr, nullptr, nullptr, &orgProperties);
		if (orgProperties)
		{
			sProperties newProperties(*orgProperties);
			newProperties.set(name, val);
			IResModel_SetMeshProperties(Self(), idx, &newProperties);
		}
	}
};

// ================================================================================== CResImage ===

class CResImage : public CResource
{
public:
	CRESOURCEEXT(ResImage, RESID_IMAGE);

	Image *GetImage(bool loadASAP = false) { return IResImage_GetImage(Self(), loadASAP); }
	uint8_t *GetSrcData() { return IResImage_GetSrcData(Get<IResImage>()); }
	size_t GetSrcSize() { return IResImage_GetSrcSize(Get<IResImage>()); }
	void ReleaseSrc() { return IResImage_ReleaseSrc(Get<IResImage>()); }
};

// ================================================================================== CResourceManager ===

class CResourceManager
{
	IResourceManager *_rm;

public:
	CResourceManager() { _rm = IResourceManager_Create(); }
	CResourceManager(IResourceManager *rm) : _rm(rm) {};
	~CResourceManager() { IResourceManager_Destroy(_rm); _rm = nullptr; }

	IResource * Create(const char *resName, uint32_t type) { return IResourceManager_CreateResource(_rm, resName, type); }

	IResource * FindOrCreate(const char *resName, uint32_t type = RESID_FORBIDEN) { return IResourceManager_FindOrCreate(_rm, resName, type); }
	IResource * FindOrCreate(const wchar_t *path, uint32_t type = RESID_FORBIDEN) { return IResourceManager_FindOrCreateByFilename(_rm, path, type); }
	IResource * FindOrCreate(const UUID &uid, uint32_t type = RESID_FORBIDEN) { return IResourceManager_FindOrCreateByUID(_rm, uid, type); }

	IResource * FindExisting(const char *resName, uint32_t type) { return IResourceManager_FindExisting(_rm, resName, type); }
	IResource * FindExisting(const wchar_t *path, uint32_t type) { return IResourceManager_FindExistingByFilename(_rm, path, type); }
	IResource * FindExisting(const UUID &uid, uint32_t type) { return IResourceManager_FindExistingByUID(_rm, uid, type); }

	void AddDir(const wchar_t *path) { IResourceManager_AddDir(_rm, path); }
	void RootDir(const wchar_t *path) { IResourceManager_RootDir(_rm, path); }

	void Filter(void(*callback)(IResource *, void *), void *localData, const char * namePattern = nullptr, const wchar_t * filenamePattern = nullptr, const char * filenamePatternUTF8 = nullptr, const wchar_t * rootPath = nullptr, uint32_t typeId = RESID_UNKNOWN, bool caseInsesitive = false)
	{ IResourceManager_Filter(_rm, callback, localData, namePattern, filenamePattern, filenamePatternUTF8, rootPath, typeId, caseInsesitive); }

	// version used to find all resources given typeid
	void Filter(uint32_t typeId, void *localData, void(*callback)(IResource *, void *)) { IResourceManager_Filter(_rm, callback, localData, nullptr, nullptr, nullptr, nullptr, typeId, false); }

	// version used to find textures for mesh property
	void Filter(CResMesh &mesh, BAMS::Property *prop, void *localData, void(*callback)(IResource *, void *)) { IResourceManager_Filter(_rm, callback, localData, nullptr, nullptr, reinterpret_cast<CSTR>(prop->val), IResource_GetPath(mesh.GetMeshSrc()), RESID_UNKNOWN, true); }
	
	template <typename F>
	void Filter(uint32_t typeId, F f)
	{
		Filter(typeId, &f, [](IResource *res, void *data) {
			auto &f = *reinterpret_cast<F*>(data);
			f(res);
		});
	}

	template<typename T>
	T Get(const char *name) { T res(FindOrCreate(name, T::GetType())); return std::move(res); }

	template<typename T>
	T Get(const UUID &uid) { T res(FindOrCreate(uid, T::GetType())); return std::move(res); }

	void LoadSync() { IResourceManager_LoadSync(_rm, nullptr); }
	void LoadSync(CResource &res) { IResourceManager_LoadSync(_rm, res.Get()); }
	void RefreshResorceFileInfo(IResource *res) { IResourceManager_RefreshResorceFileInfo(_rm, res); }

	void StartDirectoryMonitor() { IResourceManager_StartDirectoryMonitor(_rm); }
	void StopDirectoryMonitor() { IResourceManager_StopDirectoryMonitor(_rm); }

	// some helper functions
	const char * GetName(IResource *res) { return IResource_GetName(res); }
	const wchar_t * GetPath(IResource *res) { return IResource_GetPath(res); }
	uint32_t GetType(IResource *res) { return IResource_GetType(res); }
};

// ================================================================================== CEngine ===
class CResModelDraw;
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

	// =========================== messages ============
	static HandleModel AddModel(uint32_t wnd, IResource *resModel)
	{
		HandleModel hModel = 0;
		PADD_MODEL params = { wnd, resModel, &hModel };
		BAMS::CEngine::SendMsg(ADD_MODEL, RENDERING_ENGINE, 0, &params);
		return hModel;
	}

	static HandleMesh AddMesh(uint32_t wnd, IResource *resMesh, IResource *resShader)
	{
		HandleMesh hMesh;
		PADD_MESH params = { wnd, resMesh, resShader, &hMesh };
		BAMS::CEngine::SendMsg(ADD_MESH, RENDERING_ENGINE, 0, &params);
		return hMesh;
	}

	static Properties *GetMeshParams(uint32_t wnd, HandleMesh hMesh)
	{
		Properties *prop = nullptr;
		PGET_MESH_PARAMS params = { wnd, hMesh, &prop };
		BAMS::CEngine::SendMsg(GET_MESH_PARAMS, RENDERING_ENGINE, 0, &params);
		return prop;
	}

	static void ShowProperties(uint32_t wnd, HandleMesh hMesh, const char *name = nullptr)
	{
		PSHOW_PROPERTIES params = { wnd, hMesh, name };
		BAMS::CEngine::SendMsg(SHOW_PROPERTIES, RENDERING_ENGINE, 0, &params);
	}	

	static void SetTexture(uint32_t wnd, Property *p, const char *resTextureName) 
	{
		CResourceManager rm;
		SetTexture(wnd, p, rm.FindExisting(resTextureName, CResImage::GetType()));
	}

	static void SetTexture(uint32_t wnd, Property *p, IResource *resTexture)
	{
		PADD_TEXTURE params = { wnd, p->val, resTexture };
		BAMS::CEngine::SendMsg(ADD_TEXTURE, RENDERING_ENGINE, 0, &params);
	}

	static void SetModelMatrix(uint32_t wnd, HandleMesh hMesh, const float *T, const char *propName = nullptr)
	{
		PSET_MODEL_MATRIX params = { wnd, hMesh, T, propName };
		BAMS::CEngine::SendMsg(SET_MODEL_MATRIX, RENDERING_ENGINE, 0, &params);
	}
};

typedef CEngine SndMsg;

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


// ================================================================================== *** CResModelDraw ===

class CResModelDraw : public CResource
{
public:
	CRESOURCEEXT2(ResModelDraw, ResModel, RESID_MODEL);

	// CResModel methods
	void AddMesh(const char * meshResName, const char *shaderProgramName, const float * m = nullptr) { IResModel_AddMesh(Self(), meshResName, shaderProgramName, m); }
	void GetMesh(uint32_t idx, const char **mesh, const char **shader, const float **m, const Properties **properties) { IResModel_GetMesh(Self(), idx, mesh, shader, m, properties); }
	uint32_t GetMeshCount() { return IResModel_GetMeshCount(Self()); }

	// CResModelDraw methods
	void AddToWindow(uint32_t _wnd, const char *modelMatrixName)
	{
		wnd = _wnd;
		hModel = CEngine::AddModel(wnd, Get());

		//PADD_MESH p = { wnd, nullptr, nullptr, &hModel };


		//CEngine en;
		//uint32_t count = GetMeshCount();
		//meshes.resize(count);
		//Properties *pMeshProp;
		//
		//PADD_MESH p = { wnd, nullptr, nullptr, &hModel };

		//for (uint32_t i = 0; i < count; ++i)
		//{
		//	const float *M;
		//	const Properties *srcProp;
		//	GetMesh(i, &p.mesh, &p.shader, &M, &srcProp);

		//	MeshData &m = meshes[i];
		//	p.pId = &m.meshId;
		//	en.SendMsg(ADD_MESH, RENDERING_ENGINE, 0, &p);
		//	memcpy_s(m.M, sizeof(m.M), M, sizeof(m.M));
		//	m.prop = *pMeshProp;


		//	// set mesh "model" matrix (M)
		//	auto pModelProp = m.prop.Find(modelMatrixName);
		//	m.outputM = pModelProp ? reinterpret_cast<float *>(pModelProp->val) : nullptr;

		//	// set textures
		//	for (auto &p : m.prop)
		//	{
		//		if (p.type != Property::PT_TEXTURE)
		//			continue;

		//		auto src = srcProp->ConstFind(p.name);
		//		void *val = src ? src->val : nullptr;
		//		if (!val)
		//		{
		//			CResourceManager rm;
		//			val = rm.FindExisting(p.name, RESID_IMAGE);
		//		}
		//		if (val)
		//		{
		//			PADD_TEXTURE addTexParams = { wnd, p.val, nullptr, reinterpret_cast<IResource *>(val) };
		//			en.SendMsg(ADD_TEXTURE, RENDERING_ENGINE, 0, &addTexParams);
		//		}
		//	}

		//}
	}

	Properties *GetProperties(uint32_t idx)
	{
		return CEngine::GetMeshParams(wnd, hModel + idx);
	}

	Property *GetProperty(const char *name, uint32_t idx)
	{
		auto prop = GetProperties(idx);

		if (!prop)
			return nullptr;
		return prop->Find(name);

	}
	void *GetParam(const char *name, uint32_t idx)
	{
		auto p = GetProperty(name, idx);
		return p ? p->val : nullptr;
	}

	void SetParam(const char *name, void *value, uint32_t idx = -1)
	{
		if (idx == -1)
		{
			idx = 0;
			for (uint32_t max = GetMeshCount(); idx < max; ++idx)
				SetParam(name, value, idx);
		}
		auto p = GetProperty(name, idx);
		if (p) 
		{
			if (p->type == Property::PT_TEXTURE) 
			{
				CEngine::SetTexture(wnd, p, reinterpret_cast<IResource *>(value));
			}
			else {
				p->SetMem(value);
			}
		}
	}

	void SetMatrix(const float *T) 
	{
		CEngine::SetModelMatrix(wnd, hModel, T);
		//for (auto &m : meshes)
			//Tools::Mat4mul(m.outputM, T, m.M);
	};

	void ShowProperties(const char *name, uint32_t idx = 0)
	{
		CEngine::ShowProperties(wnd, hModel + idx, name);
	}

private:
	uint32_t wnd;
	HandleModel hModel;
	//struct MeshData
	//{		
	//	uint32_t meshId;
	//	float M[16];
	//	float *outputM;
	//	MProperties prop;
	//};
	//array<MeshData> meshes;
};
