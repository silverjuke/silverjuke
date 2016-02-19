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
 * File:    skin.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke skins
 *
 ******************************************************************************/


#ifndef __SJ_SKIN_H__
#define __SJ_SKIN_H__


class wxHtmlTag;
class SjSkinSkin;
class SjSkinWindow;
class SjSkinItemTimer;
class SjSkinLayout;
class SjSkinImage;
class SjImgThread;
class SjImageThereEvent;
#if SJ_USE_SCRIPTS
class SjSee;
#endif


// Currently the following options are not needed, but we may need them
// (esp. hiding items) in future.  So I've just undef'd the functionality
// as it was hard enough to implement.  Please leave the functionality --
// I've programmed the hiding functionality already two times because I removed
// it as I thought I will never need it.  Björn.
#undef SJ_SKIN_USE_BELONGSTO
#undef SJ_SKIN_USE_HIDE


#define         SJ_OP_KIOSKON           0x00000001L
#define         SJ_OP_MAIN_VOL          0x00000002L
#define         SJ_OP_PLAYPAUSE         0x00000004L
#define         SJ_OP_EDIT_QUEUE        0x00000008L // this includes SJ_OP_SHUFFLE
#define         SJ_OP_SEARCH            0x00000010L
#define         SJ_OP_STARTVIS          0x00000020L
#define         SJ_OP_TOGGLE_ELEMENTS   0x00000040L
#define         SJ_OP_MUSIC_SEL         0x00000100L
#define         SJ_OP_ALBUM_VIEW        0x00000200L
#define         SJ_OP_COVER_VIEW        0x00000400L
#define         SJ_OP_LIST_VIEW         0x00000800L
#define         SJ_OP_TOGGLE_VIEW__     0x00001000L // will not be saved, only calcualted
#define         SJ_OP_PRELISTEN         0x00002000L
#define         SJ_OP_CREDIT_SYS__      0x00004000L // will not be saved, only calcualted
#define         SJ_OP_MULTI_ENQUEUE     0x00008000L
#define         SJ_OP_UNQUEUE           0x00010000L
#define         SJ_OP_MUSIC_SEL_GENRE   0x00020000L
#define         SJ_OP_ZOOM              0x00040000L
#define         SJ_OP_REPEAT            0x00100000L
#define         SJ_OP_ALL               0x00800000L
#define         SJ_OP_DEF_KIOSK        (SJ_OP_KIOSKON|SJ_OP_SEARCH|SJ_OP_TOGGLE_ELEMENTS|SJ_OP_ALBUM_VIEW|SJ_OP_ZOOM)
#define         SJ_OP_DEF_NONKIOSK     (0x00FFFFFFL&~SJ_OP_KIOSKON)
#define         SJ_OP_OS_MAC            0x01000000L
#define         SJ_OP_OS_WIN            0x02000000L
#define         SJ_OP_OS_GTK            0x04000000L
#define         SJ_OP_OS_MASK          (SJ_OP_OS_MAC|SJ_OP_OS_WIN|SJ_OP_OS_GTK)


class SjSkinPos
{
public:
					SjSkinPos           () { Init(); }
	void            Init                () { m_relTo10000=0; m_absPixels=0; m_special=0; }
	bool            Parse               (const wxString&);
	void            SetAbs              (long absPixels)  { m_relTo10000=0;          m_absPixels=absPixels; m_special=0; }
	void            SetRel              (long relTo10000) { m_relTo10000=relTo10000; m_absPixels=0;         m_special=0; }
	long            CalcAbs             (long totalAbsPixels, long prevAbsPos, long prevAbsSize) const;
	bool            IsSet               () const { return (m_special==0 && m_relTo10000==0 && m_absPixels==0)? FALSE : TRUE; }
	int             IsSpecial           () const { return m_special; }

private:
	// both values may be given, in this case the relative position / size
	// is taken first and the pixel value is added or substracted.
	long            m_relTo10000;
	long            m_absPixels;

	// 0 or one of  SKINPOS_*
	#define         SJ_SKINPOS_NEXT     'n'
	#define         SJ_SKINPOS_SAME     's'
	#define         SJ_SKINPOS_OPPOSITE 'o'
	int             m_special;
};


class SjSkinColour
{
public:
	                SjSkinColour        ();
	void            SetFromTag          (const wxHtmlTag& tag);

	wxColour        bgColour;
	wxPen           bgPen;
	wxBrush         bgBrush;
	bool            bgSet;

	wxColour        fgColour;
	wxPen           fgPen;
	//wxBrush       fgBrush; -- not needed
	bool            fgSet;

	wxColour        hiColour;
	wxPen           hiPen;
	//wxBrush       hiBrush; -- not needed
	bool            hiSet;

