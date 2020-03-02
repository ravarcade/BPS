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
    { 4, 4}, // PF_R8G8B8A8_UNORM
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
	offsets(nullptr),
	width(0), height(0), format(PF_UNKNOWN), pitch(0),
	layers(0), faces(0), mipmaps(0),
	isCube(false), isArray(false)
{}

Image::Image(U8 *rawImage, U32 width, U32 height, U32 format, U32 pitch)
{
	if (!pitch)
	{
		auto la = GetLineAlign(format);
		pitch = (width * GetPixelSize(format) + la - 1) & (0 - la);
	}
	this->rawImage = rawImage;
	this->width = width;
	this->height = height;
	this->format = format;
	this->pitch = pitch;
	this->buffer = nullptr;
	this->alloc = nullptr;
	this->layers = 1;
	this->faces = 1;
	this->mipmaps = 1;	
	this->offsets = nullptr;
	this->isCube = false;
	this->isArray = false;
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
	this->pitch = pitch; // pitch is used to load 2D images from PNG or similiary formats
	this->isCube = false;
	this->isArray = false;
	CreateFull(width, height, format, nullptr, pitch * height);
}

void Image::CreateFull(U32 width, U32 height, U32 format, void * data, SIZE_T dataLen, SIZE_T * offsets, U32 mipmaps, U32 layers, U32 faces)
{
	this->width = width;
	this->height = height;
	this->format = format;
	this->mipmaps = mipmaps;
	this->layers = layers;
	this->faces = faces;
	this->isCube = false;

	SIZE_T numOffsets = mipmaps * layers * faces;

	if (!alloc)
		alloc = Allocators::GetMemoryAllocator();

	Release();

	SIZE_T dataLenAlligned = (dataLen + 3) & -4L;
	SIZE_T offsetsMemSize = (numOffsets + 1) * sizeof(SIZE_T);
	this->buffer = alloc->allocate(dataLenAlligned + offsetsMemSize);
	this->rawImage = static_cast<U8*>(buffer);
	this->offsets = reinterpret_cast<SIZE_T*>(rawImage + dataLenAlligned);
	if (data)
		memcpy_s(this->rawImage, dataLenAlligned, data, dataLen);
	if (offsets)
	{
		memcpy_s(this->offsets, offsetsMemSize, offsets, offsetsMemSize);
	}
	else 
	{
		SIZE_T offset = 0;

		U32 w = width;
		U32 h = height;
		for (U32 mip = 0; mip < mipmaps; ++mip)
		{
			auto la = GetLineAlign(format);
			SIZE_T pitch = (w * GetPixelSize(format) + la - 1) & (0 - la);
			SIZE_T iSize = pitch * h;
			iSize = (iSize + 3) & -4L;
			
			for (U32 layer = 0; layer < layers; ++layer) 
			{
				for (U32 face = 0; face < faces; ++face)
				{
					this->offsets[face + layer * this->faces + mip * this->faces*this->layers] = offset;
					offset += iSize;
				}
			}
			w = w > 1 ? w >> 1 : 1;
			h = h > 1 ? h >> 1 : 1;
		}
	}
	this->offsets[numOffsets] = dataLen;
}

SIZE_T Image::GetImageOffset(U32 mip, U32 layer, U32 face)
{
	if (!(mip == this->mipmaps && layer == 0 && face == 0) &&
		 (mip >= mipmaps || layer >= layers || face >= faces))
	{
		TRACE("ERROR: Image offser request out of range!");
		return 0;
	}
	if (!this->offsets) {
		return mip == 0 ? 0 : this->height * this->pitch;
	}

	return this->offsets[face + layer * this->faces + mip * this->faces*this->layers];
}

void Image::Release()
{
	if (buffer && alloc) {
		alloc->deallocate(buffer);
		buffer = nullptr;
	}

	rawImage = nullptr;
}

}; // BAMS namespace