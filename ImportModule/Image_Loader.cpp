#include "stdafx.h"

using namespace BAMS;

bool DecodeBMP(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp);
bool DecodeTGA(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp);
bool DecodePNG(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp);
bool DecodeJPG(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp);

Image_Loader::Image_Loader()
{
}

Image_Loader::~Image_Loader()
{
}

void Image_Loader::OnFinalize()
{
	tmp.release();
}

void Image_Loader::Load(CResImage & res)
{
	CResourceManager rm;
	rm.LoadSync(res);

	// check extensions:
	int extIdx = Tools::FindMatchingFileExtension(res.GetPath(), L"bmp;tga;png;jpg;jpeg");
	switch (extIdx)
	{
	case 0: // bmp
		TRACE("load BMP\n");
		DecodeBMP(res.GetImage(), res.GetSrcData(), res.GetSrcSize(), tmp);
		break;

	case 1: // tga
		TRACE("load TGA\n");
		DecodeTGA(res.GetImage(), res.GetSrcData(), res.GetSrcSize(), tmp);
		break;

	case 2: // png
		TRACE("load PNG\n");
		DecodePNG(res.GetImage(), res.GetSrcData(), res.GetSrcSize(), tmp);
		break;

	case 3: // jpg
	case 4: // jpeg
		TRACE("load JPG\n");
		DecodeJPG(res.GetImage(), res.GetSrcData(), res.GetSrcSize(), tmp);
		break;

	default:
		break;
	}
	// we don't need any more source data:
	res.ReleaseSrc();
}

/*
#define EXT4(a,b,c,d) a<<24|b<<16|c<<8|d
#define EXT3(a,b,c) a<<16|b<<8|c

bool CImage::Decode(AlignedMemory *src, const char *fileName)
{
	if (src->Size() == 0)
		return NULL;

	// pointer to decoded texture in ram
	AlignedMemory *data = NULL;

	// get extension
	UINT32 ext = 0xffffffff;
	if (fileName)
	{
		for (const char *t = fileName; *t; ++t)
		{
			ext = *t == '.' ? 0 : ((ext << 8) + tolower(*t));
		}
	}

	// try to decode with decoder selected on extension
	bool success = false;
	switch (ext)
	{
	case EXT3('p', 'n', 'g'): success = DecodePNG(this, src); break;
	case EXT3('t', 'g', 'a'): success = DecodeTGA(this, src); break;
	case EXT3('b', 'm', 'p'): success = DecodeBMP(this, src); break;
	case EXT4('j', 'p', 'e', 'g'):
	case EXT3('j', 'p', 'g'): success = DecodeJPG(this, src); break;
	}

	// if decodnig based on extension don't work, try everything
	if (!success)
		success = DecodeBMP(this, src);

	if (!success)
		success = DecodeTGA(this, src);

	if (!success)
		success = DecodePNG(this, src);

	if (!success)
		success = DecodeJPG(this, src);

	return success;
}
*/

/// <summary>
/// Converts sRGB value (U8: 0-255) to linear value (F32: 0.0 - 1.0).
/// </summary>
/// <param name="x">Color sRGB value (U8: 0-255).</param>
/// <returns>Color linear value (0.0 - 1.0)</returns>
F32 convertFromSRGB(U8 x)
{
	static F32 i2f[256] = { -1.0 };

	if (i2f[0] == -1.0) // sRGBA to linear RGBA
	{
		for (int i = 0; i < 256; ++i)
		{
			F32 cs = F32(i) / F32(255.0);
			i2f[i] = cs <= F32(0.04045) ? cs / F32(12.92) : F32(pow((cs + 0.055) / 1.055, 2.4));
		}
	}

	return i2f[x];
}

/// <summary>
/// Converts linear value (F32: 0.0 - 1.0) to sRGB value (U8 0-255).
/// </summary>
/// <param name="cl">Color linear value (F32)0.0 - 1.0)</param>
/// <returns>Color sRGB value (0-255)</returns>
U8 convertToSRGB(F32 cl)
{
	if (cl < 0.0)
		cl = 0;
	if (cl > 1.0)
		cl = 1.0;

	F32 cs = cl < F32(0.0031308) ? cl * F32(12.92) : F32(1.055) * pow(cl, F32(0.41666)) - F32(0.055);
	return BYTE(floor(F32(255.0)*cs + F32(0.5)));
}

/// <summary>
/// Converts color (4 floats) to sRGB (4 bytes = UINT32).
/// </summary>
/// <param name="color">Linear color (4 floats).</param>
/// <returns>sRGB color</returns>
U32 convertToSRGB(const F32 *color)
{
	F32 alpha = color[3];
	if (alpha < 0.0) alpha = 0.0;
	if (alpha > 1.0) alpha = 1.0;
	U8 a = U8(floor(F32(255.0)*alpha + F32(0.5)));
	return convertToSRGB(color[0]) | convertToSRGB(color[1]) << 8 | convertToSRGB(color[2]) << 16 | a << 24;
}

/// <summary>
/// Converts sRGB color (4 BYTES = U32) to linear color (4x F32).
/// </summary>
/// <param name="dst">Pointer to 4 floats array</param>
/// <param name="color">Color to convert</param>
void convertFromSRGB(F32 *dst, U32 color)
{
	dst[0] = convertFromSRGB(color & 0xff);
	dst[1] = convertFromSRGB((color >> 8) & 0xff);
	dst[2] = convertFromSRGB((color >> 16) & 0xff);
	dst[3] = float((color >> 24) & 0xff) / 255.0f;
}