	int             offsetX,    // the offsets are used when using "hiColour" as a shadow,
	                offsetY;    // defaults to +1/0
};


class SjSkinValue
{
public:
	SjSkinValue     ()
	{	value       = 0;
		vmin        = 0;
		vmax        = 0;
		thumbSize   = 0;
	}

	SjSkinValue&    operator =          (const SjSkinValue& o)
	{	value     = o.value;
		vmin      = o.vmin;
		vmax      = o.vmax;
		thumbSize = o.thumbSize;
		string    = o.string;
		return *this;
	}

	long            value;              // - 0/1/2/... for <button> normal/selected/other/...
	                                    // - position for <scrollbar>
	                                    // - VFLAG_* for <box>
	#define         SJ_VFLAG_CENTER                 0x0000001
	#define         SJ_VFLAG_VMIN_IS_TIME           0x0000002
	#define         SJ_VFLAG_VMIN_MINUS             0x0000004
	#define         SJ_VFLAG_VMAX_IS_TIME           0x0000008
	#define         SJ_VFLAG_STRING_IS_IMAGE_URL    0x0000020
	#define         SJ_VFLAG_OVERLAY                0x0000040
	#define         SJ_VFLAG_MOVABLE                0x0000100
	#define         SJ_VFLAG_BOLD                   0x0000400
	#define         SJ_VFLAG_IGNORECENTEROFFSET     0x0000800
	#define         SJ_VFLAG_ICONL_MASK             0x00FF000
	#define         SJ_VFLAG_ICONL_EMPTY            0x0001000
	#define         SJ_VFLAG_ICONL_PLAY             0x0002000
	#define         SJ_VFLAG_ICONL_PAUSE            0x0004000
	#define         SJ_VFLAG_ICONL_STOP             0x0008000
	#define         SJ_VFLAG_ICONL_PLAYED           0x0010000
	#define         SJ_VFLAG_ICONL_ERRONEOUS        0x0020000
	#define         SJ_VFLAG_ICONL_MOVED_DOWN       0x0080000
	#define         SJ_VFLAG_ICONR_MASK             0xFF00000
	#define         SJ_VFLAG_ICONR_DELETE           0x0100000

	long            vmin, vmax;         // used by <scrollbar> and <box>;
	                                    // for events skin -> client, vmin are the accelerator flags
	                                    // buttons do blink if vmax ist set to SJ_VMAX_BLINK
	#define         SJ_VMAX_BLINK                   0x0000001

	long            thumbSize;          // used by <scrollbar>, if != 0, the max. value decreases by thumbSize

	wxString        string;            // used by <box>
};


enum SjMouseUsed
{
    SJ_MOUSE_NOT_USED = 0,
    SJ_MOUSE_USED,
    SJ_MOUSE_USE_DELAYED // the caller (eg. SjSkinWindow::OnMouseLeftUp()) shall post a corresponding event
};


enum SjItemType
{
    SJ_UNKNOWNITEM = 0,
    SJ_IMAGEITEM = 1,
    SJ_BUTTONITEM,
    SJ_SCROLLBARITEM,
    SJ_WORKSPACEITEM,
    SJ_LEDITEM,
    SJ_INPUTITEM,
    SJ_RESIZERITEM,
    SJ_DIVITEM,
    SJ_DISPLAYITEM,
    SJ_BOXITEM
};


class SjSkinItem;
WX_DECLARE_LIST(SjSkinItem, SjSkinItemList);


class SjImgThreadObj;


class SjSkinItem
{
public:

	// constructor/destructor
					SjSkinItem          ();
	virtual         ~SjSkinItem         ();

	// the type and the name of the item, usually set by Create() in derived classes
	// the name may be used to re-use the item
	SjItemType      m_itemType;

	// the target ID, 0 (IDT_NONE) for unset (default)
	int             m_targetId;
	wxString*       m_targetName; // "layout:" etc
	#ifdef SJ_SKIN_USE_BELONGSTO
	int             m_belongsToId;
	#endif
	int             m_doubleClickTargetId;
	wxString*       m_doubleClickTargetName; // "layout:" etc
	bool            CheckTarget         (wxString& error);

	// the ID given by t he user
	wxString*       m_userId;

	// an optional tooltip, used esp. for buttons switching the layout
	wxString*       m_itemTooltip;

	// target flags, as they're stored together with the target ID,
	// they should be larger than 0x0000FFFFL
	#define         SJ_TARGET_REPEATBUTTON  0x00010000L
	#define         SJ_TARGET_NULLISBOTTOM  0x00020000L
	long            m_targetFlags;

	// create a new skin item
	virtual bool    Create              (const wxHtmlTag&, wxString& error);
	virtual void    SetValue            (const SjSkinValue&);

