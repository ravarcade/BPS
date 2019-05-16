#include "stdafx.h"

namespace BAMS {
namespace CORE {

ResImage::ResImage(ResourceBase *res)
	: rb(res)
{
	if (res->XML.size())
		_LoadXML(); // XML is readed from manifest, so no need to load any file from disk
}

ResImage::~ResImage()
{
	img.Release();
}

void ResImage::_LoadXML()
{
}

void ResImage::_SaveXML()
{
}

void ResImage::Update(ResourceBase * res) {

	if (!rb->isLoaded())
	{
		CEngine::SendMsg(IMPORTMODULE_UPDATEIMAGE, IMPORT_MODULE, 0, rb);
	}
}

void ResImage::Release(ResourceBase * res)
{
	img.Release();
}

/// <summary>
/// Gets the image.
/// </summary>
/// <param name="loadASAP">if set to <c>true</c> [load asap].</param>
/// <returns></returns>
Image * ResImage::GetImage(bool loadASAP)
{
	if (!rb->isLoaded() && loadASAP)
	{
		CEngine::SendMsg(IMPORTMODULE_LOADIMAGE, IMPORT_MODULE, 0, rb);
	}

	return &img;
}

/// <summary>
/// Called by ImportModule when image is loaded and properties (like size, format) are updated and image manifest should be updated.
/// </summary>
void ResImage::Updated()
{
	_SaveXML();
}

}; // CORE namespace
}; // BAMS namespace