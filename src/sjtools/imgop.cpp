/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2015 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    imgop.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke image editing
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgop.h>
#include <wx/tokenzr.h>


/*******************************************************************************
 * SjImgOp Constructor
 ******************************************************************************/


SjImgOp::SjImgOp()
{
	m_id            = 0;
	m_flags         = 0;
	m_cropX         = 0;
	m_cropY         = 0;
	m_cropW         = 0;
	m_cropH         = 0;
	m_resizeW       = 0;
	m_resizeH       = 0;
	m_contrast      = 0;
	m_brightness    = 0;
}


/*******************************************************************************
 * SjImgOp - Copy / Compare image operations
 ******************************************************************************/


void SjImgOp::CopyFrom(const SjImgOp& o)
{
	m_id            = o.m_id;
	m_flags         = o.m_flags;
	m_cropX         = o.m_cropX;
	m_cropY         = o.m_cropY;
	m_cropW         = o.m_cropW;
	m_cropH         = o.m_cropH;
	m_resizeW       = o.m_resizeW;
	m_resizeH       = o.m_resizeH;
	m_brightness    = o.m_brightness;
	m_contrast      = o.m_contrast;
}


long SjImgOp::Cmp(const SjImgOp& o) const
{
#define CHECK_ELEM(a, b) if((a)!=(b)) { return (a)-(b); }

	CHECK_ELEM(m_flags, o.m_flags)

	if( m_flags & SJ_IMGOP_CROP )
	{
		CHECK_ELEM(m_cropX, o.m_cropX)
		CHECK_ELEM(m_cropY, o.m_cropY)
		CHECK_ELEM(m_cropW, o.m_cropW)
		CHECK_ELEM(m_cropH, o.m_cropH)
	}

	if( m_flags & SJ_IMGOP_RESIZE )
	{
		CHECK_ELEM(m_resizeW, o.m_resizeW)
		CHECK_ELEM(m_resizeH, o.m_resizeH)
	}

	if( m_flags & SJ_IMGOP_CONTRAST )
	{
		CHECK_ELEM(m_contrast,   o.m_contrast);
		CHECK_ELEM(m_brightness, o.m_brightness);
	}

	return 0; /* the two edit operations are equal */
}


/*******************************************************************************
 * SjImgOp - Perform Image Edit Operations
 ******************************************************************************/


bool SjImgOp::Do(wxImage& orgImage)
{
	int         resizeDone,
	            swapResize = 0;

#undef USE_WX_RESIZE // wx resize does not support antialiasing

	/* first, crop the image for smaller image size and as
	 * the crop coordinates are relative to the original size.
	 * moreover, this saves speed for the other operations.
	 */
	if( m_flags & SJ_IMGOP_CROP )
	{
		DoCrop(orgImage, m_cropX, m_cropY, m_cropW, m_cropH);
	}

	/* resize first if the new size is smaller than the old one
	 * to get a smaller image size (saves speed for the other operations)
	 */
	if( (m_flags & SJ_IMGOP_RESIZE) )
	{
		if( (m_resizeW*m_resizeH) < (orgImage.GetWidth()*orgImage.GetHeight()) )
		{
#ifdef USE_WX_RESIZE
			wxImage modImage = orgImage.Scale(m_resizeW, m_resizeH);
			if( modImage.Ok() )
			{
				orgImage = modImage;
			}
#else
			DoResize(orgImage, m_resizeW, m_resizeH, m_flags);
#endif
			resizeDone = 1;
		}
		else
		{
			resizeDone = 0;
		}
	}
	else
	{
		resizeDone = 1;
	}

	/* rotate the image
	 */
	DoRotate(orgImage, m_flags);
	if( HasFlag(SJ_IMGOP_ROTATE90) || HasFlag(SJ_IMGOP_ROTATE270) )
	{
		swapResize = 1;
	}

	/* mirror the image
	 */
	if( m_flags & SJ_IMGOP_FLIPHORZ )
	{
		wxImage modImage = orgImage.Mirror(TRUE);
		if( modImage.Ok() )
		{
			orgImage = modImage;
		}
	}

	if( m_flags & SJ_IMGOP_FLIPVERT )
	{
		wxImage modImage = orgImage.Mirror(FALSE);
		if( modImage.Ok() )
		{
			orgImage = modImage;
		}
	}

	/* other image filters
	 */
	if( m_flags & SJ_IMGOP_GRAYSCALE )
	{
		DoGrayscale(orgImage);
	}

	if( m_flags & SJ_IMGOP_NEGATIVE )
	{
		DoNegative(orgImage);
	}

	if( m_flags & SJ_IMGOP_CONTRAST )
	{
		DoContrast(orgImage, m_contrast, m_brightness);
	}

	/* resize last if the new size is larger than the old one
	 * to enlarge the file at last (saves speed for the other operations)
	 */
	if( !resizeDone )
	{
		long resizeW = m_resizeW, resizeH = m_resizeH;

		if( swapResize )
		{
			int temp = resizeW; resizeW = resizeH; resizeH = temp;
		}

#ifdef USE_WX_RESIZE
		wxImage modImage = orgImage.Scale(resizeW, resizeH);
		if( modImage.Ok() )
		{
			orgImage = modImage;
		}
#else
		DoResize(orgImage, m_resizeW, m_resizeH, m_flags);
#endif
	}

	/* add a border?
	 */
	if( m_flags & SJ_IMGOP_BORDER )
	{
		DoAddBorder(orgImage, 1);
	}

	/* done.
	 */
	return TRUE;
}