	// OnSize() is called if the positions stored in m_rect
	// are changed. The current position is found in m_rect
	virtual void    OnSize              ();

	// Mouse events; they're only sent to items with m_clickable set to TRUE.
	// Mouse capturing is done by SjSkin on OnMouseDown()
	virtual void    OnMouseLeftDown     (long x, long y, bool doubleClick, long accelFlags);
	virtual SjMouseUsed OnMouseLeftUp   (long x, long y, long accelFlags, bool captureLost);
	virtual bool    OnMouseMiddle       (long x, long y) { return FALSE; }
	virtual void    OnMouseMotion       (long x, long y, bool leftDown);
	virtual void    OnMouseLeave        ();

	// draw an item; on drawing you can assume all underlaying
	// items are already drawn.
	virtual void    OnPaint             (wxDC&);
	virtual bool    OnImageThere        (wxDC& dc, SjImgThreadObj* obj) {return FALSE;}

	// redrawing using offscreen if wanted
	bool            m_alwaysRedrawBackground;
	int             m_hasOverlayingItems; // 1=yes, 0=no, -1=don't know
	wxRect          GetScreenRect       () const;
	void            HideDragImage       ();
	void            ShowDragImage       ();
	void            RedrawMe            ();

	// start/stop a timer, running timers will call OnTimer()
	void            CreateTimer         (long ms);
	SjSkinItemTimer* m_timer; // may be NULL!
	virtual void    OnTimer             ();

	// subimages for this item, may be NULL
	SjSkinImage*    m_image;
	long            m_imageIndex;
	bool            CheckImage          (wxString& error);

	// is the item clickable? does the item draw anything? defaults to TRUE
	bool            m_usesMouse;
	bool            m_usesPaint;
	#ifdef SJ_SKIN_USE_HIDE
	bool            m_hidden;
	#endif

	// the RELATIVE or absolute position as given in file "commands.sjs"
	SjSkinPos       m_x, m_y;
	SjSkinPos       m_width, m_height;

	// the ABSOLUTE position, set by SjSkin::CalcItemRectangles()
	wxRect          m_rect;

	// colours - each colour has a foreground, a background and a hilite
	#define         SJ_COLOUR_NORMAL            0
	#define         SJ_COLOUR_NORMALODD         1
	#define         SJ_COLOUR_SELECTION         2
	#define         SJ_COLOUR_SELECTIONODD      3
	#define         SJ_COLOUR_TITLE1            4
	#define         SJ_COLOUR_TITLE2            5
	#define         SJ_COLOUR_TITLE3            6
	#define         SJ_COLOUR_VERTTEXT          7
	#define         SJ_COLOUR_STUBTEXT          8
	#define         SJ_COLOUR_COUNT             9
	SjSkinColour*   m_colours; // points to SjSkinSkin::m_dummyColours on default

	// pointer to the skin window, NULL on Create()
	SjSkinWindow*   m_skinWindow;

	// child and parent items
	SjSkinItem*     m_parent;
	SjSkinItemList  m_children;

	// some special properties
	#define         SJ_SKIN_PROP_AUTO_START_VIS         0x01    // for SjSkinDivItem
	#define         SJ_SKIN_PROP_HIDE_CREDIT_IN_DISPLAY 0x01    // for SjSkinBoxItem
	long            m_prop;
};


class SjSkinItemTimer : public wxTimer
{
public:
	                SjSkinItemTimer     (SjSkinItem* i) { m_item = i; }
	void            Notify              () { wxASSERT(m_item); if( m_item ) { m_item->OnTimer(); } }

private:
	SjSkinItem* m_item;
};


class SjSkinBoxItem : public SjSkinItem
{
public:
	bool            Create              (const wxHtmlTag&, wxString& error);
	void            SetValue            (const SjSkinValue&);
	void            OnPaint             (wxDC&);
	bool            OnImageThere        (wxDC& dc, SjImgThreadObj* obj);
	void            OnMouseLeftDown     (long x, long y, bool doubleClick, long accelFlags);
	SjMouseUsed     OnMouseLeftUp       (long x, long y, long accelFlags, bool captureLost);
	bool            OnMouseMiddle       (long x, long y);
	void            OnMouseMotion       (long x, long y, bool leftDown);
	void            OnMouseLeave        ();
	void            OnTimer             ();

private:
	// common state
	bool            m_border;
	long            m_flags;
	wxString        m_text;
	long            m_runningMs;
	long            m_totalMs;
	long            m_centerOffset;
	wxFont          m_font;
	int             m_fontHeight;

