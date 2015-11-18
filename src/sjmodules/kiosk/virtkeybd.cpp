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
 * File:    virtkeybd.cpp
 * Authors: Björn Petersen
 * Purpose: The virtual keyboard
 *
 *******************************************************************************
 *
 * A good overview about different keyboard layouts is available at:
 * http://www.microsoft.com/globaldev/reference/keyboards.mspx
 * (Internet Explorer, JavaScript etc. required :-()
 *
 * *.sjk-file-commands are case sensitive now
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/kiosk/virtkeybd.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayVirtKeybdKey);
WX_DEFINE_OBJARRAY(SjArrayVirtKeybdLayout);


/*******************************************************************************
 * SjVirtKeybdTextCtrl class
 ******************************************************************************/


static bool s_inTimeredClose = FALSE;


#define IDO_VIRTKEYBDFWD (IDM_FIRSTPRIVATE+666)


BEGIN_EVENT_TABLE(SjVirtKeybdTextCtrl, wxTextCtrl)
	EVT_LEFT_DOWN(SjVirtKeybdTextCtrl::OnMyMouseDown    )
	EVT_MENU(IDO_VIRTKEYBDFWD, SjVirtKeybdTextCtrl::OnMyMouseDownDo)
END_EVENT_TABLE()


SjVirtKeybdTextCtrl::SjVirtKeybdTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style)
	: wxTextCtrl(parent, id, value, pos, size, style)
{
	m_forceForTesting = FALSE;
}


void SjVirtKeybdTextCtrl::OnMyMouseDown(wxMouseEvent& e)
{
	if( g_virtKeybd
	        && !s_inTimeredClose
	        && IsShown()
	        && IsEnabled() )
	{
		wxWindow* topLevel = SjDialog::FindTopLevel(this);
		if( topLevel->IsShown()
		        && topLevel->IsEnabled() )
		{
			#ifdef __WXMAC__ // if we create the window from within the mouse event on wxMac, it does not stay raised - so do this in a delayed event
				wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, IDO_VIRTKEYBDFWD);
				AddPendingEvent(fwd);
			#else
				g_virtKeybd->OpenKeybd(this, m_forceForTesting);
			#endif
		}
	}

	e.Skip();
}


void SjVirtKeybdTextCtrl::OnMyMouseDownDo(wxCommandEvent& e)
{
	g_virtKeybd->OpenKeybd(this, m_forceForTesting);
}


SjVirtKeybdTextCtrl::~SjVirtKeybdTextCtrl()
{
	if( g_virtKeybd
	        && g_virtKeybd->GetInputReceiver() == this )
	{
		g_virtKeybd->CloseKeybd();
	}
}


/*******************************************************************************
 * SjVirtKeybdKey class
 ******************************************************************************/


SjVirtKeybdKey::SjVirtKeybdKey(const wxString& keyNormal)
{
	for( int s = 0; s < SJ_VK_MAX_SHIFT; s++ )
	{
		for( int a = 0; a < SJ_VK_MAX_ALT; a++ )
		{
			m_keyFlags[s][a] = 0;
		}
	}

	m_keys[0][0] = ParseKey(keyNormal, wxT("spacer"), m_keyTitles[0][0], m_keyFlags[0][0]);
	m_relXPos = 0.0;
	m_relWidth = 1.0;
}


void SjVirtKeybdKey::SetKey(const wxString& type__, const wxString& key)
{
	wxString type(type__);

	bool shiftSet = FALSE;
	int shift = 0;
	if( type.Right(5) == wxT("shift") )
	{
		shift = 1;
		shiftSet = TRUE;
		type = type.Left(type.Len()-5);
	}

	bool altSet = FALSE;
	int alt = 0;
	if( type.Left(3) == wxT("alt") )
	{
		long l;
		if( type.Mid(3).ToLong(&l, 10)
		        && l >= 1
		        && l <  SJ_VK_MAX_ALT )
		{
			alt = l;
			altSet = TRUE;
		}
	}

	if( !shiftSet && !altSet )
	{
		return; // invalid key type
	}

	m_keys[shift][alt] = ParseKey(key, wxT(""), m_keyTitles[shift][alt], m_keyFlags[shift][alt]);
}


wxString SjVirtKeybdKey::ParseKey(const wxString& key__, const wxString& defaultKey, wxString& retKeyTitle, long& retKeyFlags)
{
	wxString key(key__);

	// strip title from key (the title may be added using double quotes)
	if( !key.IsEmpty() && key.Last() == wxT('"') )
	{
		int p = key.Find(wxT('"'));
		wxASSERT( p != -1 );
		if( p != (int)key.Len()-1 )
		{
			retKeyTitle = key.Mid(p+1);
			retKeyTitle = retKeyTitle.Left(retKeyTitle.Len()-1);
			retKeyTitle.Replace(wxT("\\n"), wxT("\n"));

			key = key.Left(p).Trim();
		}
	}

	// strip flags from key
	while( key.Find(wxT(' ')) != wxNOT_FOUND )
	{
		wxString flag = key.AfterLast(wxT(' '));    // read flag ...
		key = key.BeforeLast(wxT(' '));             // ... before overwriting key
		if( flag == wxT("lock") )
		{
			retKeyFlags |= SJ_VK_LOCK;
		}
	}


	// parse key
	if( key.IsEmpty() )
	{
		return defaultKey;
	}
	else if( key == wxT("nextline")
	         || key == wxT("shift")
	         || key == wxT("nop")
	         || key == wxT("backsp")
	         || key == wxT("clearall")
	         || key == wxT("enter")
	         || key == wxT("spacer") )
	{
		return key;
	}
	else if( key.Left(3) == wxT("alt") )
	{
		long l;
		if( key.Mid(3).ToLong(&l, 10)
		        && l >= 0
		        && l < SJ_VK_MAX_ALT )
		{
			return key;
		}
		else
		{
			return defaultKey;
		}
	}
	else if( key.Left(2) == wxT("0x") )
	{
		long l;
		if( key.Mid(2).ToLong(&l, 16)
		        && l >= 0 )
		{
			#if wxUSE_UNICODE
				// in ISO8859-1 the "Euro" sign has the code 0x80 where
				// in Unicode the code is 0x20AC - fix this little incompatibility as the file encoding
				// is defined to use ISO8859-1.
				if( l == 0x0080 )
					l = 0x20AC;
			#endif

			return wxString((wxChar)l);
		}
		else
		{
			return defaultKey;
		}
	}
	else
	{
		#if wxUSE_UNICODE
			// see remark above
			if( key[0] == 0x0080 )
				return wxString((wxChar)0x20AC);
		#endif

		return key;
	}
}


