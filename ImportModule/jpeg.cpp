#include "stdafx.h"
#include "jpeglib.h"

using namespace BAMS;

struct ErrorManager
{
	jpeg_error_mgr defaultErrorManager;
	jmp_buf jumpBuffer;
};

void ErrorExit(j_common_ptr cinfo)
{
	ErrorManager* pErrorManager = (ErrorManager*)cinfo->err;
	(*cinfo->err->output_message)(cinfo); 
	longjmp(pErrorManager->jumpBuffer, 1);
}

void OutputMessage(j_common_ptr cinfo)
{
	// disable error messages
	/*char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
	fprintf(stderr, "%s\n", buffer);*/
}

/**
* Decode jpeg to RGBx texture
*/
bool DecodeJPG(Image *dst, U8 *src, SIZE_T size, TempMemory &tmp)
{
	struct jpeg_decompress_struct cinfo;
	struct ErrorManager jerr;

	cinfo.err = jpeg_std_error(&jerr.defaultErrorManager);
	jerr.defaultErrorManager.error_exit = ErrorExit;
	jerr.defaultErrorManager.output_message = OutputMessage;
	if (setjmp(jerr.jumpBuffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return false;
	}


	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, src, static_cast<unsigned long>(size));
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	if (cinfo.num_components != 3)
	{
		jpeg_destroy_decompress(&cinfo);
		return false;
	}
	dst->CreateEmpty(cinfo.image_width, cinfo.image_height, cinfo.num_components == 4 ? PF_R8G8B8A8_UNORM : cinfo.num_components == 3 ? PF_R8G8B8_UNORM : PF_UNKNOWN, cinfo.image_width *  cinfo.num_components);

	while (cinfo.output_scanline < cinfo.image_height)
	{
		uint8_t* p = dst->rawImage + cinfo.output_scanline*cinfo.image_width*cinfo.num_components;
		jpeg_read_scanlines(&cinfo, &p, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return true;
}
