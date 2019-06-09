/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResourceUpdateNotifier
{
public:
	struct IChain {
		IChain *next;
		IChain *prev;
		bool isValide;
		virtual void Notify() = 0;
	} *first;


	ResourceUpdateNotifier() : first(nullptr) {}
	~ResourceUpdateNotifier()
	{
		for (IChain *f = first; f; f = f->next)
			f->isValide = false;
	}

	void Notify()
	{
		for (IChain *f = first; f; f = f->next)
			f->Notify();
	}

	void Add(IChain *n)
	{
		n->next = first;
		n->prev = nullptr;
		if (first)
			first->prev = n;
		first = n;
		n->isValide = true;
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
	U64 _param;
	T *_owner;
public:
	ResourceUpdateNotifyReciver() : _res(nullptr), _param(0) {}
	~ResourceUpdateNotifyReciver() { StopNotifyReciver(); }
	void SetResource(ResBase *res, T *owner, U64 param = 0)
	{
		StopNotifyReciver();
		_res = res;
		_owner = owner;
		_param = param;
		_res->AddNotifyReciver(this);
		res->AddRef();
	}

	void StopNotifyReciver()
	{
		if (_res) {
			if (isValide)
				_res->RemoveNotifyReciver(this);
			_res->Release();
		}
	}

	void Notify() { if (_owner) _owner->Notify(_res, _param); }
	ResBase *GetResource() { return _res; }
};