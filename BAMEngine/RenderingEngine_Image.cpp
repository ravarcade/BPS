#include "stdafx.h"

namespace BAMS {

// info about pixel format
struct {
	U32 size;
	U32 lineAllign;
}
pixelDescription[] = {
	{ 0, 0 }, // PF_UNKNOWN
	{ 16, 4 }, // PF_R32G32B32A32_SFLOAT
	{ 4, 4 }, // PF_R8G8B8A8_UNORM
	{ 4, 4 }, // PF_R8G8B8_UNORM
};

U32 Image::GetPixelSize(U32 format)
{
	assert(format < COUNT_OF(pixelDescription));

	return format < COUNT_OF(pixelDescription) ? pixelDescription[format].size : 0;
}

U32 Image::GetLineAlign(U32 format)
{
	assert(format < COUNT_OF(pixelDescription));

	return format < COUNT_OF(pixelDescription) ? pixelDescription[format].lineAllign : 1;
}

Image::Image() :
	alloc(nullptr),
	buffer(nullptr),
	rawImage(nullptr),
	width(0), height(0), format(PF_UNKNOWN), pitch(0)
{}

Image::Image(U8 *rawImage, U32 width, U32 height, U32 format, U32 pitch)
{
	this->rawImage = rawImage;
	CreateEmpty(width, height, format, pitch);
}

Image::~Image()
{
	Release();
}

void Image::CreateEmpty(U32 width, U32 height, U32 format, U32 pitch)
{
	if (!pitch)
	{
		auto la = GetLineAlign(format);
		pitch = (width * GetPixelSize(format) + la - 1) & (0 - la);
	}
	this->width = width;
	this->height = height;
	this->format = format;
	this->pitch = pitch;


	if (!alloc)
		alloc = Allocators::GetMemoryAllocator();

	buffer = alloc->allocate(pitch * height);
	rawImage = static_cast<U8*>(buffer);
}

void Image::Release()
{
	if (buffer && alloc) {
		alloc->deallocate(buffer);
		buffer = nullptr;
	}
}

}; // BAMS namespace