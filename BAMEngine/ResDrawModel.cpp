#include "stdafx.h"

namespace BAMS {
namespace CORE {

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;

// ---------------------------------------------------------------------------- private ---

void ResDrawModel::_LoadXML()
{
	auto &rm = globalResourceManager;
	//meshes.clear();
	//for (auto xmlEntry = rb->XML->FirstChildElement(); xmlEntry; xmlEntry = xmlEntry->NextSiblingElement())
	//{
	//	auto mesh = rm->FindOrCreate(xmlEntry->Name());
	//	if (mesh && mesh->Type == RESID_MESH)
	//	{
	//		auto entry = meshes.add_empty();
	//		entry->mesh = mesh;
	//		entry->shader = rm->FindExisting(xmlEntry->Attribute("shader", defaultShaderProgramName), RESID_SHADER);
	//		if (entry->shader == nullptr)
	//			entry->shader = rm->FindExisting("basic", RESID_SHADER);

	//		Tools::XMLReadValue(xmlEntry, entry->m, 16);
	//	}
	//}
}

void ResDrawModel::_SaveXML()
{
	auto &rm = globalResourceManager;
	//rb->XML->DeleteChildren();
	//for (auto &entry : meshes)
	//{
	//	auto xmlEntry = rm->NewXMLElement(entry.mesh->Name.c_str());
	//	Tools::XMLWriteValue(xmlEntry, entry.m, static_cast<uint32_t>(COUNT_OF(entry.m)));
	//	rb->XML->InsertEndChild(xmlEntry);
	//}
}

}; // CORE namespace
}; // BAMS namespace