/*******************************************************************************
 * SjImgOp - Crop / Rotate Image
 ******************************************************************************/


bool SjImgOp::DoCrop(wxImage& image, long cropX, long cropY, long cropW, long cropH)
{
	long orgW = image.GetWidth(),
	     orgH = image.GetHeight();

	if( cropX < 0      ) cropX = 0;
	if( cropX > orgW-1 ) cropX = orgW-1;
	if( cropY < 0      ) cropY = 0;
	if( cropY > orgH-1 ) cropY = orgH-1;

	if( cropW > orgW-cropX ) cropW = orgW-cropX;
	if( cropH > orgH-cropY ) cropH = orgH-cropY;

	if( cropW > 0 && cropH > 0 )
	{
		wxImage modImage = image.GetSubImage(wxRect(cropX, cropY, cropW, cropH));
		if( modImage.Ok() )
		{
			image = modImage;
		}
	}

	return TRUE;
}


bool SjImgOp::DoRotate(wxImage& image, long flags)
{
	if( flags & SJ_IMGOP_ROTATE90 )
	{
		wxImage modImage = image.Rotate90(TRUE);
		if( modImage.Ok() )
		{
			image = modImage;
		}
	}
	else if( flags & SJ_IMGOP_ROTATE180 )
	{
		wxImage modImage = image.Mirror(TRUE);
		if( modImage.Ok() )
		{
			wxImage modImage2 = modImage.Mirror(FALSE);
			if( modImage2.Ok() )
			{
				image = modImage2;
			}
		}
	}
	else if( flags & SJ_IMGOP_ROTATE270 )
	{
		wxImage modImage = image.Rotate90(FALSE);
		if( modImage.Ok() )
		{
			image = modImage;
		}
	}

	return TRUE;
}


/*******************************************************************************
 * SjImgOp - Resize / Resample Image
 ******************************************************************************/