wxString SjVirtKeybdKey::TranslTitle(int index) const
{
	wxString title = _("Shift|AltGr|Delete|Clear\nall|OK");
	while( index )
	{
		title = title.AfterFirst('|');
		index--;
	}
	return title.BeforeFirst('|');
}
wxString SjVirtKeybdKey::GetKeyTitle(int inShifted, int inAlt) const
{
	int outShifted, outAlt;
	bool found = FindKey(inShifted, inAlt, outShifted, outAlt);

	wxString title;
	if( !found )
	{
		return wxT("");
	}
	else if( !m_keyTitles[outShifted][outAlt].IsEmpty()
	         || m_keys[outShifted][outAlt] == wxT("nop") )
	{
		title = m_keyTitles[outShifted][outAlt];
	}
	else
	{
		title = m_keys[outShifted][outAlt];
	}

	if( title == wxT("shift") )                        { return TranslTitle(0);    }
	else if( title == wxT("altgr") || title == wxT("alt1") ) { return TranslTitle(1);    }
	else if( title == wxT("backsp") )                       { return TranslTitle(2);    }
	else if( title == wxT("clearall") )                     { return TranslTitle(3);    }
	else if( title == wxT("enter") )                        { return TranslTitle(4);    }
	else                                                    { return title;             }
}


int SjVirtKeybdKey::IsAlt(int shifted, int alt) const
{
	wxString key = GetKey(shifted, alt);
	if( key.Left(3) == wxT("alt") )
	{
		long l;
		if( key.Mid(3).ToLong(&l, 10) )
		{
			if( l >= 0 && l < SJ_VK_MAX_ALT )
			{
				return l;
			}
		}
	}
	return 0;
}


wxString SjVirtKeybdKey::GetKey(int inShifted, int inAlt) const
{
	int outShifted, outAlt;
	bool found = FindKey(inShifted, inAlt, outShifted, outAlt);

	if( !found )
	{
		return wxT("nop");
	}
	else
	{
		return m_keys[outShifted][outAlt];
	}
}


bool SjVirtKeybdKey::FindKey(int inShifted, int inAlt, int& outShifted, int& outAlt) const
{
	if( inShifted < 0 || inShifted > SJ_VK_MAX_SHIFT ) inShifted = 0;
	if( inAlt < 0 || inAlt >= SJ_VK_MAX_ALT ) inAlt = 0;

	if( !m_keys[inShifted][inAlt].IsEmpty() )
	{
		outShifted = inShifted;
		outAlt = inAlt;
		return TRUE;
	}
	else if( !m_keys[0][inAlt].IsEmpty() )
	{
		outShifted = 0;
		outAlt = inAlt;
		return TRUE;
	}

	outShifted = 0;
	outAlt = 0;
	return FALSE;
}


/*******************************************************************************
 * SjVirtKeybdLayout class
 ******************************************************************************/


SjVirtKeybdLayout::SjVirtKeybdLayout()
{
	m_lineCount = 1;
	m_totalRelWidth = 0;
	m_enterKey = NULL;
	m_enterCont = NULL;
}


bool SjVirtKeybdLayout::LoadLayoutFromFile(const wxString& file__, wxArrayString* allNames)
{
	// split index from file name
	wxString file;
	long currIndex = -1, wantedIndex = 0;
	if( file__.Find(wxT(','), TRUE) <= file__.Find(wxT('.'), TRUE) )
	{
		file = file__;
	}
	else
	{
		if( !file__.AfterLast(wxT(',')).ToLong(&wantedIndex, 10) ) { wantedIndex = 0; }
		file = file__.BeforeLast(wxT(','));
	}

	// init
	m_keys.Clear();
	m_file = file;
	m_name = file;
	m_lineCount = 1;
	m_totalRelWidth = 0;

	if( allNames )
	{
		allNames->Clear();
	}

	// read file content
	wxFileSystem fs;
	wxFSFile* fsFile = fs.OpenFile(file);
	if( fsFile == NULL )
	{
		return FALSE;
	}

	wxString fileContent = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1);
	delete fsFile;

	// parse file
	fileContent.Replace(wxT(";"), wxT("\n"));

	SjLineTokenizer tkz(fileContent);
	wxChar*         linePtr;
	wxString        line, lineType, lineValue;
	SjVirtKeybdKey* currKey = NULL;
	while( (linePtr=tkz.GetNextLine()) != NULL )
	{
		line = linePtr;

		// get key'n'value pair (currLine is already trimmed aleft and aright)
		if( line.Find('=') != -1 )
		{
			lineType = line.BeforeFirst('=').Trim().Lower();
			lineValue = line.AfterFirst('=').Trim(FALSE);
		}
		else
		{
			lineType = line.Lower();
			lineValue.Empty();
		}

		if( lineType == wxT("layout") )
		{
			if( allNames )
			{
				allNames->Add(lineValue);
			}

			currIndex++;
			if( currIndex == wantedIndex )
			{
				m_name = lineValue;
			}
		}
		else if( currIndex != wantedIndex )
		{
			;
		}
		else if( lineType == wxT("key") )
		{
			currKey = new SjVirtKeybdKey(lineValue);
			m_keys.Add(currKey);
		}
		else if( lineType == wxT("spacer") || lineType == wxT("nextline") )
		{
			currKey = new SjVirtKeybdKey(lineType);
			m_keys.Add(currKey);
		}
		else if( currKey )
		{
			if( lineType == wxT("width") )
			{
				currKey->SetRelWidth(lineValue);
			}
			else
			{
				currKey->SetKey(lineType, lineValue);
			}
		}
	}

	if( currIndex < wantedIndex )
	{
		return FALSE; // layout not found
	}

	// calculate the max. relative width and the rel. x-positions
	float currX = 0.0;
	float lineRelWidth = 0.0;
	long i, iCount = m_keys.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		currKey = &(m_keys[i]);

		if( currKey->IsNextLine() )
		{
			if( lineRelWidth > m_totalRelWidth )
			{
				m_totalRelWidth = lineRelWidth;
			}
			lineRelWidth = 0;
			m_lineCount++;
			currX = 0.0;
		}
		else
		{
			if( currKey->IsEnter() )
			{
				m_enterKey = currKey;
			}
			else if( currKey->IsEnterCont() )
			{
				m_enterCont = currKey;
			}

			currKey->SetRelXPos(currX);
			currX += currKey->GetRelWidth();
			lineRelWidth += currKey->GetRelWidth();
		}
	}

	return TRUE;
}


