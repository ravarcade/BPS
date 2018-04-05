#include "stdafx.h"
/*
namespace BAMS {

	using namespace CORE;

	template<typename T, class Alloc = MemoryAllocatorImpl<Allocators::Linear> >
	class ListWithAdditionalMemory : private Alloc
	{
	public:
		struct Node : public T
		{
			Node *pNext;
			Node *pPrev;
			SIZE_T size;
			void *GetExtraData() { return this + sizeof(Node); }

			Node & operator = (const Node &src)
			{
				if (size == src.size)
				{
					dynamic_cast<T>(*this) = dynamic_cast<T>(src);
					if (src.size)
						memcpy_s(GetExtraData(), src.size, src.GetExtraData(), src.size);
				}
			}
		};

	private:
		Node *pFirst;
		Node *pLast;

	public:
		ListWithAdditionalMemory() :
			pFirst(nullptr),
			pLast(nullptr)
		{}


		struct iterator // : std::iterator<std::bidirectional_iterator_tag, Node, ptrdiff_t, Node*, Node&>
		{
			iterator(Node *ptr = nullptr) : pNode(ptr) {}
			iterator(const iterator &i) = default;
			~iterator() {}

			Node & operator * () { return *pNode; }

			iterator & operator = (const iterator &i) = default;
			iterator & operator = (Node *ptr) { pNode = ptr; }

			operator bool() const { return pNode != nullptr; }

			bool operator == (const iterator &i) const { return pNode == i.pNode; }
			bool operator != (const iterator &i) const { return pNode != i.pNode; }

			iterator &  operator++() { pNode = pNode->pNext ? pNode->pNext : nullptr; return *this; }
			iterator &  operator--() { pNode = pNode->pPrev ? pNode->pPrev : nullptr; return *this; }

		protected:
			Node *pNode;
		};

		iterator begin() { return (pFirst); }
		iterator end() { return (nullptr); }

		Node * GetFirst() { return pFirst; }
		void Clear()
		{
			while (pLast)
			{
				Node *toDel = pLast;
				pLast = pLast->pPrev;
				pLast->pNext = nullptr;

				deallocate(toDel);
			}

			// here should be clear frame command
		}

		Node * CreateNode(SIZE_T extraMemory = 0)
		{
			SIZE_T size = sizeof(Node) + extraMemory;
			Node * ret = new (allocate(size)) Node();

			ret->pPrev = pLast;
			ret->pNext = nullptr;
			ret->size = extraMemory;

			if (pLast)
				pLast->pNext = ret;
			else {
				pLast = ret;
				pFirst = ret;
			}

			return ret;
		}

		void DeleteNode(Node *pNode)
		{
			Node * &next = pNode->pPrev ? pNode->pPrev->pNext : pFirst;
			next = pNode->pNext;
			
			Node * &prev = pNode->pNext ? pNode->pNext->pPrev : pLast;
			prev = pNode->pPrev;

			deallocate(pNode);
		}

		void Copy(ListWithAdditionalMemory<T> &src)
		{
			for (auto &srcNode : src)
			{
				Node *pDstNode = CreateNode(srcNode.size);
				*pDstNode = srcNode;
			}
		}
	};

	/// <summary>
	/// Here all data used by ResourceManifest are stored in memory.
	/// </summary>
	struct ResourceManifest::InternalData
	{
		typedef ListWithAdditionalMemory<ResourceSourceDescription, MemoryAllocatorImpl<Allocators::Linear> > TResourcesDescriptionList;

		TResourcesDescriptionList _resoucesList;
		static MemoryAllocatorImpl<> _allocator;

		InternalData() {};
		~InternalData()
		{
			for (auto p : _resoucesList)
			{
				Remove(&p);
			}
		}


		ResourceSourceDescription *Create(CSTR path, CSTR name)
		{
			SIZE_T pathLen = strlen(path) + 1;
			SIZE_T nameLen = strlen(name) + 1;
			SIZE_T extraMemoryNeeded = pathLen + nameLen;
			auto pNode = _resoucesList.CreateNode(extraMemoryNeeded);
			char *pData = static_cast<char *>(pNode->GetExtraData());
			pNode->Path = pData;
			pNode->Name = pData + pathLen;
			pNode->Type = ResourceBase::UNKNOWN;
			Tools::CreateUUID(pNode->UID);


			memcpy_s(pData, pathLen, path, pathLen);
			memcpy_s(pData + pathLen, nameLen, name, nameLen);			

			return pNode;
		}

		void Remove(ResourceSourceDescription * resDesc)
		{
			_resoucesList.DeleteNode(reinterpret_cast<TResourcesDescriptionList::Node *>(resDesc));
		}
	};

	// ============================================================================

	ResourceManifest::ResourceManifest()
	{
		_data = InternalData::_allocator.make_new<InternalData>();
	}

	ResourceManifest::~ResourceManifest()
	{
		InternalData::_allocator.make_delete<InternalData>(_data);
	}

/// <summary>
/// Creates new resource source description based on path to file (and name if given).
/// </summary>
/// <param name="path">The path to file.</param>
/// <param name="name">The name of resource (optional).</param>
/// <returns></returns>
	ResourceSourceDescription * ResourceManifest::Create(CSTR path, CSTR name)
	{
		STR normalizedPath = path;
		STR tmpName = "";

		Tools::NormalizePath(normalizedPath);
		if (name == nullptr)
		{
			const char *posibleName = strrchr(path, Tools::directorySeparatorChar);
			name = "_not_set_";
		}

		return _data->Create(normalizedPath.c_str(), name);
	}

	ResourceSourceDescription * ResourceManifest::Find(UUID && uid)
	{
		for (auto &node : _data->_resoucesList)
		{
			if (node.UID == uid)
			{
				return &node;
			}
		}

		return nullptr;
	}

	void ResourceManifest::Remove(ResourceSourceDescription * resDesc)
	{
		_data->Remove(resDesc);
	}

	void ResourceManifest::Add(ResourceManifest & resourceManifest)
	{
	}

	MemoryAllocatorImpl<> ResourceManifest::InternalData::_allocator;
}

*/