	// mouse state
	int             m_mouseState;
	int             m_mouseSubitem; // the current "hover" item or the item on "mouse down"
	long            m_mouseDownX,
	                m_mouseDownY,
	                m_mouseResumeX,
	                m_mouseResumeY;
	bool            m_mouseMoveReported;
	unsigned long   m_timerLastMoveMs, m_timerMoves;
	bool            CheckMovementTimer  ();

	// drawing
	void            DrawBackground      (wxDC& dc);
	void            DrawImage           (wxDC& dc);
	void            DrawText            (wxDC& dc);
	void            DrawOverlay         (wxDC& dc);

	void            DrawTextPart        (wxDC& dc, const wxString& text, int x, int y, bool selected);
	void            DrawIcon            (wxDC& dc, const wxRect&, long icon, bool hilite);

	// subitems
	#define         SJ_SUBITEM_NONE               0x00
	#define         SJ_SUBITEM_ICONLEFT           0x01
	#define         SJ_SUBITEM_TEXT               0x02
	#define         SJ_SUBITEM_TIME               0x03
	#define         SJ_SUBITEM_ICONRIGHT          0x04
	#define         SJ_SUBITEM_OVERLAY            0x10 // can also be used for testing for _any_ overlay item
	#define         SJ_SUBITEM_OVERLAY_TIME       0x11
	#define         SJ_SUBITEM_OVERLAY_ICONRIGHT  0x12
	#define         SJ_SUBITEM_TEXT_MOUSEDOWN     0x20 // needed for passing events only
	#define         SJ_SUBITEM_TEXT_DCLICK        0x21 // needed for passing events only
	#define         SJ_SUBITEM_TEXT_MIDDLECLICK   0x22 // needed for passing events only
	int             FindSubitem         (long x, long y, wxRect& subitemRect);

	wxCoord         m_iconLeftW,
	                m_timeXrel, m_timeW,
	                m_iconRightXrel, m_iconRightW;

	// overlay subitems
	wxCoord         m_overlayXrel, m_overlayW,
	                m_overlayTimeXrel, m_overlayTimeW,
	                m_overlayIconRightW, m_overlayIconRightXrel;
	wxString        m_overlayParam;
	wxBrush         m_fgBrush; // used to draw the overlay background, we could also enable SjSkinColour::fbBrush, however, we need this _only_ for the prelisten overlay, so we do not created hundreds of brushes
	wxColour        m_overlayFgColour;
	wxPen           m_overlayFgPen;



	friend class    SjSkinWindow;
};


class SjSkinImageItem : public SjSkinItem
{
public:
	bool            Create              (const wxHtmlTag&, wxString& error);
	void            OnPaint             (wxDC&);

private:
	bool            m_horizontal;
	wxBitmap*       m_bitmapPrologue;
	wxBitmap*       m_bitmapRepeat;
	wxBitmap*       m_bitmapEpilogue;
};


class SjSkinButtonItem : public SjSkinItem
{
public:
	bool            Create              (const wxHtmlTag&, wxString& error);
	void            SetValue            (const SjSkinValue&);
	void            OnMouseLeftDown     (long x, long y, bool doubleClick, long accelFlags);
	SjMouseUsed     OnMouseLeftUp       (long x, long y, long accelFlags, bool captureLost);
	void            OnMouseMotion       (long x, long y, bool leftDown);
	void            OnMouseLeave        ();
	void            OnTimer             ();
	void            OnPaint             (wxDC&);

private:
	// the button state
	#define         SJ_BUTTON_STATE_NORMAL      0
	#define         SJ_BUTTON_STATE_SELECTED    1
	#define         SJ_BUTTON_STATE_OTHER       2
	#define         SJ_BUTTON_STATE_COUNT       3
	int             m_buttonState;

	// the mouse state
	#define         SJ_MOUSE_STATE_NORMAL   0
	#define         SJ_MOUSE_STATE_HOVER    1
	#define         SJ_MOUSE_STATE_CLICKED  2
	#define         SJ_MOUSE_STATE_COUNT    3
	int             m_mouseState;

	// bitmaps - button/mouse state
	wxBitmap*       m_bitmaps[SJ_BUTTON_STATE_COUNT][SJ_MOUSE_STATE_COUNT];
	bool            m_useEventRepeating;
	bool            m_inTimer;
	bool            m_subsequentTimer;
	bool            m_ldownInsideContextMenu;
	wxString        m_onclick;

	// blinking buttons
	int             m_doBlink;
	int             m_doBlinkState;

	// This flag is set for buttons as "play" or "next" which actions may take a seconds
	// to perform.  Instead of showing a hourglass, we leave the button in the pressed
	// state until SetValue() is caled.
	bool            m_delayedRedraw;

	// context menu width aright of button
	int             m_contextMenuWidth;
};


