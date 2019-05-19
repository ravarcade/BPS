#include "stdafx.h"

using namespace BAMS;

/* sections */
#define TGA_IMAGE_ID	0x01
#define TGA_IMAGE_INFO	0x02
#define TGA_IMAGE_DATA	0x04
#define TGA_COLOR_MAP	0x08
	/* RLE */
#define TGA_RLE_ENCODE  0x10

	/* color format */
#define TGA_RGB		0x20
#define TGA_BGR		0x40

	/* orientation */
#define TGA_BOTTOM	0x0
#define TGA_TOP		0x1
#define	TGA_LEFT	0x0
#define	TGA_RIGHT	0x1 

#pragma pack(1)

	/* TGA image header */
struct TGAHeader {
	U8	IDLength;			/* image id length */
	U8	ColorMapType;		/* color map type */
	U8	ImageType;			/* image type */
	U16	CMapStart;			/* index of first map entry */
	U16	CMapLength;			/* number of entries in color map */
	U8	CMapDepth;			/* bit-depth of a cmap entry */
	U16	XOffset;			/* x-coordinate */
	U16	YOffset;			/* y-coordinate */
	U16	Width;				/* width of image */
	U16	Height;				/* height of image */
	U8	PixelDepth;			/* pixel-depth of image */
	U8   ImageDescriptor;	/* alpha bits */
};

#pragma pack()

/**
* Decode TGA to RGBx texture
*/
bool DecodeTGA(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp)
{
	if (size < sizeof(TGAHeader))
		return NULL;

	TGAHeader *hdr = reinterpret_cast<TGAHeader *>(src);

	// check if it is valid TGA file
	// valid ColorMapType = 0 or 1
	if (hdr->ColorMapType > 1)
		return false;

	// Compressed color-mapped data, using Huffman, Delta, and runlength encoding is not supported
	if (hdr->ImageType == 0 || (hdr->ImageType >= 4 && hdr->ImageType <= 8) || hdr->ImageType >= 12)
		return false;

	// bad pixel depth
	if (hdr->PixelDepth != 8 &&
		hdr->PixelDepth != 15 &&
		hdr->PixelDepth != 16 &&
		hdr->PixelDepth != 24 &&
		hdr->PixelDepth != 32)
		return NULL;

	dst->CreateEmpty(hdr->Width, hdr->Height, PF_R8G8B8A8_UNORM);
	BYTE *dstBin = dst->rawImage;
	unsigned int pitch = dst->pitch;
	unsigned int width = dst->width;
	unsigned int height = dst->height;
	unsigned int pixelSize = dst->GetPixelSize();


	// real read of TGA
	BYTE *p = src + sizeof(TGAHeader) + hdr->IDLength; // ignore imaged ID
	BYTE *s = p;
	BYTE *end_of_src = src + size;
//	static AlignedMemory colorMapBuffer(8192); // color map is RGBA
	BYTE *colorMap = NULL;
	if (hdr->ImageType == 1 || hdr->ImageType == 9) {

		// load color map
		size_t mapSize = hdr->CMapLength * (hdr->CMapDepth + 1) / 8;
		BYTE *e = p + mapSize;
		s += mapSize;
		tmp.resize(hdr->CMapLength * 4);
		colorMap = tmp.data();

		switch (hdr->CMapDepth) {
		case 32: // RGBA
			while (p < e) {
				colorMap[2] = *p++; colorMap[1] = *p++; colorMap[0] = *p++;
				colorMap[3] = *p++;
				colorMap += 4;
			}
			break;

		case 24: //  RGB
			while (p < e) {
				colorMap[2] = *p++; colorMap[1] = *p++; colorMap[0] = *p++;
				colorMap[3] = 255;
				colorMap += 4;
			}
			break;

		case 15:
			while (p < e) {
				U16 c = *(U16 *)p++;

				colorMap[0] = (c & 0x7c00 >> 7) + (c & 0x7000 >> 12);
				colorMap[1] = (c & 0x03e0 >> 2) + (c & 0x03e0 >> 7);
				colorMap[2] = (c & 0x001f << 3) + (c & 0x001f >> 2);
				colorMap[3] = 255;
				colorMap += 4;
			}
			break;

		case 16:
			while (p < e) {
				U16 c = *(U16 *)p++;

				colorMap[0] = (c & 0x7c00 >> 7) + (c & 0x7000 >> 12);
				colorMap[1] = (c & 0x03e0 >> 2) + (c & 0x03e0 >> 7);
				colorMap[2] = (c & 0x001f << 3) + (c & 0x001f >> 2);
				colorMap[3] = (c & 0x8000) ? 255 : 0;
				colorMap += 4;
			}
			break;
		}
	}

	// pixel by pixel
	U8 head;
	U16 dir = 0, rep = 0;

	// first line
	BYTE *dstLine = dstBin;
	int dLineStep = pitch;
	int dPixelStep = pixelSize;

	if (!(hdr->ImageDescriptor & 0x20)) { // vertical flip
		dstLine += pitch * (height - 1); // 4*tex.textureWidth*(tex.height-1); // go to last line;
		dLineStep = -dLineStep;
	}

	if (hdr->ImageDescriptor & 0x10) { // horizontal flip
		dstLine += (width - 1)*pixelSize;
		dPixelStep = -dPixelStep;
	}

	for (int y = 0; y < hdr->Height; ++y) {
		BYTE *d = dstLine;
		dstLine += dLineStep;
		if (s >= end_of_src)
			return true;

		U16 pixelsInLine = hdr->Width;
		while (pixelsInLine) {
			if (hdr->ImageType >= 9 && rep == 0 && dir == 0) {
				head = *s++;
				if (head == EOF)
					return false; // fail
				if (head >= 128) {
					rep = head - 128;
					dir = 1;
				}
				else {
					dir = head + 1;
				}
			}
			else {
				dir = width; // for uncompresed image
			}

			// dir is always > 0
			// read new pixel
			U32 pixel;
			while (dir > 0) {
				// read pixel
				switch (hdr->PixelDepth) {
				case 8:
					pixel = *s++;
					break;

				case 15:
				case 16:
					pixel = *((U16 *)s);
					s += 2;
					break;

				case 24:
					pixel = 0xff000000 | s[0] << 16 | s[1] << 8 | s[2];
					s += 3;
					break;

				case 32:
					pixel = s[3] << 24 | s[0] << 16 | s[1] << 8 | s[2];
					s += 4;
					break;
				}

				// write pixel
				switch (hdr->ImageType) {
				case 1:
				case 9: // colormap
					pixel = ((U32 *)colorMap)[pixel - hdr->CMapStart];
					break;

				case 2:
				case 10: // 16bit to 32bit or 8->32
					break;

				case 3:
				case 11: // grey
					pixel = pixel & 0xff;
					pixel |= (pixel << 24) | (pixel << 16) | (pixel << 8);
					break;
				}
				*(U32 *)d = pixel;
				d += dPixelStep;
				--pixelsInLine;
				--dir;
			}

			// repeat
			int tmpRep = std::min(rep, pixelsInLine);
			rep -= tmpRep;
			pixelsInLine -= tmpRep;
			while (tmpRep > 0) {
				--tmpRep;
				*(U32 *)d = pixel;
				d += dPixelStep;
			}
		}
	}

	return true;
}