bool SjImgOp::DoResize(wxImage& image, long destWidth, long destHeight, long flags)
{
	/* prepare data */
	const unsigned char*    srcData = image.GetData();
	long                    srcWidth = image.GetWidth();
	long                    srcHeight = image.GetHeight();
	long                    srcScanlineBytes = srcWidth * 3;

	if( flags & SJ_IMGOP_KEEPASPECT && destHeight > 0 )
	{
		float destAspect = (float)destWidth / (float)destHeight;
		float srcAspect  = (float)srcWidth / (float)srcHeight;
		if( srcAspect > destAspect )
		{
			long newSrcWidth = (long) ((float)srcHeight * destAspect);
			if( newSrcWidth > 0 && newSrcWidth < srcWidth )
			{
				srcWidth = newSrcWidth;
			}
		}
		else
		{
			long newSrcHeight = (long) ((float)srcWidth / destAspect);
			if( newSrcHeight > 0 && newSrcHeight < srcHeight )
			{
				srcHeight = newSrcHeight;
			}
		}
	}

	long                    destScanlineBytes = destWidth*3;
	long                    destDataBytes = destScanlineBytes * destHeight;
	unsigned char*          destData;

	long                    x, y, xInt, yInt, xFrac, yFrac, maxWidth, maxHeight, srcX, srcY, endX, endY;
	unsigned long           avg0, avg1, avg2, pixelWeight;
	unsigned char*          currDestLinePtr;
	const unsigned char*    currSrcLinePtr;
	unsigned char           *destPtr;
	const unsigned char     *srcPtr0, *srcPtr1, *srcPtr2, *srcPtr3;
	unsigned char           value0, value1;

	if( !srcData || srcWidth<=0 || srcHeight<=0 || destWidth<=0 || destHeight<=0 )
	{
		return FALSE; /* error */
	}

	if( srcWidth == destWidth && srcHeight == destHeight )
	{
		return TRUE; /* nothing to resize */
	}

	if( !(destData=(unsigned char*)malloc(destDataBytes)) )
	{
		return FALSE; /* error */
	}

	if( (flags & SJ_IMGOP_SMOOTH)
	        && destWidth  < srcWidth
	        && destHeight < srcHeight )
	{
		/*
		 * Resample Shrinking using Antialiasing.
		 *
		 * Shrink bitmaps filtered. Therefore, a box filter is applied for every pixel
		 * in the output bitmap, which averages the input pixel in its projected input
		 * bitmap rectangle. As this filter kernel has the same size for every output
		 * pixel (its size is rounded up), cases might appear where nonexisting input
		 * pixel are needed. They are assumed to be of colour black then.
		 */

		/* calculate integer quotient of source and destination widths and heights,
		 * rounded up. This is the size of the box filter kernel.
		 */
		maxWidth = srcWidth  / destWidth  + ((srcWidth  % destWidth)  ? 1 : 0);
		maxHeight= srcHeight / destHeight + ((srcHeight % destHeight) ? 1 : 0);

		/* buffer box filter pixel weight (value every pixel is divided by) */
		pixelWeight = maxWidth * maxHeight;

		for( y = 0, srcY = 0; y < destHeight; y++ )
		{
			/* init current output line pointer */
			currDestLinePtr = &destData[ destScanlineBytes * y ];

			for( x = 0, srcX = 0; x < destWidth; x++ )
			{
				/* init averages */
				avg0 = avg1 = avg2 = 0L;

				/* now gather pixel values in rectangular area ('box filter') */
				endY = srcY + maxHeight;
				if( endY > srcHeight ) {
					endY = srcHeight;
				}
				for( yInt = srcY; yInt < endY; yInt++ )
				{
					/* init current input line pointer */
					currSrcLinePtr = &srcData[ srcScanlineBytes * yInt + 3 * srcX ];

					endX = srcX + maxWidth;
					if( endX > srcWidth ) {
						endX = srcWidth;
					}
					for(xInt = srcX; xInt < endX; xInt++)
					{
						avg0 += *currSrcLinePtr++;
						avg1 += *currSrcLinePtr++;
						avg2 += *currSrcLinePtr++;
					}
				}

				/* now write back averages into output bitmap */
				/* calc average and write back the value */
				*currDestLinePtr++ = (unsigned char) (avg0 / pixelWeight);
				*currDestLinePtr++ = (unsigned char) (avg1 / pixelWeight);
				*currDestLinePtr++ = (unsigned char) (avg2 / pixelWeight);

				/* set new actual x position */
				srcX = srcWidth * (x+1) / destWidth;
			}

			/* set new actual y position */
			srcY = srcHeight * (y+1) / destHeight;
		}

	}
	else if( flags & SJ_IMGOP_SMOOTH )
	{
		/*
		 * Resample Enlarging using Antialiasing.
		 *
		 * The bilinear interpolation function for enlarging an image does the following:
		 * Bilinear means, that up to four pixel are linearly averaged to get the
		 * destination pixel value.  To achieve this, the exact fractional position of
		 * every destination pixel in the source pixel array is determined.  If the
		 * center of a source pixel is exactly hit, its value is taken.  If i.e. the
		 * y coordinate exactly hits a source pixel, but the x coordinate is somewhere
		 * in between two source pixels, their intensities are weighted proportional to
		 * their distance to the point in question ('linearly interpolated').  If both x
		 * and y coordinate do not hit a pixel exactly, first the two x values of the two
		 * closest y values are determined (first value is average for rounded y, second
		 * value is average of rounded y plus one), then these two values are weighted
		 * proportional to their y distance to the point in question ('bilinearly
		 * interpolated', first in x, then in y direction).
		 */

		for( y = 0; y < destHeight; y++ )
		{
			/* calculate current source y position
			 * (integer and fractional part)
			 */
			yInt = srcHeight * y / destHeight;
			yFrac= srcHeight * y % destHeight;

			/* buffer current input scanline (saves us some multiplications) */
			currSrcLinePtr  = &srcData [ srcScanlineBytes * yInt ];

			/* buffer current output scanline (saves us some multiplications) */
			currDestLinePtr = &destData[ destScanlineBytes * y   ];

			/* now determine how many pixel in y direction
			 * are involved in interpolation.
			 */
			if( yFrac == 0 || yInt >= srcHeight - 1 )
			{
				/* only one pixel in y direction */
				for( x = 0; x < destWidth; x++ )
				{
					/* calculate current source x position
					 * (integer and fractional part)
					 */
					xInt = srcWidth * x / destWidth;
					xFrac= srcWidth * x % destWidth;

					/* now determine how many pixel in x
					 * direction are involved in
					 * interpolation.
					 */
					if( xFrac == 0 || xInt >= srcWidth - 1 )
					{
						/* only one in x and in y - exactly
						 * hit a pixel.
						 */
						destPtr = &currDestLinePtr[ 3 * x    ];
						srcPtr0 = &currSrcLinePtr [ 3 * xInt ];

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth);
						*destPtr++ = (unsigned char)(value0 - value0 * yFrac / destHeight);
						srcPtr0++;

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth);
						*destPtr++ = (unsigned char)(value0 - value0 * yFrac / destHeight);
						srcPtr0++;

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth);
						*destPtr   = (unsigned char)(value0 - value0 * yFrac / destHeight);
					}
					else
					{
						/* interpolate linearly between
						 * two pixel in x direction
						 */
						destPtr = &currDestLinePtr[ 3*x ];
						srcPtr0 = &currSrcLinePtr[ 3*xInt ];
						srcPtr1 = &currSrcLinePtr[ 3*(xInt+1) ];

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth +
						                         *srcPtr1 * xFrac / destWidth);
						*destPtr++ = (unsigned char)(value0 - value0 * yFrac / destHeight);
						srcPtr0++; srcPtr1++;

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth +
						                         *srcPtr1 * xFrac / destWidth);
						*destPtr++ = (unsigned char)(value0 - value0 * yFrac / destHeight);
						srcPtr0++; srcPtr1++;

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth +
						                         *srcPtr1 * xFrac / destWidth);
						*destPtr   = (unsigned char)(value0 - value0 * yFrac / destHeight);
					}
				}
			}
			else
			{
				/* two pixel in y direction */
				for( x = 0; x < destWidth; x++ )
				{
					/* calculate current source x position
					 * (integer and fractional part)
					 */
					xInt = srcWidth * x / destWidth;
					xFrac= srcWidth * x % destWidth;

					/* now determine how many pixel in x
					 * direction are involved in
					 * interpolation.
					 */
					if( xFrac == 0 || xInt >= srcWidth - 1 )
					{
						/* only one in x and two in y -
						 * interpolate linearly between
						 * two pixel in y direction
						 */

						destPtr = &currDestLinePtr[ 3*x ];
						srcPtr0 = &currSrcLinePtr[ 3*xInt ];
						srcPtr1 = &currSrcLinePtr[ 3*xInt + srcScanlineBytes ];

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * yFrac / destHeight +
						                         *srcPtr1 * yFrac / destHeight);
						*destPtr++ = (unsigned char)(value0 - value0 * xFrac / destWidth);
						srcPtr0++; srcPtr1++;

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * yFrac / destHeight +
						                         *srcPtr1 * yFrac / destHeight);
						*destPtr++ = (unsigned char)(value0 - value0 * xFrac / destWidth);
						srcPtr0++; srcPtr1++;

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * yFrac / destHeight +
						                         *srcPtr1 * yFrac / destHeight);
						*destPtr   = (unsigned char)(value0 - value0 * xFrac / destWidth);
					}
					else
					{
						/* interpolate bilinearly between
						 * two pixel in x direction and
						 * two pixel in y direction.
						 */

						destPtr = &currDestLinePtr[ 3*x ];
						srcPtr0 = &currSrcLinePtr [ 3*xInt ];
						srcPtr1 = &currSrcLinePtr [ 3*(xInt+1) ];
						srcPtr2 = &currSrcLinePtr [ 3*xInt + srcScanlineBytes ];
						srcPtr3 = &currSrcLinePtr [ 3*(xInt+1) + srcScanlineBytes ];

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth + *srcPtr1 * xFrac / destWidth);
						value1 = (unsigned char)(*srcPtr2 - *srcPtr2 * xFrac / destWidth + *srcPtr3 * xFrac / destWidth);
						srcPtr0++; srcPtr1++; srcPtr2++; srcPtr3++;
						*destPtr++ = (unsigned char)(value0 - value0 * yFrac / destHeight + value1 * yFrac / destHeight);

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth + *srcPtr1 * xFrac / destWidth);
						value1 = (unsigned char)(*srcPtr2 - *srcPtr2 * xFrac / destWidth + *srcPtr3 * xFrac / destWidth);
						srcPtr0++; srcPtr1++; srcPtr2++; srcPtr3++;
						*destPtr++ = (unsigned char)(value0 - value0 * yFrac / destHeight + value1 * yFrac / destHeight);

						value0 = (unsigned char)(*srcPtr0 - *srcPtr0 * xFrac / destWidth + *srcPtr1 * xFrac / destWidth);
						value1 = (unsigned char)(*srcPtr2 - *srcPtr2 * xFrac / destWidth + *srcPtr3 * xFrac / destWidth);
						*destPtr = (unsigned char)(value0 - value0 * yFrac / destHeight + value1 * yFrac / destHeight);
					}
				}
			}
		}
	}
	else
	{
		/*
		 * Resize Data.
		 *
		 * Can be used for enlarging or shrinkting an image. It just takes the nearest
		 * pixel value out of the source image for every pixel in the destination.
		 */

		for( y = 0; y < destHeight; y++ )
		{
			/* calculate current source y position (rounded integer position) */
			yInt = srcHeight * y / destHeight;

			/* buffer current input scanline (saves us some multiplications) */
			currSrcLinePtr  = &srcData [ srcScanlineBytes  * yInt ];

			/* buffer current output scanline (saves us some multiplications) */
			currDestLinePtr = &destData[ destScanlineBytes * y    ];

			for( x = 0; x < destWidth; x++ )
			{
				/* calculate current source x position (rounded integer position) */
				xInt = srcWidth * x / destWidth;

				srcPtr0 = &currSrcLinePtr [ 3 * xInt ];

				*currDestLinePtr++ = *srcPtr0++;
				*currDestLinePtr++ = *srcPtr0++;
				*currDestLinePtr++ = *srcPtr0;
			}
		}
	}

	/* success - swap source and destination data */
	wxImage newImage(destWidth, destHeight, destData);
	image = newImage;
	return TRUE;
}


