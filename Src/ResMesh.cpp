#include "stdafx.h"

namespace BAMS {
namespace CORE {

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;

// ----------------------------------------------------

void ResMesh::_LoadXML()
{
	auto &rm = globalResourceManager;
	tinyxml2::XMLElement* r = rb->XML->FirstChildElement("Mesh");
	if (r == nullptr)
		return;

	WSTR srcFileName;
	srcFileName.UTF8(r->Attribute("src"));
	rm->AbsolutePath(srcFileName);
	meshSrc = rm->GetByFilename(srcFileName, RESID_IMPORTMODEL);
	meshIdx = r->IntAttribute("idx", 0);
	meshHash = r->IntAttribute("hash", 0);
	vd.m_numVertices = r->IntAttribute("vertices", 0);
	vd.m_numIndices = r->IntAttribute("indices", 0);

	// vd
	auto xml2stream = [&](Stream &o, const char *streamName) {
		auto s = r->FirstChildElement(streamName);
		if (s)
		{
			o.m_type = s->IntAttribute("type", 0);
			o.m_stride = s->IntAttribute("stride", 0);
			o.m_normalized = s->BoolAttribute("normalized", false);
		}
	};

	auto xml2streamarray = [&](Stream *o, int num, const char *streamName) {
		auto s = r->FirstChildElement(streamName);
		for (int i = 0; i < num; ++i) {
			if (!s)
				break;
			o[i].m_type = s->IntAttribute("type", 0);
			o[i].m_stride = s->IntAttribute("stride", 0);
			o[i].m_normalized = s->BoolAttribute("normalized", false);
			s = s->NextSiblingElement(streamName);
		}
	};

#define XML2STREAM(x) xml2stream(vd.m_ ## x, #x);
#define XML2STREAMARRAY(x) xml2streamarray(vd.m_ ## x, static_cast<int>(COUNT_OF(vd.m_ ## x)), #x);

	XML2STREAM(vertices);
	XML2STREAM(normals);
	XML2STREAM(tangents);
	XML2STREAM(bitangents);
	XML2STREAMARRAY(colors);
	XML2STREAMARRAY(textureCoords);
	XML2STREAMARRAY(boneWeights);
	XML2STREAMARRAY(boneIDs);
	XML2STREAM(indices);
}

void ResMesh::_SaveXML() 
{ 
//	STR ResMesh::_BuildXML(VertexDescription *pvd, U32 _meshHash, ResourceBase * _meshSrc, U32 _meshIdx)
//	{
//	using tinyxml2::XMLPrinter;
//	using tinyxml2::XMLDocument;

	auto &rm = globalResourceManager;
	auto out = rb->XML ? rb->XML : rm->NewXMLElement("temp");
	out->DeleteChildren();
//	XMLDocument out;
	auto *r = rm->NewXMLElement("Mesh");
	STR cvt;
	WSTR msf = meshSrc->Path;
	rm->RelativePath(msf);
	cvt.UTF8(msf);
	r->SetAttribute("src", cvt.c_str());
	r->SetAttribute("idx", meshIdx);
	r->SetAttribute("hash", meshHash);
	r->SetAttribute("vertices", vd.m_numVertices);
	r->SetAttribute("indices", vd.m_numIndices);
	auto stream2xml = [&](Stream &s, const char *streamName) {
		if (s.isUsed())
		{
			auto o = rm->NewXMLElement(streamName);
			o->SetAttribute("type", s.m_type);
			o->SetAttribute("stride", s.m_stride);
			o->SetAttribute("normalized", s.m_normalized);
			r->InsertEndChild(o);
		}
	};

	auto streamarray2xml = [&](Stream *s, int num, const char *streamName) {
		for (int i = 0; i < num; ++i)
			stream2xml(s[i], streamName);
	};


#define STREAM2XML(x) stream2xml(vd.m_ ## x, #x);
#define STREAMARRAY2XML(x) streamarray2xml(vd.m_ ## x, static_cast<int>(COUNT_OF(vd.m_ ## x)), #x);

	STREAM2XML(vertices);
	STREAM2XML(normals);
	STREAM2XML(tangents);
	STREAM2XML(bitangents);
	STREAMARRAY2XML(colors);
	STREAMARRAY2XML(textureCoords);
	STREAMARRAY2XML(boneWeights);
	STREAMARRAY2XML(boneIDs);
	STREAM2XML(indices);

	out->InsertFirstChild(r);
	rb->SetTimestamp(); 
}

void ResMesh::SetVertexDescription(VertexDescription * pvd, U32 _meshHash, ResourceBase * _meshSrc, U32 _meshIdx)
{ 
	// set new mesh data
	vd = *pvd; 
	meshHash = _meshHash; 
	meshSrc = _meshSrc; 
	meshIdx = _meshIdx; 

	// mark, that we don't need to load this mesh again.
	isMeshLoaded = true;  

	// save settings to XML
	_SaveXML(); 
}

VertexDescription * ResMesh::GetVertexDescription(bool loadASAP)
{
	if (!isMeshLoaded && loadASAP)
	{
		CEngine::SendMsg(IMPORTMODULE_LOADMESH, IMPORT_MODULE, 0, rb);
		isMeshLoaded = true;
	}

	return &vd;
}

MProperties *ResMesh::GetMeshProperties(bool loadASAP)
{
	if (!isMeshLoaded && loadASAP)
	{
		CEngine::SendMsg(IMPORTMODULE_LOADMESH, IMPORT_MODULE, 0, rb);
		isMeshLoaded = true;
	}
	return &meshProperties;
}

// ==========================================================

void ResImportModel::IdentifyResourceType(ResourceBase * res)
{
	if (res->Type != RESID_UNKNOWN)
		return;

	PIDETIFY_RESOURCE ir = {
		res->Path.c_str(),
		&res->Type,
		res};
	IEngine::SendMsg(IDENTIFY_RESOURCE, IMPORT_MODULE, 0, &ir);
}

}; // CORE namespace
}; // BAMS namespace