void SjVirtKeybdLayout::CalcRects(const wxRect& totalRect)
{
	// calculate the normal key widths and heights
	float normalKeyWidth = 32, normalKeyHeight = 32;
	if( m_totalRelWidth > 0.0 )
	{
		normalKeyWidth = totalRect.width / m_totalRelWidth;
	}

	if( m_lineCount > 0 )
	{
		normalKeyHeight = totalRect.height / m_lineCount;
	}

	// calculate rough X-Positions and widths
	long currY = totalRect.y;
	long i, iCount = m_keys.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		SjVirtKeybdKey* key = &(m_keys[i]);

		if( key->IsNextLine() )
		{
			key->m_rect.x = totalRect.x-8000;
			key->m_rect.y = totalRect.y-8000;
			key->m_rect.width = (int)normalKeyWidth;
			key->m_rect.height = (int)normalKeyHeight;

			currY += (int)normalKeyHeight;
		}
		else
		{
			key->m_rect.x = totalRect.x + (long)(((float)totalRect.width / m_totalRelWidth) * key->GetRelXPos());
			key->m_rect.y = currY;
			key->m_rect.width = (int)(normalKeyWidth * key->GetRelWidth());
			key->m_rect.height = (int)normalKeyHeight;

		}
	}

	// correct the widths
	SjVirtKeybdKey* lastKey = NULL;
	for( i = 0; i < iCount; i++ )
	{
		SjVirtKeybdKey* key = &(m_keys[i]);

		if( key->IsNextLine() )
		{
			lastKey = NULL;
		}
		else if( lastKey == NULL )
		{
			lastKey = key;
		}
		else
		{
			lastKey->m_rect.width = key->m_rect.x - lastKey->m_rect.x;
			lastKey = key;
		}
	}

	if( m_enterKey && m_enterCont )
	{
		m_enterCont->m_rect.width =
		    (m_enterKey->m_rect.x + m_enterKey->m_rect.width) - m_enterCont->m_rect.x;
	}
}


SjVirtKeybdKey* SjVirtKeybdLayout::FindKey(int x, int y)
{
	long i, iCount = m_keys.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		SjVirtKeybdKey* key = &(m_keys[i]);
		if( x >= key->m_rect.x
		        && x <  key->m_rect.x + key->m_rect.width
		        && y >= key->m_rect.y
		        && y <  key->m_rect.y + key->m_rect.height )
		{
			if( key->IsEnterCont() && m_enterKey )
			{
				return m_enterKey;
			}
			return key;
		}
	}
	return NULL;
}


/*******************************************************************************
 * SjVirtKeybdWindow class
 ******************************************************************************/


class SjVirtKeybdFrame : public wxFrame
{
public:
	SjVirtKeybdFrame    (wxWindow* parent);
	void            InitKeybdFrame      ();
	void            InitCloseWatchTimer () { m_closeWatchTimer.Start(300); }
	~SjVirtKeybdFrame   ();

private:
	SjVirtKeybdLayout
	m_layout;

	int             m_shift;
	int             m_alt;
	SjVirtKeybdKey* m_clicked;
	bool            m_shiftLock, m_altLock;
	bool            m_autoClose, m_isActive;

	wxBrush         m_backgroundBrush;
	wxBrush         m_buttonBrush;
	wxPen           m_buttonPen;
	wxBrush         m_textBrush;
	wxPen           m_textPen;
	wxPen           m_hilitePen;
	wxPen           m_shadowPen;
	wxFont          m_textFontLarge;
	wxCoord         m_textFontLargeHeight;
	wxFont          m_textFontSmall;
	wxCoord         m_textFontSmallHeight;

	wxRect          m_closeClickRect;
	wxRect          m_closeDrawRect;

	unsigned long   m_lastUserInput;

	wxTimer         m_closeWatchTimer;

	void            RedrawAllOffscreen  (wxDC&);
	void            RedrawAllOnscreen   (wxDC&);
	void            RedrawButtonOnscreen(wxDC&, SjVirtKeybdKey* button);
	void            DrawButtonRect      (wxDC& dc, const wxRect& rect, bool invert, bool drawTopLine=TRUE, bool drawBottomLine=TRUE);

	bool            CheckClose          (wxWindow*);

	void            SendKeyEvent        (const wxString& key);
	wxString        ModifyValue         (const wxString& oldValue, const wxString& key);

	void            OnEraseBackground   (wxEraseEvent&) { }
	void            OnPaint             (wxPaintEvent&);
	void            OnMyClose           (wxCloseEvent&);
	void            OnMouseDown         (wxMouseEvent&);
	void            OnMouseUp           (wxMouseEvent&);
	void            OnMouseLeave        (wxMouseEvent&);
	void            OnCloseWatchTimer   (wxTimerEvent&);
	void            OnActivate          (wxActivateEvent&);
	DECLARE_EVENT_TABLE ()
};


BEGIN_EVENT_TABLE(SjVirtKeybdFrame, wxFrame)
	EVT_ERASE_BACKGROUND(SjVirtKeybdFrame::OnEraseBackground    )
	EVT_PAINT           (SjVirtKeybdFrame::OnPaint              )
	EVT_CLOSE           (SjVirtKeybdFrame::OnMyClose            )
	EVT_LEFT_DOWN       (SjVirtKeybdFrame::OnMouseDown          )
	EVT_RIGHT_DOWN      (SjVirtKeybdFrame::OnMouseDown          )
	EVT_MIDDLE_DOWN     (SjVirtKeybdFrame::OnMouseDown          )
	EVT_LEFT_UP         (SjVirtKeybdFrame::OnMouseUp            )
	EVT_RIGHT_UP        (SjVirtKeybdFrame::OnMouseUp            )
	EVT_MIDDLE_UP       (SjVirtKeybdFrame::OnMouseUp            )
	EVT_LEAVE_WINDOW    (SjVirtKeybdFrame::OnMouseLeave         )
	EVT_TIMER           (IDTIMER_VIRTKEYBD,
	                     SjVirtKeybdFrame::OnCloseWatchTimer    )
	EVT_ACTIVATE        (SjVirtKeybdFrame::OnActivate           )
