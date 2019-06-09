#include "stdafx.h"


namespace BAMS {
namespace CORE {

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;

static F32 I[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

// ----------------------------------------------------------------------------

void ResModel::AddMesh(CSTR meshResourceName, const float * m)
{
	auto &rm = globalResourceManager;
	auto mesh = rm->Get(meshResourceName, RESID_MESH);
	if (mesh)
	{
		auto entry = meshes.add_empty();
		entry->mesh = mesh;
		memcpy_s(entry->m, sizeof(entry->m), (m ? m : I), sizeof(I));

		// add entry to xml
		auto xmlEntry = rm->NewXMLElement(entry->mesh->Name.c_str());
		Tools::XMLWriteValue(xmlEntry, entry->m, static_cast<uint32_t>(COUNT_OF(entry->m)));
		rb->XML->InsertEndChild(xmlEntry);
	}
}

// ---------------------------------------------------------------------------- private ---

void ResModel::_LoadXML()
{
	auto &rm = globalResourceManager;
	meshes.clear();
	for (auto xmlEntry = rb->XML->FirstChildElement(); xmlEntry; xmlEntry = xmlEntry->NextSiblingElement())
	{
		auto mesh = rm->Get(xmlEntry->Name(), RESID_MESH);
		if (mesh)
		{
			auto entry = meshes.add_empty();
			entry->mesh = mesh;
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