/*********************************************************************************
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
 * File:    skin.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke skins
 *
 *******************************************************************************
 *
 * We have an official Apple creator code for Silverjuke Skin *.sjs files ...
 * Application: Silverjuke
 * Application Signatures:
 * SjSk  (Hex) 536A536B
 * ...however, I'n not sure if we really need this.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjbase/skin.h>
#include <sjbase/skinml.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/kiosk/virtkeybd.h>
#include <sjmodules/vis/vis_bg.h>
#include <see_dom/sj_see.h>
#include <wx/display.h>

#include <wx/listimpl.cpp> // sic!
WX_DEFINE_LIST(SjSkinImageList);
WX_DEFINE_LIST(SjSkinItemList);
WX_DEFINE_LIST(SjSkinLayoutList);


/*******************************************************************************
 * SjSkinItem
 ******************************************************************************/


SjSkinItem::SjSkinItem()
{
	m_timer                 = NULL;
	m_itemType              = SJ_UNKNOWNITEM;
	m_targetId              = IDT_NONE;
	m_userId                = NULL;
	#ifdef SJ_SKIN_USE_HIDE
	m_hidden                = FALSE;
	#endif
	#ifdef SJ_SKIN_USE_BELONGSTO
	m_belongsToId           = IDT_NONE;
	#endif
	m_targetFlags           = 0;
	m_targetName            = NULL;
	m_doubleClickTargetId   = IDT_NONE;
	m_doubleClickTargetName = NULL;
	m_usesMouse             = TRUE;
	m_usesPaint             = TRUE;
	m_image                 = NULL;
	m_alwaysRedrawBackground= FALSE;
	m_skinWindow            = NULL;
	m_colours               = NULL; // set to SjSkinSkin::m_itemDefColours()
	m_parent                = NULL;
	m_prop                  = 0;
	m_itemTooltip           = NULL;
}


SjSkinItem::~SjSkinItem()
{
	if( m_timer )
		delete m_timer;

	if( m_userId )
		delete m_userId;

	if( m_targetName )
		delete m_targetName;

	if( m_doubleClickTargetName )
		delete m_doubleClickTargetName;

	if( m_itemTooltip )
		delete m_itemTooltip;
}


bool SjSkinItem::Create(const wxHtmlTag& tag, wxString& error)
{
	// this function should not be called!
	wxASSERT_MSG(0, wxT("SjSkinItem::Create() is a virtual function and should only be called for derived objects!"));
	return FALSE;
}


void SjSkinItem::SetValue(const SjSkinValue& value)
{
	// for derived classes
}


void SjSkinItem::CreateTimer(long ms)
{
	if( ms > 0 )
	{
		if( m_timer == NULL )
		{
			m_timer = new SjSkinItemTimer(this);
			if( m_timer == NULL )
				return;
		}

		m_timer->Start(ms);
	}
	else
	{
		if( m_timer )
		{
			delete m_timer;
			m_timer = 0;
		}
	}
}


bool SjSkinItem::CheckImage(wxString& error)
{
	if( !m_image )
	{
		error = wxT("Image not found.")/*n/t*/;
		return FALSE;
	}

	return TRUE;
}


bool SjSkinItem::CheckTarget(wxString& error)
{
	if( !m_targetId )
	{
		// error = "Target not found."/*n/t*/; -- don't print an error as the targets may be expanded in future versions
		return FALSE;
	}

	return TRUE;
}


void SjSkinItem::OnSize()
{
	// for derived classes
}


void SjSkinItem::OnMouseLeftDown(long x, long y, bool doubleClick, long accelFlags)
{
	// for derived classes
}


SjMouseUsed SjSkinItem::OnMouseLeftUp(long x, long y, long accelFlags, bool captureLost)
{
	// for derived classes
	return SJ_MOUSE_NOT_USED; // don't send a delayed event
}


void SjSkinItem::OnMouseMotion(long x, long y, bool leftDown)
{
	// for derived classes
}


void SjSkinItem::OnMouseLeave()
{
	// for derived classes
}


void SjSkinItem::OnTimer()
{
	// for derived classes
}


void SjSkinItem::OnPaint(wxDC& dc)
{
	// for derived classes
	wxASSERT_MSG(0, wxT("SjSkinItem::OnPaint() must be implemented in derived classes."));
}


wxRect SjSkinItem::GetScreenRect() const
{
	wxRect scrRect = m_rect;
	m_skinWindow->ClientToScreen(&scrRect.x, &scrRect.y);
	return scrRect;
}


static long s_globalDragImageHidden = 0;
void SjSkinItem::HideDragImage()
{
	if( m_skinWindow->m_dragImage || s_globalDragImageHidden )
	{
		if( s_globalDragImageHidden == 0
		 && m_skinWindow->m_dragRect.Intersects(GetScreenRect()) )
		{
			m_skinWindow->m_dragImage->Hide();
		}

		// always increase the "hidden" counter - even if the drag image is not hidden at the moment;
		// this is needed as recursive paiting calls may go offscreen and have no chance to compare
		// their rectangle against any screen coordinated.  the very outer call should
		// make sure to use screen coordinates!
		s_globalDragImageHidden++;
	}
}


void SjSkinItem::ShowDragImage()
{
	if( s_globalDragImageHidden )
	{
		s_globalDragImageHidden--;
		if( s_globalDragImageHidden == 0
		        && m_skinWindow->m_dragImage )
		{
			m_skinWindow->m_dragImage->Show();
		}
	}
}


void SjSkinItem::RedrawMe()
{
	wxASSERT(m_skinWindow);
	wxClientDC dc(m_skinWindow);

	// anything to redraw?
	if( m_rect.width == 0 || m_rect.height == 0
	 || m_skinWindow == NULL || m_skinWindow->m_currLayout == NULL /*see https://mail.google.com/a/b44t.com/#all/11fa3299de85ce8f*/ )
	{
		return;
	}

	// check if there are items ABOVE this item
	if( m_hasOverlayingItems==-1 /*don't know yet*/ )
	{
		m_hasOverlayingItems = 0;
		bool                     thisItemFound = FALSE;
		SjSkinItem*           item;
		SjSkinItemList::Node* itemnode = m_skinWindow->m_currLayout->m_itemList.GetFirst();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			if( thisItemFound )
			{
				if( m_rect.Intersects(item->m_rect) )
				{
					m_hasOverlayingItems = 1;
					break;
				}
			}
			else if( item == this )
			{
				thisItemFound = TRUE;
			}

			itemnode = itemnode->GetNext();
		}
	}

	// draw...
	if( m_alwaysRedrawBackground
	#ifdef SJ_SKIN_USE_HIDE
	 || m_hidden
	#endif
	 || m_hasOverlayingItems )
	{
		// ...the item needs the background to be repainted first (maybe it
		// uses a mask) or the item has overlaying items.  In any case, we'll
		// paint several items in the item rect...
		bool drawDone = FALSE;

		// ...drawing offscreen to avoid flickering
		wxBitmap    memBitmap(m_rect.width, m_rect.height);
		wxMemoryDC  memDc;
		memDc.SelectObject(memBitmap);
		if( memDc.IsOk() )
		{
			HideDragImage();
			m_skinWindow->RedrawAll(memDc, &m_rect, 0-m_rect.x, 0-m_rect.y);
			m_skinWindow->RedrawFinalLines(memDc, 0-m_rect.x, 0-m_rect.y);
			dc.Blit(m_rect.x, m_rect.y, m_rect.width, m_rect.height, &memDc, 0, 0);
			ShowDragImage();
			drawDone = TRUE;
		}

		if( !drawDone )
		{
			// ...drawing onscreen, faster but with flickering
			dc.DestroyClippingRegion();
			dc.SetClippingRegion(m_rect);
			HideDragImage();
			m_skinWindow->RedrawAll(dc, &m_rect);
			m_skinWindow->RedrawFinalLines(dc);
			ShowDragImage();
		}
	}
	else
	{
		// ...very good and very fast: the item does not need the background
		// to be drawn first and the item has no overlaying items. We can
		// just call the drawing function.
		HideDragImage();
		OnPaint(dc);
		m_skinWindow->RedrawFinalLines(dc);
		ShowDragImage();
	}
}


/*******************************************************************************
 * SjSkinBoxItem - Constructor and Misc.
 ******************************************************************************/


bool SjSkinBoxItem::Create(const wxHtmlTag& tag, wxString& error)
{
	wxString fontFace = SJ_DEF_FONT_FACE;
	if( tag.HasParam(wxT("FONT")) )
	{
		wxArrayString tryFaces = SjTools::Explode(tag.GetParam(wxT("FONT")), wxT(','), 1);
		for( size_t t = 0; t < tryFaces.GetCount(); t++ )
		{
			wxString tryFace = tryFaces.Item(t).Trim(true).Trim(false);
			if( g_tools->HasFacename(tryFace) )
			{
				fontFace = tryFace;
				break;
			}
		}
	}

	m_runningMs     = 0;
	m_totalMs       = 0;
	m_flags         = 0;
	m_font          = wxFont(10/*size changed later*/, wxSWISS, wxNORMAL, wxNORMAL, FALSE, fontFace);
	m_fontHeight    = 0;

	if( tag.HasParam(wxT("TEXT")) )
	{
		m_text = tag.GetParam(wxT("TEXT"));
	}

	m_border = FALSE;
	int test;
	if( tag.GetParamAsInt(wxT("BORDER"), &test)
	        && test )
	{
		m_border = TRUE;
	}

	m_centerOffset = 0;
	if( tag.GetParamAsInt(wxT("CENTEROFFSET"), &test)
	        && test )
	{
		m_centerOffset = test;
	}

	if( m_targetId == IDT_CURR_CREDIT )
	{
		int test;
		if( tag.GetParamAsInt(wxT("HIDECREDITINDISPLAY"), &test) )
		{
			if( test == 1 )
				m_prop |= SJ_SKIN_PROP_HIDE_CREDIT_IN_DISPLAY;
		}
	}


	wxString dummy;
	if( !CheckTarget(dummy) )
	{
		m_usesMouse = FALSE;
	}

	m_itemType = SJ_BOXITEM;

	m_mouseSubitem  = SJ_SUBITEM_NONE;
	m_mouseDownX    = -10000;
	m_mouseDownY    = -10000;
	m_mouseState    = SJ_MOUSE_STATE_NORMAL;

	m_alwaysRedrawBackground = !m_colours[SJ_COLOUR_NORMAL].bgSet;


	if( !m_width.IsSet() )
	{
		m_width.SetAbs(16);
	}

	if( !m_height.IsSet() )
	{
		m_height.SetAbs(16);
	}

	return TRUE;
}


void SjSkinBoxItem::SetValue(const SjSkinValue& value)
{
	long flags = value.value;
	long runningMs = (flags&SJ_VFLAG_VMIN_IS_TIME)? value.vmin : 0;
	long totalMs = (flags&SJ_VFLAG_VMAX_IS_TIME)? value.vmax : 0;

	if( m_runningMs != runningMs
	 || m_totalMs != totalMs
	 || m_flags != flags
	 || m_text != value.string )
	{
		m_text = value.string;
		m_flags = flags;
		m_runningMs = runningMs;
		m_totalMs = totalMs;
		RedrawMe();
	}
}


int SjSkinBoxItem::FindSubitem(long x, long y, wxRect& subitemRect)
{
	subitemRect = m_rect;
	subitemRect.height--;

	if( x >= m_rect.x
	        && y >= m_rect.y
	        && x < (m_rect.x + m_rect.width)
	        && y < (m_rect.y + m_rect.height) )
	{
		if( x > m_rect.x+m_timeXrel
		        && x < m_rect.x+m_timeXrel+m_timeW
		        && m_flags & SJ_VFLAG_TIME_CLICKABLE )
		{
			subitemRect.x = m_rect.x+m_timeXrel;
			subitemRect.width = m_timeW;
			return SJ_SUBITEM_TIME;
		}
		else if( x > m_rect.x+m_iconRightXrel
		         && x < m_rect.x+m_iconRightXrel+m_iconRightW  )
		{
			subitemRect.x = m_rect.x+m_iconRightXrel;
			subitemRect.width = m_iconRightW;
			return SJ_SUBITEM_ICONRIGHT;
		}
		else if( x < m_rect.x+m_iconLeftW )
		{
			subitemRect.width = m_iconLeftW;
			return /*((m_flags&SJ_VFLAG_ICON_MASK) != SJ_VFLAG_ICON_EMPTY)? */SJ_SUBITEM_ICONLEFT /*: SJ_SUBITEM_NONE*/;
		}
		else
		{
			subitemRect.x += m_iconLeftW;
			subitemRect.width -= m_timeW+m_iconRightW;
			return SJ_SUBITEM_TEXT;
		}
	}
	else
	{
		return SJ_SUBITEM_NONE;
	}
}


bool SjSkinBoxItem::OnImageThere(wxDC& dc, SjImgThreadObj* obj)
{
	bool ret = false;

	if( (m_flags & SJ_VFLAG_STRING_IS_IMAGE_URL)
	 && obj->m_url == m_text )
	{
		bool drawBg = true;

		if( obj->IsOk()
		 && obj->GetWidth() == m_rect.width
		 && obj->GetHeight() == m_rect.height )
		{
			wxBitmap* bitmap = obj->CreateBitmap();
			if( bitmap )
			{
				dc.DrawBitmap(*bitmap, m_rect.x, m_rect.y, FALSE/*not transparent*/);
				delete bitmap;
				drawBg = false;
			}
		}

		if( drawBg )
		{
			DrawBackground(dc);
		}

		ret = true;
	}

	return ret;
}


/*******************************************************************************
 * SjSkinBoxItem - Drawing
 ******************************************************************************/


