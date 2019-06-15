/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResBase;
class ResourceManager;
class ResMesh; // we have special Filter method in Reasource Manager, so it is declared erly

extern ResourceManager *globalResourceManager;

class BAMS_EXPORT ResourceManager : public Allocators::Ext<>
{
private:
	struct InternalData;
	InternalData *_data;

protected:
	void CreateResourceImplementation(ResBase *res);

	friend class ResBase;

public:
	ResourceManager();
	~ResourceManager();

	static ResourceManager *Create();
	static void Destroy(ResourceManager *);

	void Filter(
		void(*callback)(ResBase *, void *), 
		void * localData, 
		CSTR namePattern = nullptr, 
		CWSTR filenamePattern = nullptr, 
		CSTR filenamePatternUTF8 = nullptr, 
		CWSTR rootPath = nullptr, 
		U32 typeId = RESID_UNKNOWN, 
		bool caseInsesitive = false);

	ResBase *Create(CSTR name, U32 type);
	ResBase *FindOrCreate(CSTR name, U32 type = RESID_FORBIDEN);
	ResBase *FindOrCreate(const UUID &resUID, U32 type = RESID_FORBIDEN);
	ResBase *FindOrCreate(CWSTR filename, U32 type = RESID_FORBIDEN);
	template<class T>
	ResBase *FindOrCreate(CSTR resName) { return FindOrCreate(resName, T::GetTypeId()); }

	ResBase *FindExisting(CSTR name, U32 type);
	ResBase *FindExisting(CWSTR filename, U32 type);
	ResBase *FindExisting(const UUID &resUID, U32 type);

	void AddDir(CWSTR path);
	void RootDir(CWSTR path);

	void LoadSync(ResBase *res = nullptr);
	void RefreshResorceFileInfo(ResBase *res);

	void StartDirectoryMonitor();
	void StopDirectoryMonitor();
	void AbsolutePath(WSTR &filename, const WSTR *_root = nullptr);
	void RelativePath(WSTR &filename, const WSTR *_root = nullptr);

	WSTR GetDirPath(const WString &filename);

	tinyxml2::XMLElement *NewXMLElement(const char* name = "");
};

// ============================================================================

class ResourceManagerModule : public IModule
{
public:
	void Update(float dt);
	void Initialize();
	void Finalize();
	void SendMsg(Message *msg);
	~ResourceManagerModule();

	U32 GetModuleId() { return IModule::ResourceManagerModule; }

	ResourceManager *GetResourceManager();
};

// ============================================================================

#include "ResImp.h"
#include "ResourceNotifier.h"
#include "ResBase.h"
#include "ResRawData.h"
#include "ResShader.h"
#include "ResMesh.h"
#include "ResModel.h"
#include "ResImage.h"
#include "ResDrawModel.h"