/*******************************************************************************
 * SjImgOp - Grayscale Image
 ******************************************************************************/


bool SjImgOp::DoGrayscale(wxImage& image)
{
	/* prepare data */
	unsigned char*  srcData = image.GetData();
	long            srcWidth = image.GetWidth();
	long            srcHeight = image.GetHeight();
	long            srcScanlineBytes = srcWidth * 3;

	register long           x, gray;
	long                    y;
	register unsigned char* currSrcLinePtr;
	bool                    hasMask = image.HasMask();
	unsigned char           maskR = image.GetMaskRed(),
	                        maskG = image.GetMaskGreen(),
	                        maskB = image.GetMaskBlue();

	/* Set RGB Coefficents
	 * (intensity factors: 0.2125, 0.7154, 0.0721 (ITU Rec. BT.709))
	 */

#define COEFF_RED   6968L
#define COEFF_GREEN 23434L
#define COEFF_BLUE  2366L
#define COEFF_SUM   32768L

#if (COEFF_RED+COEFF_GREEN+COEFF_BLUE)!=COEFF_SUM
#error The Sum of the RGB Coefficents is not correct!
#endif

	/*
	 * grayscale image
	 */

	for( y = 0; y < srcHeight; y++ )
	{
		/* buffer current input scanline (saves us some multiplications) */
		currSrcLinePtr  = &srcData [ srcScanlineBytes  * y ];

		for( x = 0; x < srcWidth; x++ )
		{
			/* convert this pixel? */
			if( hasMask )
			{
				if( currSrcLinePtr[0] == maskR
				        && currSrcLinePtr[1] == maskG
				        && currSrcLinePtr[2] == maskB )
				{
					currSrcLinePtr += 3;
					continue;
				}
			}

			/* get gray value from current RGB pixel */
			gray = (    (long)currSrcLinePtr[0] * COEFF_RED
			            +       (long)currSrcLinePtr[1] * COEFF_GREEN
			            +       (long)currSrcLinePtr[2] * COEFF_BLUE    ) / COEFF_SUM;

			/* set gray value to current RGB pixel */
			*currSrcLinePtr++ = (unsigned char)gray;
			*currSrcLinePtr++ = (unsigned char)gray;
			*currSrcLinePtr++ = (unsigned char)gray;
		}
	}

	return TRUE;
}