void SjSkinBoxItem::DrawBackground(wxDC& dc)
{
	if( (m_border && m_colours[SJ_COLOUR_NORMAL].fgSet)
	 ||  m_colours[SJ_COLOUR_NORMAL].bgSet )
	{
		dc.SetPen((m_border&&m_colours[SJ_COLOUR_NORMAL].fgSet)? m_colours[SJ_COLOUR_NORMAL].fgPen : *wxTRANSPARENT_PEN);
		dc.SetBrush(m_colours[SJ_COLOUR_NORMAL].bgSet? m_colours[SJ_COLOUR_NORMAL].bgBrush : *wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(m_rect.x, m_rect.y, m_rect.width, m_rect.height);
	}
}


void SjSkinBoxItem::DrawIcon(wxDC& dc, const wxRect& rect, long icon, bool selected)
{
	SjSkinColour* base = &m_colours[selected? SJ_COLOUR_SELECTION : SJ_COLOUR_NORMAL];
	if( base->hiSet )
	{
		dc.SetPen(base->hiPen);

		wxRect rectHilite(rect);
		rectHilite.x++;
		SjTools::DrawIcon(dc, rectHilite, icon);
	}

	dc.SetPen(base->fgPen);
	SjTools::DrawIcon(dc, rect, icon);
}


void SjSkinBoxItem::DrawText(wxDC& dc, const wxString& text, int x, int y, bool selected)
{
	SjSkinColour* base = &m_colours[selected? SJ_COLOUR_SELECTION : SJ_COLOUR_NORMAL];
	if( base->hiSet )
	{
		dc.SetTextForeground(base->hiColour);
		dc.DrawText(text, x+base->offsetX, y+base->offsetY);
	}

	dc.SetTextForeground(base->fgColour);
	dc.DrawText(text, x, y);
}


void SjSkinBoxItem::DrawText(wxDC& dc)
{
	wxCoord w, h;

	// change font if needed
	if( m_fontHeight != m_rect.height )
	{
		m_fontHeight = m_rect.height;
		int fontPtSize = 32;
		while( 1 )
		{
			m_font.SetPointSize(fontPtSize);
			dc.SetFont(m_font);
			dc.GetTextExtent(wxT("Ag"), &w, &h);
			dc.SetFont(*wxNORMAL_FONT);
			if( h<=m_fontHeight || fontPtSize<=6 )
			{
				break;
			}
			fontPtSize--;
		}
	}

	// draw background if needed
	DrawBackground(dc);

	// set clipping
	#ifdef SJ_BOXTEXT_WITH_CLIPPING
		wxRect oldClipping;
		dc.GetClippingBox(&oldClipping.x, &oldClipping.y, &oldClipping.width, &oldClipping.height);
		dc.DestroyClippingRegion();
		dc.SetClippingRegion(m_rect);
	#endif

	// set font
	dc.SetFont(m_font);

	// init rect
	wxRect drawRect = m_rect;

	// draw icons
	if( m_flags & SJ_VFLAG_ICONL_MASK )
	{
		wxRect rect1(drawRect);
		rect1.width = rect1.height;
		int sub = rect1.width/7;
		rect1.width -= sub*2;

		wxRect rect2(rect1);
		rect2.y += sub;
		rect2.height -= sub*2;

		// left icon
		bool hilite = (m_mouseSubitem==SJ_SUBITEM_ICONLEFT && m_mouseState==SJ_MOUSE_STATE_CLICKED);
		switch( m_flags & SJ_VFLAG_ICONL_MASK )
		{
			case SJ_VFLAG_ICONL_PLAY:       DrawIcon(dc, rect2, SJ_DRAWICON_PLAY, hilite);          break;
			case SJ_VFLAG_ICONL_PAUSE:      DrawIcon(dc, rect2, SJ_DRAWICON_PAUSE, hilite);         break;
			case SJ_VFLAG_ICONL_STOP:       DrawIcon(dc, rect2, SJ_DRAWICON_STOP, hilite);          break;
			case SJ_VFLAG_ICONL_PLAYED:     DrawIcon(dc, rect2, SJ_DRAWICON_CHECK, hilite);         break;
			case SJ_VFLAG_ICONL_ERRONEOUS:  DrawIcon(dc, rect2, SJ_DRAWICON_DELETE, hilite);        break;
			case SJ_VFLAG_ICONL_MOVED_DOWN: DrawIcon(dc, rect2, SJ_DRAWICON_MOVED_DOWN, hilite);    break;
			case SJ_VFLAG_ICONL_VOLDOWN:    DrawIcon(dc, rect2, SJ_DRAWICON_VOLDOWN, hilite);       break;
			default:                                                                                break;
		}

		m_iconLeftW = rect1.width;

		drawRect.x += m_iconLeftW;
		drawRect.width -= m_iconLeftW;

		// right icon
		rect2.x = (drawRect.x + drawRect.width) - rect1.width;
		if( m_flags & SJ_VFLAG_ICONR_MASK )
		{
			hilite = (m_mouseSubitem==SJ_SUBITEM_ICONRIGHT && m_mouseState==SJ_MOUSE_STATE_CLICKED);
			switch( m_flags & SJ_VFLAG_ICONR_MASK )
			{
				case SJ_VFLAG_ICONR_DELETE:  DrawIcon(dc, rect2, SJ_DRAWICON_DELETE, hilite);   break;
				case SJ_VFLAG_ICONR_VOLUP:   DrawIcon(dc, rect2, SJ_DRAWICON_VOLUP, hilite);    break;
				case SJ_VFLAG_ICONR_VOLDOWN: DrawIcon(dc, rect2, SJ_DRAWICON_VOLDOWN, hilite);  break;
			}

			m_iconRightW    = rect1.width + sub*2;
			m_iconRightXrel = (rect2.x - sub*2) - m_rect.x;

			drawRect.width -= m_iconRightW;
		}
	}

	// draw time
	if( m_flags & SJ_VFLAG_VMIN_IS_TIME )
	{
		// calculate time string
		wxString playtimeMs = SjTools::FormatTime(m_runningMs<0? -1 : m_runningMs/1000, SJ_FT_ALLOW_ZERO);

		if( m_flags & SJ_VFLAG_VMIN_MINUS)
		{
			playtimeMs.Prepend(wxT('-'));
		}

		if( m_flags & SJ_VFLAG_VMAX_IS_TIME )
		{
			playtimeMs.Append(wxT(" / ") + SjTools::FormatTime(m_totalMs<=0? -1 : m_totalMs/1000));
		}

		// draw time string if it fits
		dc.GetTextExtent(playtimeMs, &m_timeW, &h);
		if( m_timeW < drawRect.width/2 )
		{
			DrawText(dc, playtimeMs, drawRect.x + drawRect.width - m_timeW, drawRect.y,
			         (m_flags&SJ_VFLAG_BOLD) || (m_mouseState==SJ_MOUSE_STATE_CLICKED));

			m_timeXrel = (drawRect.x + drawRect.width - m_timeW) - m_rect.x;
			drawRect.width -= m_timeW+8;
		}
	}

	// draw image or text
	#define POINTS_STRING wxT("..")
	#define HIVENT_STRING wxT(" - ")
	if( m_flags & SJ_VFLAG_CENTER )
	{
		// draw centered text - you may give two alternative texts  as "longer text\tshort text"
		wxString text1 = m_text.BeforeFirst('\t');
		wxCoord points1w = 0, quoter1w = 0, text1w;
		bool shortTextTried = FALSE;
		wxRect rect1(drawRect);

		if( !(m_flags & SJ_VFLAG_IGNORECENTEROFFSET) )
		{
			if( m_centerOffset < 0 ) { rect1.width -= m_centerOffset*-2; } else { rect1.x += m_centerOffset; rect1.width -= m_centerOffset; }
		}

		while( 1 )
		{
			dc.GetTextExtent(text1, &text1w, &h);
			if( (text1w + points1w + quoter1w) <= rect1.width
			        || text1.IsEmpty() )
			{
				// string that fits found; append points and
				// quotes if needed
				w = text1w;
				if( points1w )
				{
					w += points1w;
					text1.Append(POINTS_STRING);
				}
				if( quoter1w && ((text1.Freq('"')%2)==1) )
				{
					w += quoter1w;
					text1.Append('"');
				}

				DrawText(dc, text1, rect1.x + rect1.width/2 - w/2, rect1.y, FALSE);
				break; // done
			}
			else if( !shortTextTried && m_text.Find('\t')!=-1 )
			{
				text1 = m_text.AfterFirst('\t');
				shortTextTried = TRUE;
			}
			else
			{
				if( m_text.Last()=='"' )
				{
					// persever last quotes, a string like 'Search in "Random Selection"'
					// will become 'Search in "Random Sele.."'
					if( quoter1w==0 ) dc.GetTextExtent(POINTS_STRING, &quoter1w, &h);
				}

				text1.Truncate(text1.Len()>2? text1.Len()-2 : 0);
				text1.Trim();
				if( points1w==0 ) dc.GetTextExtent(POINTS_STRING, &points1w, &h);
			}
		}
	}
	else
	{
		// draw left-aligned text. the text may be divided by a TAB which
		// will be replaced by " - "; the text BEFORE and AFTER text text may
		// be truncated
		wxString text1(m_text.BeforeFirst('\t').Trim());
		wxString text2(m_text.AfterFirst('\t').Trim());
		wxCoord points1w = 0, points2w = 0, hiventW = 0, text1w, text2w;
		if( !text2.IsEmpty() ) dc.GetTextExtent(HIVENT_STRING, &hiventW, &h);
		while( 1 )
		{
			dc.GetTextExtent(text1, &text1w, &h);
			dc.GetTextExtent(text2, &text2w, &h);
			if( (text1w + points1w + hiventW + text2w + points2w) <= drawRect.width
			        || (text1.IsEmpty() && text2.IsEmpty()) )
			{
				if( points1w ) text1.Append(POINTS_STRING);
				if( !text2.IsEmpty() )
				{
					text1.Append(HIVENT_STRING);
					text1.Append(text2);
					if( points2w ) text1.Append(POINTS_STRING);
				}

				// draw!
				DrawText(dc, text1, drawRect.x, drawRect.y,
				         (m_flags&SJ_VFLAG_BOLD) || (m_mouseState==SJ_MOUSE_STATE_CLICKED && m_mouseSubitem!=SJ_SUBITEM_TIME));
				break; // done
			}

			if( text1.Len() > text2.Len() )
			{
				text1.Truncate(text1.Len() - (points1w? 1 : 3));
				text1.Trim();
				if( points1w==0 ) dc.GetTextExtent(POINTS_STRING, &points1w, &h);
			}
			else
			{
				text2.Truncate(text2.Len() - (points2w? 1 : 3));
				text2.Trim();
				if( points2w==0 ) dc.GetTextExtent(POINTS_STRING, &points2w, &h);
			}
		}
	}

	// strike?
	if(  m_mouseSubitem==SJ_SUBITEM_ICONRIGHT
	 && (m_flags&SJ_VFLAG_ICONR_DELETE) )
	{
		dc.SetPen(m_colours[SJ_COLOUR_NORMAL].fgPen);

		int strikeY = drawRect.y + drawRect.height/2;
		dc.DrawLine(m_rect.x+m_iconLeftW-2, strikeY, m_rect.x+m_iconRightXrel+2, strikeY);
	}

	// restore clipping/font
	#ifdef SJ_BOXTEXT_WITH_CLIPPING
		dc.DestroyClippingRegion();
		if( oldClipping.width && oldClipping.height )
		{
			dc.SetClippingRegion(oldClipping);
		}
	#endif

	// set normal font, release our font from DC
	dc.SetFont(*wxNORMAL_FONT);
}


void SjSkinBoxItem::DrawImage(wxDC& dc)
{
	wxASSERT(m_skinWindow);

	SjImgThread* imgThread = m_skinWindow->m_imgThread;
	if( !imgThread ) return;

	imgThread->RequireStart(m_skinWindow);
	SjImgOp op;
	op.LoadFromDb(m_text);
	op.m_flags |= SJ_IMGOP_RESIZE|SJ_IMGOP_SMOOTH;
	op.m_resizeW = m_rect.width;
	op.m_resizeH = m_rect.height;
	SjImgThreadObj* obj = imgThread->RequireImage(m_skinWindow, m_text, op);
	if( obj )
	{
		OnImageThere(dc, obj);

		imgThread->ReleaseImage(m_skinWindow, obj);
	}
	imgThread->RequireEnd(m_skinWindow);
}


void SjSkinBoxItem::OnPaint(wxDC& dc)
{
	m_iconLeftW     = 0;
	m_timeXrel      = -600000;
	m_timeW         = 0;
	m_iconRightXrel = -600000;
	m_iconRightW    = 0;

	if( m_flags & SJ_VFLAG_STRING_IS_IMAGE_URL )
	{
		DrawImage(dc);
	}
	else if( !m_text.IsEmpty() )
	{
		DrawText(dc);
	}
	else
	{
		DrawBackground(dc);
	}
}


/*******************************************************************************
 * SjSkinBoxItem - Mouse Handling
 ******************************************************************************/


void SjSkinBoxItem::OnMouseLeftDown(long x, long y, bool doubleClick, long accelFlags)
{
	if( m_mouseState != SJ_MOUSE_STATE_CLICKED )
	{
		wxRect subitemRect;
		int newSubitem = FindSubitem(x, y, subitemRect);
		if( newSubitem != SJ_SUBITEM_NONE )
		{
			m_mouseSubitem  = newSubitem;
			m_mouseDownX    = x;
			m_mouseDownY    = y;

			if( doubleClick
			 && m_mouseSubitem == SJ_SUBITEM_TEXT )
			{
				m_mouseState    = SJ_MOUSE_STATE_NORMAL;
				RedrawMe();

				SjSkinValue value;
				value.value = SJ_SUBITEM_TEXT_DCLICK;
				m_skinWindow->OnSkinTargetEvent(m_targetId, value, accelFlags);
			}
			else if( m_mouseSubitem == SJ_SUBITEM_TEXT )
			{
				m_mouseState    = SJ_MOUSE_STATE_CLICKED;

				SjSkinValue value;
				value.value = SJ_SUBITEM_TEXT_MOUSEDOWN;
				value.vmin  = accelFlags;
				m_skinWindow->OnSkinTargetEvent(m_targetId, value, accelFlags);

				RedrawMe();
			}
			else
			{
				m_mouseState    = SJ_MOUSE_STATE_CLICKED;
				RedrawMe();
			}
		}
	}
}


SjMouseUsed SjSkinBoxItem::OnMouseLeftUp(long x, long y, long accelFlags, bool captureLost)
{
	CreateTimer(0);

	int orgSubitem = m_mouseSubitem;
	SjMouseUsed ret = SJ_MOUSE_NOT_USED;  // don't send a delayed event

	m_mouseState   = SJ_MOUSE_STATE_NORMAL;
	m_mouseSubitem = SJ_SUBITEM_NONE;

	wxRect subitemRect;
	if( !captureLost && FindSubitem(x, y, subitemRect) == orgSubitem )
	{
		if( m_skinWindow->m_mouseInDisplayMove )
		{
			RedrawMe();

			ret = SJ_MOUSE_USED;
		}
		else if( orgSubitem == SJ_SUBITEM_TIME )
		{
			RedrawMe();
			SjSkinValue dummy;
			m_skinWindow->OnSkinTargetEvent(IDT_TOGGLE_TIME_MODE, dummy, accelFlags);

			ret = SJ_MOUSE_USED;
		}
		else if( orgSubitem != SJ_SUBITEM_NONE )
		{
			SjSkinValue value;
			value.value = orgSubitem;
			value.vmin  = accelFlags;
			m_skinWindow->OnSkinTargetEvent(m_targetId, value, accelFlags);
			RedrawMe();

			ret = SJ_MOUSE_USED;
		}
		else
		{
			RedrawMe();
		}
	}
	else
	{
		RedrawMe();
	}


	// always reset the cursor
	if( m_skinWindow->m_mouseInDisplayMove )
	{
		m_skinWindow->SetCursor(SjVirtKeybdModule::GetStandardCursor());
		m_skinWindow->m_mouseInDisplayMove = FALSE;
	}

	return ret;
}


bool SjSkinBoxItem::OnMouseMiddle(long x, long y)
{
	SjSkinValue value;
	value.value = SJ_SUBITEM_TEXT_MIDDLECLICK;
	m_skinWindow->OnSkinTargetEvent(m_targetId, value, 0);
	return TRUE;
}


void SjSkinBoxItem::OnMouseMotion(long x, long y, bool leftDown)
{
	wxRect          subitemRect;
	int             motionSubitem = FindSubitem(x, y, subitemRect);
	static bool     inHere = FALSE;
	if( !inHere )
	{
		inHere = TRUE;
		if( leftDown )
		{
			SjSkinItem* motionItem = m_skinWindow->FindClickableItem(x, y); // may be NULL!
			#define         MOVESTART_DELTA 4
			long            hDifference = x - m_mouseDownX;
			long            vDifference = y - m_mouseDownY;

			if( m_skinWindow->m_mouseInDisplayMove )
			{
				// report the derived class that the item is moved up/down if not yet done
				if(  motionItem != this
				 && !m_mouseMoveReported )
				{
					m_mouseResumeX = x;
					m_mouseResumeY = y;
					if( CheckMovementTimer() )
					{
						m_mouseMoveReported = TRUE;

						long lineDifference;
						if( m_targetId >= IDT_DISPLAY_LINE_FIRST
						 && m_targetId <= IDT_DISPLAY_LINE_LAST
						 && motionItem // may be NULL!
						 && motionItem->m_targetId >= IDT_DISPLAY_LINE_FIRST
						 && motionItem->m_targetId <= IDT_DISPLAY_LINE_LAST )
						{
							lineDifference = motionItem->m_targetId - m_targetId;
						}
						else
						{
							lineDifference = vDifference>0? 1 : -1;
						}

						if( motionItem )
						{
							CreateTimer(0);
						}

						m_skinWindow->OnSkinTargetMotion(m_targetId, lineDifference);
						m_timerLastMoveMs = SjTools::GetMsTicks();
						// normally, contined in (***)
					}
				}
				else
				{
					CreateTimer(0);
				}
			}
			else if( ( m_mouseSubitem == SJ_SUBITEM_TEXT )
			         && ( m_flags & SJ_VFLAG_MOVABLE )
			         && ( motionItem != this
			              || hDifference >  MOVESTART_DELTA
			              || hDifference < -MOVESTART_DELTA
			              || vDifference >  MOVESTART_DELTA
			              || vDifference < -MOVESTART_DELTA ) )
			{
				m_skinWindow->SetCursor(g_tools->m_staticMovehandCursor);
				m_skinWindow->m_mouseInDisplayMove = TRUE;
				m_mouseMoveReported = FALSE;
				m_timerLastMoveMs = 0;
			}
			else if( motionSubitem == m_mouseSubitem )
			{
				if( m_mouseState != SJ_MOUSE_STATE_CLICKED )
				{
					m_mouseState = SJ_MOUSE_STATE_CLICKED;
					RedrawMe();
				}
			}
			else
			{
				if( m_mouseState != SJ_MOUSE_STATE_NORMAL )
				{
					m_mouseState = SJ_MOUSE_STATE_NORMAL;
					RedrawMe();
				}
			}
		}
		else
		{
			if( motionSubitem != m_mouseSubitem
			        || m_mouseState != SJ_MOUSE_STATE_HOVER )
			{
				m_mouseSubitem  = motionSubitem;
				m_mouseState    = SJ_MOUSE_STATE_HOVER;
				RedrawMe();

				#if SJ_USE_TOOLTIPS
					if( m_mouseSubitem )
					{
						g_tools->m_toolTipManager.SetToolTipProvider(m_skinWindow->GetToolTipProvider(m_targetId, m_mouseSubitem, subitemRect));
					}
					else
					{
						g_tools->m_toolTipManager.ClearToolTipProvider();
					}
				#endif
			}
		}
		inHere = FALSE;
	}
}
void SjSkinWindow::ResumeSkinTargetMotion(int clickedTargetId, int resumeTargetId)
{
	// contined from (***)

	if( clickedTargetId >= IDT_DISPLAY_LINE_FIRST
	 && clickedTargetId <= IDT_DISPLAY_LINE_LAST
	 && resumeTargetId  >= IDT_DISPLAY_LINE_FIRST
	 && resumeTargetId  <= IDT_DISPLAY_LINE_LAST )
	{
		SjSkinBoxItem* clickedItem = (SjSkinBoxItem*)FindFirstItemByTargetId(clickedTargetId);
		SjSkinBoxItem* resumeItem  = (SjSkinBoxItem*)FindFirstItemByTargetId(resumeTargetId );
		if( clickedItem && resumeItem )
		{
			// set the clicked item to the normal state and give control
			// to the given resume target ID
			if( clickedItem == resumeItem )
			{
				resumeItem->m_mouseMoveReported = FALSE;
				if( resumeItem->m_timer == NULL )
				{
					resumeItem->CreateTimer(50);
					resumeItem->m_timerMoves = 0;
				}
			}
			else
			{
				resumeItem->m_mouseState        = clickedItem->m_mouseState;
				resumeItem->m_mouseSubitem      = clickedItem->m_mouseSubitem;
				resumeItem->m_mouseDownX        = clickedItem->m_mouseResumeX;
				resumeItem->m_mouseDownY        = clickedItem->m_mouseResumeY;
				resumeItem->m_mouseMoveReported = FALSE;

				clickedItem->m_mouseState       = SJ_MOUSE_STATE_NORMAL;
				clickedItem->m_mouseSubitem     = SJ_SUBITEM_NONE;
				clickedItem->m_mouseMoveReported= FALSE;

				m_mouseItem                     = resumeItem;
				clickedItem->CreateTimer(0);
				resumeItem->CreateTimer(0);
			}
		}
	}
}
void SjSkinBoxItem::OnTimer()
{
	OnMouseMotion(m_mouseResumeX, m_mouseResumeY, TRUE);
}
bool SjSkinBoxItem::CheckMovementTimer()
{
	unsigned long thisMs = SjTools::GetMsTicks();
	unsigned long timeout = 100;

	if( m_timer == NULL || thisMs >= m_timerLastMoveMs+timeout )
	{
		m_timerMoves++;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void SjSkinBoxItem::OnMouseLeave()
{
	if( m_mouseState == SJ_MOUSE_STATE_HOVER )
	{
		m_mouseState = SJ_MOUSE_STATE_NORMAL;
		m_mouseSubitem = SJ_SUBITEM_NONE;
		RedrawMe();
	}
}


/*******************************************************************************
 * SjSkinImageItem
 ******************************************************************************/


/* A horizontal image must have the following subimages:

    +----------+------------+----------+
    | prologue | repeatable | epilogue |
    |          | part       |          |
    +----------+------------+----------+

Prologue and epilogue may be skipped. A vertival image looks like:

    +------------+
    | prologue   |
    +------------+
    | repeatable |
    | part       |
    +------------+
    | epilogie   |
    +------------+

Simple Images have only one horizonzally- and vertically-repeatable subimage:

    +---------------+
    |  repeatable   |
    |  part         |
    +---------------+

Prologue and epilogue may be skipped. */


bool SjSkinImageItem::Create(const wxHtmlTag& tag, wxString& error)
{
	if( !CheckImage(error) )
	{
		return FALSE;
	}

	// hotizontal or vertical image?
	m_horizontal = TRUE;
	if( m_image->GetSubimageYCount() > m_image->GetSubimageXCount() )
	{
		m_horizontal = FALSE;
	}

	// get the image bitmaps
	if( m_image->GetSubimageXCount() == m_image->GetSubimageYCount() )
	{
		m_bitmapPrologue = NULL;
		m_bitmapRepeat = m_image->GetSubimage(0, 0); // never NULL
		m_bitmapEpilogue = NULL;
	}
	else
	{
		m_bitmapPrologue = m_image->GetSubimage(0, 0); // may be NULL
		if( m_horizontal )
		{
			m_bitmapRepeat = m_image->GetSubimage(1, 0); // never NULL
			m_bitmapEpilogue = m_image->GetSubimage(2, 0); // may be NULL
		}
		else
		{
			m_bitmapRepeat = m_image->GetSubimage(0, 1); // never NULL
			m_bitmapEpilogue = m_image->GetSubimage(0, 2); // may be NULL
		}
	}

	if( m_bitmapRepeat == NULL )
	{
		error = wxT("Invalid subimages. An image should have 1x1, 3x1 or 1x3 subimages.")/*n/t*/;
		return FALSE;
	}

	// init width, height and events
	if( !m_width.IsSet() )
	{
		m_width.SetAbs(m_bitmapRepeat->GetWidth());
	}

	if( !m_height.IsSet() )
	{
		m_height.SetAbs(m_bitmapRepeat->GetHeight());
	}

	m_usesMouse = FALSE;
	m_itemType = SJ_IMAGEITEM;

	return TRUE;
}


void SjSkinImageItem::OnPaint(wxDC& dc)
{
	wxASSERT(m_skinWindow);
	wxASSERT(g_tools);

	if( m_horizontal )
	{
		g_tools->DrawBitmapHBg(dc, m_bitmapPrologue, m_bitmapRepeat, m_bitmapEpilogue, m_rect);
	}
	else
	{
		g_tools->DrawBitmapVBg(dc, m_bitmapPrologue, m_bitmapRepeat, m_bitmapEpilogue, m_rect);
	}
}


/*******************************************************************************
 *  SjSkinButtonItem
 ******************************************************************************/


/* A button image can have the following subimages:

    +------------------+
    | normal           |
    +------------------+
    | normal hover     |
    +------------------+
    | normal clicked   |
    +------------------+
    | selected         |
    +------------------+
    | selected hover   |
    +------------------+
    | selected clicked |
    +------------------+
    | other states     |
    +------------------+
    |                  .
    ...

Most subimages may be skipped. The width and height defaults to the size of the
"normal" subimage. If a larger sizer is set later, the image will be centered
vertically and/or horizontally. */


#define BUTTON_BLINK_FREQ 990


bool SjSkinButtonItem::Create(const wxHtmlTag& tag, wxString& error)
{
	long            subimageX;

	if( !CheckImage(error) )
	{
		return FALSE;
	}

	CheckTarget(error);

	// is this a repeat button?
	m_useEventRepeating = m_targetFlags&SJ_TARGET_REPEATBUTTON? TRUE : FALSE;

	// get subimage index, check for [1]
	subimageX = m_imageIndex;
	if( !m_image->GetSubimage(subimageX, 0) )
	{
		error = wxString::Format(wxT("Invalid subimage-index %i.")/*n/t*/, (int)subimageX);
		return FALSE;
	}

	// get context menu width
	m_contextMenuWidth = 0;
	if( tag.HasParam(wxT("CMW")) )
	{
		tag.GetParamAsInt(wxT("CMW"), &m_contextMenuWidth);
	}

	// store events
	if( tag.HasParam(wxT("ONCLICK")) )
	{
		m_onclick = tag.GetParam(wxT("ONCLICK"));
	}

	// go through all possible button states
	int buttonState;
	for( buttonState = 0; buttonState < SJ_BUTTON_STATE_COUNT; buttonState++ )
	{
		// get normal button (never NULL, checked at [1])
		m_bitmaps[buttonState][0] = m_image->GetSubimage(subimageX, buttonState*SJ_BUTTON_STATE_COUNT+0);
		if( m_bitmaps[buttonState][0] == NULL )
		{
			m_bitmaps[buttonState][0] = m_bitmaps[0][0];
		}

		// get hover button (may be NULL)
		m_bitmaps[buttonState][1] = m_image->GetSubimage(subimageX, buttonState*SJ_BUTTON_STATE_COUNT+1);

		// get clicked button (never NULL)
		m_bitmaps[buttonState][2] = m_image->GetSubimage(subimageX, buttonState*SJ_BUTTON_STATE_COUNT+2);
		if( m_bitmaps[buttonState][2] == NULL )
		{
			m_bitmaps[buttonState][2] = m_bitmaps[buttonState][0];
		}
	}

	// set width and height if not yet done
	if( !m_width.IsSet() )
	{
		m_width.SetAbs(m_bitmaps[0][0]->GetWidth());
	}

	if( !m_height.IsSet() )
	{
		m_height.SetAbs(m_bitmaps[0][0]->GetHeight());
	}

	// set item clickable
	m_buttonState   = SJ_BUTTON_STATE_NORMAL;
	m_mouseState    = SJ_MOUSE_STATE_NORMAL;
	m_itemType      = SJ_BUTTONITEM;
	m_inTimer       = FALSE;
	m_delayedRedraw = FALSE;
	m_doBlink       = false;
	m_doBlinkState  = 0;

	return TRUE;
}


void SjSkinButtonItem::SetValue(const SjSkinValue& value)
{
	int newDoBlink = 0;
	if( (value.vmax&SJ_VMAX_BLINK) && value.value == 0 && !m_useEventRepeating )
		newDoBlink = 1;

	if( m_buttonState != value.value || m_delayedRedraw || m_doBlink != newDoBlink )
	{
		if( m_doBlink != newDoBlink )
		{
			m_doBlink = newDoBlink;
			m_doBlinkState = 0;
			if( m_doBlink )
				CreateTimer(BUTTON_BLINK_FREQ);
			else
				CreateTimer(0);
		}

		m_buttonState = value.value;
		m_delayedRedraw = FALSE;
		RedrawMe();
	}
}


void SjSkinButtonItem::OnMouseLeftDown(long x, long y, bool doubleClick, long accelFlags)
{
	if( m_mouseState != SJ_MOUSE_STATE_CLICKED )
	{
		m_mouseState = SJ_MOUSE_STATE_CLICKED;
		RedrawMe();

		m_ldownInsideContextMenu = FALSE;
		if( m_contextMenuWidth > 0
		        && x > (m_rect.x+m_rect.width)-m_contextMenuWidth )
		{
			m_ldownInsideContextMenu = TRUE;
		}

		if( m_useEventRepeating && !m_ldownInsideContextMenu )
		{
			m_subsequentTimer = FALSE;
			OnTimer();
		}
	}
}


SjMouseUsed SjSkinButtonItem::OnMouseLeftUp(long x, long y, long accelFlags, bool captureLost)
{
	wxASSERT(m_skinWindow);
	SjMouseUsed ret = SJ_MOUSE_NOT_USED;

	if( m_mouseState == SJ_MOUSE_STATE_CLICKED
	        || m_mouseState == SJ_MOUSE_STATE_HOVER )
	{
		// First, set mouse state to "normal".
		m_mouseState = SJ_MOUSE_STATE_NORMAL;

		// Then, stop timer or send event and
		// redraw the item.  Redrawing should be latest
		// to avoid button state flickering
		if( !captureLost && m_useEventRepeating && !m_ldownInsideContextMenu )
		{
			ret = SJ_MOUSE_USED;

			CreateTimer(0);
			RedrawMe();
		}
		else if( !captureLost && m_rect.Contains(x, y) )
		{
			ret = SJ_MOUSE_USED;

			if( (m_targetId >= IDT_WORKSPACE_GOTO_A && m_targetId <= IDT_WORKSPACE_GOTO_0_9)
			  || m_targetId == IDT_PLAY
			  || m_targetId == IDT_PAUSE
			  || m_targetId == IDT_STOP
			  || m_targetId == IDT_PREV
			  || m_targetId == IDT_NEXT
			  || m_targetId == IDT_WORKSPACE_ALBUM_VIEW
			  || m_targetId == IDT_WORKSPACE_COVER_VIEW
			  || m_targetId == IDT_WORKSPACE_LIST_VIEW )
			{
				m_delayedRedraw = TRUE; // Leave the button in the pressed state until the action
				// (which may take a second) is performed. The derived class
				// must call SetValue() to redraw the button in this case.
			}
			else
			{
				RedrawMe();
			}

			if( m_ldownInsideContextMenu )
			{
				m_skinWindow->OnSkinTargetContextMenu(m_targetId, m_rect.x, m_rect.y+m_rect.height);
			}
			else
			{
				ret = SJ_MOUSE_USE_DELAYED;

				if( !m_onclick.IsEmpty() )
				{
					#if SJ_USE_SCRIPTS
						SjSee* see = m_skinWindow->m_currSkin->m_see;

						see->ExecuteAsFunction(m_onclick);
						// ExecuteAsFunction() is a little hack as long as we have no real DOM for the skinning tree
						// ExecuteAsFunction() allows to use eg. "return false;" from the handler - a simple
						// Execute() would throw a "not in function" error.

						if( see->IsResultDefined() )
						{
							if( !see->GetResultLong() )
								ret = SJ_MOUSE_USED;

							if( ret == SJ_MOUSE_USED && m_delayedRedraw )
							{
								// no default processing - redraw now!
								m_delayedRedraw = false;
								RedrawMe();
							}
						}
					#else
                        wxLogError(wxT("Scripts are not supported in this build."));
					#endif
				}
			}
		}
		else
		{
			RedrawMe();
		}
	}

	return ret;
}


void SjSkinButtonItem::OnMouseMotion(long x, long y, bool leftDown)
{
	if( leftDown )
	{
		if( m_rect.Contains(x, y) )
		{
			if( m_mouseState != SJ_MOUSE_STATE_CLICKED )
			{
				m_mouseState = SJ_MOUSE_STATE_CLICKED;
				RedrawMe();

				if( m_useEventRepeating && !m_ldownInsideContextMenu )
				{
					m_subsequentTimer = FALSE;
					OnTimer();
				}
			}
		}
		else
		{
			if( m_mouseState == SJ_MOUSE_STATE_CLICKED )
			{
				m_mouseState = m_bitmaps[m_buttonState][SJ_MOUSE_STATE_HOVER]?
				               SJ_MOUSE_STATE_HOVER : SJ_MOUSE_STATE_NORMAL;
				RedrawMe();

				if( m_useEventRepeating && m_timer )
				{
					m_timer->Stop();
				}
			}
		}
	}
	else
	{
		if( m_mouseState != SJ_MOUSE_STATE_HOVER
		        && m_bitmaps[m_buttonState][SJ_MOUSE_STATE_HOVER] )
		{
			m_mouseState = SJ_MOUSE_STATE_HOVER;
			RedrawMe();
		}
	}
}


void SjSkinButtonItem::OnMouseLeave()
{
	if( m_mouseState == SJ_MOUSE_STATE_HOVER )
	{
		m_mouseState = SJ_MOUSE_STATE_NORMAL;
		RedrawMe();
	}
}


void SjSkinButtonItem::OnTimer()
{
	wxASSERT(m_skinWindow);

	if( !m_inTimer )
	{
		m_inTimer = TRUE;

		if( m_doBlink )
		{
			m_doBlinkState = m_doBlinkState? 0 : 1;
			RedrawMe();
		}
		else
		{
			if( m_timer )
			{
				m_timer->Stop();
			}

			SjSkinValue value;
			m_skinWindow->OnSkinTargetEvent(m_targetId, value, 0);

			CreateTimer(m_subsequentTimer? 50 : 500);
			m_subsequentTimer = TRUE;
		}

		m_inTimer = FALSE;
	}
}


void SjSkinButtonItem::OnPaint(wxDC& dc)
{
	// draw the button regarding the state of the button and the mouse
	wxASSERT(g_tools);
	wxASSERT(m_buttonState >= SJ_BUTTON_STATE_NORMAL && m_buttonState < SJ_BUTTON_STATE_COUNT);
	wxASSERT(m_mouseState >= SJ_MOUSE_STATE_NORMAL && m_mouseState < SJ_MOUSE_STATE_COUNT);

	int buttonState = m_buttonState;
	int mouseState = m_mouseState;

	if( m_doBlink )
	{
		buttonState = m_doBlinkState;
	}

	g_tools->DrawBitmap(dc, m_bitmaps[buttonState][mouseState], m_rect.x, m_rect.y, m_rect.width, m_rect.height);
}


/*******************************************************************************
 * SjSkinScrollbarItem
 ******************************************************************************/


/* An image for a vertival scrollbar looks like for following:

    +--------------+--------------+--------------+
    | page up      | page up      | page up      |
    | normal       | hover        | clicked      |
    +--------------+--------------+--------------+
    | thumb pro-   | thumb pro-   |              .
    | logue normal | logue hover  |              .
    +--------------+--------------+....... . . . .
    | thumb repeat-| thumb repeat-.              .
    | able part    | able part    .              .
    | normal       | hover        .
    +--------------+....... . . . . . .
    | thumb epi-   |              .
    | logue normal |
    +--------------+. . .
    | page down    |
    | normal       |
    +--------------+. .

The page up/down subimages must also be repeatable, the thumb will have a
minimal height of prologue+epiloge height. If the repeatable part of the
thumb is skipped, the thumb will have a fixed size.

An image for a horizontal scrollbar looks like:

    +--------+---------+------------+----------+--------+
    | page   | thumb   | thumb      | thumb    | page   |
    | left   | prologue| repeatable | epilogue | right  |
    | normal | normal  | normal     | normal   | normal |
    +--------+---------+------------+----------+--------+
    | page   |         |            .          .        .
    | left   .         .            .
    . hover  .
    ...... . . . . .
    . page   .
    . left
    . clicked
    .

Page left/right/up/down functionality is done by normal buttons if wanted. */


SjSkinScrollbarItem::SjSkinScrollbarItem()
{
	// init scrollbar values, we're doing this in the constructor
	// for save overwriting Create()
	m_value     = 0;
	m_vmin      = 0;
	m_vmax      = 0;
	m_vrange    = 0;
	m_thumbSize = 0;
	m_itemType  = SJ_SCROLLBARITEM;
	m_inTimer   = FALSE;

	m_hoverPart = NULL;
	m_trackPart = NULL;

	m_flip      = FALSE;
	m_horizontal= TRUE;

	m_hideIfUnused = FALSE;
	m_hideScrollbar = FALSE;
}


bool SjSkinScrollbarItem::Create(const wxHtmlTag& tag, wxString& error)
{
	// check parameters
	if( !CheckImage(error) )
	{
		return FALSE;
	}

	CheckTarget(error);

	int test;
	if( tag.GetParamAsInt(wxT("HIDEIFUNUSED"), &test) && test!=0 )
	{
		m_hideIfUnused = TRUE;
		m_hideScrollbar = TRUE;
	}


	// hotizontal or vertical scrollbar? flip scrollbar?
	if( m_image->GetSubimageYCount() >= 5 )
	{
		m_horizontal = FALSE;
	}

	if( !m_horizontal && (m_targetFlags&SJ_TARGET_NULLISBOTTOM) )
	{
		m_flip = TRUE;
	}

	// load the images
	int mouseState, scrollbarPart;
	for( mouseState = 0; mouseState < 3; mouseState++ )
	{
		scrollbarPart = 0; m_pageLeftPart.m_bitmapRepeat[mouseState]  = m_image->GetSubimage(m_horizontal? scrollbarPart : mouseState, m_horizontal? mouseState : scrollbarPart);
		scrollbarPart++;   m_thumbPart.m_bitmapPrologue[mouseState]   = m_image->GetSubimage(m_horizontal? scrollbarPart : mouseState, m_horizontal? mouseState : scrollbarPart);
		scrollbarPart++;   m_thumbPart.m_bitmapRepeat[mouseState]     = m_image->GetSubimage(m_horizontal? scrollbarPart : mouseState, m_horizontal? mouseState : scrollbarPart);
		scrollbarPart++;   m_thumbPart.m_bitmapEpilogue[mouseState]   = m_image->GetSubimage(m_horizontal? scrollbarPart : mouseState, m_horizontal? mouseState : scrollbarPart);
		scrollbarPart++;   m_pageRightPart.m_bitmapRepeat[mouseState] = m_image->GetSubimage(m_horizontal? scrollbarPart : mouseState, m_horizontal? mouseState : scrollbarPart);
	}

	if( /*m_pageLeftPart.m_bitmapRepeat[0] == NULL
     || */m_thumbPart.m_bitmapPrologue[0] == NULL )
	{
		error = wxT("A scrollbar should have 5x3 or 3x5 subimages. Thumb prologue may not be skipped.")/*n/t*/;
		return FALSE;
	}

	// correct the images: if "page right" is unset, use bitmap of "page left"
	if( m_pageRightPart.m_bitmapRepeat[0] == NULL )
	{
		m_pageRightPart.m_bitmapRepeat[0] = m_pageLeftPart.m_bitmapRepeat[0];
	}

	// correct the images: if there is no normal thumb repeat/epilogue, disable hover and clicked thumb repeat/epilogue
	if( !m_thumbPart.m_bitmapRepeat[0] )
	{
		m_thumbPart.m_bitmapRepeat[1] = NULL;
		m_thumbPart.m_bitmapRepeat[2] = NULL;
	}

	if( !m_thumbPart.m_bitmapEpilogue[0] )
	{
		m_thumbPart.m_bitmapEpilogue[1] = NULL;
		m_thumbPart.m_bitmapEpilogue[2] = NULL;
	}

	// correct the images: if unset, set the clicked states to the normal states if unset
	if( !m_pageLeftPart.m_bitmapRepeat[2] ) { m_pageLeftPart.m_bitmapRepeat[2] = m_pageLeftPart.m_bitmapRepeat[0]; }
	if( !m_thumbPart.m_bitmapPrologue[2] )  { m_thumbPart.m_bitmapPrologue[2]  = m_thumbPart.m_bitmapPrologue[0];  }
	if( !m_thumbPart.m_bitmapRepeat[2] )    { m_thumbPart.m_bitmapRepeat[2]    = m_thumbPart.m_bitmapRepeat[0];    }
	if( !m_thumbPart.m_bitmapEpilogue[2] )  { m_thumbPart.m_bitmapEpilogue[2]  = m_thumbPart.m_bitmapEpilogue[0];  }
	if( !m_pageRightPart.m_bitmapRepeat[2] ) { m_pageRightPart.m_bitmapRepeat[2]= m_pageRightPart.m_bitmapRepeat[0];}

	// set initial width and height
	long w, h;
	if( m_horizontal )
	{
		w = m_thumbPart.m_bitmapPrologue[0]->GetWidth() + (m_thumbPart.m_bitmapEpilogue[0]? m_thumbPart.m_bitmapEpilogue[0]->GetWidth() : 0);
		h = m_thumbPart.m_bitmapPrologue[0]->GetHeight();
		m_minThumbWidth = w;
	}
	else
	{
		w = m_thumbPart.m_bitmapPrologue[0]->GetWidth();
		h = m_thumbPart.m_bitmapPrologue[0]->GetHeight() + (m_thumbPart.m_bitmapEpilogue[0]? m_thumbPart.m_bitmapEpilogue[0]->GetHeight() : 0);
		m_minThumbWidth = h; // swap width/height
	}

	if( !m_width.IsSet() )
	{
		m_width.SetAbs(w);
	}


	if( !m_height.IsSet() )
	{
		m_height.SetAbs(h);
	}

	return TRUE;
}


void SjSkinScrollbarItem::SetValue(const SjSkinValue& value)
{
	wxASSERT(value.thumbSize >= 0);

	long newMin   = value.vmin;
	long newMax   = value.vmax;
	long newValue = value.value;

	// check max. agains min.
	if( newMax < newMin )
	{
		newMax = newMin;
	}

	// force value to be between min. and max.
	if( newValue > newMax )
	{
		newValue = newMax;
	}

	if( newValue < newMin )
	{
		newValue = newMin;
	}

	// calculate range, make value zero-based (min./max. stay unchanged)
	long newRange = newMax - newMin;
	newValue -= newMin;

	// flip?
	if( m_flip )
	{
		newValue = (newRange-value.thumbSize) - newValue;
	}

	// any changes?
	if( m_value     != newValue
	 || m_vmin      != newMin
	 || m_vmax      != newMax
	 || m_thumbSize != value.thumbSize )
	{
		// save settings
		m_value     = newValue;
		m_vmin      = newMin;
		m_vmax      = newMax;
		m_vrange    = newRange;
		m_thumbSize = value.thumbSize;

		//wxLogDebug("min=%i max=%i range=%i value=%i thumbSize=%i", (int)m_vmin, (int)m_vmax, (int)m_vrange, (int)m_value, (int)m_thumbSize);

		if( m_thumbSize > m_vrange )
		{
			m_thumbSize = m_vrange;
		}

		// redraw
		bool orgBgRedrawState = m_alwaysRedrawBackground;

		m_hideScrollbar = (m_hideIfUnused && newMin==newMax);
		if( m_hideScrollbar ) { m_alwaysRedrawBackground = TRUE; }

		OnSize();
		RedrawMe();

		m_alwaysRedrawBackground = orgBgRedrawState;
	}
}


void SjSkinScrollbarItem::OnSize()
{
	wxASSERT(m_skinWindow);

	// get total size
	if( m_horizontal )
	{
		m_allRect = m_rect;
	}
	else
	{
		m_allRect.x      = m_rect.y; // swap x/y and width/height
		m_allRect.y      = m_rect.x;
		m_allRect.width  = m_rect.height;
		m_allRect.height = m_rect.width;
	}

	// get size of thumb
	if( m_thumbPart.m_bitmapRepeat[0] )
	{
		if( m_vrange <= 0 )
		{
			m_thumbPart.m_rect.width = m_allRect.width;
		}
		else
		{
			m_thumbPart.m_rect.width = (m_thumbSize * m_allRect.width) / m_vrange;
		}

		if( m_thumbPart.m_rect.width < m_minThumbWidth )
		{
			m_thumbPart.m_rect.width = m_minThumbWidth;
		}
	}
	else
	{
		m_thumbPart.m_rect.width = m_minThumbWidth;
	}

	if( m_thumbPart.m_rect.width > m_allRect.width )
	{
		m_thumbPart.m_rect.width = m_allRect.width;
	}

	// calculate "page left" rect
	long rest = m_allRect.width - m_thumbPart.m_rect.width;

	if( m_vrange - m_thumbSize <= 0 )
	{
		m_pageLeftPart.m_rect.width = 0;
	}
	else
	{
		m_pageLeftPart.m_rect.width = (m_value * rest) / (m_vrange - m_thumbSize);
	}

	m_pageLeftPart.m_rect.x        = m_allRect.x;
	m_pageLeftPart.m_rect.y        = m_pageLeftPart.m_rect.width>0? m_allRect.y : -10000;
	m_pageLeftPart.m_rect.height   = m_allRect.height;

	// calculate "thumb" rect
	m_thumbPart.m_rect.x           = m_pageLeftPart.m_rect.x + m_pageLeftPart.m_rect.width;
	m_thumbPart.m_rect.y           = m_allRect.y;
	m_thumbPart.m_rect.height      = m_allRect.height;

	// calculate "page right" rect
	m_pageRightPart.m_rect.x       = m_thumbPart.m_rect.x + m_thumbPart.m_rect.width;
	m_pageRightPart.m_rect.y       = m_allRect.y;
	m_pageRightPart.m_rect.width   = m_allRect.x + m_allRect.width - m_pageRightPart.m_rect.x;
	m_pageRightPart.m_rect.height  = m_allRect.height;

	if( m_pageRightPart.m_rect.width <= 0 )
	{
		m_pageRightPart.m_rect = wxRect(-10000, -10000, 10, 10);
	}
}


void SjSkinScrollbarItem::OnMouseLeftDown(long x, long y, bool doubleClick, long accelFlags)
{
	if( m_hideScrollbar )
	{
		return;
	}

	if( m_trackPart == NULL )
	{
		m_trackPart = SwapNFindPart(x, y);
		if( m_trackPart )
		{
			if( m_trackPart == &m_thumbPart
			 || m_thumbSize )
			{
				if( m_hoverPart )
				{
					m_hoverPart->m_mouseState = 0;
					m_hoverPart = NULL;
				}

				m_trackPart->m_mouseState   = 2;
				m_trackStartValue           = m_value;
				m_trackStartX               = x;
				m_trackStartY               = y;

				RedrawMe();

				if( m_trackPart != &m_thumbPart )
				{
					m_subsequentTimer = FALSE;
					OnTimer();
				}
			}
			else
			{
				m_trackPart = 0;
			}
		}
	}
}


SjMouseUsed SjSkinScrollbarItem::OnMouseLeftUp(long x, long y, long accelFlags, bool captureLost)
{
	SjMouseUsed ret = SJ_MOUSE_NOT_USED;

	if( m_trackPart )
	{
		m_trackPart->m_mouseState   = 0;
		m_trackPart = NULL;

		if( !captureLost )
		{
			OnMouseMotion(x, y, FALSE);
		}

		CreateTimer(0);

		if( !m_hoverPart )
		{
			RedrawMe();
		}

		ret = SJ_MOUSE_USED;  // don't send a delayed event
	}

	return ret;
}


void SjSkinScrollbarItem::OnMouseMotion(long x, long y, bool leftDown)
{
	if( m_hideScrollbar )
	{
		return;
	}

	if( m_trackPart )
	{
		if( m_trackPart == &m_thumbPart )
		{
			long scrollX;

			if( m_horizontal )
			{
				scrollX = x - m_trackStartX;
			}
			else
			{
				scrollX = y - m_trackStartY;
			}

			if( (m_allRect.width - m_thumbPart.m_rect.width)<=0 )
			{
				return; // avoid division by zero
			}

			long scrollDiff = (scrollX*(m_vrange-m_thumbSize)) / (m_allRect.width - m_thumbPart.m_rect.width);

			CheckNSendValue(m_trackStartValue + scrollDiff);
		}
	}
	else
	{
		SjSkinScrollbarItemPart* newHoverPart = SwapNFindPart(x, y);
		if( newHoverPart != m_hoverPart )
		{
			bool doRedraw = FALSE;

			if( m_hoverPart )
			{
				m_hoverPart->m_mouseState = 0;
				m_hoverPart = 0;
				doRedraw = TRUE;
			}

			if( newHoverPart )
			{
				if( newHoverPart->m_bitmapRepeat[1] )
				{
					newHoverPart->m_mouseState = 1;
					m_hoverPart = newHoverPart;
					doRedraw = TRUE;
				}
			}

			if( doRedraw )
			{
				RedrawMe();
			}
		}
	}
}


void SjSkinScrollbarItem::OnMouseLeave()
{
	if( m_hoverPart )
	{
		m_hoverPart->m_mouseState = 0;
		m_hoverPart = 0;
		RedrawMe();
	}
}


void SjSkinScrollbarItem::OnTimer()
{
	if( m_trackPart == &m_pageLeftPart
	 || m_trackPart == &m_pageRightPart )
	{
		if( !m_inTimer )
		{
			m_inTimer = TRUE;

			if( m_timer )
			{
				m_timer->Stop();
			}

			if( m_trackPart == &m_pageLeftPart )
			{
				CheckNSendValue(m_value - m_thumbSize);
			}
			else
			{
				CheckNSendValue(m_value + m_thumbSize);
			}

			CreateTimer(m_subsequentTimer? 50 : 500);
			m_subsequentTimer = TRUE;

			m_inTimer = FALSE;
		}
	}
	else
	{
		CreateTimer(0);
	}
}


void SjSkinScrollbarItem::CheckNSendValue(long newValue)
{
	wxASSERT(m_skinWindow);

	if( newValue < 0 )
	{
		newValue = 0;
	}
	else if( newValue >= (m_vrange-m_thumbSize) )
	{
		newValue = m_vrange-m_thumbSize;
	}

	if( newValue != m_value )
	{
		SjSkinValue event;
		if( m_flip )
		{
			newValue = (m_vrange-m_thumbSize) - newValue;
		}
		event.value         = m_vmin + newValue;

		event.vmin          = m_vmin;
		event.vmax          = m_vmax;
		event.thumbSize     = m_thumbSize;
		m_skinWindow->OnSkinTargetEvent(m_targetId, event, 0);
	}
}


void SjSkinScrollbarItem::OnPaint(wxDC& dc)
{
	wxASSERT(m_skinWindow);

	if( !m_hideScrollbar )
	{
		SwapNDrawPart(dc, m_pageLeftPart);
		SwapNDrawPart(dc, m_thumbPart);
		SwapNDrawPart(dc, m_pageRightPart, TRUE);
	}
}


void SjSkinScrollbarItem::SwapNDrawPart(wxDC& dc, SjSkinScrollbarItemPart& part, bool alignMToR)
{
	wxASSERT(g_tools);

	if( part.m_bitmapPrologue[part.m_mouseState]
	 || part.m_bitmapRepeat  [part.m_mouseState]
	 || part.m_bitmapEpilogue[part.m_mouseState] )
	{
		if( m_horizontal )
		{
			g_tools->DrawBitmapHBg(dc,
			                       part.m_bitmapPrologue[part.m_mouseState],
			                       part.m_bitmapRepeat  [part.m_mouseState],
			                       part.m_bitmapEpilogue[part.m_mouseState],
			                       part.m_rect, alignMToR);
		}
		else
		{
			wxRect rect(part.m_rect.y, part.m_rect.x, part.m_rect.height, part.m_rect.width);

			g_tools->DrawBitmapVBg(dc,
			                       part.m_bitmapPrologue[part.m_mouseState],
			                       part.m_bitmapRepeat  [part.m_mouseState],
			                       part.m_bitmapEpilogue[part.m_mouseState],
			                       rect, alignMToR); // swap x/y and width/height
		}
	}
}


SjSkinScrollbarItemPart* SjSkinScrollbarItem::SwapNFindPart(long x, long y)
{
	Swap(x, y);

	if( m_thumbPart.m_rect.Contains(x, y) )
	{
		return &m_thumbPart;
	}
	else if( m_pageLeftPart.m_rect.Contains(x, y) )
	{
		return &m_pageLeftPart;
	}
	else if( m_pageRightPart.m_rect.Contains(x, y) )
	{
		return &m_pageRightPart;
	}

	return NULL;
}


/*******************************************************************************
 * SjSkinSliderItem
 ******************************************************************************/


/* A vertical slider looks like the following:

    +---------+---------+---------+---------+---------+... . .
    | thumb   | thumb   | thumb   | not     | not     | not
    | normal  | hover   | clicked | used    | used    . used
    +---------+---------+---------+---------+--.. . . . .
    | bg pro- | bg pro- | bg pro- | bg pro- | bg pro- .
    | logue 0 | logue 1 | logue 2 | logue 3 | logue 4 .
    +---------+---------+---------+---------+.... . . . .
    |         |         |         |         |         .
    |         |         |         |         |         .
    | bg      | bg      | bg      | bg      | bg      .
    | repeat 0| repeat 1| repeat 2| repeat 3| repeat 4
    |         |         |         |         |
    |         |         |         |         |
    +---------+---------+---------+---------+.. .
    | bg epi- | bg epi- | bg epi- | bg epi- |
    | logue 0 | logue 1 | logue 2 | logue 3 |
    +---------+---------+---------+---------+ .

The only clickable thing is the thumb.  Depending of the value
of the slider, one background is choosen - the first for the
minimal value, the middle for the middle value etc.  The hover,
clicked, repeat and epilogue subimages may be skipped; you have
to define at least one background.  A horizonzal slider looks
like:

    +---------+---------+-------------+---------+
    | thumb   | bg pro- |   bg        | bg epi- |
    | normal  | logue 0 |   repeat 0  | logue 0 |
    +---------+---------+-------------+---------+
    | thumb   | bg pro- |   bg        | bg epi- .
    | hover   | logue 1 |   repeat 1  | logue 1 .
    +---------+---------+...... . . . . .
    | thumb   | bg pro- .             .
    | clicked | logue 2 .
    +---------+. .
    | not     .
    . used

By the way, sliders and scrollbars have a compatible interface,
you can use one of these items for any scrolling/sliding item. */


/*******************************************************************************
 *  SjSkinWorkspaceItem
 ******************************************************************************/


bool SjSkinWorkspaceItem::Create(const wxHtmlTag& tag, wxString& error)
{
	m_targetId = IDT_WORKSPACE;

	// init width, height and events
	if( !m_width.IsSet() )
	{
		m_width.SetAbs(100);
	}

	if( !m_height.IsSet() )
	{
		m_height.SetAbs(100);
	}

	m_usesMouse = FALSE;
	m_usesPaint = FALSE;
	m_itemType  = SJ_WORKSPACEITEM;

	return TRUE;
}


void SjSkinWorkspaceItem::OnSize()
{
	wxASSERT(m_skinWindow);

	if( m_skinWindow->m_workspaceWindow && !m_skinWindow->m_workspaceMovedAway )
	{
		m_skinWindow->m_workspaceWindow->SetSize(m_rect);
	}
}


/*******************************************************************************
 * SjSkinInputItem
 ******************************************************************************/


bool SjSkinInputItem::Create(const wxHtmlTag& tag, wxString& error)
{
	m_targetId = IDT_SEARCH;

	if( !m_width.IsSet() )
	{
		m_width.SetAbs(64);
	}

	if( !m_height.IsSet() )
	{
		m_height.SetAbs(16);
	}

	m_usesMouse     = false;
	m_usesPaint     = false;
	m_itemType      = SJ_INPUTITEM;

	return TRUE;
}


void SjSkinInputItem::OnSize()
{
	wxASSERT(m_skinWindow);

	wxWindow* iw = m_skinWindow->m_inputWindow;
	if( iw )
	{
		// update colour
		if( m_skinWindow->m_inputWindowBgColour != m_colours[SJ_COLOUR_NORMAL].bgColour
		 || m_skinWindow->m_inputWindowFgColour != m_colours[SJ_COLOUR_NORMAL].fgColour )
		{
			iw->SetForegroundColour(m_colours[SJ_COLOUR_NORMAL].fgColour);
			iw->SetBackgroundColour(m_colours[SJ_COLOUR_NORMAL].bgColour);

			m_skinWindow->m_inputWindowBgColour = m_colours[SJ_COLOUR_NORMAL].bgColour;
			m_skinWindow->m_inputWindowFgColour = m_colours[SJ_COLOUR_NORMAL].fgColour;

			iw->Refresh();
		}

		// update font
		if( m_skinWindow->m_inputWindowFontHeight != m_rect.height )
		{
			if( !m_skinWindow->m_inputWindowDefFont.IsOk() )
			{
				m_skinWindow->m_inputWindowDefFont = iw->GetFont();
			}

			if( m_rect.height >= 18 || !m_skinWindow->m_inputWindowDefFont.IsOk() )
			{
				wxClientDC dc(m_skinWindow);                        // use a larger font
				wxFont iwFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
				SjVisBg::SetFontPixelH(dc, iwFont, m_rect.height-2);
				iw->SetFont(iwFont);
			}
			else
			{
				iw->SetFont(m_skinWindow->m_inputWindowDefFont);    // use the default font for smaller controls
			}

			m_skinWindow->m_inputWindowFontHeight = m_rect.height;

			iw->Refresh();
		}

		iw->SetSize(m_rect);
	}
}


void SjSkinInputItem::OnPaint(wxDC& dc)
{
	wxASSERT(m_skinWindow);
}


/*******************************************************************************
 * SjSkinDivItem
 ******************************************************************************/


bool SjSkinDivItem::Create(const wxHtmlTag& tag, wxString& error)
{
	// init width, height and events
	if( !m_width.IsSet() )
	{
		m_width.SetRel(10000);
	}

	if( !m_height.IsSet() )
	{
		m_height.SetRel(10000);
	}

	if( m_targetId == IDT_VIS_RECT )
	{
		if( tag.HasParam(wxT("INDENT")) )
		{
			wxString s = tag.GetParam(wxT("INDENT"));
			long l;

			if( !s.BeforeFirst(wxT(',')).ToLong(&l) ) { l = 0; } s = s.AfterFirst(wxT(','));
			m_indent.x = l;

			if( !s.BeforeFirst(wxT(',')).ToLong(&l) ) { l = 0; } s = s.AfterFirst(wxT(','));
			m_indent.y = l;

			if( !s.BeforeFirst(wxT(',')).ToLong(&l) ) { l = 0; } s = s.AfterFirst(wxT(','));
			m_indent.width = l;

			if( !s.ToLong(&l) ) { l = 0; }
			m_indent.height = l;
		}

		int test;
		if( tag.GetParamAsInt(wxT("VISAUTOSTART"), &test) )
		{
			if( test == 1 )
			{
				m_prop |= SJ_SKIN_PROP_AUTO_START_VIS;
			}
		}
	}

	m_usesPaint     = FALSE;
	m_itemType      = SJ_DIVITEM;

	return TRUE;
}


/*******************************************************************************
 * SjSkin - Loading And Selecting Skins
 ******************************************************************************/


BEGIN_EVENT_TABLE(SjSkinWindow, wxFrame)
	EVT_SIZE                (SjSkinWindow::OnSize               )
	EVT_MOVE                (SjSkinWindow::OnMove               )
	EVT_SET_FOCUS           (SjSkinWindow::OnSetFocus           )
	EVT_LEFT_DOWN           (SjSkinWindow::OnMouseLeftDown      )
	EVT_LEFT_DCLICK         (SjSkinWindow::OnMouseLeftDown      )
	EVT_LEFT_UP             (SjSkinWindow::OnMouseLeftUp        )
	EVT_MOUSE_CAPTURE_LOST  (SjSkinWindow::OnMouseCaptureLost   )
	EVT_CONTEXT_MENU        (SjSkinWindow::OnMouseRight         )
	EVT_MIDDLE_UP           (SjSkinWindow::OnMouseMiddleUp      )
	EVT_MOTION              (SjSkinWindow::OnMouseMotion        )
	EVT_LEAVE_WINDOW        (SjSkinWindow::OnMouseLeave         )
	EVT_PAINT               (SjSkinWindow::OnPaint              )
	EVT_ERASE_BACKGROUND    (SjSkinWindow::OnEraseBackground    )
	EVT_IMAGE_THERE         (SjSkinWindow::OnImageThere         )
	#ifdef __WXGTK__
	EVT_TIMER               (IDTIMER_SETSIZEHACK, SjMainFrame::OnSetSizeHackTimer)
	#endif
END_EVENT_TABLE()


SjSkinWindow::SjSkinWindow(wxWindow* parent, wxWindowID id, long skinFlags, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
	: wxFrame(parent, id, title, pos, size,
	          style | wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE)
{
	m_currSkin              = NULL;
	m_currLayout            = NULL;
	m_workspaceWindow       = NULL;
	m_workspaceMovedAway    = FALSE;
	m_workspaceColours      = m_workspaceColours__;
	m_mouseItem             = NULL;
	m_imgThread             = NULL;
	m_dragImage             = NULL;
	m_mouseInDisplayMove    = false;
	m_inputWindowFontHeight = -1;

	m_skinFlags             = skinFlags;
}


SjSkinWindow::~SjSkinWindow()
{
	if( m_currSkin )
	{
		delete m_currSkin;
	}
}


void SjSkinWindow::SetWorkspaceWindow(wxWindow* workspaceWindow)
{
	wxASSERT(workspaceWindow);
	m_workspaceWindow = workspaceWindow;
}


void SjSkinWindow::MoveWorkspaceAway(bool moveAway)
{
	// this function may not be used from within a thread
	// (the caller, normally SjMainFrame::SetVisEmbedded()
	// should ensure this)

	wxASSERT( wxThread::IsMain() );

	if( m_workspaceMovedAway != moveAway )
	{
		m_workspaceMovedAway = moveAway;

		if( m_workspaceWindow )
		{
			if( moveAway )
			{
				// move the window away; however, leave the size of the window
				// as this makes is easier for the workspace to track its state when shown again
				wxSize oldSize = m_workspaceWindow->GetSize();
				m_workspaceWindow->SetSize(-1000-oldSize.x, 0, oldSize.x, oldSize.y, wxSIZE_ALLOW_MINUS_ONE);
			}
			else
			{
				SjSkinItemList::Node* itemnode = m_targets[IDT_WORKSPACE].m_itemList.GetFirst();
				if ( itemnode )
				{
					SjSkinItem* item = itemnode->GetData();
					wxASSERT(item);
					m_workspaceWindow->SetSize(item->m_rect);
				}
			}

			// any Update() should be called by the caller who should know better the
			// exact moment to avoid flickering.  A good idea are the following sequences :
			//
			//  create workspace replacement window  ->  show it  ->  MoveWorkspaceAway(TRUE)  ->  Update()
			// or
			//  MoveWorkspaceAway(FALSE)  ->  hide replacement  -> Update()
			//
			// however, is some cases other directions may be better, esp. as MoveWorkspaceAway()
			// is not thread-safe (and probably will never be)
		}
	}
}


void SjSkinWindow::SetInputWindow(wxWindow* inputWindow)
{
	wxASSERT(inputWindow);
	m_inputWindow = inputWindow;
}


void SjSkinWindow::SaveSizes()
{
	if( m_currLayout )
	{
		wxRect currRect = GetRect();

		m_currLayout->m_currRect = currRect;

		// also save the size (or parts of it) to all layouts with the same usage
		int l, layoutCount = m_currSkin->GetLayoutCount();
		for( l = 0; l < layoutCount; l++ )
		{
			SjSkinLayout* dependingLayout = m_currSkin->GetLayout(l);
			if( dependingLayout
			 && dependingLayout != m_currLayout )
			{
				if( !dependingLayout->m_useWidth.IsEmpty()
				 &&  dependingLayout->m_useWidth == m_currLayout->m_useWidth )
				{
					dependingLayout->m_currRect.width = currRect.width;
				}

				if( !dependingLayout->m_useHeight.IsEmpty()
				 &&  dependingLayout->m_useHeight == m_currLayout->m_useHeight )
				{
					dependingLayout->m_currRect.height = currRect.height;
				}

				if( !dependingLayout->m_usePos.IsEmpty()
				 &&  dependingLayout->m_usePos == m_currLayout->m_usePos )
				{
					dependingLayout->m_currRect.x   = currRect.x;
					dependingLayout->m_currRect.y   = currRect.y;
					dependingLayout->m_alwaysOnTop  = m_currLayout->m_alwaysOnTop; // in 2.51beta2 hinzugefuegt, s. http://www.silverjuke.net/forum/topic-2392.html
				}
			}
		}
	}
}


void SjSkinWindow::LoadLayout(SjSkinLayout* newLayout /*may be NULL*/,
                              SjLoadLayoutFlag sizeChangeFlag)       /*esp. set when switching to/from kiosk mode
                                                                        as the kiosk state is undefined during this time*/
{
	// unset item clicked or entered with the mouse
	if( HasCapture() )
	{
		ReleaseMouse();
	}

	if( m_mouseItem )
	{
		m_mouseItem->OnMouseLeave();
		m_mouseItem = NULL;
	}

	// init workspace colours
	m_workspaceColours = m_workspaceColours__;

	// remove all timers of the old layout
	SjSkinItem*           item;
	SjSkinItemList::Node* itemnode;
	if( m_currLayout )
	{
		SjSkinItem*           item;
		SjSkinItemList::Node* itemnode = m_currLayout->m_itemList.GetFirst();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			item->CreateTimer(0);

			itemnode = itemnode->GetNext();
		}
	}

	// save the old size
	SaveSizes();


	//  <<<<<<<<<<<<<<<<<<
	//      set the new layout
	//          >>>>>>>>>>>>>>>>>>

	m_currLayout = newLayout; // m_currLayout may be NULL now

	// create an item list for every target
	int i;
	for( i = 0/*don't use IDT_FIRST here!*/; i <= IDT_LAST; i++ )
	{
		m_targets[i].m_itemList.Clear();
	}

	if( m_currLayout )
	{
		itemnode = m_currLayout->m_itemList.GetFirst();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			if( item->m_targetId )
			{
				m_targets[item->m_targetId].m_itemList.Append(item);

				#ifdef SJ_SKIN_USE_HIDE
				if( m_targets[item->m_targetId].m_hidden )
				{
					HideSkinTarget(item->m_targetId, TRUE/*hide*/, FALSE/*redraw*/);
				}
				#endif

				item->SetValue(m_targets[item->m_targetId].m_value);
			}

			itemnode = itemnode->GetNext();
		}
	}

	#ifdef SJ_SKIN_USE_HIDE
	HidingSkinTargetsDone(FALSE/*redraw*/);
	#endif

	// create a direct link to the workspace colours;
	// if the current layout has no workspace, we leave the pointer unchanged.
	// it defaults to a dummy item with black and white colours
	wxASSERT(m_targets);
	wxASSERT(IDT_WORKSPACE <= IDT_LAST);

	itemnode = m_targets[IDT_WORKSPACE].m_itemList.GetFirst();
	if( itemnode )
	{
		item = itemnode->GetData();
		wxASSERT(item);
		m_workspaceColours = item->m_colours;
	}

	// set the new size and position
	if( sizeChangeFlag != SJ_NO_SIZE_CHANGE )
	{
		bool doSizeChange = (sizeChangeFlag==SJ_FORCE_SIZE_CHANGE
		                  || g_kioskModule==NULL
		                  || !g_kioskModule->KioskStarted());
		wxRect wantedRect = GetRect();
		if( m_currLayout )
		{
			if( m_currLayout->m_currRect.width>0 && m_currLayout->m_currRect.height>0 )
			{
				// use saved rect, this may be the same rect as before if defined by the
				// layout using the "use*" attributes.
				wantedRect = m_currLayout->m_currRect;
			}
			else
			{
				// use default size
				if( m_currLayout->m_defSize.x ) wantedRect.width = m_currLayout->m_defSize.x;
				if( m_currLayout->m_defSize.y ) wantedRect.height = m_currLayout->m_defSize.y;
			}

			// set the new tooltip colours
			#if SJ_USE_TOOLTIPS
			if( m_currSkin->HasTooltipColours() )
			{
				g_tools->m_toolTipManager.SetColours(m_currSkin->GetTooltipFgColour(), m_currSkin->GetTooltipBgColour(), m_currSkin->GetTooltipBorderColour());
			}
			else
			{
				g_tools->m_toolTipManager.SetDefColours();
			}
			#endif

			// set "always on top"
			if( doSizeChange )
			{
				ShowAlwaysOnTop(m_currLayout->m_alwaysOnTop);
			}
		}

		if( doSizeChange )
		{
			SjDialog::EnsureRectDisplayVisibility(wantedRect);
			wxRect rectToSet = CheckLayoutWindowRect(m_currLayout, wantedRect);
			SetSize(rectToSet);
			#ifdef __WXGTK__
				m_setSizeHackRect = rectToSet; // SETSIZEHACK, Hack the bad SetSize() implementation
				m_setSizeHackCnt = 0;
				m_setSizeHackTimer.SetOwner(this, IDTIMER_SETSIZEHACK);
				m_setSizeHackTimer.Start(50);
			#endif
		}
	}

	// calculate the item rectangles
	wxSize clientSize = GetClientSize();

	CalcItemRectangles(clientSize.x, clientSize.y);

	// set minimal size - the maximal size is ignored there seems not to be a good reason for this (eg. maximizing the window should always be possible)
	if( m_currLayout )
	{
		wxSize skinMinSize, skinMaxSize;
		GetLayoutMinMax(m_currLayout, skinMinSize, skinMaxSize);
		SetMinSize(skinMinSize);
	}


	// redraw the window
	if( IsShown() )
	{
		Refresh();
	}
}


#ifdef __WXGTK__
void SjSkinWindow::OnSetSizeHackTimer(wxTimerEvent& event)
{
	// the first moments, GetRect() is simelar to the wanted rect, but after realisation, it is different.
	// exactly, at this moment, we call SetSize() again with the wanted rect.
	// if all this takes longer than 1 seconds, we give up (and we do not want to disturb systems that have no bug).
	if( GetRect() != m_setSizeHackRect )
	{
		m_setSizeHackTimer.Stop();
		SetSize(m_setSizeHackRect);
		return;
	}

	m_setSizeHackCnt++;
	if( m_setSizeHackCnt > 20 )
	{
		m_setSizeHackTimer.Stop();
	}
}
#endif


void SjSkinWindow::ShowAlwaysOnTop(bool alwaysOnTop)
{
	if( m_currLayout )
	{
		m_currLayout->m_alwaysOnTop = alwaysOnTop;

		if( alwaysOnTop )
		{
			SetWindowStyle(GetWindowStyle() | wxSTAY_ON_TOP);
		}
		else
		{
			SetWindowStyle(GetWindowStyle() & ~wxSTAY_ON_TOP);
		}

		SetSkinTargetValue(IDT_ALWAYS_ON_TOP, IsAlwaysOnTop()? 1 : 0);
	}
}


bool SjSkinWindow::LoadSkin(const wxString& path, long conditions, const wxString& skinSettings, bool reloadScripts)
{
	SjSkinMlParser parser(NULL, conditions);

	// update the list of fonts
	g_tools->UpdateFacenames();

	// load the new skin - if reloadScripts is false, SjSee of the current skin is reused and the scripts are not parsed again.
	#if SJ_USE_SCRIPTS
		SjSee* see = NULL;
		if( reloadScripts || m_currSkin == NULL )
		{
			see = new SjSee();
			see->SetExecutionScope(path);
		}
	#endif

	SjSkinSkin* newSkin = parser.ParseFile(path, false
											#if SJ_USE_SCRIPTS
	                                       , see
											#endif
	                                      );
	if( !newSkin )
	{
		wxLogError(_("Cannot open \"%s\"."), path.c_str());
		return FALSE;
	}

	#if SJ_USE_SCRIPTS
	if( see == NULL )
	{
		wxASSERT( m_currSkin );
		newSkin->m_see = m_currSkin->m_see;
		m_currSkin->m_see = NULL;
	}
	#endif

	// deselect the old layout (if any)
	LoadLayout(NULL);

	// select the new skin
	delete m_currSkin;
	m_currSkin = newSkin;
	m_currSkin->ConnectToSkinWindow(this);

	// load the skin settings, if given
	bool loadDefaultLayout = TRUE;
	if( !skinSettings.IsEmpty() )
	{
		// remember, m_currLayout is not yet set; therefore functions as
		// this->GetLayoutCount() do not work - use m_currSkin->GetLayoutCount() instead
		#define SAVABLE_SETTINGS_VER 0L
		SjStringSerializer ser(skinSettings);
		long        serVersion = ser.GetLong();
		wxString    serSkinName = ser.GetString();
		long        serLayoutCount = ser.GetLong();
		wxString    serDefLayoutName = ser.GetString();
		if(  serVersion == SAVABLE_SETTINGS_VER
		 &&  serSkinName == m_currSkin->GetName()
		 &&  serLayoutCount == m_currSkin->GetLayoutCount()
		 && !ser.HasErrors() )
		{
			// load settings as saved from GetSavableSkinSettings()
			int              l;
			wxString         serLayoutName;
			wxRect           serRect;
			SjSkinLayout* serLayout;
			bool             serBadSize;
			for( l = 0; l < serLayoutCount; l++ )
			{
				serLayout = m_currSkin->GetLayout(l);
				serLayoutName = ser.GetString();
				serRect = SjTools::ParseRect(ser.GetString());
				if(  serLayout
				 &&  serLayoutName == serLayout->GetName()
				 && !ser.HasErrors() )
				{
					if( serRect.width > 0
					 && serRect.height > 0 )
					{
						serRect = CheckLayoutWindowRect(serLayout, serRect, &serBadSize);
						if( !serBadSize )
						{
							serLayout->m_currRect = serRect;
						}
					}

					if( serDefLayoutName == serLayoutName )
					{
						LoadLayout(serLayout);
						loadDefaultLayout = FALSE;
					}
				}
			}
		}
	}

	// select the default layout (a successfully loaded
	// skin has at least one selectable layout)
	if( loadDefaultLayout )
	{
		wxASSERT(m_currSkin->GetLayout(0));
		LoadLayout(m_currSkin->GetLayout(0));
	}

	return SkinOk();
}


wxString SjSkinWindow::GetSavableSkinSettings()
{
	SjStringSerializer ser;

	ser.AddLong(SAVABLE_SETTINGS_VER); // version/flag - for future use
	if(  m_currLayout )
	{
		int l, layoutCount = GetLayoutCount();

		SaveSizes();

		ser.AddString(GetSkinName());
		ser.AddLong(GetLayoutCount());
		ser.AddString(m_currLayout->GetName());

		for( l = 0; l < layoutCount; l++ )
		{
			SjSkinLayout* layout = m_currSkin->GetLayout(l);

			ser.AddString(layout->GetName());
			ser.AddString(SjTools::FormatRect(layout->m_currRect));
		}
	}

	return ser.GetResult();
}


wxRect SjSkinWindow::CheckLayoutWindowRect(const SjSkinLayout* layout, const wxRect& r__, bool* retBadSize) const
{
	wxRect r(r__);

	// init "bad size"; a size is only bad if it does not align to the layout sizes
	// (otherwise, the user may just have changed the screen resolution between two calls of Silverjuke)s
	bool  dummy;
	bool* badSize = retBadSize? retBadSize : &dummy;

	*badSize = FALSE;

	// check agains skin
	wxSize skinMinSize, skinMaxSize;
	GetLayoutMinMax(layout, skinMinSize, skinMaxSize);

	if( r.width  < skinMinSize.x ) { r.width  = skinMinSize.x; *badSize = TRUE; }
	if( r.width  > skinMaxSize.x ) { r.width  = skinMaxSize.x; *badSize = TRUE; }

	if( r.height < skinMinSize.y ) { r.height = skinMinSize.y; *badSize = TRUE; }
	if( r.height > skinMaxSize.y ) { r.height = skinMaxSize.y; *badSize = TRUE; }

	SjDialog::EnsureRectDisplayVisibility(r);

	return r;
}


void SjSkinWindow::ReloadSkin(long conditions, bool reloadScripts, SjLoadLayoutFlag sizeChangeFlag)
{
	if( m_currLayout )
	{
		wxString oldUrl        = m_currSkin->GetUrl();
		wxString oldLayoutName = m_currLayout->m_name;

		LoadSkin(oldUrl, conditions, wxT(""), reloadScripts);
		LoadLayout(GetLayout(oldLayoutName), sizeChangeFlag);
		Refresh();
	}
}


void SjSkinWindow::CalcChildItemRectangles(SjSkinItem* parent)
{
	// pass 1: calculate width / height
	SjSkinItemList::Node *childNode = parent->m_children.GetFirst();
	SjSkinItem           *child, *prevChild = NULL;
	while( childNode )
	{
		child = childNode->GetData();
		wxASSERT(child);

		if( prevChild == NULL )
		{
			child->m_rect.width  = child->m_width .CalcAbs(parent->m_rect.width,  0, 0);
			child->m_rect.height = child->m_height.CalcAbs(parent->m_rect.height, 0, 0);

			if( child->m_width.IsSpecial() == SJ_SKINPOS_OPPOSITE )
				child->m_rect.width  = child->m_width .CalcAbs(child->m_rect.height, 0, 0);
			else if( child->m_height.IsSpecial() == SJ_SKINPOS_OPPOSITE )
				child->m_rect.height = child->m_height.CalcAbs(child->m_rect.width,  0, 0);

			child->m_rect.x      = parent->m_rect.x + child->m_x.CalcAbs(parent->m_rect.width,  0, 0);
			child->m_rect.y      = parent->m_rect.y + child->m_y.CalcAbs(parent->m_rect.height, 0, 0);
		}
		else
		{
			child->m_rect.width  = child->m_width .CalcAbs(parent->m_rect.width,  prevChild->m_rect.width,  0);
			child->m_rect.height = child->m_height.CalcAbs(parent->m_rect.height, prevChild->m_rect.height, 0);

			if( child->m_width.IsSpecial() == SJ_SKINPOS_OPPOSITE )
				child->m_rect.width  = child->m_width .CalcAbs(child->m_rect.height, 0, 0);
			else if( child->m_height.IsSpecial() == SJ_SKINPOS_OPPOSITE )
				child->m_rect.height = child->m_height.CalcAbs(child->m_rect.width,  0, 0);

			child->m_rect.x      = parent->m_rect.x + child->m_x.CalcAbs(parent->m_rect.width,  prevChild->m_rect.x-parent->m_rect.x, prevChild->m_rect.width );
			child->m_rect.y      = parent->m_rect.y + child->m_y.CalcAbs(parent->m_rect.height, prevChild->m_rect.y-parent->m_rect.y, prevChild->m_rect.height);
		}

		child->m_hasOverlayingItems = -1; // don't know - really, even if an item has no children,
		// if may be overlapped by its siblings
		// set item size
		child->OnSize();

		// recursive call
		CalcChildItemRectangles(child);

		// next child
		prevChild = child;
		childNode = childNode->GetNext();
	}
}


void SjSkinWindow::CalcItemRectangles(long width, long height)
{
	// skin okay?
	if( !m_currLayout )
	{
		return;
	}

	// set root item size and start recursion
	SjSkinItemList::Node* itemnode = m_currLayout->m_itemList.GetFirst();
	if( itemnode )
	{
		SjSkinItem* item = itemnode->GetData();
		wxASSERT(item);

		item->m_rect.x = 0;
		item->m_rect.y = 0;
		item->m_rect.width = width;
		item->m_rect.height = height;
		item->m_hasOverlayingItems = 1; // jupp

		CalcChildItemRectangles(item);
	}

	// move away unused windows
	if(   m_workspaceWindow
	        && (       !m_currLayout->m_hasWorkspace
#ifdef SJ_SKIN_USE_HIDE
	                   ||  m_targets[IDT_WORKSPACE].m_hidden
#endif
	                   ||  m_workspaceMovedAway
	           )
	  )
	{
		wxSize oldSize = m_workspaceWindow->GetSize();
		m_workspaceWindow->SetSize(-1000-oldSize.x, 0, oldSize.x, oldSize.y, wxSIZE_ALLOW_MINUS_ONE);
	}

	if(   m_inputWindow
	        && (!m_currLayout->m_hasInput
#ifdef SJ_SKIN_USE_HIDE
	            || m_targets[IDT_SEARCH].m_hidden
#endif
	           )
	  )
	{
		m_inputWindow->SetSize(-1000, -1000, 100, 100, wxSIZE_ALLOW_MINUS_ONE);
	}

	// notify derived class about the sizing
	SjSkinValue dummy;
	OnSkinTargetEvent(IDMODMSG_WINDOW_SIZED_MOVED, dummy, 0);
}


SjSkinItem* SjSkinWindow::FindClickableItem(long x, long y) const
{
	// try all other items (from last to first as the last
	// item is atop)
	if( m_currLayout )
	{
		SjSkinItem*           item;
		SjSkinItemList::Node* itemnode = m_currLayout->m_itemList.GetLast();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			if(  item->m_usesMouse
#ifdef SJ_SKIN_USE_HIDE
			        && !item->m_hidden
#endif
			  )
			{
				if( x >= item->m_rect.x
				        && y >= item->m_rect.y
				        && x < (item->m_rect.x + item->m_rect.width)
				        && y < (item->m_rect.y + item->m_rect.height) )
				{
					return item;
				}
			}

			itemnode = itemnode->GetPrevious();
		}
	}

	return NULL;
}


SjSkinItem* SjSkinWindow::FindFirstItemByTargetId(int targetId) const
{
	// currently, there is no FindNextItemByTargetId() function, for this purpose,
	// use the target list directly.

	SjSkinItemList::Node* itemnode = m_targets[targetId].m_itemList.GetFirst();

	return itemnode? itemnode->GetData() : NULL;
}


bool SjSkinWindow::HasSkinTarget(int targetId) const
{
	return FindFirstItemByTargetId(targetId) != NULL;
}


void SjSkinWindow::SetSkinTargetValue(int targetId, const SjSkinValue& value, bool onlyIfPossible)
{
	SjSkinItem*           item;
	SjSkinItemList::Node* itemnode;

	if(targetId <= 0 || targetId > IDT_LAST )
	{
		wxLogDebug(wxT("Invalid target ID %i")/*n/t*/, (int)targetId);
		return;
	}

	// check if there are intersections with the drag image, if any
	if( onlyIfPossible && m_dragImage )
	{
		itemnode = m_targets[targetId].m_itemList.GetFirst();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			if( m_dragRect.Intersects(item->GetScreenRect()) )
			{
				return; // currently, this will not look smart
			}

			itemnode = itemnode->GetNext();
		}
	}

	// save the value
	m_targets[targetId].m_value = value;

	// promote the value to all items using this targets
	itemnode = m_targets[targetId].m_itemList.GetFirst();
	while( itemnode )
	{
		item = itemnode->GetData();
		wxASSERT(item);

		item->SetValue(value);

		itemnode = itemnode->GetNext();
	}

	UpdateMenuBarValue(targetId, value);
}


