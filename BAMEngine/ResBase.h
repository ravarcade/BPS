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

	bool _isDeleted;
	bool _isModified;
	bool _isLoaded;						// Is data from disk in memory.
	bool _isLoadable;					// Is resource loadable at all. If FALSE then it will get Update notification immediately, but no data will be loaded into memory.	
	bool _isLoadFromDiskAllowed;		// If FALSE, when data will not be loaded from disk. This state may be changed.
	time _waitWithUpdateNotification;

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
	ResBase(CSTR name, tinyxml2::XMLElement *xml) :
		_resData(nullptr),
		_resSize(0),
		_isLoaded(false),
		_isDeleted(false),
		_isModified(false),
		_isLoadable(true),
		_isLoadFromDiskAllowed(true),
		_refCounter(0),
		_pResImp(nullptr),
		Type(RESID_UNKNOWN),
		UID(Tools::NOUID),
		Name(name),
		XML(xml)
	{};

	~ResBase()
	{
		if (_pResImp)
			_pResImp->GetFactory()->Destroy(_pResImp);
	};
	
	void ReleaseMemory() 
	{		
		if (_resData) {
			auto alloc = GetMemoryAllocator();
			assert(alloc); // we can't have reserved memory and don't have allocaator for it!
			if (alloc) {
				alloc->deallocate(_resData);
			}
			_resData = nullptr;
		}
	}

	inline bool isLoaded() { return _isLoaded; }
	inline bool isDeleted() { return _isDeleted; }
	inline bool isModified() { return _isModified; }
	inline bool isLoadable() { return _isLoadFromDiskAllowed; }

	void *GetData() { return _resData; }
	template<typename T> T *GetData() { return reinterpret_cast<T*>(_resData); }
	SIZE_T GetSize() { return _resSize; }
	time_t GetTimestamp() { return _resTimestamp; }
	void SetTimestamp() { time(_resTimestamp); }
	IResImp *GetImplementation() { return _pResImp; }

	void AddRef() {
		if (!_pResImp && !_refCounter) {
			_isLoadFromDiskAllowed = _isLoadable;
			_CreateResourceImplementation();
		}

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

	void UpdateDataMemory(const void *newData, SIZE_T newSize) 
	{
		assert(_pResImp);
		if (_resSize < newSize)
		{
			ReleaseMemory();
			_resSize = newSize;
			_resData = GetMemoryAllocator()->allocate(newSize);
		}
		if (newData)
			memcpy_s(_resData, _resSize, newData, newSize);
	}

	friend class ResourceManager;
};