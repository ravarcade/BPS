#include "stdafx.h"


namespace BAMS {
namespace CORE {

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;

static const float I[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
static const char * defaultShaderProgramName = "basic";

// ----------------------------------------------------------------------------

void ResModel::AddMesh(CSTR meshResourceName, CSTR shaderProgramName, const float * m)
{
	auto rm = globalResourceManager;
	auto mesh = rm->FindExisting(meshResourceName, RESID_MESH);
	auto shader = rm->FindExisting(shaderProgramName, RESID_SHADER);
	AddMesh(mesh, shader, m);
}

void ResModel::AddMesh(ResBase * mesh, ResBase * shader, const float * m)
{
	auto rm  = globalResourceManager;
	if (shader == nullptr)
		auto shader = rm->FindExisting(defaultShaderProgramName, RESID_SHADER);

	if (mesh)
	{
		auto &entry = meshes.emplace_back(mesh, shader, (m ? m : I));
		// add entry to xml
		auto xmlEntry = rm->NewXMLElement(entry.mesh->Name.c_str());
		xmlEntry->SetAttribute("shader", entry.shader->Name.c_str());
		Tools::XMLWriteValue(xmlEntry, entry.M, static_cast<uint32_t>(COUNT_OF(entry.M)));
		Tools::XMLWriteProperties(xmlEntry, entry.prop);
		rb->XML->InsertEndChild(xmlEntry);
	}
}

void ResModel::GetMesh(U32 idx, const char ** pMesh, const char ** pShader, const float **pM, const Properties **pProp)
{
	if (idx < meshes.size())
	{
		auto &entry = meshes[idx];

		if (pMesh)
			*pMesh = entry.mesh->Name.c_str();

		if (pShader)
			*pShader = entry.shader->Name.c_str();

		if (pM)
			*pM = entry.M;

		if (pProp)
			*pProp = &entry.prop;
	}
}

void ResModel::GetMesh(U32 idx, ResBase ** pMesh, ResBase ** pShader, const float ** pM, const Properties ** pProp)
{
	if (idx < meshes.size())
	{
		auto &entry = meshes[idx];

		if (pMesh)
			*pMesh = entry.mesh;

		if (pShader)
			*pShader = entry.shader;

		if (pM)
			*pM = entry.M;

		if (pProp)
			*pProp = &entry.prop;
	}
}

void ResModel::SetMeshProperties(U32 idx, const Properties *prop)
{
	if (idx < meshes.size())
	{
		auto &entry = meshes[idx];
		entry.prop = *prop;
		_SaveXML();
	}
}

// ---------------------------------------------------------------------------- private ---

void ResModel::_LoadXML()
{
	auto &rm = globalResourceManager;
	meshes.clear();
	for (auto xmlEntry = rb->XML->FirstChildElement(); xmlEntry; xmlEntry = xmlEntry->NextSiblingElement())
	{
		auto mesh = rm->FindOrCreate(xmlEntry->Name());
		if (mesh && mesh->Type == RESID_MESH)
		{
			auto &entry = meshes.emplace_back();
			entry.mesh = mesh;
			entry.shader = rm->FindExisting(xmlEntry->Attribute("shader", defaultShaderProgramName), RESID_SHADER);
			if (entry.shader == nullptr)
				entry.shader = rm->FindExisting("basic", RESID_SHADER);

			Tools::XMLReadValue(xmlEntry, entry.M, 16);
			Tools::XMLReadProperties(xmlEntry, entry.prop);
		}
	}
}

void ResModel::_SaveXML()
{
	auto &rm = globalResourceManager;
	rb->XML->DeleteChildren();
	for (auto &entry : meshes)
	{

		auto xmlEntry = rm->NewXMLElement(entry.mesh->Name.c_str());
		xmlEntry->SetAttribute("shader", entry.shader->Name.c_str());
		Tools::XMLWriteValue(xmlEntry, entry.M, static_cast<uint32_t>(COUNT_OF(entry.M)));
		Tools::XMLWriteProperties(xmlEntry, entry.prop);
		rb->XML->InsertEndChild(xmlEntry);
	}
}

void ResModel::MeshEntry::FindTexturesInProperties(ResBase *meshSrc)
{
	static const char *map[] = {
		"samplerColor", "albedo", "file", nullptr,
		"samplerNormal", "normal", nullptr,
		nullptr
	};

	auto srcPath = meshSrc ? meshSrc->Path.c_str() : nullptr;
	for (auto &p : prop)
	{
		if (p.type != Property::PT_CSTR)
			continue;

		const char **m = map;
		while (*m)
		{
			for (auto fixedName = *m++; *m; ++m)
			{
				if (_stricmp(p.name, *m) == 0)
				{
					// find file							
					globalResourceManager->Filter([](ResBase *res, void *local) {
						auto &p = *reinterpret_cast<Property *>(local);
						p.type = Property::PT_TEXTURE;
						p.val = res;
					}, &p, nullptr, nullptr, reinterpret_cast<CSTR>(p.val), srcPath, RESID_UNKNOWN, true);
					if (p.type == Property::PT_TEXTURE)
					{
						p.name = fixedName;
						break;
					}
				}
			}
			++m;
		}
	}
}

}; // CORE namespace
}; // BAMS namespace

template<> struct std::is_trivially_move_constructible<BAMS::CORE::ResModel::MeshEntry> { static constexpr bool value = true; };


