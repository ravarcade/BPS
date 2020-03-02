/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

enum ResourceNotifyType {
	UPDATE = 1,
	ERASE = 2,
	MODIFY = 3,
	DESTROY = 4,
};

class ResourceUpdateNotifier
{
public:
	struct IChain {
		IChain *next;
		IChain *prev;
		bool isValid;
		virtual void Notify(U32 type) = 0;
	} *first;


	ResourceUpdateNotifier() : first(nullptr) {}
	~ResourceUpdateNotifier()
	{
		for (IChain *f = first; f; f = f->next)
			f->isValid = false;
	}

	void Notify(U32 type )
	{
		for (IChain *f = first; f; f = f->next)
			f->Notify(type);
	}

	void Add(IChain *n)
	{
		n->next = first;
		n->prev = nullptr;
		if (first)
			first->prev = n;
		first = n;
		n->isValid = true;
	}

	void Remove(IChain *n)
	{
		if (n->next)
			n->next->prev = n->prev;

		if (first == n)
			first = n->next;
		else
			n->prev->next = n->next;
	}
};

template<typename T>
class ResourceUpdateNotifyReciver : public ResourceUpdateNotifier::IChain
{
private:
	ResBase *_res;
	T *_owner;
public:
	ResourceUpdateNotifyReciver() : _res(nullptr), _owner(nullptr) {}
	~ResourceUpdateNotifyReciver() { 
//		TRACE("~ResourceUpdateNotifyReciver: " << this << ", res: " << _res << " (" << (_res ? _res->Name.c_str() : "empty") << "), owner: " << (_owner ? _owner->rb->Name.c_str() : "missing") <<"\n");
		StopNotifyReciver(); 
	}
	void SetResource(ResBase *res, T *owner)
	{
		StopNotifyReciver();
		_res = res;
		_owner = owner;
		_res->AddNotifyReciver(this);
		res->AddRef();
	}

	void StopNotifyReciver()
	{
		if (_res) {
			if (isValid)
				_res->RemoveNotifyReciver(this);
			_res->Release();
		}
	}

	void Notify(U32 type) { if (_owner) _owner->Notify(_res, type); }
	ResBase *GetResource() { return _res; }
};