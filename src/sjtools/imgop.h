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
 * File:    imgop.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke image editing
 *
 ******************************************************************************/


#ifndef __SJ_IMGOP_H__
#define __SJ_IMGOP_H__


class SjImgOp
{
public:
	// Constructor
	SjImgOp             ();

	// Copy operation
	void            CopyFrom            (const SjImgOp& o);
	SjImgOp&        operator =          (const SjImgOp& o) { CopyFrom(o); return *this; }

	// Comparison operations (the ID is NOT compared!)
	long            Cmp                 (const SjImgOp& o) const;
	bool            operator ==         (const SjImgOp& o) const  { return Cmp(o)==0; }
	bool            operator !=         (const SjImgOp& o) const  { return Cmp(o)!=0; }

	// Available operations -- do not modify the flags
	// as they're written eg. to the database
	#define         SJ_IMGOP_RESIZE          0x00000001L
	#define         SJ_IMGOP_SMOOTH          0x00000002L // used with SJ_IMGOP_RESIZE
	#define         SJ_IMGOP_ROTATE90        0x00000004L
	#define         SJ_IMGOP_ROTATE180       0x00000008L
	#define         SJ_IMGOP_ROTATE270       0x00000010L
	#define         SJ_IMGOP_FLIPHORZ        0x00000020L
	#define         SJ_IMGOP_FLIPVERT        0x00000040L
	#define         SJ_IMGOP_GRAYSCALE       0x00000080L
	#define         SJ_IMGOP_NEGATIVE        0x00000100L
	#define         SJ_IMGOP_CROP            0x00000200L
	#define         SJ_IMGOP_CONTRAST        0x00000400L
	#define         SJ_IMGOP_BORDER          0x00000800L
	#define         SJ_IMGOP_KEEPASPECT      0x00004000L // used with SJ_IMGOP_RESIZE
	long            m_flags;
	long            m_cropX, m_cropY, m_cropW, m_cropH;
	long            m_resizeW, m_resizeH;
	long            m_contrast, m_brightness;

	// Perform all operations
	bool            Do                  (wxImage& image);

	// perform single operations
	static bool     DoCrop              (wxImage& image, long x, long y, long w, long h);
	bool            DoCrop              (wxImage& image) { return DoCrop(image, m_cropX, m_cropY, m_cropW, m_cropH); }
	static bool     DoRotate            (wxImage& image, long flags);
	bool            DoRotate            (wxImage& image) { return DoRotate(image, m_flags); }
	static bool     DoGrayscale         (wxImage& image);
	static bool     DoNegative          (wxImage& image);
	static bool     DoAddBorder         (wxImage& image, long borderWidth);
	static bool     DoContrast          (wxImage& image, long contrast, long brightness);
	static bool     DoResize            (wxImage& image, long destWidth, long destHeight, long flags);

	// creating "dummy" covers
	static wxString GetDummyCoverUrl    (const wxString& artist, const wxString& album);
	static wxImage  CreateDummyCover    (const wxString& albumDummyUrl, int wh);

	// handle image database
	wxString        GetAsString         () const;
	void            SetFromString       (const wxString&);

	bool            SaveToDb            (const wxString& url) const;
	bool            LoadFromDb          (const wxString& url);

	// the ID is only valid after a successful call to LoadFromDb()
	long            GetId               () const { return m_id; }

	// easy bitfield manipulation for m_flags...
	void            ToggleFlag          (long f) { if(m_flags&f) {m_flags&=~f;} else {m_flags|=f;} }
	bool            HasFlag             (long f) { return (m_flags&f)? TRUE : FALSE; }

	// ...flipping
	bool            HasFlip             () { return (m_flags&(SJ_IMGOP_FLIPHORZ|SJ_IMGOP_FLIPVERT))? TRUE : FALSE; }
	void            ClearFlip           () { m_flags&=~(SJ_IMGOP_FLIPHORZ|SJ_IMGOP_FLIPVERT); }

	// ...rotating
	bool            IsRotated           () { return (m_flags&(SJ_IMGOP_ROTATE90|SJ_IMGOP_ROTATE180|SJ_IMGOP_ROTATE270))? TRUE : FALSE; }
	bool            IsRightRotated      () { return (m_flags&(SJ_IMGOP_ROTATE90|SJ_IMGOP_ROTATE180))? TRUE : FALSE; }
	bool            IsLeftRotated       () { return (m_flags&SJ_IMGOP_ROTATE270)? TRUE : FALSE; }
	void            ClearRotate         () { m_flags&=~(SJ_IMGOP_ROTATE90|SJ_IMGOP_ROTATE180|SJ_IMGOP_ROTATE270); }
	void            SetRotate           (long flag) { ClearRotate(); m_flags|=flag; }
	void            SetRotatePlus90     () {
		long f=m_flags;
		ClearRotate();
		if(f&SJ_IMGOP_ROTATE90 ) {m_flags|=SJ_IMGOP_ROTATE180;}
		else if(f&SJ_IMGOP_ROTATE180) {m_flags|=SJ_IMGOP_ROTATE270;}
		else if(f&SJ_IMGOP_ROTATE270) {}
		else                          {m_flags|=SJ_IMGOP_ROTATE90;}
	}
	void            SetRotateMinus90    () {
		long f=m_flags;
		ClearRotate();
		if(f&SJ_IMGOP_ROTATE90 ) {}
		else if(f&SJ_IMGOP_ROTATE180) {m_flags|=SJ_IMGOP_ROTATE90;}
		else if(f&SJ_IMGOP_ROTATE270) {m_flags|=SJ_IMGOP_ROTATE180;}
		else                          {m_flags|=SJ_IMGOP_ROTATE270;}
	}
	// ...cropping
	void            ClearCrop           () { m_cropX=0; m_cropY=0; m_cropW=0; m_cropH=0; m_flags &= ~SJ_IMGOP_CROP; }
	void            SetCrop             (const wxRect& r) { m_cropX=r.x; m_cropY=r.y; m_cropW=r.width; m_cropH=r.height; m_flags |= SJ_IMGOP_CROP; }

	// ...brightness / contrast
	long            GetContrast         () { return (m_flags&SJ_IMGOP_CONTRAST)? m_contrast : 0; }
	long            GetBrightness       () { return (m_flags&SJ_IMGOP_CONTRAST)? m_brightness : 0; }
	void            SetContrast         (long v);
	void            SetBrightness       (long v);
	void            SetContrastFlag     ();

private:
	long            m_id;
};


// Set RGB Coefficents
// (intensity factors: 0.2125, 0.7154, 0.0721 (ITU Rec. BT.709))
#define SJ_COEFF_RED   6968L
#define SJ_COEFF_GREEN 23434L
#define SJ_COEFF_BLUE  2366L
#define SJ_COEFF_SUM   32768L


#endif /* __SJ_IMGOP_H__ */