class SjSkinScrollbarItemPart
{
private:
	SjSkinScrollbarItemPart()
	{
		m_mouseState = 0;
		int i;
		for( i = 0; i < 3; i++ )
		{
			m_bitmapPrologue[i] = 0;
			m_bitmapRepeat[i] = 0;
			m_bitmapEpilogue[i] = 0;
		}
	}

	wxRect          m_rect;
	int             m_mouseState; // 0=normal, 1=hover, 2=clicked
	wxBitmap*       m_bitmapPrologue[3]; // 0=normal, 1=hover, 2=clicked
	wxBitmap*       m_bitmapRepeat[3];
	wxBitmap*       m_bitmapEpilogue[3];

	friend class    SjSkinScrollbarItem;
};


class SjSkinScrollbarItem : public SjSkinItem
{
public:
	                SjSkinScrollbarItem ();
	bool            Create              (const wxHtmlTag&, wxString& error);
	void            SetValue            (const SjSkinValue&);
	void            OnMouseLeftDown     (long x, long y, bool doubleClick, long accelFlags);
	SjMouseUsed     OnMouseLeftUp       (long x, long y, long accelFlags, bool captureLost);
	void            OnMouseMotion       (long x, long y, bool leftDown);
	void            OnMouseLeave        ();
	void            OnTimer             ();
	void            OnSize              ();
	void            OnPaint             (wxDC&);

private:
	// state of the scrollbar
	long            m_value,
	                m_vmin,
	                m_vmax,
	                m_vrange,
	                m_thumbSize;

	// everything here is calculated for a horizontal scrollbar,
	// x/y and width/height are swapped on input/output as needed.
	long            m_minThumbWidth;
	wxRect          m_allRect;

	SjSkinScrollbarItemPart m_pageLeftPart, m_thumbPart, m_pageRightPart;

	SjSkinScrollbarItemPart* m_hoverPart;

	SjSkinScrollbarItemPart* m_trackPart;
	long            m_trackStartValue,
	                m_trackStartX,
	                m_trackStartY;

	bool            m_horizontal,
	                m_flip,
	                m_inTimer,
	                m_subsequentTimer;
	bool            m_hideIfUnused,
	                m_hideScrollbar;
	void            SwapNDrawPart       (wxDC& dc, SjSkinScrollbarItemPart&, bool alignMToR = FALSE);
	SjSkinScrollbarItemPart* SwapNFindPart (long x, long y);
	void            CheckNSendValue     (long newValue);
	void            Swap                (long& x, long& y) { if( !m_horizontal ) { long temp = x; x = y; y = temp; } }
};



class SjSkinWorkspaceItem : public SjSkinItem
{
public:
	bool            Create              (const wxHtmlTag&, wxString& error);
	void            OnSize              ();
	void            OnPaint             (wxDC&) { }
};


class SjSkinInputItem : public SjSkinItem
{
public:
	bool            Create              (const wxHtmlTag&, wxString& error);
	void            OnPaint             (wxDC&) ;
	void            OnSize              ();

private:
};


class SjSkinDivItem : public SjSkinItem
{
public:
	bool            Create              (const wxHtmlTag&, wxString& error);
	wxRect          GetIndent           () const { return m_indent; }

private:
	wxRect          m_indent;
};


class SjSkinTarget
{
private:
	SjSkinTarget        ()
	{
		#ifdef SJ_SKIN_USE_HIDE
		m_hidden = FALSE;
		#endif
	}
	// this list reflects the items handling the given target in the current layout
	// (normally identified by the index in an array of targets)
	SjSkinItemList  m_itemList;

	// the value of the target
	SjSkinValue  m_value;
	#ifdef SJ_SKIN_USE_HIDE
	bool            m_hidden;
	#endif

	friend class    SjSkinWindow;
};


class SjSkinImage
{
public:
	                SjSkinImage         ();
	                ~SjSkinImage        ();

	wxBitmap*       GetSubimage         (int indexX, int indexY);
	int             GetSubimageXCount   () { return m_subimageXCount; }
	int             GetSubimageYCount   () { return m_subimageYCount; }
	bool            HasMaskOrAlpha      () { return m_hasMaskOrAlpha; }

private:
	#define         SJ_SKIN_SUBIMAGES_MAX 168 // 28 (A..Z + 0-9 + Curr) * 6
	wxBitmap*       m_subimages[SJ_SKIN_SUBIMAGES_MAX];
	int             m_subimageWidths[SJ_SKIN_SUBIMAGES_MAX];
	int             m_subimageHeights[SJ_SKIN_SUBIMAGES_MAX];
	int             m_subimageXCount;
	int             m_subimageYCount;
	wxString        m_file;
	bool            m_hasMaskOrAlpha;