void SjSkinWindow::SetSkinText(const wxString& userId, const wxString& text)
{
	// go through all items
	int l, layoutCount = m_currSkin->GetLayoutCount();
	SjSkinLayout* layout;
	SjSkinItem* item;
	for( l = 0; l < layoutCount; l++ )
	{
		layout = m_currSkin->GetLayout(l);
		if( layout )
		{
			SjSkinItemList::Node* itemnode = layout->m_itemList.GetFirst();
			while( itemnode )
			{
				item = itemnode->GetData();
				wxASSERT(item);

				if(  item->m_userId
				        && *item->m_userId == userId )
				{
					if( item->m_itemType == SJ_BOXITEM )
					{
						// set the value of a <box> tag
						SjSkinValue value;
						value.string = text;
						item->SetValue(value);
					}
				}

				itemnode = itemnode->GetNext();
			}
		}
	}
}


#ifdef SJ_SKIN_USE_HIDE
bool SjSkinWindow::HideSkinTarget(int targetId, bool hide, bool redraw)
{
	if(targetId <= 0 || targetId > IDT_LAST )
	{
		wxLogDebug("Invalid target ID %i"/*n/t*/, (int)targetId);
		return FALSE;
	}

	if( !redraw/*=init*/ || m_targets[targetId].m_hidden != hide )
	{
		// save the value
		m_targets[targetId].m_hidden = hide;

		// hide all item with the given id
		SjSkinItem*           item;
		SjSkinItemList::Node* itemnode = m_targets[targetId].m_itemList.GetFirst();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			item->m_hidden = hide;
			if( redraw )
			{
				item->RedrawMe();
			}

			itemnode = itemnode->GetNext();
		}
		return TRUE; // state changed - you should call HidingSkinTargetsDone() afterwards
	}
	return FALSE;
}
#endif


