#include "stdafx.h"
#include "jpeglib.h"
#include <ktx.h>
#include <ktxvulkan.h>

using namespace BAMS;

bool DecodeKTX(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp)
{
//	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	ktxResult result;
	ktxTexture* ktxTexture;

	result = ktxTexture_CreateFromMemory(src, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
	
	if (result != KTX_SUCCESS) 
	{		
		return false;
	}

	if (ktxTexture->isCompressed) {
		TRACE("Texture compression not supported... yet\n");
	//	return false;
	}

	// single image only (right now)
	ktx_size_t offset;
	KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, 0, 0, 0, &offset);
	TRACE("format="<<ktxTexture->glFormat);
	
	//	dst->CreateEmpty(ktxTexture->baseWidth, ktxTexture->baseHeight, 4 == 4 ? PF_R8G8B8A8_UNORM : cinfo.num_components == 3 ? PF_R8G8B8_UNORM : PF_UNKNOWN, cinfo.image_width *  cinfo.num_components);
	uint32_t mipLevel = 0;
	uint32_t format = PF_UNKNOWN;
	switch (ktxTexture->glFormat) 
	{
	case 0x1908: // GL_RGBA:
		format = PF_R8G8B8A8_UNORM; break;
	case 0x1907: // GL_RGB:
		format = PF_R8G8B8_UNORM; break;
	case 0: // no gl forma
		switch (ktxTexture->glInternalformat) {
		case 0x83F3: // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  
			format = PF_BC2_UNORM; break;
		default:
			TRACE("unknown texture format");

		}
		break;
	}

	if (format == PF_UNKNOWN) 
	{
		assert(format != PF_UNKNOWN);
		ktxTexture_Destroy(ktxTexture);
		return false;
	}

	if (!ktxTexture->isArray && !ktxTexture->isCubemap && ktxTexture->numLevels == 1) 
	{
		dst->CreateEmpty(ktxTexture->baseWidth >> mipLevel, ktxTexture->baseHeight >> mipLevel, format);
		size_t s = dst->pitch * dst->height;
		assert(s == ktxTexture->dataSize);
		memcpy_s(dst->rawImage, s, ktxTexture->pData, s);
	}
	else {
		std::vector<size_t> offsets;
		offsets.reserve(ktxTexture->numLevels * ktxTexture->numLayers * ktxTexture->numFaces);
		for (U32 mip = 0; mip < ktxTexture->numLevels; ++mip) {
			for (U32 layer = 0; layer < ktxTexture->numLayers; ++layer) {
				for (U32 face = 0; face < ktxTexture->numFaces; ++face) 
				{
					ktxTexture_GetImageOffset(ktxTexture, mip, layer, face, &offset);
					offsets.push_back(offset);
				}
			}
		}
		dst->CreateFull(
			ktxTexture->baseWidth,
			ktxTexture->baseHeight,
			format,
			ktxTexture->pData,
			ktxTexture->dataSize,
			offsets.data(),
			ktxTexture->numLevels,
			ktxTexture->numLayers,
			ktxTexture->numFaces);
		dst->isCube = ktxTexture->isCubemap;
	}

	ktxTexture_Destroy(ktxTexture);
	return true;
}