/*******************************************************************************
 * SjImgOp - Negative Images
 ******************************************************************************/


bool SjImgOp::DoNegative(wxImage& image)
{
	/* prepare data */
	unsigned char*  srcData = image.GetData();
	long            srcWidth = image.GetWidth();
	long            srcHeight = image.GetHeight();
	long            srcScanlineBytes = srcWidth * 3;

	long                    x, y;
	unsigned char*          currSrcLinePtr;

	/* negative image */
	for( y = 0; y < srcHeight; y++ )
	{
		/* buffer current input scanline (saves us some multiplications) */
		currSrcLinePtr  = &srcData [ srcScanlineBytes  * y ];

		for( x = 0; x < srcWidth; x++ )
		{
			*currSrcLinePtr = 255 - *currSrcLinePtr;
			currSrcLinePtr++;
			*currSrcLinePtr = 255 - *currSrcLinePtr;
			currSrcLinePtr++;
			*currSrcLinePtr = 255 - *currSrcLinePtr;
			currSrcLinePtr++;
		}
	}

	return TRUE;
}


/*******************************************************************************
 * SjImgOp - Border / Contrast
 ******************************************************************************/


static void s_buildContrastMap( unsigned char*  cmap,
                                long            contrast/*-100..100*/,
                                long            brightness/*-100..100*/ )
{
	long            range, add;
	register long   x, y;

	if( brightness )
	{
		range = brightness>0? brightness : brightness*-1;
		range = 256 - (256*range)/100;
		add = brightness>0? (256-range) : 0;
	}
	else
	{
		range = 256;
		add = 0;
	}

	for(x = 0; x < 256; x++)
	{
		/* apply brighness */
		y = add + (x*range)/256;

		/* apply contrast */
		y = y + (((y - 127) * contrast) / 100);

		/* check overfows and store value */
		if( y < 0 ) y = 0; if( y > 255 ) y = 255;
		cmap[x] = (unsigned char)y;
	}
}


