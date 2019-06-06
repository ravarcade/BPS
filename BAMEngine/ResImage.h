/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResImage : public ResoureImpl<ResImage, RESID_IMAGE>
{
	ResourceBase *rb;
	Image img;

	void _LoadXML();
	void _SaveXML();

public:
	ResImage(ResourceBase *res);
	~ResImage();

	void Update(ResourceBase *res);
	void Release(ResourceBase *res);

	Image *GetImage(bool loadASAP = false);
	void Updated();

	U8 *GetSrcData();
	SIZE_T GetSrcSize();
	void ReleaseSrc();
};