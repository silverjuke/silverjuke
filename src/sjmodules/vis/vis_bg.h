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
 * File:    vis_bg.h
 * Authors: Björn Petersen
 * Purpose: Background functions, used eg. by the karaoke module
 *
 ******************************************************************************/



#ifndef __SJ_VIS_BG_H__
#define __SJ_VIS_BG_H__




class SjVisBg
{
public:
	SjVisBg             () { m_tempLine = NULL; m_tempLineWidth = 0; }
	virtual                 ~SjVisBg            () { if(m_tempLine) delete m_tempLine; }
	virtual void            SetSize             (const wxSize&) = 0;
	virtual void            DrawBackground      (wxDC&) = 0;
	virtual const unsigned char*
	GetBackgroundBits   (long x, long y, long desiredWidth) = 0;

	// some tools
	static wxCoord          SetFontPixelH       (wxDC& dc, wxFont& font, wxCoord pixelH, wxCoord startTestFontPtSize=64);
	static wxCoord          SetFontPixelH       (wxDC& dc, wxFont& font, const wxSize& sizeToFit, const wxString& strToFit);

protected:
	void                    AllocTempLine       (long desiredWidth)
	{
		if( desiredWidth>m_tempLineWidth)
		{
			free(m_tempLine);
			m_tempLine = (unsigned char*)malloc(desiredWidth*3);
			m_tempLineWidth = desiredWidth;
		}
	}
	unsigned char*          m_tempLine;

private:
	long                    m_tempLineWidth;
};




class SjVisGrBg : public SjVisBg
{
public:
	SjVisGrBg           (long imgOpFlags) : m_brush(*wxBLACK) { m_imgOpFlags = imgOpFlags; m_rgb = NULL; m_height = 0; }
	~SjVisGrBg          () { if(m_rgb)free(m_rgb); }
	void                    SetSize             (const wxSize&);
	void                    DrawBackground      (wxDC&);
	const unsigned char*    GetBackgroundBits   (long x, long y, long desiredWidth);

private:
	wxBrush                 m_brush;
	long                    m_height;
	unsigned char*          m_rgb;
	long                    m_imgOpFlags;
};



class SjVisImgBg : public SjVisBg
{
public:
	SjVisImgBg          (const wxString& url, long imgOpFlags);
	~SjVisImgBg         () { if(m_scaledBitmap) delete m_scaledBitmap; }
	void                    SetSize             (const wxSize&);
	void                    DrawBackground      (wxDC&);
	const unsigned char*    GetBackgroundBits   (long x, long y, long desiredWidth);

private:
	wxImage                 m_orgImage;
	wxImage                 m_scaledImage;
	wxBitmap*               m_scaledBitmap;
	long                    m_scaledWidth;
	long                    m_scaledHeight;
	long                    m_imgOpFlags;
};










#endif // __SJ_VIS_BG_H__
