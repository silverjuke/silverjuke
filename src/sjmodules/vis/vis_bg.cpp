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
 * File:    vis_bg.cpp
 * Authors: Björn Petersen
 * Purpose: Background functions, used eg. by the karaoke module
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjmodules/vis/vis_bg.h>


/*******************************************************************************
 * SjKaraokeGrBg
 ******************************************************************************/


void SjVisGrBg::SetSize(const wxSize& size)
{
	long newHeight = size.y;

	if( newHeight <= 0 )
		return;

	if( newHeight > m_height )
	{
		if( m_rgb ) free(m_rgb);
		m_rgb = (unsigned char*)malloc(newHeight*3);
	}

	m_height = newHeight;

	// blue gradient background
#define BG_STEPS 88
#define BG_GRAYSCALE 0.9F
	int rowH = (newHeight/BG_STEPS)+1, y, p, pEnd, yCol;
	unsigned char* rgb = m_rgb;
	for( y = 0; y < BG_STEPS; y++ )
	{
		p    = y * rowH;
		pEnd = p + rowH;
		if( pEnd > newHeight )
			pEnd = newHeight;
		if( m_imgOpFlags & SJ_IMGOP_GRAYSCALE )
		{
			yCol = int(float(y)*BG_GRAYSCALE);
			for( ; p<pEnd; p++ )
			{
				rgb[p*3]   = yCol;
				rgb[p*3+1] = yCol;
				rgb[p*3+2] = yCol;
			}
		}
		else
		{
			for( ; p<pEnd; p++ )
			{
				rgb[p*3]   = 0;
				rgb[p*3+1] = 0;
				rgb[p*3+2] = y;
			}
		}
	}
}


void SjVisGrBg::DrawBackground(wxDC& dc)
{
	// blue gradient background
	wxSize size = dc.GetSize();
	int rowH = (m_height/BG_STEPS)+1, y, yCol;
	dc.SetPen(*wxTRANSPARENT_PEN);
	for( y = 0; y < BG_STEPS; y++ )
	{
		if( m_imgOpFlags & SJ_IMGOP_GRAYSCALE )
		{
			yCol = int(float(y)*BG_GRAYSCALE);
			m_brush.SetColour(yCol, yCol, yCol);
		}
		else
		{
			m_brush.SetColour(0, 0, y);
		}

		dc.SetBrush(m_brush);
		dc.DrawRectangle(0, y*rowH, size.x, rowH);
	}
}


const unsigned char* SjVisGrBg::GetBackgroundBits(long x, long y, long desiredWidth)
{
	AllocTempLine(desiredWidth);

	if( y >= 0 && y < m_height )
	{
		unsigned char       *ptr = m_tempLine;
		const unsigned char *ptrEnd = ptr+desiredWidth*3,
		                     *rgb = m_rgb + y*3;
		while( ptr < ptrEnd )
		{
			*ptr++ = rgb[0];
			*ptr++ = rgb[1];
			*ptr++ = rgb[2];
		}
	}

	return m_tempLine;
}


/*******************************************************************************
 * SjVisImgBg
 ******************************************************************************/


SjVisImgBg::SjVisImgBg(const wxString& url, long imgOpFlags)
{
	m_scaledWidth = 0;
	m_scaledHeight = 0;
	m_scaledBitmap = NULL;
	m_imgOpFlags = imgOpFlags;

	wxFileSystem fs;
	wxFSFile* fsFile = fs.OpenFile(url, wxFS_READ|wxFS_SEEKABLE);
	if( fsFile )
	{
		m_orgImage.LoadFile(*(fsFile->GetStream()), wxBITMAP_TYPE_ANY);
		delete fsFile;
	}
}


void SjVisImgBg::SetSize(const wxSize& newSize)
{
	wxASSERT( wxThread::IsMain() );

	if( (newSize.x == m_scaledWidth && newSize.y == m_scaledHeight)
	 || !m_orgImage.IsOk() )
		return;

	m_scaledImage = m_orgImage.Copy();

	if( m_imgOpFlags & SJ_IMGOP_GRAYSCALE )
		SjImgOp::DoGrayscale(m_scaledImage);

	SjImgOp::DoResize(m_scaledImage, newSize.x, newSize.y, m_imgOpFlags);

	m_scaledWidth = newSize.x;
	m_scaledHeight = newSize.y;

	if( m_scaledBitmap )
	{
		delete m_scaledBitmap;
		m_scaledBitmap = NULL;
	}
}


void SjVisImgBg::DrawBackground(wxDC& dc)
{
	wxASSERT( wxThread::IsMain() );

	if( m_scaledImage.IsOk() )
	{
		if( m_scaledBitmap == NULL )
		{
			m_scaledBitmap = new wxBitmap(m_scaledImage);
		}

		if( m_scaledBitmap->IsOk() )
		{
			dc.DrawBitmap(*m_scaledBitmap, 0, 0);
			return;
		}
	}

	// error
	wxSize size = dc.GetSize();
	dc.SetBrush(*wxBLACK_BRUSH);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, size.x, size.y);
}


const unsigned char* SjVisImgBg::GetBackgroundBits(long x, long y, long desiredWidth)
{
	wxASSERT( wxThread::IsMain() );

	if( x+desiredWidth < m_scaledWidth && y < m_scaledHeight )
	{
		return m_scaledImage.GetData() + (y*m_scaledWidth+x) * 3;
	}

	AllocTempLine(desiredWidth);
	memset(m_tempLine, 0, desiredWidth*3);

	return m_tempLine;
}


/*******************************************************************************
 * Some Tools
 ******************************************************************************/


wxCoord SjVisBg::SetFontPixelH(wxDC& dc, wxFont& font, wxCoord pixelH, wxCoord fontPtSize)
{
	wxCoord w, h;

	while( 1 )
	{
		font.SetPointSize(fontPtSize);
		dc.SetFont(font);
		dc.GetTextExtent(wxT("Ag"), &w, &h);
		if( h<=pixelH || fontPtSize<=6 )
		{
			break;
		}
		fontPtSize--;
	}

	return fontPtSize;
}


wxCoord SjVisBg::SetFontPixelH(wxDC& dc, wxFont& font,
                               const wxSize& sizeToFit, const wxString& strToFit)
{
	wxCoord fontPtSize = 64, w, h;

	while( 1 )
	{
		font.SetPointSize(fontPtSize);
		dc.SetFont(font);
		dc.GetTextExtent(strToFit, &w, &h);
		if( (h<=sizeToFit.y && w<=sizeToFit.x) || fontPtSize<=6 )
		{
			break;
		}
		fontPtSize--;
	}

	return fontPtSize;
}
