/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResImage : public ResImp<ResImage, RESID_IMAGE>
{
	ResBase *rb;
	Image img;

	void _LoadXML();
	void _SaveXML();

public:
	ResImage(ResBase *res);
	~ResImage();

	void Update(ResBase *res);
	void Release(ResBase *res);

	Image *GetImage(bool loadASAP = false);
	void Updated();

	U8 *GetSrcData();
	SIZE_T GetSrcSize();
	void ReleaseSrc();
};