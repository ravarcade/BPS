#include "stdafx.h"


namespace BAMS {
namespace CORE {

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;

static F32 I[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
static const char * defaultShaderProgramName = "basic";

// ----------------------------------------------------------------------------

void ResModel::AddMesh(CSTR meshResourceName, CSTR shaderProgramName, const float * m)
{
	auto &rm = globalResourceManager;
	auto mesh = rm->FindExisting(meshResourceName, RESID_MESH);
	auto shader = rm->FindExisting(shaderProgramName, RESID_SHADER);
	if (shader == nullptr)
		auto shader = rm->FindExisting(defaultShaderProgramName, RESID_SHADER);

	if (mesh)
	{
		auto entry = meshes.add_empty();
		entry->mesh = mesh;
		entry->shader = shader;
		memcpy_s(entry->m, sizeof(entry->m), (m ? m : I), sizeof(I));

		// add entry to xml
		auto xmlEntry = rm->NewXMLElement(entry->mesh->Name.c_str());
		xmlEntry->SetAttribute("shader", entry->shader->Name.c_str());
		Tools::XMLWriteValue(xmlEntry, entry->m, static_cast<uint32_t>(COUNT_OF(entry->m)));
		rb->XML->InsertEndChild(xmlEntry);
	}
}

void ResModel::GetMesh(U32 idx, const char ** mesh, const char ** shader, const float **m, MProperties **prop)
{
	if (idx < meshes.size())
	{
		auto &entry = meshes[idx];

		if (mesh)
			*mesh = entry.mesh->Name.c_str();

		if (shader)
			*shader = entry.shader->Name.c_str();

		if (m)
			*m = entry.m;

		if (prop)
			*prop = &entry.prop;
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
			auto entry = meshes.add_empty();
			entry->mesh = mesh;
			entry->shader = rm->FindExisting(xmlEntry->Attribute("shader", defaultShaderProgramName), RESID_SHADER);
			if (entry->shader == nullptr)
				entry->shader = rm->FindExisting("basic", RESID_SHADER);

			Tools::XMLReadValue(xmlEntry, entry->m, 16);
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
		Tools::XMLWriteValue(xmlEntry, entry.m, static_cast<uint32_t>(COUNT_OF(entry.m)));
		rb->XML->InsertEndChild(xmlEntry);
	}
}

}; // CORE namespace
}; // BAMS namespace