END_EVENT_TABLE()


SjVirtKeybdFrame::SjVirtKeybdFrame(wxWindow* parent)
	: wxFrame(parent, IDO_VIRTKEYBDFRAME, wxEmptyString, wxDefaultPosition, wxDefaultSize,
	          wxNO_BORDER|wxFRAME_NO_TASKBAR|wxFRAME_FLOAT_ON_PARENT  )
{
	// set the window shaded
	int transparency = g_virtKeybd->GetTransparency();
	if( transparency
	        && g_tools->CanSetWindowTransparency() )
	{
		g_tools->SetWindowTransparency(this, transparency);
	}

	// initialize the "last user input" timestamp; as this is checked in the timer, this must be done
	// before the timer is started below!
	m_lastUserInput = SjTools::GetMsTicks();

	// start the timer (the timer is used to check when
	// the virtual keyboard should diappear)
	m_closeWatchTimer.SetOwner(this, IDTIMER_VIRTKEYBD);

	m_autoClose = true;
	m_isActive = true;

	SetCursor(SjVirtKeybdModule::GetStandardCursor());
}


SjVirtKeybdFrame::~SjVirtKeybdFrame()
{
}


void SjVirtKeybdFrame::OnActivate(wxActivateEvent& event)
{
	if( !event.GetActive() )
	{
		m_isActive = false;
	}
}


void SjVirtKeybdFrame::InitKeybdFrame()
{
	// create the needed pens and colours
	if( g_virtKeybd->GetKeybdFlags() & SJ_VIRTKEYBD_BLACK )
	{
		// black
		m_backgroundBrush.SetColour(0x00, 0x00, 0x00);
		m_buttonBrush    .SetColour(0x10, 0x10, 0x10);
		m_textBrush      .SetColour(0xD0, 0xD0, 0xD0);
		m_hilitePen      .SetColour(0x64, 0x64, 0x64);
		m_shadowPen      .SetColour(0x32, 0x32, 0x32);
	}
	else
	{
		// white
		m_backgroundBrush.SetColour(0xF0, 0xF0, 0xF0);
		m_buttonBrush    .SetColour(0xE0, 0xE0, 0xE0);
		m_textBrush      .SetColour(0x80, 0x80, 0x80);
		m_hilitePen      .SetColour(0x90, 0x90, 0x90);
		m_shadowPen      .SetColour(0x40, 0x40, 0x40);
	}

	m_buttonPen  .SetColour(m_buttonBrush.GetColour());
	m_textPen    .SetColour(m_textBrush.GetColour());

	// load the layout
	if( !m_layout.LoadLayoutFromFile(g_virtKeybd->GetKeybdLayout(), NULL) ) {
		if( g_virtKeybd->GetKeybdLayout().IsEmpty() ) {
			wxLogError(_("No virtual keyboard found, please add the *.sjk files to the search paths."));
		}
		else {
			wxLogError(_("Cannot open \"%s\"."), wxT("*.sjk"));
		}
	}
	m_shift = 0;
	m_alt = 0;
	m_clicked = NULL;
	m_shiftLock = false;
	m_altLock = false;

	// get layout width/height
	float aspectRatio = m_layout.GetAspectRatio();

	// get available screen rectangle
	wxRect displayRect;
	if( g_mainFrame->IsKioskStarted() )
	{
		displayRect = g_mainFrame->GetRect();
	}
	else
	{
		displayRect = SjDialog::GetDisplayWorkspaceRect(GetParent());
	}

	// get input receiver parent rectangle
	wxWindow* inputReceiver = g_virtKeybd->GetInputReceiver();
	wxRect frameRect;
	if( inputReceiver )
	{
		wxWindow* inputReceiverParent = SjDialog::FindTopLevel(inputReceiver);
		if( inputReceiverParent )
		{
			frameRect = inputReceiverParent->GetRect();
		}
	}

	// get input receiver rectangle
	wxRect inputRect;
	if( inputReceiver )
	{
		inputRect = inputReceiver->GetRect();
		inputRect.x = 0;
		inputRect.y = 0;
		inputReceiver->ClientToScreen(&inputRect.x, &inputRect.y);
	}

	// calculate the desired borders
	long borderTopBottom = displayRect.width/128;
	long borderLeft = displayRect.width / 64;
	long borderRight = displayRect.width / 32;

	// calculate the total width
	long preferredButtonWH = displayRect.width / 16;
	long totalW = (long)(preferredButtonWH * m_layout.GetRelWidth());
	if( totalW+borderLeft+borderRight > displayRect.width )
	{
		totalW = displayRect.width - borderLeft+borderRight;
	}

	if( totalW < 280 )
	{
		totalW = 280;
	}

	// calculate the total height
	long totalH = (long)((float)totalW / aspectRatio);
	if( totalH+borderTopBottom > displayRect.height/2 )
	{
		totalH = displayRect.height/2 - borderTopBottom*2;
	}

	if( totalH < 200 )
	{
		totalH = 200;
	}

	// add the borders to the total width/height
	totalW += borderLeft+borderRight;
	totalH += borderTopBottom*2;

	// calculate the x-postion
	long totalX = (frameRect.x + frameRect.width/2) - totalW / 2;
	if( totalX < displayRect.x )
	{
		totalX = displayRect.x;
	}
	else if( totalX + totalW > displayRect.x + displayRect.width )
	{
		totalX = (displayRect.x+displayRect.width) - totalW;
	}

	// calculate the y-postion
	long totalY = frameRect.y + frameRect.height;
	if( totalY + totalH > displayRect.y + displayRect.height )
	{
		totalY = (displayRect.y+displayRect.height) - totalH;
	}

	// set the size if the window
	SetSize(totalX, totalY, totalW, totalH);

	// calculate the keyboard button positions (these are client positions);
	wxRect buttonsRect(borderLeft, borderTopBottom, totalW-borderLeft-borderRight, totalH-borderTopBottom*2);

	m_layout.CalcRects(buttonsRect);

	// calculate the close rect
	m_closeClickRect = wxRect(totalW-borderRight, 0, borderRight, borderRight);
	m_closeDrawRect = m_closeClickRect;
	m_closeDrawRect.x += borderTopBottom;
	m_closeDrawRect.y += borderTopBottom + (borderTopBottom/2);
	m_closeDrawRect.width -= borderTopBottom*2;
	m_closeDrawRect.height -= borderTopBottom*2;

	// calculate the height of the font
	m_textFontLarge = wxFont(10, wxSWISS, wxNORMAL, wxBOLD, FALSE, g_mainFrame->GetBaseFontFace());

	wxCoord testW;
	{
		wxClientDC testDC(this);
		long testPt = 32;
		long wantedH = 32;
		if( m_layout.m_keys.GetCount() )
		{
			wantedH = m_layout.m_keys[0].m_rect.height / 2;
		}

		while( 1 )
		{
			m_textFontLarge.SetPointSize(testPt);
			testDC.SetFont(m_textFontLarge);
			testDC.GetTextExtent(wxT("Ag"), &testW, &m_textFontLargeHeight);
			testDC.SetFont(*wxNORMAL_FONT);
			if( m_textFontLargeHeight<=wantedH || testPt<=8 )
			{
				break;
			}
			testPt--;
		}

		m_textFontSmall = wxFont((long)((float)m_textFontLarge.GetPointSize()*0.8),
		                         wxSWISS, wxNORMAL, wxBOLD, FALSE, g_mainFrame->GetBaseFontFace());

		testDC.SetFont(m_textFontSmall);
		testDC.GetTextExtent(wxT("Ag"), &testW, &m_textFontSmallHeight);
		testDC.SetFont(*wxNORMAL_FONT);
	}
}


