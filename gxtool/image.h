#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "stdafx.h"

class CImage
{
public:
	CImage();
	CImage(const CImage& rImage);
	CImage(FIBITMAP *pImage,FIBITMAP *pImagePalettized = NULL);
	virtual ~CImage();
	
	int Load(const char *pzsFilename);

	int GetXSize() const { return m_nXSize; }
	int GetYSize() const { return m_nYSize; }

	int GetPalette(RGBQUAD **ppRcvColors,int nReqColors);

	void Resize(int nDestWidth,int nDestHeight);
	void DiffuseError(int aBits,int rBits,int gBits,int bBits);

	bool IsTransparent();

	CImage* BoxFilter(int nDestWidth,int nDestHeight);

	BYTE* GetPixel();
	BYTE* GetPixelPalettized();
	BYTE* GetPixelRGBA();
	FIBITMAP* GetImage() const { return m_pImage; }
	FIBITMAP* GetImagePalettized() const { return m_pImagePalettized; }

private:
	int m_nXSize,m_nYSize;
	FIBITMAP *m_pImage;
	FIBITMAP *m_pImagePalettized;
};

#endif
