// ImportModule.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

using namespace BAMS;
using namespace ::Assimp;

size_t cstringlen(const char *p) { return strlen(p); }
size_t cstringlen(const wchar_t *p) { return wcslen(p); }

template<typename T, typename U>
bool endsWith(T *s, size_t slen, U *p, size_t plen, bool caseInsesitive = true)
{
	if (plen > slen)
		return false;

	s += slen - plen;
	while (*s)
	{
		if (caseInsesitive ? towlower(*s) != towlower(*p) : *s != *p)
			return false;
		++s;
		++p;
	}
	return true;
}

template<typename T>
bool endsWith(T *s, size_t slen, const char *p, bool caseInsesitive = true) { return endsWith(s, slen, p, cstringlen(p), caseInsesitive); }

template<typename T, typename U>
bool endsWith(T *s, U *p, bool caseInsesitive = true) { return endsWith(s, cstringlen(s), p, cstring(p), caseInsesitive); }

// --------------------------------------------------------------------------------

class Importer
{
private:
	std::vector<CResource> m_importedModels;
	CEngine *en;
	AssImp_Loader aiLoader;

	void StartImport(CResource &res)
	{
		aiLoader.PreLoad(res.GetPath());
		CResourceManager rm;
		rm.Filter([](BAMS::IResource *res, void *parm) {

		}, nullptr, nullptr, RESID_MESH);
//		rm.	
		uint32_t num = 0;
		aiLoader.ForEachMesh([&] (VertexDescription &vd) {
			auto x = CMesh::BuildXML(&vd, res, num);
			TRACE(x);
			++num;
		});
	}

public:
	Importer() : en(nullptr) {}

	void ResIdentify(void *params)
	{
		auto p = static_cast<PIDETIFY_RESOURCE *>(params);
		auto fn = (p->filename);
		auto len = cstringlen(fn);

		aiString aiExt;
		aiGetExtensionList(&aiExt);

		size_t s = aiExt.length;
		char *next_token;
		const char* sz = strtok_s(aiExt.data, ";", &next_token);
		do {
			if (endsWith(fn, len,sz[0] == '*' ? sz + 1 : sz))
			{
				*(p->pType) = RESID_IMPORTMODEL;
				CResource res(p->res);
				m_importedModels.emplace_back(res);
				StartImport(res);
				break;
			}
		} while ((sz = strtok_s(nullptr, ";", &next_token)));
	}

	void ResUpdate(void *params)
	{
		CResource res(params);
		TRACEW(L"ResUp: " << res.GetPath() << L"\n");
	}

	void Initialize()
	{
		BAMS::CORE::STR::Initialize();
		BAMS::CORE::WSTR::Initialize();
		en = new CEngine();
	}

	void Finalize()
	{
		m_importedModels.clear();
		BAMS::CORE::WSTR::Finalize();
		BAMS::CORE::STR::Finalize();
		delete en;
		en = nullptr;
	}
};

::Importer im;

/// <summary>
///  
/// </summary>
/// <seealso cref="IExternalModule{CImportModule, IMPORT_MODULE}" />
class CImportModule : public IExternalModule<CImportModule, IMPORT_MODULE>
{

public:
	CImportModule() {}

	void Initialize()
	{
		printf("IM: Initialize()\n");
		im.Initialize();
	}

	void Finalize() {
		printf("IM: Finalize()\n");
		im.Finalize();
	}

	void Update(float dt)
	{
//		printf("IM: Update()\n");
	}

	void SendMsg(Message *msg)
	{
		switch (msg->id)
		{
		case IDENTIFY_RESOURCE: im.ResIdentify(msg->data); break;
		case IMPORTMODEL_UPDATE: im.ResUpdate(msg->data); break;
		default:
			printf("Not supported message");
		}
	}

	void Destroy() {}
};


CImportModule ImportModule;

extern "C" {
	IM_EXPORT void RegisterModule()
	{
		printf("HELLO IMPORT\n");
		ImportModule.Register();

	}
}