void SjVirtKeybdFrame::OnMyClose(wxCloseEvent&)
{
	g_virtKeybd->CloseKeybd();
}


void SjVirtKeybdFrame::DrawButtonRect(wxDC& dc, const wxRect& rect, bool invert, bool drawTopLine, bool drawBottomLine)
{
	dc.SetBrush(invert? m_textBrush : m_buttonBrush);
	dc.SetPen(invert? m_textPen : m_buttonPen);
	dc.DrawRectangle(rect);

	dc.SetPen(m_hilitePen);
	if( drawTopLine )
	{
		dc.DrawLine(rect.x, rect.y, rect.x+rect.width, rect.y);
	}
	dc.DrawLine(rect.x, rect.y, rect.x, rect.y+rect.height);

	dc.SetPen(m_shadowPen);
	if( drawBottomLine )
	{
		dc.DrawLine(rect.x, rect.y+rect.height-1, rect.x+rect.width-1, rect.y+rect.height-1);
	}
	dc.DrawLine(rect.x+rect.width-1, rect.y, rect.x+rect.width-1, rect.y+rect.height);
}
void SjVirtKeybdFrame::RedrawButtonOnscreen(wxDC& dc, SjVirtKeybdKey* button)
{
	wxRect rect(button->m_rect);
	rect.Deflate(1);

	// invert?
	bool invert = FALSE;
	if( (m_shift && button->IsShift()               == m_shift)
	        || (m_alt   && button->IsAlt  (m_shift, m_alt) == m_alt  )
	        || (button == m_clicked)  )
	{
		invert = TRUE;
	}

	// get text to draw
	wxString textAll = button->GetKeyTitle(m_shift, m_alt);

	if( button->IsEnterCont() )
	{
		// don't draw the continued enter button here
		;
	}
	else if( button->IsEnter() && m_layout.m_enterCont )
	{
		// draw the continued enter button
		// (spans over two rows)
		wxRect spannedRect;

		wxRect rect2(m_layout.m_enterCont->m_rect);
		rect2.Deflate(1);

		if( rect.width >= rect2.width )
		{
			DrawButtonRect(dc, rect, invert);

			rect2.y -= 3;
			rect2.height += 3;
			DrawButtonRect(dc, rect2, invert, FALSE);

			spannedRect = rect2;
		}
		else
		{
			DrawButtonRect(dc, rect2, invert);

			rect.height += 3;
			DrawButtonRect(dc, rect, invert, TRUE, FALSE);

			spannedRect = rect;
		}

		if( !textAll.IsEmpty() )
		{
			spannedRect.y = rect.y;
			spannedRect.height = (rect2.y+rect2.height) - rect.y;

			dc.SetFont(m_textFontSmall);
			dc.SetTextForeground(invert? m_buttonBrush.GetColour() : m_textBrush.GetColour());
			dc.SetTextBackground(invert? m_textBrush.GetColour() : m_buttonBrush.GetColour());

			wxCoord textW, textH, x, y;
			dc.GetTextExtent(textAll, &textW, &textH);

			x = (spannedRect.x + spannedRect.width/2) - textW/2;
			if( x < spannedRect.x ) x = spannedRect.x;

			y = (spannedRect.y + spannedRect.height/2) - textH/2;

			dc.DestroyClippingRegion();
			dc.SetClippingRegion(spannedRect);

			dc.DrawText(textAll, x, y);

			dc.DestroyClippingRegion();
		}
	}
	else
	{
		// draw normal button
		DrawButtonRect(dc, rect, invert);

		if( !textAll.IsEmpty() )
		{
			bool useSmallFont = (textAll.Len()>1);

			dc.DestroyClippingRegion();
			dc.SetClippingRegion(rect);

			wxString textLine1 = textAll.BeforeFirst('\n');
			wxString textLine2 = textAll.AfterFirst('\n');

			dc.SetFont(useSmallFont? m_textFontSmall : m_textFontLarge);
			dc.SetTextForeground(invert? m_buttonBrush.GetColour() : m_textBrush.GetColour());
			dc.SetTextBackground(invert? m_textBrush.GetColour() : m_buttonBrush.GetColour());

			long border = rect.height/8;
			long offsetSmallFont = (m_textFontLargeHeight-m_textFontSmallHeight) / 2;
			dc.DrawText(textLine1, rect.x+border,
			            (rect.y+border) + (useSmallFont? offsetSmallFont : 0));

			if( !textLine2.IsEmpty() )
			{
				dc.SetBackgroundMode(wxTRANSPARENT);
				dc.DrawText(textLine2, rect.x+border,
				            (rect.y+rect.height) - border - m_textFontSmallHeight);
			}

			dc.DestroyClippingRegion();
		}
	}
}