bool SjImgOp::DoAddBorder(wxImage& image, long borderWidth)
{
	unsigned char*  srcData = image.GetData();
	long            srcWidth = image.GetWidth();
	long            srcHeight = image.GetHeight();
	long            srcScanlineBytes = srcWidth * 3;

	long                    x, y, b;
	unsigned char*          currSrcLinePtr;

	static unsigned char    cmap[256];
	static int              cmapInitialized = 0;

	if( borderWidth < 1 )
	{
		borderWidth = 1;
	}

	if( srcWidth <= borderWidth*2
	        || srcHeight <= borderWidth*2 )
	{
		return TRUE; /* no place for the border */
	}

	if( !cmapInitialized )
	{
		s_buildContrastMap(cmap, 0, -15);
		cmapInitialized = 1;
	}

#define SET_BORDER  currSrcLinePtr[0] = cmap[currSrcLinePtr[0]]; \
                     currSrcLinePtr[1] = cmap[currSrcLinePtr[1]]; \
                     currSrcLinePtr[2] = cmap[currSrcLinePtr[2]];

	/* top/bottom border (without the pixels very left and very right) */
	for( y=0; y<2; y++ )
	{
		for( b=0; b<borderWidth; b++ )
		{
			currSrcLinePtr = y? &srcData[b*srcScanlineBytes] : &srcData[(srcHeight-b-1)*srcScanlineBytes];
			currSrcLinePtr += borderWidth * 3;

			for( x=borderWidth*2; x<srcWidth; x++ )
			{
				SET_BORDER;
				currSrcLinePtr += 3;
			}
		}
	}

	/* left/right border */
	for( x=0; x<2; x++ )
	{
		for( b=0; b<borderWidth; b++ )
		{
			currSrcLinePtr = x? &srcData[b*3] : &srcData[(srcWidth-b-1)*3];
			for( y=0; y<srcHeight; y++ )
			{
				SET_BORDER;
				currSrcLinePtr += srcScanlineBytes;
			}
		}
	}

	return TRUE;
}


bool SjImgOp::DoContrast(wxImage& image, long contrast, long brightness)
{
	unsigned char*  srcData = image.GetData();
	long            srcWidth = image.GetWidth();
	long            srcHeight = image.GetHeight();
	long            srcScanlineBytes = srcWidth * 3;

	register long           x, y;
	register unsigned char* currSrcLinePtr;

	unsigned char           cmap[256];

	if( contrast < -100 ) { contrast = -100; }
	else if( contrast >  100 ) { contrast =  100; }

	if( brightness < -100 ) { brightness = -100; }
	else if( brightness >  100 ) { brightness =  100; }

	if( contrast == 0 && brightness == 0 ) {
		return 1;
	}

	/* Create Contrast Map.
	 *
	 * To perfectly determine the contrast, you would have to first
	 * find the average brightness of the image; I use the shortcut
	 * method and assume that the average is 127 (close enough for
	 * our purposes).
	 */

	s_buildContrastMap(cmap, contrast, brightness);

	/*
	 * apply contrast map
	 */

	for( y = 0; y < srcHeight; y++ )
	{
		/* buffer current input scanline (saves us some multiplications) */
		currSrcLinePtr  = &srcData [ srcScanlineBytes  * y ];

		for( x = 0; x < srcWidth; x++ )
		{
			*currSrcLinePtr = cmap[*currSrcLinePtr];
			currSrcLinePtr++;
			*currSrcLinePtr = cmap[*currSrcLinePtr];
			currSrcLinePtr++;
			*currSrcLinePtr = cmap[*currSrcLinePtr];
			currSrcLinePtr++;
		}
	}

	return TRUE;
}


void SjImgOp::SetContrast(long v)
{
	if( v < -100 ) v = -100;
	if( v >  100 ) v =  100;

	m_contrast = v;

	SetContrastFlag();
}


void SjImgOp::SetBrightness(long v)
{
	if( v < -100 ) v = -100;
	if( v >  100 ) v =  100;

	m_brightness = v;

	SetContrastFlag();
}


void SjImgOp::SetContrastFlag()
{
	if(m_brightness||m_contrast)
	{
		m_flags|=SJ_IMGOP_CONTRAST;
	}
	else
	{
		m_flags&=~SJ_IMGOP_CONTRAST;
	}
}


/*******************************************************************************
 * SjImgOp - Handling the Edit Object
 ******************************************************************************/


wxString SjImgOp::GetAsString() const
{
	wxString ret = wxString::Format(wxT("%x"), (int)m_flags);

	if( m_flags & SJ_IMGOP_CROP )
	{
		ret += wxString::Format(wxT("g%xg%xg%xg%x"), (int)m_cropX, (int)m_cropY, (int)m_cropW, (int)m_cropH);
	}

	if( m_flags & SJ_IMGOP_CONTRAST )
	{
		// we're moving negative values to the area 101...200
		int contrast   = m_contrast;    if( contrast   < 0 ) contrast   += 201;
		int brightness = m_brightness;  if( brightness < 0 ) brightness += 201;
		ret += wxString::Format(wxT("g%xg%x"), (int)contrast, (int)brightness);
	}

	if( m_flags & SJ_IMGOP_RESIZE )
	{
		ret += wxString::Format(wxT("g%xg%x"), (int)m_resizeW, (int)m_resizeH);
	}

	return ret;
}