#ifdef SJ_SKIN_USE_HIDE
void SjSkinWindow::HidingSkinTargetsDone(bool redraw)
{
#ifdef SJ_SKIN_USE_BELONGSTO
	// hide all items belonging to other item targets
	if( m_currLayout /*may be NULL when calling eg. from LoadLayout()*/ )
	{
		SjSkinItem*           item;
		SjSkinItemList::Node* itemnode = m_currLayout->m_itemList.GetFirst();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			if( item->m_belongsToId != IDT_NONE )
			{
				wxASSERT( item->m_belongsToId > 0 && item->m_belongsToId <= IDT_LAST );

				if( !redraw/*=init*/ || item->m_hidden != m_targets[item->m_belongsToId].m_hidden )
				{
					item->m_hidden = m_targets[item->m_belongsToId].m_hidden;
					if( redraw )
					{
						item->RedrawMe();
					}
				}
			}

			itemnode = itemnode->GetNext();
		}
	}
#endif
}
#endif


void SjSkinWindow::OnSetFocus(wxFocusEvent& event)
{
	if( m_workspaceWindow )
	{
		m_workspaceWindow->SetFocus();
	}
}


void SjSkinWindow::OnSize(wxSizeEvent& event_doNotUse_mayBeUnsetByOurForwardings)
{
	wxSize clientSize = GetClientSize();

	CalcItemRectangles(clientSize.x, clientSize.y);

	// also create a motion event as the item under the mouse may change
	wxMouseEvent mouseEvent;
	wxPoint mousePos = ::wxGetMousePosition();
	ScreenToClient(mousePos);
	mouseEvent.m_x = mousePos.x;
	mouseEvent.m_y = mousePos.y;
	OnMouseMotion(mouseEvent);

#ifdef __WXMAC__
	Refresh(FALSE /*eraseBackground*/);
	Update();
#endif
}