void SjVirtKeybdFrame::RedrawAllOnscreen(wxDC& dc)
{
	wxSize clientSize = GetClientSize();

	// draw background
	dc.SetBrush(m_backgroundBrush);
	dc.SetPen(*wxTRANSPARENT_PEN);

	dc.DrawRectangle(0, 0, clientSize.x, clientSize.y);

	dc.SetPen(m_hilitePen);
	dc.DrawLine(0, 0, clientSize.x, 0);
	dc.DrawLine(0, 0, 0, clientSize.y);


	dc.SetPen(m_shadowPen);
	dc.DrawLine(0, clientSize.y-1, clientSize.x-1, clientSize.y-1);
	dc.DrawLine(clientSize.x-1, 0, clientSize.x-1, clientSize.y);

	// draw close button
	dc.SetPen(m_shadowPen);
	SjTools::DrawIcon(dc, m_closeDrawRect, SJ_DRAWICON_DELETE);

	// draw buttons
	long i, iCount = m_layout.m_keys.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		SjVirtKeybdKey* button = &(m_layout.m_keys[i]);
		if( !button->IsSpacer() && !button->IsNextLine() )
		{
			RedrawButtonOnscreen(dc, button);
		}
	}
}


void SjVirtKeybdFrame::RedrawAllOffscreen(wxDC& clientDc)
{
	wxSize clientSize = GetClientSize();

	// prepare offscreen drawing
	wxBitmap            offscreenBitmap(clientSize.x, clientSize.y);
	wxMemoryDC          offscreenDc;
	offscreenDc.SelectObject(offscreenBitmap);

	// draw offscreen
	RedrawAllOnscreen(offscreenDc);

	// copy offscreen to screen
	clientDc.Blit(0, 0, clientSize.x, clientSize.y, &offscreenDc, 0, 0);
}


void SjVirtKeybdFrame::OnPaint(wxPaintEvent&)
{
	wxPaintDC dc(this);

	RedrawAllOnscreen(dc);
}


void SjVirtKeybdFrame::OnMouseDown(wxMouseEvent& event)
{
	m_lastUserInput = SjTools::GetMsTicks();

	m_clicked = NULL;
	int x = event.GetX(), y = event.GetY();

	SjVirtKeybdKey* button = m_layout.FindKey(x, y);
	if( button == NULL )
	{
		if( x > m_closeClickRect.x
		        && x < m_closeClickRect.x+m_closeClickRect.width
		        && y > m_closeClickRect.y
		        && y < m_closeClickRect.y+m_closeClickRect.height )
		{
			s_inTimeredClose = TRUE;
			m_closeWatchTimer.Stop();
			g_virtKeybd->CloseKeybd();
			s_inTimeredClose = FALSE;
		}
	}
	else if( button == NULL || button->IsSpacer() )
	{
		;
	}
	else if( button->IsShift() )
	{
		m_shift = m_shift? 0 : 1;

		m_shiftLock = false;
		if( m_shift && button->GetKeyFlags(0, 0)&SJ_VK_LOCK )
			m_shiftLock = true;

		wxClientDC dc(this);
		RedrawAllOffscreen(dc);
	}
	else if( button->IsAlt(m_shift, m_alt) )
	{
		int  newAlt = button->IsAlt(m_shift, m_alt);
		bool newAltLock = (button->GetKeyFlags(m_shift, m_alt)&SJ_VK_LOCK)!=0;

		m_alt = newAlt == m_alt? 0 : newAlt;
		m_shift = 0; // pressing an "alt" button resets "shift"
		// (eg see the german "shift-quote");
		// however, pressing "shift" does not reset "alt"

		m_shiftLock = false;
		m_altLock = false;
		if( m_alt && newAltLock )
			m_altLock = true;

		wxClientDC dc(this);
		RedrawAllOffscreen(dc);
	}
	else if( !button->GetKey(m_shift, m_alt).IsEmpty() )
	{
		m_clicked = button;

		wxClientDC dc(this);
		RedrawButtonOnscreen(dc, button);
	}
}


void SjVirtKeybdFrame::OnMouseUp(wxMouseEvent& event)
{
	if( m_clicked )
	{
		SjVirtKeybdKey* clicked = m_clicked;
		m_clicked = NULL;

		if( clicked->IsEnter() )
		{
			wxWindow* inputReceiver = g_virtKeybd->GetInputReceiver();

			s_inTimeredClose = TRUE;
			m_closeWatchTimer.Stop();
			g_virtKeybd->CloseKeybd();
			s_inTimeredClose = FALSE;

			if( inputReceiver && g_mainFrame
			        && inputReceiver == g_mainFrame->GetInputWindow() )
			{
				// this is needed if the option "Search while typing" is OFF,
				// see http://www.silverjuke.net/forum/topic-1432.html
				wxCommandEvent fwd(wxEVT_COMMAND_TEXT_ENTER, inputReceiver->GetId());
				fwd.SetEventObject(inputReceiver);
				inputReceiver->GetEventHandler()->AddPendingEvent(fwd);
			}

			return;
		}

		wxString key = clicked->GetKey(m_shift, m_alt);
		if( !key.IsEmpty() && key != wxT("nop") )
		{
			SendKeyEvent(key);
		}

		// reset the alt/shift keys
		bool redrawAll = false;
		if( m_alt || m_shift )
		{
			if( !m_altLock )
				m_alt = 0;

			if( !m_shiftLock )
				m_shift = 0;

			redrawAll = true;
		}

		// redraw
		if( redrawAll )
		{
			wxClientDC dc(this);
			RedrawAllOffscreen(dc);
		}
		else
		{
			wxClientDC dc(this);
			RedrawButtonOnscreen(dc, clicked);
		}
	}
}


void SjVirtKeybdFrame::OnMouseLeave(wxMouseEvent& event)
{
	if( m_clicked )
	{
		m_clicked = NULL;
		wxClientDC dc(this);
		RedrawAllOffscreen(dc);
	}
}