void SjImgOp::SetFromString(const wxString& str)
{
	wxStringTokenizer tkz(str, wxT("g")/*the hex. numbers are separated by the character "g"*/);
	if( tkz.HasMoreTokens() )
	{
		tkz.GetNextToken().ToLong(&m_flags, 16);

		if( m_flags & SJ_IMGOP_CROP )
		{
			if( tkz.HasMoreTokens() ) tkz.GetNextToken().ToLong(&m_cropX, 16);
			if( tkz.HasMoreTokens() ) tkz.GetNextToken().ToLong(&m_cropY, 16);
			if( tkz.HasMoreTokens() ) tkz.GetNextToken().ToLong(&m_cropW, 16);
			if( tkz.HasMoreTokens() ) tkz.GetNextToken().ToLong(&m_cropH, 16);
		}

		if( m_flags & SJ_IMGOP_CONTRAST )
		{
			// negative values are found in the area 101..200
			if( tkz.HasMoreTokens() )
			{
				tkz.GetNextToken().ToLong(&m_contrast, 16);
				if( m_contrast >= 101 && m_contrast <= 200 ) m_contrast -= 201;
				SetContrast(m_contrast);
			}
			if( tkz.HasMoreTokens() )
			{
				tkz.GetNextToken().ToLong(&m_brightness, 16);
				if( m_brightness >= 101 && m_brightness <= 200 ) m_brightness -= 201;
				SetBrightness(m_brightness);
			}
		}

		if( m_flags & SJ_IMGOP_RESIZE )
		{
			if( tkz.HasMoreTokens() ) tkz.GetNextToken().ToLong(&m_resizeW, 16);
			if( tkz.HasMoreTokens() ) tkz.GetNextToken().ToLong(&m_resizeH, 16);
		}
	}
}


bool SjImgOp::SaveToDb(const wxString& url) const
{
	wxString    str = GetAsString();
	wxSqlt      sql;
	bool        ret;

	sql.Query(wxT("SELECT id FROM arts WHERE url='") + sql.QParam(url) + wxT("';"));
	if( sql.Next() )
	{
		ret = sql.Query(wxT("UPDATE arts SET operations='") + sql.QParam(str) + wxT("' WHERE id=") + sql.LParam(sql.GetLong(0)) + wxT(";"));
	}
	else
	{
		ret = sql.Query(wxT("INSERT INTO arts (url, operations) VALUES ('") + sql.QParam(url) + wxT("', '") + sql.QParam(str)+ wxT("');"));
	}

	if( !ret )
	{
		wxLogError(_("Cannot write \"%s\"."), url.c_str());
	}

	return ret;
}


bool SjImgOp::LoadFromDb(const wxString& url)
{
	wxSqlt      sql;
	wxString    op;

	m_id = 0;
	sql.Query(wxT("SELECT operations, id FROM arts WHERE url='") + sql.QParam(url) + wxT("' ORDER BY id;"));
	if( sql.Next() )
	{
		op = sql.GetString(0);
		m_id = sql.GetLong(1);

		SetFromString(op);
	}

	return (m_id!=0);
}



/*******************************************************************************
 *  SjImgOp - Creating dummy images
 ******************************************************************************/


wxString SjImgOp::GetDummyCoverUrl(const wxString& artist, const wxString& album)
{
	return wxT("cover:") + SjTools::Urlencode(artist) + wxT("/") + SjTools::Urlencode(album);
}


static const long g_colours[] =
{
	/* bg,      text    */
	0x080808,   0xF8F8F8,
	0xE9D6BA,   0x256B9B,   // RAL-1050
	0xBE450C,   0xFFBB9B,   // RAL-2010
	0xBB243B,   0x95CAFF,   // RAL-3020
	0x964078,   0xA0D2B8,   // RAL-4006
	0x308AB9,   0xF5C9CE,   // RAL-5012
	0x66874F,   0xC7D8B9,   // RAL-6017
	0x9F6C44,   0xEACAB4,   // RAL-8001

	/* alternatives from above, bg as text, bg white
	 * (some alternatives are left out if they have a bad contrast)
	 */
	0xF8F8F8,   0x202020,
	0xF8F8F8,   0x256B9B,
	0xF8F8F8,   0xBE450C,
	0xF8F8F8,   0xBB243B,
	0xF8F8F8,   0x964078,
	0xF8F8F8,   0x308AB9,
	0xF8F8F8,   0x66874F,
};


static wxFont createAnyFont(long& holdrand, int wh, bool big=FALSE)
{
	return wxFont
	       (
	           big? wh/2 : wh/18,                      // point size
	           wxSWISS,                                // family (overwritten by facename)
	           SjTools::PrivateRand(holdrand, 2)==0?   // style
	           wxNORMAL : wxITALIC,
	           SjTools::PrivateRand(holdrand, 2)==0?   // weight
	           wxNORMAL : wxBOLD,
	           FALSE,                                  // underline
	           SjTools::PrivateRand(holdrand, 2)==0?   // facename
	           SJ_DEF_FONT_FACE :
	           SJ_ALT_FONT_FACE
	       );
}


