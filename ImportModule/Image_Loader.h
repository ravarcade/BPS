
class Image_Loader
{
	BAMS::TempMemory tmp;

public:
	Image_Loader();
	~Image_Loader();

	void OnFinalize();

	void Load(BAMS::CResImage & res);
};
//bool REBAM_API EncodePNG(AlignedMemory *dst, CImage *src);
//bool REBAM_API EncodeTGA(AlignedMemory *dst, CImage *src);

BYTE  convertToSRGB(float cl);
UINT32  convertToSRGB(const float *color);
float  convertFromSRGB(BYTE x);
void  convertFromSRGB(float *dst, UINT32 color);