wxString SjVirtKeybdFrame::ModifyValue(const wxString& oldValue, const wxString& key)
{
	wxString newValue(oldValue);

	if( key == wxT("clearall") )
	{
		newValue.Clear();
	}
	else if( key == wxT("backsp") )
	{
		if( newValue.Len() )
		{
			newValue = newValue.Left(newValue.Len()-1);
		}
	}
	else
	{
		newValue += key;
	}

	return newValue;
}


void SjVirtKeybdFrame::SendKeyEvent(const wxString& key_)
{
	wxWindow* inputReceiver = g_virtKeybd->GetInputReceiver();
	if( inputReceiver )
	{
		wxString className = inputReceiver->GetClassInfo()->GetClassName();
		if( className == wxT("wxTextCtrl") )
		{
			wxTextCtrl* textCtrl = (wxTextCtrl*)inputReceiver;
			wxString oldText = textCtrl->GetValue();

			wxString newText = ModifyValue(oldText, key_);

			textCtrl->SetValue(newText);
		}
	}
}


bool SjVirtKeybdFrame::CheckClose(wxWindow* focusWindow)
{
	#define SJ_VIRTKEYBD_AUTO_CLOSE_MINUTES 3
	if( SjTools::GetMsTicks() > m_lastUserInput + SJ_VIRTKEYBD_AUTO_CLOSE_MINUTES*60*1000 )
	{
		return true; // close as the keyboard is unused for some minutes
	}

	wxWindow* inputReceiver = g_virtKeybd->GetInputReceiver();
	if( inputReceiver == NULL )
	{
		return true; // close as there is not input receiver
	}

	if( !m_autoClose )
	{
		return false; // keep keyboard - autoclose is disabled
	}

	if(
		#if defined(__WXMAC__) || defined(__WXGTK__)
			m_isActive
		#else
			focusWindow == this ||  focusWindow == inputReceiver
		#endif
	)
	{
		return false; // keep keyboard
	}

	const wxArrayPtrVoid* dontCloseOn = NULL;
	wxString className = inputReceiver->GetClassInfo()->GetClassName();
	if( className == wxT("wxTextCtrl") )
	{
		SjVirtKeybdTextCtrl* textCtrl = (SjVirtKeybdTextCtrl*)inputReceiver;
		dontCloseOn = textCtrl->GetDontCloseOn();
	}

	if( dontCloseOn )
	{
		long i, iCount = dontCloseOn->GetCount();
		for( i = 0; i < iCount; i++ )
		{
			if( dontCloseOn->Item(i) == (void*)focusWindow )
			{
				return false; // keep keyboard
			}
		}
	}

	return true; // close keyboard as no relevant control has the focus
}


void SjVirtKeybdFrame::OnCloseWatchTimer(wxTimerEvent&)
{
	static bool s_inHere = FALSE;
	if( !s_inHere )
	{
		s_inHere = TRUE;

		wxWindow* focusWindow = wxWindow::FindFocus();
		if( CheckClose(focusWindow) )
		{
			s_inTimeredClose = TRUE;
			m_closeWatchTimer.Stop();
			g_virtKeybd->CloseKeybd();
			s_inTimeredClose = FALSE;
		}

		s_inHere = FALSE;
	}
}


/*******************************************************************************
 * SjVirtKeybdModule class - constructor and settings
 ******************************************************************************/


SjVirtKeybdModule* g_virtKeybd = NULL;


SjVirtKeybdModule::SjVirtKeybdModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file              = wxT("memory:virtkeybd.lib");
	m_name              = _("Virtual keyboard");
	m_flags             = SJ_VIRTKEYBD_DEFAULT;
	m_transp_dontUse    = -1;
	m_forceForTesting   = FALSE;
	m_inputReceiver     = NULL;
}


bool SjVirtKeybdModule::FirstLoad()
{
	m_flags = g_tools->m_config->Read(wxT("virtkeybd/flags"), SJ_VIRTKEYBD_DEFAULT);
	g_virtKeybd = this;
	return TRUE;
}


void SjVirtKeybdModule::LastUnload()
{
	CloseKeybd();
	g_virtKeybd = NULL;
}


void SjVirtKeybdModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_KIOSK_STARTING
	 || msg == IDMODMSG_KIOSK_ENDED
	 || msg == IDMODMSG_WINDOW_CLOSE )
	{
		CloseKeybd();
	}
}


void SjVirtKeybdModule::SetFlag(long flag, bool set)
{
	SjTools::SetFlag(m_flags, flag, set);
	g_tools->m_config->Write(wxT("virtkeybd/flags"), m_flags);

	SjVirtKeybdFrame* frame = GetVirtKeybdFrame();
	if( frame
	 && flag == SJ_VIRTKEYBD_BLACK )
	{
		frame->InitKeybdFrame();
		frame->Refresh();
	}
}


/*******************************************************************************
 * SjVirtKeybdModule class - get / set transparency
 ******************************************************************************/


int SjVirtKeybdModule::GetTransparency()
{
	if( m_transp_dontUse == -1 )
	{
		m_transp_dontUse = g_tools->m_config->Read(wxT("virtkeybd/transp"), SJ_DEF_VIRTKEYBD_TRANSP);
		if( m_transp_dontUse < 0 ) m_transp_dontUse = 0;
		if( m_transp_dontUse > 100 ) m_transp_dontUse = 100;
	}

	return m_transp_dontUse;
}


void SjVirtKeybdModule::SetTransparency(int transp)
{
	if( transp < 0 ) transp = 0;
	if( transp > 100 ) transp = 100;
	m_transp_dontUse = transp;
	g_tools->m_config->Write(wxT("virtkeybd/transp"), m_transp_dontUse);

	SjVirtKeybdFrame* frame = GetVirtKeybdFrame();
	if( frame
	 && g_tools->CanSetWindowTransparency() )
	{
		g_tools->SetWindowTransparency(frame, transp);
	}
}


/*******************************************************************************
 * SjVirtKeybdModule class - get / set keyboard layout
 ******************************************************************************/