void SjSkinWindow::OnMove(wxMoveEvent& event)
{
	SjSkinValue dummy;
	OnSkinTargetEvent(IDMODMSG_WINDOW_SIZED_MOVED, dummy, 0);
}


void SjSkinWindow::OnMouseLeftDown(wxMouseEvent& event)
{
	bool dClick = event.LeftDClick();

	/* left down
	 */
	long    x = event.GetX(),
	        y = event.GetY();

	long    accelFlags = (event.ShiftDown()? wxACCEL_SHIFT : 0)
	                     | (event.ControlDown()? wxACCEL_CTRL : 0);

	if( m_mouseItem == NULL )
	{
		OnMouseMotion(event);
	}

#if SJ_USE_TOOLTIPS
	g_tools->m_toolTipManager.ClearToolTipProvider();
#endif

	if( m_mouseItem )
	{
		CaptureMouse();
		m_mouseItem->OnMouseLeftDown(x, y, dClick? TRUE : FALSE, accelFlags);
	}
}


static int findCorner(const wxRect& r, const wxSize& delta, int x, int y)
{
	if( x >= r.x && x < r.x+delta.x )
	{
		if( y > r.y && y < r.y+delta.y )                       { return 1; } // top-left
		else if( y >= r.y+r.height-delta.y && y < r.y+r.height )    { return 3; } // bottom-left
	}
	else if( x >= r.x+r.width-delta.x && x < r.x+r.width )
	{
		if( y > r.y && y < r.y+delta.y )                       { return 2; } // top-right
		else if( y >= r.y+r.height-delta.y && y < r.y+r.height )    { return 4; } // bottom-right
	}

	return 0;
}


