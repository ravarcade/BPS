/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

/// <summary>
/// Base Resource Class.
/// It provides common for all resources field, like pointer to memory with data, length, state if it is loaded to mem.
/// </summary>
class ResBase
{
protected:
	void *_resData;
	SIZE_T _resSize;
	time_t _resTimestamp;
	IResImp *_pResImp;
	U32 _refCounter;

	bool _isLoaded;
	bool _isDeleted;
	bool _isModified;
	bool _isLoadable;
	time _waitWithUpdate;

	static constexpr time::duration defaultDelay = 200ms;
	ResourceUpdateNotifier _updateNotifier;

	void _CreateResourceImplementation();
public:
	UUID UID;
	U32  Type;
	STR  Name;
	WSTR Path;
	tinyxml2::XMLElement *XML;

	/// <summary>
	/// Initializes a new instance of the <see cref="ResBase"/> class.
	/// It is empty resource without assigned any file on disk.
	/// </summary>
	ResBase(tinyxml2::XMLElement *xml) :
		_resData(nullptr),
		_isLoaded(false),
		_isDeleted(false),
		_isModified(false),
		_isLoadable(true),
		_refCounter(0),
		_pResImp(nullptr),
		Type(RESID_UNKNOWN),
		UID(Tools::NOUID),
		XML(xml)
	{};

	~ResBase()
	{
		if (_pResImp)
			_pResImp->GetFactory()->Destroy(_pResImp);
	};

	void Init(CWSTR path);
	void Init(CSTR name);

	void ResourceLoad(void *data, SIZE_T size = -1)
	{
		_resData = data;
		if (size != -1)
			_resSize = size;

		_isLoaded = data != nullptr;
	}

	inline bool isLoaded() { return _isLoaded; }
	inline bool isDeleted() { return _isDeleted; }
	inline bool isModified() { return _isModified; }
	inline bool isLoadable() { return _isLoadable; }



	void *GetData() { return _resData; }
	template<typename T> T *GetData() { return reinterpret_cast<T*>(_resData); }
	SIZE_T GetSize() { return _resSize; }
	time_t GetTimestamp() { return _resTimestamp; }
	void SetTimestamp() { time(_resTimestamp); }
	IResImp *GetImplementation() { return _pResImp; }

	void AddRef() {
		if (!_pResImp && !_refCounter)
			_CreateResourceImplementation();

		++_refCounter;
	}

	U32 GetRefCounter() { return _refCounter; }

	void Release()
	{
		--_refCounter;
		if (_refCounter == 0 && _pResImp != nullptr)
		{	// resource not used (with _refCounter == 0) don't need resource implementation... "Destroy" will delete it.
			_pResImp->Release(this);
			_pResImp->GetFactory()->Destroy(_pResImp);
			_pResImp = nullptr;
		}
	}


	void Update() { if (_pResImp) _pResImp->Update(this); _updateNotifier.Notify(); }
	IMemoryAllocator *GetMemoryAllocator() { return !_pResImp ? nullptr : _pResImp->GetFactory()->GetMemoryAllocator(); }

	void AddNotifyReciver(ResourceUpdateNotifier::IChain *nr) { _updateNotifier.Add(nr); }
	void RemoveNotifyReciver(ResourceUpdateNotifier::IChain *nr) { _updateNotifier.Remove(nr); }
	friend class ResourceManager;
};