	friend class    SjSkinMlParserData;
};


WX_DECLARE_LIST(SjSkinImage, SjSkinImageList);


class SjSkinLayout
{
public:
	// constructor / destructor
	                SjSkinLayout        (SjSkinSkin*, const wxString& name, int index);
	                ~SjSkinLayout       ();

	// get information
	wxString        GetName             () const { return m_name; }
	void            GetMinMax           (wxSize& min, wxSize& max) const {min=m_minSize; max=m_maxSize;}

private:
	// layout settings
	SjSkinSkin*     m_skin;  // pointer back to the skin
	int             m_index;
	wxString        m_name;
	SjSkinItemList  m_itemList;
	int             m_linesCount;
	bool            m_hasWorkspace,
	                m_hasInput;
	wxSize          m_minSize,
	                m_maxSize,
	                m_defSize;
	wxRect          m_currRect;
	wxString        m_useWidth,
	                m_useHeight,
	                m_usePos;
	bool            m_alwaysOnTop;

	friend class    SjSkinWindow;
	friend class    SjSkinSkin;
	friend class    SjSkinItem;
	friend class    SjSkinMlParser;
	friend class    SjSkinMlParserData;
	friend class    SjSkinMlTagHandler;
};


WX_DECLARE_LIST(SjSkinLayout, SjSkinLayoutList);


class SjSkinSkin : public wxObject // wxObject needed for SjSkinMlParser::GetProduct()
{
public:
	               SjSkinSkin           ();
	                ~SjSkinSkin         ();

	wxString        GetUrl              () const { return m_givenUrl; }
	wxString        GetName             () const { return m_name; }
	wxString        GetAbout            () const { return m_about; }
	bool            GetDebugOutline     () const { return m_debugOutline; }
	int             GetLayoutCount      () const { return m_layoutList.GetCount(); }
	SjSkinLayout*   GetLayout           (const wxString& name);
	SjSkinLayout*   GetLayout           (int index);
	void            ConnectToSkinWindow (SjSkinWindow*);

	bool            HasTooltipColours   () const { return m_tooltipColoursSet; }
	wxColour        GetTooltipFgColour  () const { return m_tooltipFgColour; }
	wxColour        GetTooltipBgColour  () const { return m_tooltipBgColour; }
	wxColour        GetTooltipBorderColour () const { return m_tooltipBorderColour; }

	// m_hasScripts is TRUE if either m_globalScripts is set or there is any onclick-handler
	bool            m_hasScripts;
	wxString        m_globalScript;
	#if SJ_USE_SCRIPTS
	SjSee*          m_see;  // may be used by the caller to execute scripts - we only load the scripts but do not execute them
	#endif

private:
	// skin settings
	wxString        m_givenUrl;
	wxString        m_baseDir;
	wxString        m_name;
	wxString        m_about;
	SjSkinLayoutList m_layoutList;

	// the dummy colours used by an item if no other colours are set
	SjSkinColour    m_itemDefColours[SJ_COLOUR_COUNT];

	// debug settings
	bool            m_debugInfo,
	                m_debugOutline;

	// image handling
	SjSkinImageList
	m_imageList;

	// tooltips
	wxColour        m_tooltipBgColour,
	                m_tooltipFgColour,
	                m_tooltipBorderColour;
	bool            m_tooltipColoursSet;

	friend class    SjSkinMlParser;
	friend class    SjSkinMlParserData;
	friend class    SjSkinMlTagHandler;
	friend class    SjSkinLayout;
};


#if SJ_USE_TOOLTIPS
class SjSkinWindowToolTipProvider : public SjToolTipProvider
{
public:
	wxString        GetText             (long& flags) { return ::wxGetTranslation(m_item->m_itemTooltip? *m_item->m_itemTooltip : wxT("")); }
	wxWindow*       GetWindow           () { return (wxWindow*)m_item->m_skinWindow; }
	wxRect          GetLocalRect        () { return m_item->m_rect; }

private:
	SjSkinItem* m_item;
	friend class    SjSkinWindow;
};
#endif


enum SjLoadLayoutFlag
{
    SJ_AUTO_SIZE_CHANGE=0, // =SJ_NO_SIZE_CHANGE in kiosk mode, else SJ_FORCE_SIZE_CHANGE
    SJ_NO_SIZE_CHANGE=1,
    SJ_FORCE_SIZE_CHANGE=2
};


class SjSkinWindow : public wxFrame
{
public:
	                SjSkinWindow        (wxWindow* parent, wxWindowID id, long skinFlags, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE);
	                ~SjSkinWindow       ();

