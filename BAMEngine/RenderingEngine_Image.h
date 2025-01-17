#ifndef _RENDERINGENGINE_IMAGE_H_
#define _RENDERINGENGINE_IMAGE_H_

enum {
	// pixel format
	PF_UNKNOWN = 0,
	PF_R32G32B32A32_SFLOAT = 1,
	PF_R8G8B8A8_UNORM = 2,
	PF_R8G8B8_UNORM = 3,
	PF_BC2_UNORM = 4,
};

class BAMS_EXPORT Image {
public:
	Image();
	Image(U8 *rawImage, U32 width, U32 height, U32 format, U32 pitch = 0);
	~Image();

	void CreateEmpty(U32 width, U32 height, U32 format, U32 pitch = 0);
	void CreateFull(U32 width, U32 height, U32 format, void *data, SIZE_T dataLen, SIZE_T *offsets = nullptr, U32 mipmaps = 1, U32 layers = 1, U32 faces = 1);
	SIZE_T GetImageOffset(U32 mip, U32 layer, U32 face);
	void Release();

	U8 *rawImage;
	U32 width, height;
	U32 format;
	U32 pitch;

	// cubemap & array textures support
	U32 layers;
	U32 faces;
	U32 mipmaps;
	bool isCube;
	bool isArray;

	bool IsLoaded() { return rawImage != nullptr; }

	U32 GetPixelSize() { return GetPixelSize(format); }

	static U32 GetPixelSize(U32 format);
	static U32 GetLineAlign(U32 format);

private:
	IMemoryAllocator *alloc;
	void *buffer;
	SIZE_T *offsets;
};

#endif // _RENDERINGENGINE_IMAGE_H_