bool EncodeTGA(TempMemory *dst, Image *src)
{
	if ((src->format != PF_R8G8B8A8_UNORM && src->format != PF_R8G8B8_UNORM) || src->GetPixelSize() != sizeof(U32))
		return false;

	dst->clear();

	TGAHeader *hdr = (TGAHeader *)dst->reserveForNewData(sizeof(TGAHeader));
	hdr->IDLength = 0;
	hdr->ColorMapType = 0;	// no Color map
	hdr->ImageType = 10;	// RLE compresion
	hdr->CMapStart = 0;
	hdr->CMapLength = 0;
	hdr->CMapDepth = 32;
	hdr->XOffset = 0;
	hdr->YOffset = 0;
	hdr->Width = src->width;
	hdr->Height = src->height;
	hdr->PixelDepth = 32;
	hdr->ImageDescriptor = 0x08;

	for (int i = (int)src->width - 1; i >= 0; --i) {
		U32 *s = (U32 *)(src->rawImage + i * src->pitch);
		U32 *e = s + src->width;
		while (s < e) {
			// check if RLE will do some good
			if (s + 1 < e && s[0] == s[1]) { // rle
				U8 cmd = 0x81;
				++s;
				while (s + 1 < e && cmd < 0xff && s[0] == s[1]) {
					++s;
					++cmd;
				}

				dst->pushData(&cmd, 1);
				U8 buf[4];
				buf[2] = ((U8 *)s)[0];
				buf[1] = ((U8 *)s)[1];
				buf[0] = ((U8 *)s)[2];
				buf[3] = ((U8 *)s)[3];
				dst->pushData(buf, 4); // 1 pixel rgba
				++s;
			}
			else { // no rle
				U8 cmd = 0x00;
				U8 *src = (U8 *)s;
				++s;
				while (cmd < 0x7f && ((s + 1 < e && s[0] != s[1]) || s + 1 == e)) {
					++s;
					++cmd;
				}

				dst->pushData(&cmd, 1);
				U8 buf[4];
				U8 *ts = src;
				for (int i = (cmd & 0x7f) + 1; i > 0; --i) {
					buf[2] = ts[0];
					buf[1] = ts[1];
					buf[0] = ts[2];
					buf[3] = ts[3];
					ts += 4;
					dst->pushData(buf, 4); // 1 pixel rgba
				}
			}
		}
	}
	/*
			// The file footer. This part is totally optional.
			static const char footer[ 26 ] =
				"\0\0\0\0"  // no extension area
				"\0\0\0\0"  // no developer directory
				"TRUEVISION-XFILE"  // yep, this is a TGA file
				".";
			data->PushData((const U8 *)footer, sizeof(footer));
	*/
	return true;
}