	// load a skin, errors are given to wxLog* and should be displayed automatically
	bool            LoadSkin            (const wxString& path, long conditions, const wxString& skinSettings=wxEmptyString, bool reloadScripts=true, wxWindow* parent = NULL);
	void            LoadLayout          (SjSkinLayout*, SjLoadLayoutFlag sizeChangeFlag=SJ_AUTO_SIZE_CHANGE);
	bool            SkinOk              () const { return m_currLayout? TRUE : FALSE; }
	void            ReloadSkin          (long conditions, bool alsoReloadScripts, SjLoadLayoutFlag sizeChangeFlag=SJ_AUTO_SIZE_CHANGE);

	// retrieve some skin settings
	wxString        GetSkinUrl          () const { if(m_currLayout) { return m_currSkin->GetUrl(); } else { return wxEmptyString; } }
	wxString        GetSkinName         () const { if(m_currLayout) { return m_currSkin->GetName(); } else { return wxEmptyString; } }
	wxString        GetSkinAbout        () const { if(m_currLayout) { return m_currSkin->GetAbout(); } else { return wxEmptyString; } }
	int             GetLayoutCount      () const { if(m_currLayout) { return m_currSkin->GetLayoutCount(); } else { return 0; } }
	SjSkinLayout*   GetLayout           (int index) const { return m_currLayout? m_currSkin->GetLayout(index) : NULL; }
	SjSkinLayout*   GetLayout           (const wxString& name) const { return m_currLayout? m_currSkin->GetLayout(name) : NULL; }
	SjSkinLayout*   GetLayout           () const { return m_currLayout; }
	bool            IsDefaultLayout     () const { return m_currLayout? (m_currLayout==m_currSkin->GetLayout(0)) : FALSE; }
	int             GetLinesCount       () const { return m_currLayout? m_currLayout->m_linesCount : 0; }
	void            GetLayoutMinMax     (const SjSkinLayout* l, wxSize& min, wxSize& max)  const { if(l) {l->GetMinMax(min,max);} else {min.x=32; min.y=32; max.x=32000; max.y=32000;} }
	void            GetVisEmbedRect     (wxRect* r=NULL, bool* retIsOverWorkspace=NULL, bool* retVisAutoStart=NULL) const;
	wxString        GetSavableSkinSettings();
	long            GetTargetProp       (int targetId);

	// tooltip handling, using "data", the tooltips may also be
	// used for child windows
	#if SJ_USE_TOOLTIPS
	virtual SjToolTipProvider* GetToolTipProvider  (long targetId, long subitem, const wxRect& rect) = 0;
	#endif
	bool            IsMouseInDisplayMove() const { return m_mouseInDisplayMove; }

	// for setting values for skin targets, use the following
	// function
	#ifdef SJ_SKIN_USE_HIDE
	bool            HideSkinTarget      (int id, bool hide=TRUE, bool redraw=TRUE);
	void            HidingSkinTargetsDone (bool redraw=TRUE);
	#endif
	void            SetSkinTargetValue  (int id, const SjSkinValue&, bool onlyIfPossible=FALSE);
	void            SetSkinTargetValueIfPossible(int id, const SjSkinValue& v) { SetSkinTargetValue(id, v, TRUE); }
	void            SetSkinTargetValue  (int id, long v) { SjSkinValue vo; vo.value=v; SetSkinTargetValue(id, vo); }
	const SjSkinValue& GetSkinTargetValue  (int id) const { return m_targets[id].m_value; }
	bool            HasSkinTarget       (int id) const;
	void            SetSkinText         (const wxString& userId, const wxString& text);

	// retrieving events from the skin targets; for this purpose, you have
	// to subclass SjSkin and implement OnSkinTargetEvent()
	virtual void    OnSkinTargetEvent   (int targetId, SjSkinValue&, long accelFlags) = 0;
	virtual void    OnSkinTargetContextMenu (int targetId, long x, long y) = 0;
	virtual void    OnSkinTargetMotion  (int targetId, int motionAmount) = 0;
	virtual void    UpdateMenuBarValue  (int targetId, const SjSkinValue&) {}
	void            ResumeSkinTargetMotion (int clickTargetId, int resumeTargetId);
	wxWindow*       GetInputWindow      () const {return m_inputWindow;}

	// find the target ID at a given postion,
	// only clickable targets are found
	int             FindTargetId        (long x, long y) const { SjSkinItem* i=FindClickableItem(x, y); return i? i->m_targetId : 0; }

	// direct links to some items in the current layout
	SjSkinColour*   m_workspaceColours;
	SjSkinColour    m_workspaceColours__[SJ_COLOUR_COUNT];

	// The Image Thread
	SjImgThread*    m_imgThread;

