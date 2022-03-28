#include "stdafx.h"
#include "image.h"

inline unsigned char Clamp(short Val)
{
	if(Val > 255) Val = 255;
	else if(Val < 0) Val = 0;
	return (unsigned char)Val;
}

CImage::CImage()
{
	m_nXSize = 0;
	m_nYSize = 0;
	m_pImage = NULL;
	m_pImagePalettized = NULL;
}

CImage::CImage(const CImage &rImage)
{
	m_nXSize = 0;
	m_nYSize = 0;
	m_pImage = FreeImage_Clone(rImage.GetImage());
	m_pImagePalettized = FreeImage_Clone(rImage.GetImagePalettized());

	if(m_pImage) {
		m_nXSize = FreeImage_GetWidth(m_pImage);
		m_nYSize = FreeImage_GetHeight(m_pImage);
	}
}

CImage::CImage(FIBITMAP *pImage,FIBITMAP *pImagePalettized)
{
	m_nXSize = 0;
	m_nYSize = 0;
	m_pImage = FreeImage_Clone(pImage);
	m_pImagePalettized = FreeImage_Clone(pImagePalettized);

	if(m_pImage) {
		m_nXSize = FreeImage_GetWidth(m_pImage);
		m_nYSize = FreeImage_GetHeight(m_pImage);
	}
}

CImage::~CImage()
{
	FreeImage_Unload(m_pImage);
	FreeImage_Unload(m_pImagePalettized);

	m_pImage = NULL;
	m_pImagePalettized = NULL;
	m_nXSize = 0;
	m_nYSize = 0;
}

int CImage::Load(const char *pszFilename)
{
	unsigned bpp;
	FIBITMAP *img = NULL;
	FREE_IMAGE_FORMAT fif;
	int nWidth,nHeight;

	fif = FreeImage_GetFileType(pszFilename,0);
	if(fif==FIF_UNKNOWN) fif = FreeImage_GetFIFFromFilename(pszFilename);

	if(fif!=FIF_UNKNOWN && FreeImage_FIFSupportsReading(fif))
		img = FreeImage_Load(fif, pszFilename, 0);

	if(img) {
		bpp = FreeImage_GetBPP(img);
		if(bpp<=8)
			m_pImagePalettized = FreeImage_ConvertTo8Bits(img);
		if(bpp!=32) {
			FIBITMAP *tmp = FreeImage_ConvertTo32Bits(img);

			FreeImage_Unload(img); 
			if(tmp==NULL) return 0;
			
			img = tmp;
		}

		nWidth = m_nXSize = FreeImage_GetWidth(img);
		nHeight = m_nYSize = FreeImage_GetHeight(img);

		if(m_nXSize&3) nWidth = ((m_nXSize+3)&~3);
		if(m_nYSize&3) nHeight = ((m_nYSize+3)&~3);

		if(m_nXSize!=nWidth || m_nYSize!=nHeight) {
			FIBITMAP *tmp = FreeImage_Rescale(img,nWidth,nHeight,FILTER_BILINEAR);

			FreeImage_Unload(img);
			if(tmp==NULL) return 0;

			img = tmp;
		}

		m_pImage = img;
		m_nXSize = nWidth;
		m_nYSize = nHeight;

		FreeImage_FlipVertical(m_pImage);
		FreeImage_FlipVertical(m_pImagePalettized);

		return 1;
	}
	return 0;
}

CImage* CImage::BoxFilter(int nDestWidth,int nDestHeight)
{
	CImage *pImage = NULL;

	if(nDestWidth!=m_nXSize || nDestHeight!=m_nYSize) {
		FIBITMAP *fib = FreeImage_Rescale(m_pImage,nDestWidth,nDestHeight,FILTER_BILINEAR);
		if(fib!=NULL) {
			pImage = new CImage(fib);
			FreeImage_Unload(fib);
		}
	} else
		pImage = new CImage(m_pImage,m_pImagePalettized);

	return pImage;
}

void CImage::Resize(int nDestWidth,int nDestHeight)
{
	FIBITMAP *fib = NULL;

	fib = FreeImage_Rescale(m_pImage,nDestWidth,nDestHeight,FILTER_BILINEAR);
	if(fib!=NULL) {
		FreeImage_Unload(m_pImage);
		FreeImage_Unload(m_pImagePalettized);

		m_pImage = fib;
		m_pImagePalettized = NULL;
		m_nXSize = FreeImage_GetWidth(m_pImage);
		m_nYSize = FreeImage_GetHeight(m_pImage);
	}
}