void SjSkinWindow::OnMouseLeftUp(wxMouseEvent& event)
{
	long    x = event.GetX(),
	        y = event.GetY();
	long    accelFlags = (event.ShiftDown()? wxACCEL_SHIFT : 0)
	                     | (event.ControlDown()? wxACCEL_CTRL : 0);

	// normal clicks
	SjMouseUsed mouseUsed = SJ_MOUSE_NOT_USED;
	if( HasCapture() )
	{
		ReleaseMouse();

		if( m_mouseItem != NULL )
		{
			SjSkinItem* mi = m_mouseItem; // backup to "mi" as m_mouseItem may change eg. through scripts
			mouseUsed = mi->OnMouseLeftUp(x, y, accelFlags, false);
			if( mouseUsed == SJ_MOUSE_USE_DELAYED )
			{
				// post a delayed event as some (button) events may destroy the layout
				SjSkinValue value;
				value.value = m_targets[mi->m_targetId].m_value.value;
				OnSkinTargetEvent(mi->m_targetId, value, accelFlags);
			}
		}
	}

#ifdef __WXDEBUG__
	switch(mouseUsed)
	{
		case SJ_MOUSE_NOT_USED:     wxLogDebug(wxT("SJ_MOUSE_NOT_USED"));       break;
		case SJ_MOUSE_USED:         wxLogDebug(wxT("SJ_MOUSE_USED"));           break;
		case SJ_MOUSE_USE_DELAYED:  wxLogDebug(wxT("SJ_MOUSE_USE_DELAYED"));    break;
		default:                    wxASSERT(0);                                break;
	}
#endif

	// check for "corner clicks"
	//
	// with Silverjuke 2.52beta2 we have optimized the corner click as follows:
	//
	//      HHHH--------HHHH        H = Always "hot" part or corner
	//      HH%%        %%HH        % = Partly "hot" part (only hot if the click was not consumed before)
	//      |              |
	//      HH%%        %%HH
	//      HHHH--------HHHH
	//
	// see http://www.silverjuke.net/forum/topic-1475.html for details

	wxRect      clientRect = GetClientRect();
	wxSize      cornerSize(clientRect.width/8, clientRect.height/8);
	static int  lastCorner = 0;
	int         thisCorner = findCorner(clientRect, cornerSize, x, y);

	if( thisCorner )
	{
		bool hotPartOfCorner = (findCorner(clientRect.Deflate(10), cornerSize, x, y)==0);
		if( hotPartOfCorner || mouseUsed == SJ_MOUSE_NOT_USED )
		{
#define                 CORNER_TIMEOUT_MS 3500
			static unsigned long    lastCornerTime = 0;
			unsigned long           thisCornerTime = SjTools::GetMsTicks();

			if( thisCornerTime < lastCornerTime+CORNER_TIMEOUT_MS
			        && thisCorner != lastCorner
			        && lastCorner != 0 )
			{
				SjSkinValue value;
				OnSkinTargetEvent(IDO_CORNERCLICK, value, accelFlags);
				lastCornerTime = 0;
			}
			else
			{
				lastCornerTime = thisCornerTime;
			}
		}
	}

	lastCorner = thisCorner;
}


void SjSkinWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	wxLogDebug(wxT("+++++++++++++++ CAPTURE LOST!"));
	if( m_mouseItem )
	{
		m_mouseItem->OnMouseLeftUp(0, 0, 0, true);
		m_mouseItem = NULL;
	}
}


void SjSkinWindow::OnMouseRight(wxContextMenuEvent& event)
{
	if( !HasCapture() )
	{
		wxPoint clickPoint = ScreenToClient(event.GetPosition());

		#if SJ_USE_TOOLTIPS
			g_tools->m_toolTipManager.ClearToolTipProvider();
		#endif

		SjSkinItem* item = FindClickableItem(clickPoint.x, clickPoint.y);
		if( item )
		{
			OnSkinTargetContextMenu(item->m_targetId, clickPoint.x, clickPoint.y);
		}
		else
		{
			OnSkinTargetContextMenu(IDT_NONE, clickPoint.x, clickPoint.y);
		}
	}
}


void SjSkinWindow::OnMouseMiddleUp(wxMouseEvent& event)
{
	if( !HasCapture() )
	{
		long    x = event.GetX(),
		        y = event.GetY();

#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ClearToolTipProvider();
#endif

		SjSkinItem* item = FindClickableItem(x, y);
		if( item )
		{
			if( item->OnMouseMiddle(x, y) )
			{
				return;
			}
		}
	}

	event.Skip();
}


