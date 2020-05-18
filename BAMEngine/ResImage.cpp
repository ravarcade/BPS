#include "stdafx.h"

namespace BAMS {
namespace CORE {

ResImage::ResImage(ResBase *res)
	: rb(res)
{
	if (res->XML->FirstChild())
		_LoadXML(); // XML is readed from manifest, so no need to load any file from disk
}

ResImage::~ResImage()
{
	img.Release();
	ReleaseSrc();
}

void ResImage::_LoadXML()
{
}

void ResImage::_SaveXML()
{
}

void ResImage::Update(ResBase * res) {

	if (rb->isLoaded())
	{
		// Image is in memory, but in file format (BMP, PNG, JPG or TGA).
		// It must be decoded.... This is job for ImportModule
		CEngine::SendMsg(IMPORTMODULE_UPDATEIMAGE, IMPORT_MODULE, 0, rb); 
		_SaveXML();
		PUPDATE_TEXTURE msg = { static_cast<uint32_t>(-1), reinterpret_cast<IResource *>(rb) };
		CEngine::PostMsg(UPDATE_TEXTURE, RENDERING_ENGINE, 0, &msg, sizeof(msg));
	}
}

void ResImage::Release(ResBase * res)
{
	img.Release();

	auto pData = GetSrcData();
	if (pData)
	{
		rb->ReleaseMemory();
	}
}

/// <summary>
/// Gets the image.
/// </summary>
/// <param name="loadASAP">if set to <c>true</c> [load asap].</param>
/// <returns></returns>
Image * ResImage::GetImage(bool loadASAP)
{
	if ((!rb->isLoaded() || rb->isModified()) && loadASAP)
	{	
		// We force resource to be loaded now.
		// Image will be decoded right after it is loaded
		auto &rm = globalResourceManager;
		rm->LoadSync(rb);
	}

	return &img;
}

U8 * ResImage::GetSrcData() { return rb->GetData<U8>(); }

SIZE_T ResImage::GetSrcSize() { return rb->GetSize(); }

void ResImage::ReleaseSrc() {
	auto pData = GetSrcData();
	if (pData)
	{
		rb->ReleaseMemory();
	}
}

}; // CORE namespace
}; // BAMS namespace