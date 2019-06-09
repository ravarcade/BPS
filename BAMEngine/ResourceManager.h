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

	ResBase *FindOrCreate_withFilename(const WSTR &filename, U32 typeId = RESID_UNKNOWN);
	ResBase *Get(const STR &resName, U32 typeId = RESID_UNKNOWN);
	ResBase *Get(CSTR resName, U32 typeId = RESID_UNKNOWN) { return Get(STR(resName), typeId); };
	ResBase *FindOrCreate_WithUID(const UUID &resUID);

	template<class T>
	ResBase *Get(const STR &resName) { return Get(resName, T::GetTypeId()); }

	template<class T>
	ResBase *Get(CSTR resName) { return Get(resName, T::GetTypeId()); }

	ResBase *Add(CWSTR path);
	ResBase *Add(CSTR resName, U32 type = 0);

	void AddDir(CWSTR path);
	void RootDir(CWSTR path);

	void LoadSync(ResBase *res = nullptr);
	void RefreshResorceFileInfo(ResBase *res);

	void StartDirectoryMonitor();
	void StopDirectoryMonitor();
	void AbsolutePath(WSTR &filename, const WSTR *_root = nullptr);
	void RelativePath(WSTR &filename, const WSTR *_root = nullptr);
	WSTR GetResourceAbsoluteDir(ResBase *res);
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