BYTE* CImage::GetPixel()
{
	if(m_pImage!=NULL)
		return FreeImage_GetBits(m_pImage);
	else
		return NULL;
}

BYTE* CImage::GetPixelPalettized()
{
	if(m_pImagePalettized!=NULL)
		return FreeImage_GetBits(m_pImagePalettized);
	else
		return NULL;
}

BYTE* CImage::GetPixelRGBA()
{
	int x,y;
	BYTE *bits,*target;

	if(m_pImage!=NULL) {
		bits = FreeImage_GetBits(m_pImage);
		target = new BYTE[(m_nXSize*m_nYSize)<<2];
		for(y=0;y<m_nYSize;y++) {
			for(x=0;x<m_nXSize;x++) {
				target[(y*(m_nXSize<<2))+(x<<2)+0] = bits[(y*(m_nXSize<<2))+(x<<2)+FI_RGBA_RED];
				target[(y*(m_nXSize<<2))+(x<<2)+1] = bits[(y*(m_nXSize<<2))+(x<<2)+FI_RGBA_GREEN];
				target[(y*(m_nXSize<<2))+(x<<2)+2] = bits[(y*(m_nXSize<<2))+(x<<2)+FI_RGBA_BLUE];
				target[(y*(m_nXSize<<2))+(x<<2)+3] = bits[(y*(m_nXSize<<2))+(x<<2)+FI_RGBA_ALPHA];
			}
		}
		return target;
	}
	return NULL;
}

typedef struct
{
	short	r, g, b, a;
} ShortCol;

void CImage::DiffuseError(int aBits,int rBits,int gBits,int bBits)
{
	long x, y;
	ShortCol *pTempPix, *pDest;
	short r, g, b, a, rErr, gErr, bErr, aErr;
	short rMask, gMask, bMask, aMask;
	BYTE *bits;

	rMask = (1 << (12-rBits)) - 1;
	gMask = (1 << (12-gBits)) - 1;
	bMask = (1 << (12-bBits)) - 1;
	aMask = (1 << (12-aBits)) - 1;

	bits = GetPixel();
	pTempPix = new ShortCol[m_nXSize*m_nYSize];
	pDest = pTempPix;

	for(y=0;y<m_nYSize;y++) {
		for(x=0;x<m_nXSize;x++) {
			r = bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_RED];
			g = bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_GREEN];
			b = bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_BLUE];
			a = bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_ALPHA];
			pDest[x].r = r << 4;
			pDest[x].g = g << 4;
			pDest[x].b = b << 4;
			pDest[x].a = a << 4;
		}
		pDest += m_nXSize;
	}

	pDest = pTempPix;
	for(y=0;y<m_nYSize-1;y++) {
		for(x=0;x<m_nXSize-1;x++) {
			r = pDest[x].r;
			g = pDest[x].g;
			b = pDest[x].b;
			a = pDest[x].a;

			rErr = r - ((r + rMask/2) & ~rMask);
			gErr = g - ((g + gMask/2) & ~gMask);
			bErr = b - ((b + bMask/2) & ~bMask);
			aErr = a - ((a + aMask/2) & ~aMask);

			r -= rErr;
			g -= gErr;
			b -= bErr;
			a -= aErr;

			pDest[x].r = r;
			pDest[x].g = g;
			pDest[x].b = b;
			pDest[x].a = a;

			pDest[x+1].r += rErr / 2;
			pDest[x+1].g += gErr / 2;
			pDest[x+1].b += bErr / 2;
			pDest[x+1].a += aErr / 2;

			pDest[x+m_nXSize].r += rErr / 4;
			pDest[x+m_nXSize].g += gErr / 4;
			pDest[x+m_nXSize].b += bErr / 4;
			pDest[x+m_nXSize].a += aErr / 4;

			if(x)
			{
				pDest[x+m_nXSize-1].r += rErr / 8;
				pDest[x+m_nXSize-1].g += gErr / 8;
				pDest[x+m_nXSize-1].b += bErr / 8;
				pDest[x+m_nXSize-1].a += aErr / 8;

				if(x > 2)
				{
					pDest[x+m_nXSize-3].r += rErr / 8;
					pDest[x+m_nXSize-3].g += gErr / 8;
					pDest[x+m_nXSize-3].b += bErr / 8;
					pDest[x+m_nXSize-3].a += aErr / 8;
				}
			}
		}

		r = pDest[x].r;
		g = pDest[x].g;
		b = pDest[x].b;
		a = pDest[x].a;

		rErr = r - ((r + rMask/2) & ~rMask);
		gErr = g - ((g + gMask/2) & ~gMask);
		bErr = b - ((b + bMask/2) & ~bMask);
		aErr = a - ((a + aMask/2) & ~aMask);

		r -= rErr;
		g -= gErr;
		b -= bErr;
		a -= aErr;

		pDest[x].r = r;
		pDest[x].g = g;
		pDest[x].b = b;
		pDest[x].a = a;

		pDest += m_nXSize;
	}

	for(x=0;x<m_nXSize;x++) {
		r = pDest[x].r;
		g = pDest[x].g;
		b = pDest[x].b;
		a = pDest[x].a;

		rErr = r - ((r + rMask/2) & ~rMask);
		gErr = g - ((g + gMask/2) & ~gMask);
		bErr = b - ((b + bMask/2) & ~bMask);
		aErr = a - ((a + aMask/2) & ~aMask);

		r -= rErr;
		g -= gErr;
		b -= bErr;
		a -= aErr;

		pDest[x].r = r;
		pDest[x].g = g;
		pDest[x].b = b;
		pDest[x].a = a;
	}

	rMask >>= 4;
	gMask >>= 4;
	bMask >>= 4;
	aMask >>= 4;
	pDest = pTempPix;
	for(y=0;y<m_nYSize;y++) {
		for(x=0;x<m_nXSize;x++) {
			bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_RED] = Clamp(pDest[x].r >> 4) & ~rMask;
			bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_GREEN] = Clamp(pDest[x].g >> 4) & ~gMask;
			bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_BLUE] = Clamp(pDest[x].b >> 4) & ~bMask;
			bits[((y*(m_nXSize<<2))+(x<<2))+FI_RGBA_ALPHA] = Clamp(pDest[x].a >> 4) & ~aMask;
		}
		pDest += m_nXSize;
	}
	delete [] pTempPix;
}

