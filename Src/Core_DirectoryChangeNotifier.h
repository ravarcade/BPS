
struct DiskEvent
{
	U32 action;
	WString fileName;
	WString fileNameRenameTo;
};

class DirectoryChangeNotifier : public Allocators::Ext<>
{
	struct InternalData;
	InternalData *_data;

public:
	DirectoryChangeNotifier();
	~DirectoryChangeNotifier();

	void AddDir(PathSTR &&sPath, U32 type);
	void Start();
	void Stop();
	DiskEvent *GetDiskEvent();
	
};