void SjSkinWindow::OnMouseMotion(wxMouseEvent& event)
{
	long    x = event.GetX(),
	        y = event.GetY();

	if( HasCapture() )
	{
		if( m_mouseItem )
		{
			m_mouseItem->OnMouseMotion(x, y, TRUE);
		}
	}
	else
	{
		SjSkinItem* item = FindClickableItem(x, y);

		if( m_mouseItem && m_mouseItem == item )
		{
			m_mouseItem->OnMouseMotion(x, y, FALSE);
		}
		else
		{
			if( m_mouseItem )
			{
				m_mouseItem->OnMouseLeave();
				m_mouseItem = NULL;
			}

			if( item )
			{
				m_mouseItem = item;
				m_mouseItem->OnMouseMotion(x, y, FALSE);

#if SJ_USE_TOOLTIPS
				if( item->m_itemType == SJ_BOXITEM )
				{
					; // nothing to do, the box items handle the tooltips theirselved,
					// however, this should be checked before the other conditions
				}
				else if( item->m_targetId>=IDT_LAYOUT_FIRST && item->m_targetId<=IDT_LAYOUT_LAST )
				{
					m_skinWindowToolTipProvider.m_item = item;
					g_tools->m_toolTipManager.SetToolTipProvider(&m_skinWindowToolTipProvider);
				}
				else if( item->m_targetId )
				{
					g_tools->m_toolTipManager.SetToolTipProvider(GetToolTipProvider(item->m_targetId, 0, item->m_rect));
				}
				else
				{
					g_tools->m_toolTipManager.ClearToolTipProvider();
				}
#endif
			}
#if SJ_USE_TOOLTIPS
			else
			{

				g_tools->m_toolTipManager.ClearToolTipProvider();
			}
#endif
		}
	}
}


void SjSkinWindow::OnMouseLeave(wxMouseEvent& event)
{
	//wxLogDebug("...leaving skin window...");
	if( !HasCapture() )
	{
		if( m_mouseItem )
		{
			m_mouseItem->OnMouseLeave();
			m_mouseItem = NULL;
		}
	}
}


void SjSkinWindow::OnPaint(wxPaintEvent& event)
{
	// start painting, this MUST be done for validating the window list
	wxPaintDC   dc(this);
	bool        drawDone = FALSE;

	// draw offscreen
#if 0
	// simple but slow, not using the update region
	wxSize size = GetClientSize();

	wxBitmap    memBitmap(size.x, size.y);
	wxMemoryDC  memDc;
	memDc.SelectObject(memBitmap);
	if( memDc.IsOk() )
	{
		RedrawAll(memDc);
		RedrawFinalLines(memDc);

		dc.Blit(0, 0, size.x, size.y, &memDc, 0, 0);
		drawDone = TRUE;
	}
#else
	// more complex and fast using the update region
	wxRegion updateRegion = GetUpdateRegion();
	wxRect updateRect = updateRegion.GetBox();

	wxBitmap    memBitmap(updateRect.width, updateRect.height);
	wxMemoryDC  memDc;
	memDc.SelectObject(memBitmap);
	if( memDc.IsOk() )
	{
		RedrawAll(memDc, &updateRect, 0-updateRect.x, 0-updateRect.y);
		RedrawFinalLines(memDc, 0-updateRect.x, 0-updateRect.y);

		dc.Blit(updateRect.x, updateRect.y, updateRect.width, updateRect.height, &memDc, 0, 0);
		drawDone = TRUE;
	}
#endif

	// draw onscreen
	if( !drawDone )
	{
		RedrawAll(dc);
		RedrawFinalLines(dc);
	}
}


void SjSkinWindow::OnImageThere(SjImageThereEvent& event)
{
	wxClientDC dc(this);

	SjSkinItem*           item;
	SjSkinItemList::Node* itemnode = m_currLayout->m_itemList.GetFirst();
	SjImgThreadObj*          obj = event.GetObj();
	while( itemnode )
	{
		item = itemnode->GetData();
		wxASSERT(item);

		if( item->m_usesPaint )
		{
			item->OnImageThere(dc, obj);
		}

		itemnode = itemnode->GetNext();
	}

	m_imgThread->ReleaseImage(this, obj);
}


void SjSkinWindow::RedrawAll(wxDC& dc,
                             const wxRect* rect /*may be NULL*/,
                             long finalMoveX, long finalMoveY)
{
	// layout okay?
	if( !m_currLayout )
	{
		wxSize size = GetClientSize();
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(*wxWHITE_BRUSH);
		dc.DrawRectangle(0, 0, size.x, size.y);
		return; // error
	}

	// draw all items
	SjSkinItem*           item;
	SjSkinItemList::Node* itemnode = m_currLayout->m_itemList.GetFirst();
	while( itemnode )
	{
		item = itemnode->GetData();
		wxASSERT(item);

		// paint item
		if( item->m_usesPaint )
		{
			if( rect == NULL
			        || rect->Intersects(item->m_rect) )
			{
				if( finalMoveX || finalMoveY )
				{
					item->m_rect.x += finalMoveX;
					item->m_rect.y += finalMoveY;
					item->OnSize();
				}

#ifdef SJ_SKIN_USE_HIDE
				if( !item->m_hidden )
				{
#endif
					item->HideDragImage();
					item->OnPaint(dc);
					item->ShowDragImage();
#ifdef SJ_SKIN_USE_HIDE
				}
#endif

				if( finalMoveX || finalMoveY )
				{
					item->m_rect.x -= finalMoveX;
					item->m_rect.y -= finalMoveY;
					item->OnSize();
				}
			}
		}

		itemnode = itemnode->GetNext();
	}
}


void SjSkinWindow::RedrawFinalLines(wxDC& dc, long finalMoveX, long finalMoveY)
{
	// draw debug outline
	if( m_currLayout )
	{
		if( m_currSkin->GetDebugOutline() )
		{
			wxPen greenPen(wxColour(0x00,0xFF,0x00), 1, wxSOLID);
			wxPen redPen(wxColour(0xFF,0x00,0x00), 1, wxSOLID);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);

			SjSkinItem*           item;
			SjSkinItemList::Node* itemnode = m_currLayout->m_itemList.GetFirst();
			while( itemnode )
			{
				item = itemnode->GetData();
				wxASSERT(item);

				if( item->m_usesPaint
				        && (item->m_alwaysRedrawBackground || item->m_hasOverlayingItems==1/*not:-1*/) )
				{
					dc.SetPen(redPen); // red to indicate complex and slow drawing
				}
				else
				{
					dc.SetPen(greenPen); // green to indicate simple and fast drawing
				}

				item->HideDragImage();
				dc.DrawRectangle(item->m_rect.x+finalMoveX, item->m_rect.y+finalMoveY, item->m_rect.width, item->m_rect.height);
				item->ShowDragImage();

				itemnode = itemnode->GetNext();
			}
		}

		#ifdef SJHOOK_FINALIZE_PAINTING
		SJHOOK_FINALIZE_PAINTING
		#endif
	}
}


void SjSkinWindow::OnEraseBackground(wxEraseEvent&)
{
	// we won't erease the background explcitly, this is done on
	// indirectly by OnPaint().
}


long SjSkinWindow::GetTargetProp(int targetId)
{
	long retProp = 0;
	SjSkinItemList::Node* itemnode = m_targets[targetId].m_itemList.GetFirst();
	while( itemnode )
	{
		SjSkinItem* item = itemnode->GetData();
		wxASSERT(item);

		retProp |= item->m_prop;

		itemnode = itemnode->GetNext();
	}
	return retProp;
}


bool SjSkinWindow::GetVisEmbedRect(wxRect* retRect, bool* retIsOverWorkspace, bool* retVisAutoStart) const
{
	wxRect dummy1; if( retRect == NULL ) retRect = &dummy1;
	bool   dummy2; if( retVisAutoStart == NULL ) retVisAutoStart = &dummy2;
	bool retRectOk = FALSE;

	// "visrect" target set?
	*retVisAutoStart = false;
	if( !retRectOk )
	{
		SjSkinItemList::Node* itemnode = m_targets[IDT_VIS_RECT].m_itemList.GetFirst();
		if( itemnode )
		{
			SjSkinDivItem* item = (SjSkinDivItem*)itemnode->GetData();
			wxASSERT(item);

			if( item && item->m_itemType == SJ_DIVITEM )
			{
				*retRect = item->m_rect;
				*retVisAutoStart = (item->m_prop&SJ_SKIN_PROP_AUTO_START_VIS)!=0;

				wxRect indent = item->GetIndent();
				retRect->x += indent.x;
				retRect->y += indent.y;
				retRect->width -= indent.x+indent.width;
				retRect->height -= indent.y+indent.height;
				retRectOk = TRUE;
			}
		}
	}

	// use workspace as vis. rect
	if( !retRectOk )
	{
		if( m_currLayout->m_hasWorkspace && m_workspaceWindow )
		{
			SjSkinItemList::Node* itemnode = m_targets[IDT_WORKSPACE].m_itemList.GetFirst();
			if ( itemnode )
			{
				SjSkinItem* item = itemnode->GetData();
				wxASSERT(item);

				*retRect = item->m_rect;
				retRectOk = TRUE;
			}
		}
	}

	// is over workspace?
	if( retIsOverWorkspace )
	{
		*retIsOverWorkspace = false;
		if( retRectOk && m_currLayout->m_hasWorkspace && m_workspaceWindow )
		{
			SjSkinItemList::Node* itemnode = m_targets[IDT_WORKSPACE].m_itemList.GetFirst();
			if ( itemnode )
			{
				SjSkinItem* item = itemnode->GetData();
				wxASSERT(item);

				*retIsOverWorkspace = item->m_rect.Intersects(*retRect);
			}
		}
	}

	return retRectOk;
}