bool CImage::IsTransparent()
{
	if(m_pImage!=NULL)
		return (FreeImage_IsTransparent(m_pImage)==TRUE);
	return 
		false;
}

int CImage::GetPalette(RGBQUAD **ppRcvColors,int nReqColors)
{
	int i,nNumCols,nNumTrns;
	RGBQUAD *pPal;
	BYTE *pTrns;

	if(m_pImage==NULL) return 0;

	// we only support 4/8 bit color indexed images
	// convert down to 24 bits and quantize to desired color count
	if(m_pImagePalettized==NULL) {
		m_pImagePalettized = FreeImage_ColorQuantizeEx(m_pImage,FIQ_LFPQUANT,nReqColors);
		if(m_pImagePalettized==NULL)
			m_pImagePalettized = FreeImage_ColorQuantizeEx(m_pImage,FIQ_WUQUANT,nReqColors);
	}

	if(m_pImagePalettized!=NULL) {
		pPal = FreeImage_GetPalette(m_pImagePalettized);
		nNumCols = FreeImage_GetColorsUsed(m_pImagePalettized);
		pTrns = FreeImage_GetTransparencyTable(m_pImagePalettized);
		nNumTrns = FreeImage_GetTransparencyCount(m_pImagePalettized);
		if(ppRcvColors!=NULL) {
			*ppRcvColors = new RGBQUAD[nNumCols];
			for(i=0;i<nNumCols;i++) {
				(*ppRcvColors)[i].rgbRed = pPal[i].rgbRed;
				(*ppRcvColors)[i].rgbGreen = pPal[i].rgbGreen;
				(*ppRcvColors)[i].rgbBlue = pPal[i].rgbBlue;
				(*ppRcvColors)[i].rgbReserved = (i<nNumTrns)?pTrns[i]:0xff;
			}
		}
		return nNumCols;
	}
	return 0;
}


