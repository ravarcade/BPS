
class DirectoryChangeNotifier : public MemoryAllocatorStatic<>
{
	struct InternalData;
	InternalData *_data;

public:
	DirectoryChangeNotifier();
	~DirectoryChangeNotifier();

	void AddDir(PathSTR &&sPath, U32 type);
	void Start();
	void Stop();
};