	// Flags (public read)
	#define         SJ_SKIN_SHOW_DISPLAY_TRACKNR    0x00000008L
	#define         SJ_SKIN_SHOW_DISPLAY_ARTIST     0x00000010L
	#define         SJ_SKIN_SHOW_DISPLAY_TOTAL_TIME 0x00000020L
	#define         SJ_SKIN_IMG_SMOOTH              0x00000080L
	#define         SJ_SKIN_SHOW_DISPLAY_AUTOPLAY   0x00010000L
	#define         SJ_SKIN_PREFER_TRACK_COVER      0x00020000L
	#define         SJ_SKIN_DEFAULT_FLAGS           0x0000FFFFL
	long            m_skinFlags;

	// a drag'n'drop bitmat that may be used by any instance,
	// when setting m_dragImage, the caller should update
	// m_dragRect (in screen coordinates) on every move as we
	// check intersections with this rectangle on our drawings.
	wxDragImage*    m_dragImage;
	wxRect          m_dragRect;
	wxPoint         m_dragHotspot;

	// "always on top" handling
	bool            IsAlwaysOnTop       () const { return m_currLayout? m_currLayout->m_alwaysOnTop : FALSE; }
	void            ShowAlwaysOnTop     (bool alwaysOnTop);
	void            MoveWorkspaceAway   (bool);
	bool            IsWorkspaceMovedAway() const { return (m_workspaceMovedAway || (m_currLayout&&!m_currLayout->m_hasWorkspace)); }

	// our workaround for the GTK-SetSizeHack
	#ifdef __WXGTK__
		wxTimer         m_setSizeHackTimer;
		wxRect			m_setSizeHackRect;
		long            m_setSizeHackCnt;
		void            OnSetSizeHackTimer  (wxTimerEvent& event);
	#endif

protected:
	void            SetWorkspaceWindow  (wxWindow*);
	void            SetInputWindow      (wxWindow*);

	wxRect          CheckLayoutWindowRect(const SjSkinLayout*, const wxRect&, bool* retBadSize=NULL) const;

private:
	// skin settings of the loaded skin
	SjSkinSkin*     m_currSkin;
	SjSkinLayout*   m_currLayout; // use the layout to check if the skin is okay!
	wxWindow*       m_workspaceWindow;
	bool            m_workspaceMovedAway;
	wxWindow*       m_inputWindow;
	wxFont          m_inputWindowDefFont;
	long            m_inputWindowFontHeight;
	wxColour        m_inputWindowFgColour;
	wxColour        m_inputWindowBgColour;

	// target states, SjSkinTarget::m_itemList reflect
	// the items of the current layout
	SjSkinTarget m_targets[IDT_LAST+1];

	// calculate all positions, working with positions
	void            CalcItemRectangles  (long width, long height);
	void            CalcChildItemRectangles (SjSkinItem* parent);
	SjSkinItem*     FindClickableItem   (long x, long y) const;
	SjSkinItem*     FindFirstItemByTargetId (int targetId) const;
	void            SaveSizes           ();

	// mouse handling
	SjSkinItem*  m_mouseItem;
	#if SJ_USE_TOOLTIPS
	SjSkinWindowToolTipProvider m_skinWindowToolTipProvider;
	#endif
	bool            m_mouseInDisplayMove;

	// maximize handling
	wxRect          m_rectBeforeMaximize;

	// events
	void            OnSetFocus          (wxFocusEvent&);
	void            OnSize              (wxSizeEvent&);
	void            OnMove              (wxMoveEvent&);
	void            OnMouseLeftDown     (wxMouseEvent&);
	void            OnMouseLeftUp       (wxMouseEvent&);
	void            OnMouseCaptureLost  (wxMouseCaptureLostEvent&);
	void            OnMouseRight        (wxContextMenuEvent&);
	void            OnMouseMiddleUp     (wxMouseEvent&);
	void            OnMouseMotion       (wxMouseEvent&);
	void            OnMouseLeave        (wxMouseEvent&);
	void            OnEraseBackground   (wxEraseEvent&);
	void            OnPaint             (wxPaintEvent&);
	void            OnImageThere        (SjImageThereEvent&);

	// drawing
	void            RedrawAll           (wxDC&, const wxRect* rect = NULL, long finalMoveX = 0, long finalMoveY = 0);
	void            RedrawFinalLines    (wxDC&, long finalMoveX = 0, long finalMoveY = 0);
	                DECLARE_EVENT_TABLE ()

	// friend classes
	friend class    SjSkinItem;
	friend class    SjSkinWorkspaceItem;
	friend class    SjSkinInputItem;
	friend class    SjSkinDivItem;
	friend class    SjSkinBoxItem;
	friend class    SjSkinButtonItem;
	friend class    SjMainFrame;
};



#endif // __SJ_SKIN_H__