wxString SjVirtKeybdModule::GetKeybdLayout()
{
	if( m_layoutFile_dontUse.IsEmpty() )
	{
		bool useDefault = FALSE;
		m_layoutFile_dontUse = g_tools->m_config->Read(wxT("virtkeybd/layout"), wxT(""));

		if( m_layoutFile_dontUse == wxT("") )
		{
			useDefault = TRUE;
		}
		else
		{
			SjVirtKeybdLayout test;
			if( !test.LoadLayoutFromFile(m_layoutFile_dontUse, NULL) )
			{
				useDefault = TRUE;
			}
		}

		if( useDefault )
		{
			m_layoutFile_dontUse.Clear();
			SjArrayVirtKeybdLayout layouts;
			GetAvailKeybdLayouts(layouts);
			if( layouts.GetCount() ) {
				m_layoutFile_dontUse = layouts[0].m_file;
			}

		}
	}

	return m_layoutFile_dontUse;
}


void SjVirtKeybdModule::SetKeybdLayout(const wxString& file)
{
	m_layoutFile_dontUse = file;
	g_tools->m_config->Write(wxT("virtkeybd/layout"), file);

	SjVirtKeybdFrame* frame = GetVirtKeybdFrame();
	if( frame )
	{
		frame->InitKeybdFrame();
		frame->Refresh();
	}
}


/*******************************************************************************
 * SjVirtKeybdModule class - get available keyboard layout
 ******************************************************************************/


int SjVirtKeybdModule_CmpLayouts(SjVirtKeybdLayout** m1, SjVirtKeybdLayout** m2)
{
	wxASSERT(m1 && *m1 && m2 && *m2);

	return (*m1)->m_name.Cmp((*m2)->m_name);
}
void SjVirtKeybdModule::GetAvailKeybdLayoutsFromFile(SjArrayVirtKeybdLayout& list, const wxString& file)
{
	SjVirtKeybdLayout   test;
	wxArrayString       testNames;
	if( test.LoadLayoutFromFile(file, &testNames) )
	{
		int i, iCount = testNames.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			SjVirtKeybdLayout* curr = new SjVirtKeybdLayout;
			curr->m_name = testNames[i];
			curr->m_file.Printf(wxT("%s,%i"), file.c_str(), (int)i);

			// if the name is already used, add the file to the name
			int j, jCount = list.GetCount();
			for( j = 0; j < jCount; j++ )
			{
				if( list[j].m_name == curr->m_name )
				{
					curr->m_name += wxT(" (") + SjTools::ShortenUrl(curr->m_file.BeforeLast(wxT(','))) + wxT(")");
				}
			}

			list.Add(curr);
		}

	}
}
void SjVirtKeybdModule::GetAvailKeybdLayoutsFromDir(SjArrayVirtKeybdLayout& list, const wxString& dir)
{
	if( wxDirExists(dir) )
	{
		wxFileSystem fs;
		fs.ChangePathTo(dir, TRUE);
		wxString entryStr = fs.FindFirst(wxT("*.*"));

		while( !entryStr.IsEmpty() )
		{
			if( SjTools::GetExt(entryStr) == wxT("sjk") )
			{
				GetAvailKeybdLayoutsFromFile(list, entryStr);
			}

			entryStr = fs.FindNext();
		}
	}
}
void SjVirtKeybdModule::GetAvailKeybdLayouts(SjArrayVirtKeybdLayout& list)
{
	int currSearchDirIndex;
	for( currSearchDirIndex = 0; currSearchDirIndex < g_tools->GetSearchPathCount(); currSearchDirIndex++ )
	{
		GetAvailKeybdLayoutsFromDir(list, g_tools->GetSearchPath(currSearchDirIndex));
		GetAvailKeybdLayoutsFromDir(list, g_tools->GetSearchPath(currSearchDirIndex) + wxT("/keyboards"));
	}

	list.Sort(SjVirtKeybdModule_CmpLayouts);
}


/*******************************************************************************
 * SjVirtKeybdModule class - open / close the keyboard
 ******************************************************************************/


SjVirtKeybdFrame* SjVirtKeybdModule::GetVirtKeybdFrame()
{
	return (SjVirtKeybdFrame*)wxWindow::FindWindowById(IDO_VIRTKEYBDFRAME, NULL);
}


void SjVirtKeybdModule::OpenKeybd(wxWindow* newInputReceiver, bool forceForTesting)
{
	wxBusyCursor busy;

	// if there is no input receiver set, there is nothing to do
	if( newInputReceiver == NULL )
	{
		return;
	}

	// see if we shall use the virtual keyboard
	m_forceForTesting = forceForTesting;
	if( !m_forceForTesting )
	{
		if( !(m_flags&SJ_VIRTKEYBD_USE_IN_KIOSK) )
		{
			return;
		}

		if( !g_mainFrame->IsKioskStarted() )
		{
			if( !(m_flags&SJ_VIRTKEYBD_USE_OUTSIDE_KIOSK) )
			{
				return;
			}
		}
	}

	// set the new input receiver
	m_inputReceiver = newInputReceiver;

	// is the virtual keyboard already opened?
	if( GetVirtKeybdFrame() )
	{
		return;
	}

	// open the virtual keyboard (if not yet opened)
	new SjVirtKeybdFrame(SjDialog::FindTopLevel(newInputReceiver));

	// set the new input receiver, this will also move the window, if needed
	GetVirtKeybdFrame()->InitKeybdFrame();

	// show the window
	GetVirtKeybdFrame()->Show();

	// init the "closing" timer
	GetVirtKeybdFrame()->InitCloseWatchTimer();
}


void SjVirtKeybdModule::CloseKeybd()
{
	SjVirtKeybdFrame* frameToClose = GetVirtKeybdFrame();
	if( frameToClose )
	{
		m_inputReceiver = NULL;
		frameToClose->SetId(wxID_ANY);
		frameToClose->Hide();
		frameToClose->Destroy();
	}
}


bool SjVirtKeybdModule::IsKeybdOpened()
{
	return (GetVirtKeybdFrame()!=NULL);
}


wxCursor SjVirtKeybdModule::GetStandardCursor()
{
	if(  g_mainFrame==NULL
	 || !g_mainFrame->IsKioskStarted()
	 ||  g_virtKeybd==NULL
	 || !(g_virtKeybd->m_flags&SJ_VIRTKEYBD_HIDE_CURSOR) )
	{
		return *wxSTANDARD_CURSOR; // wxNullCursor does not work!, see http://www.silverjuke.net/forum/topic-1478.html
	}
	else
	{
		if( g_debug )
		{
			return wxCursor(wxCURSOR_CROSS);
		}
		else
		{
			return wxCursor(wxCURSOR_BLANK);
		}
	}
}
