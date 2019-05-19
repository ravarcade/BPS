#include "stdafx.h"
#include "png.h"

using namespace BAMS;

void pngWriteDataToMemory(png_structp pngPtr, png_bytep data, png_size_t length)
{
	auto dst = reinterpret_cast<TempMemory *>(png_get_io_ptr(pngPtr));
	dst->pushData(data, length);
}

void pngWriteDataToMemoryFlush(png_structp png_ptr)
{
}

bool EncodePNG(TempMemory *dst, Image *src)
{
	if (src->format != PF_R8G8B8A8_UNORM)
		return false;

	// create png write stucture
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;

	// create png info struct
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return false;
	}

	// the code in this if statement gets called if libpng encounters an error
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}
	//	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	dst->clear();

	png_set_write_fn(png_ptr, (png_voidp)(dst), pngWriteDataToMemory, pngWriteDataToMemoryFlush);
	png_set_IHDR(png_ptr, info_ptr, src->width, src->height,
		8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	// Set title
	char *title = "Created in ReBAM";
	if (title != NULL) {
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_NONE;
		title_text.key = "Title";
		title_text.text = title;
		png_set_text(png_ptr, info_ptr, &title_text, 1);
	}

	png_write_info(png_ptr, info_ptr);

	BYTE *row = src->rawImage;
	size_t rowLength = src->pitch;
	for (unsigned int y = 0; y < src->height; ++y) {
		png_write_row(png_ptr, row);
		row += rowLength;
	}
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return true;
}

void pngReadDataFromMemory(png_structp pngPtr, png_bytep data, png_size_t length) {
	png_voidp a = png_get_io_ptr(pngPtr);
	BYTE **pBuffer = (BYTE **)a;

	memcpy_s(data, length, *pBuffer, length);
	*pBuffer += length;
}

bool DecodePNG(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp)
{
	if (png_sig_cmp(src, 0, 8))
		return false;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;

	// create png info struct
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return false;
	}

	// create png info struct
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		return false;
	}

	// the code in this if statement gets called if libpng encounters an error
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return false;
	}

	// init png reading
	BYTE *srcData = src;
	srcData += 8;
	png_set_read_fn(png_ptr, (png_voidp)(&srcData), pngReadDataFromMemory);

	// let libpng know you already read the first 8 bytes
	png_set_sig_bytes(png_ptr, 8);

	// read all the info up to the image data
	png_read_info(png_ptr, info_ptr);

	// variables to pass to get info
	int bit_depth, color_type;
	png_uint_32 temp_width, temp_height;

	// get info about png
	png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type, NULL, NULL, NULL);

	dst->CreateEmpty(temp_width, temp_height, PF_R8G8B8A8_UNORM);
	BYTE *dstBin = dst->rawImage;
	unsigned int pitch = dst->pitch;
	unsigned int width = dst->width;
	unsigned int height = dst->height;
	/*
			// time to reserve new memory for raw data or use own buffer
			static AlignedMemory rawDataBuffer(8192);

			size_t rawDataLen  = tex.textureWidth * tex.textureHeight * 4;	// RGBA
			size_t rawDataRowSize = tex.textureWidth * 4;					// RGBA
			AlignedMemory *dst = tex.m_storeBinData ? &tex.rawBinData : &rawDataBuffer;
			dst->Alloc(rawDataLen);
	*/

	//Number of channels
	png_uint_32 channels = png_get_channels(png_ptr, info_ptr);

	switch (color_type) {
	case PNG_COLOR_TYPE_PALETTE:
		png_set_palette_to_rgb(png_ptr);
		//Don't forget to update the channel info (thanks Tom!)
		//It's used later to know how big a buffer we need for the image
		channels = 3;
		break;

	case PNG_COLOR_TYPE_GRAY:
		if (bit_depth < 8)
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		//And the bitdepth info
		bit_depth = 8;
		break;
	}

	/*if the image has a transperancy set.. convert it to a full Alpha channel..*/
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	//We don't support 16 bit precision.. so if the image Has 16 bits per channel
	//precision... round it down to 8.
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	// Update the png info struct.
	png_read_update_info(png_ptr, info_ptr);

	//Color type. (RGB, RGBA, Luminance, luminance alpha... palette... etc)
	color_type = png_get_color_type(png_ptr, info_ptr);

	// Row size in bytes.
	size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	png_bytep outBuf = dstBin;
	png_bytep rowDataEnd = outBuf + pitch * temp_height;
	png_bytep rowData = rowDataEnd - rowbytes;

	switch (color_type) {
	case PNG_COLOR_TYPE_RGB:
		for (unsigned int i = 0; i < temp_height; ++i) {
			png_read_row(png_ptr, (png_bytep)rowData, NULL);
			for (BYTE *s = rowData, *d = outBuf; s < rowDataEnd;) {
				*d++ = *s++;
				*d++ = *s++;
				*d++ = *s++;
				*d++ = 255; // alpha
			}

			outBuf += pitch;
		}
		break;

	case PNG_COLOR_TYPE_RGBA:
		for (unsigned int i = 0; i < temp_height; ++i) {
			png_read_row(png_ptr, (png_bytep)outBuf, NULL);
			outBuf += pitch;
		}
		break;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	return true;
}

