#include "stdafx.h"

NAMESPACE_CORE_BEGIN

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;
using BAMS::RENDERINENGINE::VertexDescription;
using BAMS::RENDERINENGINE::Stream;

// ----------------------------------------------------

void ResMesh::_LoadXML()
{
	auto &rm = globalResourceManager;
	XMLDocument xml;
	xml.Parse(rb->XML.c_str(), rb->XML.size());

	auto r = xml.FirstChildElement("Mesh");
	WSTR srcFileName;
	srcFileName.UTF8(r->Attribute("Src"));
	rm->AbsolutePath(srcFileName);
	meshSrc = rm->GetByFilename(srcFileName, RESID_IMPORTMODEL);
	meshIdx = r->IntAttribute("MeshIdx", 0);

	// vd
	r = r->FirstChildElement("VertexDescription");
	auto xml2stream = [&](Stream &o, const char *streamName) {
		auto s = r->FirstChildElement(streamName);
		if (s)
		{


			o.m_type = s->IntAttribute("type", 0);
			o.m_stride = s->IntAttribute("stride", 0);
			o.m_normalized = s->BoolAttribute("normalized", false);
		}
	};

	vd.m_numVertices = r->IntAttribute("numVertices", 0);
	vd.m_numIndices = r->IntAttribute("numIndices", 0);
	

#define XML2STREAM(x) xml2stream(vd.m_ ## x, #x);
	XML2STREAM(vertices);
	XML2STREAM(normals);
	XML2STREAM(tangents);
	XML2STREAM(bitangents);
	XML2STREAM(colors[0]);
	XML2STREAM(colors[1]);
	XML2STREAM(colors[2]);
	XML2STREAM(colors[3]);
	XML2STREAM(textureCoords[0]);
	XML2STREAM(textureCoords[1]);
	XML2STREAM(textureCoords[2]);
	XML2STREAM(textureCoords[3]);
	XML2STREAM(boneWeights[0]);
	XML2STREAM(boneWeights[1]);
	XML2STREAM(boneIDs[0]);
	XML2STREAM(boneIDs[1]);

}

void ResMesh::_SaveXML() { rb->XML = BuildXML(vd, meshSrc, meshIdx); }

STR ResMesh::BuildXML(BAMS::RENDERINENGINE::VertexDescription &_vd, ResourceBase * _meshSrc, U32 _meshIdx)
{
	using tinyxml2::XMLPrinter;
	using tinyxml2::XMLDocument;

	auto &rm = globalResourceManager;
	XMLDocument out;
	auto *root = out.NewElement("Mesh");
	STR cvt;
	WSTR msf = _meshSrc->Path;
	rm->RelativePath(msf);
	cvt.UTF8(msf);
	root->SetAttribute("Src", cvt.c_str());
	root->SetAttribute("MeshIdx", _meshIdx);
	auto r = out.NewElement("VertexDescription");

	r->SetAttribute("numVertices", _vd.m_numVertices);
	r->SetAttribute("numIndices", _vd.m_numIndices);
	auto stream2xml = [&](Stream &s, const char *streamName) {
		if (s.isUsed())
		{
			auto o = out.NewElement(streamName);
			o->SetAttribute("type", s.m_type);
			o->SetAttribute("stride", s.m_stride);
			o->SetAttribute("normalized", s.m_normalized);
			r->InsertEndChild(o);
		}
	};

#define STREAM2XML(x) stream2xml(_vd.m_ ## x, #x);
	STREAM2XML(vertices);
	STREAM2XML(normals);
	STREAM2XML(tangents);
	STREAM2XML(bitangents);
	STREAM2XML(colors[0]);
	STREAM2XML(colors[1]);
	STREAM2XML(colors[2]);
	STREAM2XML(colors[3]);
	STREAM2XML(textureCoords[0]);
	STREAM2XML(textureCoords[1]);
	STREAM2XML(textureCoords[2]);
	STREAM2XML(textureCoords[3]);
	STREAM2XML(boneWeights[0]);
	STREAM2XML(boneWeights[1]);
	STREAM2XML(boneIDs[0]);
	STREAM2XML(boneIDs[1]);

	root->InsertEndChild(r);
	out.InsertFirstChild(root);
	XMLPrinter prn;
	out.Print(&prn);

	STR ret(prn.CStr());
	return std::move(ret);
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

NAMESPACE_CORE_END