static void drawText(wxDC& dc, const wxString& str__, int x, int y, int maxW)
{
	wxString str(str__);
	wxCoord w, h, dotsW = 0;
	bool truncated = FALSE;
	while( 1 )
	{
		dc.GetTextExtent(str, &w, &h);
		if( w+dotsW <= maxW )
		{
			if( truncated ) str = str.Trim() + wxT("...");
			dc.DrawText(str, x, y);
			return;
		}

		str.Truncate(str.Len()-1);
		if( str.IsEmpty() )
		{
			return;
		}

		dc.GetTextExtent(wxT("..."), &dotsW, &h);

		truncated = TRUE;
	}
}


wxImage SjImgOp::CreateDummyCover(const wxString& albumDummyUrl, int wh)
{
	wxImage ret;

	// create memory DC
	wxBitmap    memBitmap(wh, wh);
	wxMemoryDC  memDc;
	memDc.SelectObject(memBitmap);
	if( !memDc.Ok() )
	{
		memDc.SelectObject(wxNullBitmap);
		return ret;
	}

	// get album data, initialize random generators
	wxString text1 = SjTools::Urldecode(albumDummyUrl.Mid(6/*get rid of "cover:"*/).BeforeFirst('/'));
	wxString text2 = SjTools::Urldecode(albumDummyUrl.AfterFirst('/'));

	if( text1.IsEmpty() ) { text1 = text2; text2.Clear(); }
	if( text1.IsEmpty() ) { text1 = _("Unsorted tracks"); }

	long holdrandArtist = g_tools->Crc32AddString(g_tools->Crc32Init(), text1.Lower());
	long holdrandAlbum = g_tools->Crc32AddString(g_tools->Crc32Init(), albumDummyUrl.Lower());

	switch( SjTools::PrivateRand(holdrandArtist, 3) )
	{
		case 0: text1.MakeLower(); break;
		case 1: text1.MakeUpper(); break;
	}

	switch( SjTools::PrivateRand(holdrandArtist, 3) )
	{
		case 0: text2.MakeLower(); break;
		case 1: text2.MakeUpper(); break;
	}

	// ...get colours
	wxColour colourBg;
	wxColour colourText;
	int i = SjTools::PrivateRand(holdrandArtist, sizeof(g_colours)/(sizeof(long)*2)) * 2;
#define GETR(a) ((a)>>16)
#define GETG(a) (((a)>>8)&0xFF)
#define GETB(a) ((a)&0xFF)
	colourBg = wxColour(GETR(g_colours[i]), GETG(g_colours[i]), GETB(g_colours[i]));
	colourText = wxColour(GETR(g_colours[i+1]), GETG(g_colours[i+1]), GETB(g_colours[i+1]));

	// create dummy data ...background
	memDc.SetPen(*wxTRANSPARENT_PEN);
	memDc.SetBrush(wxBrush(colourBg, wxSOLID));
	memDc.DrawRectangle(0, 0, wh, wh);

	// ...big letters
	memDc.SetFont(createAnyFont(holdrandAlbum, wh, TRUE));

	wxString bigtext = text2.IsEmpty()? text1 : text2;
	bigtext.MakeUpper();
	bigtext.Replace(wxT(" "), wxT(""));
	if( SjTools::PrivateRand(holdrandAlbum, 16)==0 ) bigtext.Prepend(wxT("*"));

	int x = -wh/10, y = wh-wh/2;
	memDc.SetTextForeground(colourText);
	drawText(memDc, bigtext,
	         x, y, wh*2);

	int diffx = wh/(40+SjTools::PrivateRand(holdrandAlbum, 40));
	if( diffx < 2 ) diffx = 2;
	int diffy = diffx;

	if( SjTools::PrivateRand(holdrandAlbum, 2)==0 ) diffx *= -1;
	if( SjTools::PrivateRand(holdrandAlbum, 2)==0 ) diffy *= -1;

	memDc.SetTextForeground(colourBg);
	drawText(memDc, bigtext,
	         x+diffx, y+diffy, wh*2);

	memDc.SetBrush(*wxTRANSPARENT_BRUSH);
	memDc.DrawRectangle(0, 0, wh, wh);

	// ...line 1 (usually the artist)
	memDc.SetTextForeground(colourText);
	memDc.SetFont(createAnyFont(holdrandArtist, wh));
	drawText(memDc, text1, wh/20, wh/20, wh-wh/10);

	// ...line 2 (usually the album)
	memDc.SetFont(createAnyFont(holdrandArtist, wh));
	drawText(memDc, text2, wh/20, wh/10, wh-wh/10);

	// create image
	ret = memBitmap.ConvertToImage();

	memDc.SelectObject(wxNullBitmap);

	return ret;
}
