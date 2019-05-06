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

using namespace BAMS;

class Importer
{
private:
	struct ImportedModelRepo
	{
		IResource *res;
		AssImp_Loader aiLoader;

		void Preload()
		{
			CResource r(res);
			aiLoader.Clear();
			aiLoader.PreLoad(r.GetPath());
		}

		bool IsLoaded()
		{
			return aiLoader.IsLoaded();
		}
	};

	CEngine *en;
	std::vector<ImportedModelRepo *> m_importedModelRepos;

	ImportedModelRepo *GetImportedModelRepo(IResource *res)
	{
		for (auto pimr : m_importedModelRepos)
			if (pimr->res == res)
				return pimr;

		auto pimr = new ImportedModelRepo();
		pimr->res = res;
		m_importedModelRepos.emplace_back(pimr);
		return pimr;
	}


	BAMS::CORE::CWStringHastable<IResource *> m_modelRepoName2modelRepo;
	
	void _StartImport(IResource *res)
	{
		// this is temporaly
		// it should work like this:
		// create wroking threade for loading and parsing file with objects
		// after parsing working thread shoud send "parsig done event" to main thread
		// resources should be added to RM
		// working thread should wait for main thread until all resources are added, when working thread should be terminated (and used resources released)

		CResourceManager rm;
		auto pimr = GetImportedModelRepo(res);

		pimr->Preload();

		// we have hash calculated for every mesh... so we want: 
		// (1) remove broken meshes from RM: hash is wrong or something
		rm.Filter([](IResource *res, void *param) {
			auto *pimr = reinterpret_cast<ImportedModelRepo*>(param);

			CMesh r(res);
			if (r.GetMeshSrc() == pimr->res)
			{
				auto hash = r.GetMeshHash();
				auto &vd = *reinterpret_cast<VertexDescription *>(r.GetVertexDescription());

				// check
				U32 i = 0;
				U32 idx = -1;
				if (!pimr->aiLoader.ForEachMesh([&](AssImp_Loader::ImportedMesh &m) {
					if (m.match == false &&
						m.hash == hash &&
						m.vd == vd)
					{
						m.match = true;
						idx = i;
						return true;
					}
					++i;
					return false;
				}))
				{
					// remove res ... or it was just moddified?
					// wait for next pass...
				}
				else {
					if (idx == r.GetMeshIdx())
					{
						// same idx... all is fine
					}
					else
					{
						r.SetMeshIdx(idx);
					}
				}
			}
		}, pimr, nullptr, RESID_MESH);

		// (2) second pass... assign all meshes in RM even if "hash" is not matching, but only if it is not used. So, update of one mesh should works fine.
		rm.Filter([](IResource *res, void *param) {
			auto *pimr = reinterpret_cast<ImportedModelRepo*>(param);

			CMesh r(res);
			auto &al = pimr->aiLoader;
			if (r.GetMeshSrc() == pimr->res)
			{
				auto pm = al.Mesh(r.GetMeshIdx());
				if (pm && pm->match == false)
				{
					pm->match = true;
				}
				else {
					r.SetMeshIdx(-1);  // mark it as "broken"
				}
			}
		}, pimr, nullptr, RESID_MESH);

		// (3) add new meshes
		uint32_t num = 0;
		pimr->aiLoader.ForEachMesh([&](AssImp_Loader::ImportedMesh &m) {
			if (!m.match) 
			{ // add new mesh
				CMesh mesh(rm.AddResource("Mesh_1", RESID_MESH));
				mesh.SetVertexDescription(&m.vd, m.hash, res, num);
			}
			++num;
			return false;
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
				_StartImport(p->res);
				break;
			}
		} while ((sz = strtok_s(nullptr, ";", &next_token)));
	}

	void ResLoadMesh(void *params)
	{
		CMesh m(params);
		auto res = m.GetMeshSrc();
		if (res) {
			auto *pimr = GetImportedModelRepo(res);
			if (!pimr->IsLoaded())
				pimr->Preload();

			if (pimr->IsLoaded()) 
			{
				auto pvd = reinterpret_cast<VertexDescription *>(m.GetVertexDescription());
				auto pim = pimr->aiLoader.Mesh(m.GetMeshIdx());
				if (pim)
					*pvd = pim->vd;

			}
		}
		else {
			// fill VD with "default" model.... tada!
			TRACE("[NO MESH]\n");
		}
	}

	void ResUpdate(void *params)
	{
		CResource res(params);
		TRACEW(L"ResUp: " << res.GetPath() << L"\n");
	}

	
	void OnResourceManifestParsed()
	{
		m_modelRepoName2modelRepo.clear();
		CResourceManager rm;

		// add all modelRepos
		rm.Filter([](IResource *res, void *param) {
			auto *pim = reinterpret_cast<Importer*>(param);
			pim->m_modelRepoName2modelRepo.add(IResource_GetPath(res), res);
		}, this, nullptr, RESID_IMPORTMODEL);

		// add all meshes
		if (false) // right now, we don't need to do anything with meshes... maybe in "vaidation"
		rm.Filter([](IResource *res, void *param) {
			auto *pim = reinterpret_cast<Importer*>(param);
			
			CResource r(res);
			uint32_t rxs = 0;
			auto rx = r.GetXML(&rxs);
			CORE::STR rxml(rx, rx + rxs);

			// check if mesh mach
		}, this, nullptr, RESID_MESH);
	}

	void Initialize()
	{
		BAMS::CORE::STR::Initialize();
		BAMS::CORE::WSTR::Initialize();
		en = new CEngine();
	}

	void Finalize()
	{
		m_modelRepoName2modelRepo.reset();
		m_importedModelRepos.clear();
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
		case RESOURCE_MANIFEST_PARSED: im.OnResourceManifestParsed(); break;
		case IDENTIFY_RESOURCE: im.ResIdentify(msg->data); break;
		case IMPORTMODEL_UPDATE: im.ResUpdate(msg->data); break;
		case IMPORTMODEL_LOADMESH: im.ResLoadMesh(msg->data); break;
//		case IMPORTMODEL_RESCAN: im.Rescan(); break;
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