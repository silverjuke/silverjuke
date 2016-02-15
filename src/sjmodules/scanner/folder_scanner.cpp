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
 * this program.  If not, see  http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    folderscanner.cpp
 * Authors: Björn Petersen
 * Purpose: The "folder scanner" module
 *
 *******************************************************************************
 *
 * Settings are stored in the database.  See library.cpp for some statistics.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/scanner/folder_scanner.h>
#include <sjtools/msgbox.h>
#include <tagger/tg_a_tagger_frontend.h>
#include <wx/dir.h>

#include <wx/listimpl.cpp> // sic!
WX_DEFINE_LIST(SjFolderScannerSourceList);


/*******************************************************************************
 * SjFolderScannerSource
 ******************************************************************************/


wxString SjFolderScannerSource::UrlPlusFile()
{
	if( m_file.IsEmpty() )
	{
		return m_url;
	}
	else
	{
		wxFileName fn(m_url, m_file);
		fn.Normalize();
		return fn.GetFullPath();
	}
}


SjIcon SjFolderScannerSource::GetIcon()
{
	return IsDir()? SJ_ICON_MUSIC_FOLDER : SJ_ICON_MUSIC_FILE;
}


/*******************************************************************************
 *  Settings
 ******************************************************************************/


class SjFolderSettingsDialog : public SjDialog
{
public:
	                SjFolderSettingsDialog (SjFolderScannerModule*,SjFolderScannerSource* source, wxWindow* parent);

private:
	bool            m_initDone;

	wxCheckBox*     m_enabledCheckBox;
	wxCheckBox*     m_doUpdateCheckBox;
	wxCheckBox*     m_readHiddenFilesCheckBox;
	wxCheckBox*     m_readHiddenDirsCheckBox;
	wxCheckBox*     m_readZipCheckBox;
	wxCheckBox*     m_readId3CheckBox;
	wxTextCtrl*     m_ignoreExtTextCtrl;
	wxTextCtrl*     m_infoMaskTextCtrl;

	void            OnEnableCheck       (wxCommandEvent&) { EnableDisable(); }
	void            OnReset             (wxCommandEvent&);
	void            EnableDisable       ();
	DECLARE_EVENT_TABLE ()

	friend class    SjFolderScannerModule;
};


#define IDC_ENABLECHECK     (IDM_FIRSTPRIVATE+0)
#define IDC_STATICBOX       (IDM_FIRSTPRIVATE+1)
#define IDC_RESET           (IDM_FIRSTPRIVATE+3)


BEGIN_EVENT_TABLE(SjFolderSettingsDialog, SjDialog)
	EVT_CHECKBOX    (   IDC_ENABLECHECK,    SjFolderSettingsDialog::OnEnableCheck   )
	EVT_BUTTON      (   IDC_RESET,          SjFolderSettingsDialog::OnReset         )
END_EVENT_TABLE()


SjFolderSettingsDialog::SjFolderSettingsDialog(SjFolderScannerModule* folderScannerModule, SjFolderScannerSource* source, wxWindow* parent)
	: SjDialog(parent, "", SJ_MODAL, SJ_RESIZEABLE_IF_POSSIBLE)
{
	// init dialog
	m_initDone = FALSE;
	bool isDir = source->m_file.IsEmpty();

	SetTitle(wxString::Format(_("Options for \"%s\""), source->UrlPlusFile().c_str()));

	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxBoxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, IDC_STATICBOX, wxEmptyString), wxVERTICAL);
	sizer1->Add(sizer2, 1/*grow*/, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// enable checkbox
	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxALL, SJ_DLG_SPACE);

	sizer3->Add(SJ_DLG_SPACE*4, 2); // some space

	m_enabledCheckBox = new wxCheckBox(this, IDC_ENABLECHECK, isDir? _("Read folders:") : _("Read file:"));
	m_enabledCheckBox->SetValue(source->m_flags&SJ_FOLDERSCANNER_ENABLED? TRUE : FALSE);
	sizer3->Add(m_enabledCheckBox, 0, wxALIGN_CENTER_VERTICAL);

	// ignore ext combobox
	m_ignoreExtTextCtrl = NULL;
	m_doUpdateCheckBox = NULL;
	m_readHiddenFilesCheckBox = NULL;
	m_readHiddenDirsCheckBox = NULL;
	m_readZipCheckBox = NULL;
	if( isDir )
	{
		sizer2->Add(new wxStaticText(this, -1,
		                             _("Ignore music-files and images with the following extensions:")), 0, wxLEFT|wxTOP|wxRIGHT, SJ_DLG_SPACE);

		m_ignoreExtTextCtrl = new wxTextCtrl(this, -1, source->m_ignoreExt.GetExt());
		sizer2->Add(m_ignoreExtTextCtrl, 0, wxLEFT|wxRIGHT|wxGROW, SJ_DLG_SPACE);

		wxStaticText* staticText = new wxStaticText(this, -1,
		        _("(separate the extensions using the comma, case is ignored)"));
		sizer2->Add(staticText, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

		m_doUpdateCheckBox = new wxCheckBox(this, -1, _("Include folder to the update process"));
		m_doUpdateCheckBox->SetValue(source->m_flags&SJ_FOLDERSCANNER_DOUPDATE? TRUE : FALSE);
		sizer2->Add(m_doUpdateCheckBox, 0, wxLEFT|wxTOP|wxRIGHT, SJ_DLG_SPACE);
		sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE/2); // some space

		// only implemented and usefull for MSW - esp for
		// reading hidden covers from the WMP
		m_readHiddenFilesCheckBox = new wxCheckBox(this, -1, _("Read hidden files"));
		m_readHiddenFilesCheckBox->SetValue(source->m_flags&SJ_FOLDERSCANNER_READHIDDENFILES? TRUE : FALSE);
		sizer2->Add(m_readHiddenFilesCheckBox, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);
		sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE/2); // some space

		m_readHiddenDirsCheckBox = new wxCheckBox(this, -1, _("Read hidden folders"));
		m_readHiddenDirsCheckBox->SetValue(source->m_flags&SJ_FOLDERSCANNER_READHIDDENDIRS? TRUE : FALSE);
		sizer2->Add(m_readHiddenDirsCheckBox, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);
		sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE/2); // some space

		m_readZipCheckBox = new wxCheckBox(this, -1, _("Read inside ZIP-/TAR-archives"));
		m_readZipCheckBox->SetValue(source->m_flags&SJ_FOLDERSCANNER_READZIP? TRUE : FALSE);
		sizer2->Add(m_readZipCheckBox, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);
		sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE/2); // some space
	}
	else
	{
		sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE/2); // some space
	}

	// read ID3-Tags?
	m_readId3CheckBox = new wxCheckBox(this, -1, _("Read (ID3)-tags"));
	m_readId3CheckBox->SetValue(source->m_flags&SJ_FOLDERSCANNER_READID3? TRUE : FALSE);
	sizer2->Add(m_readId3CheckBox, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);
	sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE/2); // some space

	// file mask combobox
	sizer2->Add(new wxStaticText(this, -1,
	                             _("Path and file pattern for track-information if (ID3-)tags are missing:")), 0, wxLEFT|wxTOP|wxRIGHT, SJ_DLG_SPACE);

	m_infoMaskTextCtrl = new wxTextCtrl(this, -1, source->m_trackInfoMatcher.GetPattern());
	sizer2->Add(m_infoMaskTextCtrl, 0, wxLEFT|wxRIGHT|wxGROW, SJ_DLG_SPACE);

	wxStaticText* staticText = new wxStaticText(this, -1,
	        wxT("(")+wxString(_("Placeholders:"))+wxT(" <Nr>, <Title>, <Artist>, <Album>, <Genre>, <Group>, <Year>)"));
	sizer2->Add(staticText, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// reset button
	wxButton* button = new wxButton(this, IDC_RESET, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(button, 0, wxALL, SJ_DLG_SPACE);

	// state stuff
	{
		AddStateBox(sizer1);

		long trackCount = folderScannerModule->GetTrackCount__(source);
		AddState(_("Track count")+wxString(":"), trackCount>-1? SjTools::FormatNumber(trackCount) : wxString(_("n/a")));
	}

	// buttons
	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// init done, center dialog
	sizer1->SetSizeHints(this);
	CentreOnParent();

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_SETTINGS));

	m_initDone = TRUE;
	EnableDisable();
}


void SjFolderSettingsDialog::EnableDisable()
{
	if( m_initDone )
	{
		bool                enable = m_enabledCheckBox->GetValue();

		wxWindowList&       children = GetChildren();
		wxWindowList::Node* childNode = children.GetFirst();
		while( childNode )
		{
			wxWindow*   child = childNode->GetData();
			int         childId = child->GetId();

			if( childId != wxID_OK
			 && childId != wxID_CANCEL
			 && childId != IDC_ENABLECHECK
			 && childId != IDC_STATICBOX
			 && !wxString(child->GetClassInfo()->GetClassName()).StartsWith("wxStatic") )
			{
				child->Enable(enable);
			}

			childNode = childNode->GetNext();
		}
	}
}


void SjFolderSettingsDialog::OnReset(wxCommandEvent&)
{
	if( m_ignoreExtTextCtrl )
	{
		m_ignoreExtTextCtrl->SetValue(wxT(""));
	}

	if( m_doUpdateCheckBox )
	{
		m_doUpdateCheckBox->SetValue((SJ_FOLDERSCANNER_DOUPDATE&SJ_FOLDERSCANNER_DEFFLAGS)!=0);
	}

	if( m_readHiddenFilesCheckBox )
	{
		m_readHiddenFilesCheckBox->SetValue((SJ_FOLDERSCANNER_READHIDDENFILES&SJ_FOLDERSCANNER_DEFFLAGS)!=0);
	}

	if( m_readHiddenDirsCheckBox )
	{
		m_readHiddenDirsCheckBox->SetValue((SJ_FOLDERSCANNER_READHIDDENDIRS&SJ_FOLDERSCANNER_DEFFLAGS)!=0);
	}

	if( m_readZipCheckBox )
	{
		m_readZipCheckBox->SetValue((SJ_FOLDERSCANNER_READZIP&SJ_FOLDERSCANNER_DEFFLAGS)!=0);
	}

	if( m_readId3CheckBox )
	{
		m_readId3CheckBox->SetValue((SJ_FOLDERSCANNER_READID3&SJ_FOLDERSCANNER_DEFFLAGS)!=0);
	}

	m_infoMaskTextCtrl->SetValue(SjTrackInfoMatcher::GetDefaultPattern());
}


bool SjFolderScannerModule::ConfigSource(long index, wxWindow* parent)
{
	// get source
	SjFolderScannerSource* currSourceObj = GetSourceObj__(index);
	if( currSourceObj == NULL )
	{
		return FALSE; // error, no update required
	}

	// show dialog
	SjFolderSettingsDialog dlg(this, currSourceObj, parent);
	if( dlg.ShowModal() != wxID_OK )
	{
		return FALSE; // dialog canceled, no update required
	}

	// get new settings
	wxBusyCursor busy;

	bool needsUpdate = FALSE,
	     needsDeepUpdate = FALSE;

	// check enable state
	if( SjDialog::ApplyToBitfield(dlg.m_enabledCheckBox, currSourceObj->m_flags, SJ_FOLDERSCANNER_ENABLED) )
	{
		needsUpdate = TRUE;
	}

	// check update flags
	if( dlg.m_doUpdateCheckBox )
	{
		SjDialog::ApplyToBitfield(dlg.m_doUpdateCheckBox, currSourceObj->m_flags, SJ_FOLDERSCANNER_DOUPDATE);
	}

	// check HIDDEN / ID3 read
	if( dlg.m_readHiddenFilesCheckBox )
	{
		if( SjDialog::ApplyToBitfield(dlg.m_readHiddenFilesCheckBox, currSourceObj->m_flags, SJ_FOLDERSCANNER_READHIDDENFILES) )
		{
			needsUpdate = TRUE;
		}
	}

	if( dlg.m_readHiddenDirsCheckBox )
	{
		if( SjDialog::ApplyToBitfield(dlg.m_readHiddenDirsCheckBox, currSourceObj->m_flags, SJ_FOLDERSCANNER_READHIDDENDIRS) )
		{
			needsUpdate = TRUE;
		}
	}

	if( dlg.m_readZipCheckBox )
	{
		if( SjDialog::ApplyToBitfield(dlg.m_readZipCheckBox, currSourceObj->m_flags, SJ_FOLDERSCANNER_READZIP) )
		{
			needsUpdate = TRUE;
		}
	}

	if( SjDialog::ApplyToBitfield(dlg.m_readId3CheckBox, currSourceObj->m_flags, SJ_FOLDERSCANNER_READID3) )
	{
		needsDeepUpdate = TRUE;
	}

	// check ignore extension
	if( dlg.m_ignoreExtTextCtrl )
	{
		SjExtList newIgnoreExt(dlg.m_ignoreExtTextCtrl->GetValue());
		if( newIgnoreExt != currSourceObj->m_ignoreExt )
		{
			currSourceObj->m_ignoreExt = newIgnoreExt;
			needsUpdate = TRUE;
		}
	}

	// check info mask
	wxString newInfoMask = dlg.m_infoMaskTextCtrl->GetValue();
	if( newInfoMask != currSourceObj->m_trackInfoMatcher.GetPattern() )
	{
		currSourceObj->m_trackInfoMatcher.Compile(SJ_TI_URL, newInfoMask);
		needsDeepUpdate = TRUE;
	}

	// deep update?
	if( needsDeepUpdate )
	{
		wxSqlt sql;
		sql.ConfigWrite("folderscanner/deepupdate/"+currSourceObj->UrlPlusFile(), 1);
	}

	// done so far
	SaveSettings__();

	return (needsUpdate||needsDeepUpdate);
}


/*******************************************************************************
 *  Constructor / Destructor
 ******************************************************************************/


SjFolderScannerModule::SjFolderScannerModule(SjInterfaceBase* interf)
	: SjScannerModule(interf)
{
	m_file                  = "memory:folderscanner.lib";
	m_sort                  = 0; // start of list
	m_name                  = _("Read files and folders");

	m_addSourceTypes_.Add(_("Add a folder to search for music-files"));
	m_addSourceIcons_.Add(SJ_ICON_MUSIC_FOLDER);

	m_addSourceTypes_.Add(_("Add a single music-file"));
	m_addSourceIcons_.Add(SJ_ICON_MUSIC_FILE);

	m_listOfSources.DeleteContents(TRUE);
}


void SjFolderScannerModule::LoadSettings__()
{
	wxLogNull logNull; // esp. for SjFileNameParser::Init()

	m_listOfSources.Clear();
	wxSqlt sql;

	int                     sourceCount = sql.ConfigRead(wxT("folderscanner/sourceCount"), 0L);
	int                     currSourceIndex;
	wxString                currSourceStr;
	SjFolderScannerSource*  currSourceObj;
	for( currSourceIndex = 0; currSourceIndex < sourceCount; currSourceIndex++ )
	{
		currSourceStr = sql.ConfigRead(wxString::Format(wxT("folderscanner/url%i"), (int)currSourceIndex), wxT(""));
		if( !currSourceStr.IsEmpty() )
		{
			currSourceStr = SjTools::EnsureTrailingSlash(currSourceStr);
			currSourceObj = new SjFolderScannerSource;
			if( currSourceObj )
			{
				currSourceObj->m_url        = currSourceStr;
				currSourceObj->m_file       = sql.ConfigRead(wxString::Format(wxT("folderscanner/file%i"), (int)currSourceIndex), wxT(""));
				currSourceObj->m_ignoreExt  = sql.ConfigRead(wxString::Format(wxT("folderscanner/ignoreExt%i"), (int)currSourceIndex), wxT(""));
				currSourceObj->m_trackInfoMatcher.Compile(SJ_TI_URL, sql.ConfigRead(wxString::Format(wxT("folderscanner/pattern%i"), (int)currSourceIndex), SjTrackInfoMatcher::GetDefaultPattern()));
				currSourceObj->m_flags      =  sql.ConfigRead(wxString::Format(wxT("folderscanner/flags%i"), (int)currSourceIndex), SJ_FOLDERSCANNER_DEFFLAGS);
				m_listOfSources.Append(currSourceObj);
			}
		}
	}
}


void SjFolderScannerModule::SaveSettings__()
{
	wxBusyCursor busy;
	wxSqlt sql;

	int sourceCount = m_listOfSources.GetCount();
	sql.ConfigWrite("folderscanner/sourceCount", (long)sourceCount);

	SjFolderScannerSourceList::Node* currSourceNode = m_listOfSources.GetFirst();
	SjFolderScannerSource*           currSourceObj;
	int                              currSourceIndex = 0;
	while( currSourceNode )
	{
		// write source
		currSourceObj = currSourceNode->GetData();
		wxASSERT(currSourceObj);
		sql.ConfigWrite(wxString::Format(wxT("folderscanner/url%i"), (int)currSourceIndex), currSourceObj->m_url);
		sql.ConfigWrite(wxString::Format(wxT("folderscanner/file%i"), (int)currSourceIndex), currSourceObj->m_file);
		sql.ConfigWrite(wxString::Format(wxT("folderscanner/ignoreExt%i"), (int)currSourceIndex), currSourceObj->m_ignoreExt.GetExt());
		sql.ConfigWrite(wxString::Format(wxT("folderscanner/pattern%i"), (int)currSourceIndex), currSourceObj->m_trackInfoMatcher.GetPattern());
		sql.ConfigWrite(wxString::Format(wxT("folderscanner/flags%i"), (int)currSourceIndex), currSourceObj->m_flags);

		// next source
		currSourceIndex++;
		currSourceNode = currSourceNode->GetNext();
	}

	// remove previously written and now unneeded data
	while( 1 /*exit by break*/ )
	{
		#define MAX_DEL 1000
		if( sql.ConfigRead(wxString::Format(wxT("folderscanner/url%i"), (int)currSourceIndex), wxT("*"))==wxT("*") /*check for existance (`*` is an invalid URL)*/
		 || currSourceIndex > MAX_DEL /*avoid deadlocks*/ )
		{
			break; // done
		}

		wxString url = sql.ConfigRead(wxString::Format(wxT("folderscanner/url%i"), (int)currSourceIndex), wxT(""));

		sql.ConfigDeleteEntry(wxT("folderscanner/deepupdate/")+url);
		sql.ConfigDeleteEntry(wxT("folderscanner/trackCount/")+url);
		sql.ConfigDeleteEntry(wxString::Format(wxT("folderscanner/url%i"), (int)currSourceIndex));
		sql.ConfigDeleteEntry(wxString::Format(wxT("folderscanner/ignoreExt%i"), (int)currSourceIndex));
		sql.ConfigDeleteEntry(wxString::Format(wxT("folderscanner/pattern%i"), (int)currSourceIndex));
		sql.ConfigDeleteEntry(wxString::Format(wxT("folderscanner/flags%i"), (int)currSourceIndex));

		currSourceIndex++;
	}
}


bool SjFolderScannerModule::FirstLoad()
{
	LoadSettings__();
	return TRUE;
}


/*******************************************************************************
 * Handling Sources
 ******************************************************************************/


long SjFolderScannerModule::GetSourceCount()
{
	return m_listOfSources.GetCount();
}


SjFolderScannerSource* SjFolderScannerModule::GetSourceObj__(long index)
{
	SjFolderScannerSourceList::Node* currSourceNode = m_listOfSources.Item(index);
	if( currSourceNode == NULL )
	{
		wxLogError("Cannot get folder source object at index %i."/*n/t*/, (int)index);
		return NULL;
	}

	SjFolderScannerSource* currSourceObj = currSourceNode->GetData();
	wxASSERT(currSourceObj);

	return currSourceObj;
}


wxString SjFolderScannerModule::GetSourceUrl(long index)
{
	SjFolderScannerSource* currSourceObj = GetSourceObj__(index);

	return currSourceObj? currSourceObj->UrlPlusFile() : wxString("");
}


SjIcon SjFolderScannerModule::GetSourceIcon(long index)
{
	SjFolderScannerSource* currSourceObj = GetSourceObj__(index);

	if( currSourceObj )
	{
		if( !(currSourceObj->m_flags&SJ_FOLDERSCANNER_ENABLED) )
		{
			return SJ_ICON_EMPTY;
		}
		else
		{
			return currSourceObj->GetIcon();
		}
	}

	return SJ_ICON_ANYFOLDER;
}


wxString SjFolderScannerModule::GetSourceNotes(long index)
{
	SjFolderScannerSource* currSourceObj = GetSourceObj__(index);
	if( currSourceObj )
	{
		if( !(currSourceObj->m_flags&SJ_FOLDERSCANNER_ENABLED) )
		{
			return _("Disabled");
		}
		else if( !(currSourceObj->m_flags&SJ_FOLDERSCANNER_DOUPDATE) )
		{
			return _("No update");
		}
	}
	return "";
}


bool SjFolderScannerModule::DeleteSource(long index, wxWindow* parent)
{
	SjFolderScannerSource* currSourceObj = GetSourceObj__(index);
	if( currSourceObj == NULL )
	{
		return FALSE;
	}

	if( SjMessageBox(wxString::Format(_("Remove \"%s\" from the music library?"), currSourceObj->UrlPlusFile().c_str()),
	                   SJ_PROGRAM_NAME, wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, parent) == wxYES )
	{
		m_listOfSources.DeleteObject(currSourceObj); // searchDir is deleted automatically as we use DeleteContents()
		SaveSettings__();
		return TRUE;
	}

	return FALSE;
}


long SjFolderScannerModule::AddSources(int sourceType, wxWindow* parent)
{
	long sourceCount = GetSourceCount();
	SjFolderScannerSource* defSource = sourceCount>0? GetSourceObj__(sourceCount-1) : NULL;

	wxString newUrl;
	wxString newFile;
	if( sourceType == 0 )
	{
		// show "select dir" dialog
		wxDirDialog dirDialog(parent,
		                      _("Select a folder with music-files"),
		                      defSource? defSource->m_url : wxString(""),
		                      wxDD_DEFAULT_STYLE & ~(wxDD_NEW_DIR_BUTTON ));
		if( dirDialog.ShowModal() != wxID_OK )
		{
			return -1; // nothing added
		}
		wxFileName fn(dirDialog.GetPath());
		fn.Normalize();
		newUrl = fn.GetFullPath();
	}
	else
	{
		// show "select file" dialog
		wxFileDialog fileDialog(parent,
		                        _("Select music-file"),
		                        defSource? defSource->m_url : wxString(""),
		                        "", g_mainFrame->m_moduleSystem.GetAssignedExt(SJ_EXT_MUSICFILES|SJ_EXT_ARCHIVES).GetFileDlgStr(), wxFD_OPEN|wxFD_CHANGE_DIR);
		if( fileDialog.ShowModal() != wxID_OK )
		{
			return -1; // nothing added
		}

		wxFileName fn(fileDialog.GetPath());
		fn.Normalize();
		newUrl = fn.GetPath(wxPATH_GET_VOLUME);
		newFile = fn.GetFullName();
	}

	bool sthAdded;
	return DoAddUrl(newUrl, newFile, sthAdded);
}


long SjFolderScannerModule::DoAddUrl(const wxString& newUrl__, const wxString& newFile, bool& sthAdded)
{
	wxString newUrl = SjTools::EnsureTrailingSlash(newUrl__);

	long sourceCount = GetSourceCount();
	SjFolderScannerSource* defSource = sourceCount>0? GetSourceObj__(sourceCount-1) : NULL;

	// make sure, the source is not yet added
	int i;
	for( i = 0; i < sourceCount; i++ )
	{
		SjFolderScannerSource* currSourceObj = GetSourceObj__(i);
		if( currSourceObj->m_url == newUrl
		 && currSourceObj->m_file == newFile )
		{
			currSourceObj->m_flags |= SJ_FOLDERSCANNER_ENABLED;
			sthAdded = FALSE;
			return i; // nothing added, return the index of the existing source
		}
	}

	// add new source
	SjFolderScannerSource* currSourceObj = new SjFolderScannerSource;
	currSourceObj->m_url        = newUrl;
	currSourceObj->m_file       = newFile;
	currSourceObj->m_ignoreExt  = (defSource && newFile.IsEmpty())? defSource->m_ignoreExt : SjExtList();
	currSourceObj->m_trackInfoMatcher.Compile(SJ_TI_URL, defSource? defSource->m_trackInfoMatcher.GetPattern() : SjTrackInfoMatcher::GetDefaultPattern());
	currSourceObj->m_flags      = SJ_FOLDERSCANNER_DEFFLAGS;

	if( !newFile.IsEmpty() )
	{
		currSourceObj->m_flags |= SJ_FOLDERSCANNER_READZIP|SJ_FOLDERSCANNER_READHIDDENFILES|SJ_FOLDERSCANNER_READHIDDENDIRS;
	}

	m_listOfSources.Append(currSourceObj);
	SaveSettings__();

	// done
	sthAdded = TRUE;
	return GetSourceCount()-1;
}


bool SjFolderScannerModule::AddUrl(const wxString& url)
{
	bool sthAdded;
	DoAddUrl(url, "", sthAdded);
	return sthAdded;
}


/*******************************************************************************
 * Iterate Tracks
 ******************************************************************************/


static void FileNamesToURLs(wxArrayString& inOut)
{
	size_t i, i_cnt = inOut.GetCount();
	for( i = 0; i < i_cnt; i++ )
	{
		wxFileName fn(inOut[i]);
		inOut[i] = wxFileSystem::FileNameToURL(fn);
	}
}


bool SjFolderScannerModule::IterateFile__(const wxString&        url,
                                          bool                   deepUpdate,
                                          const wxString&        arts,
                                          uint32_t               crc32,
                                          SjFolderScannerSource* source,
                                          SjColModule*           receiver,
                                          long&                  retTrackCount )
{
	wxASSERT( url.Left(5) == "file:" );
	wxASSERT( url.Find('\\') ==  wxNOT_FOUND );
	wxASSERT( url.Last()!='\\' && url.Last()!='/' );

	bool                ret = FALSE;
	wxFileSystem        fileSystem;
	wxFSFile*           fsFile = NULL;
	long                fileSize;
	SjTrackInfo*        trackInfo = NULL;

	// update info
	if( !SjBusyInfo::Set(url, false) )
	{
		goto Cleanup; // user abort
	}

	// get wxFilesSystem object (must be deleted on return), get file size
	fsFile = fileSystem.OpenFile(url,
	                             (source->m_flags & SJ_FOLDERSCANNER_READID3)? (wxFS_READ|wxFS_SEEKABLE) : wxFS_READ); // when ID3 reading is enabled, we need seeking
	if( fsFile == NULL )
	{
		ret = TRUE;  // error, but continue
		goto Cleanup;
	}
	fileSize = fsFile->GetStream()->GetSize();
	wxASSERT( fsFile->GetLocation() == url );

	// check if the track or any arts was modified since the last update process,
	// we don't catch the cases an art was deleted this way...
	crc32 = SjTools::Crc32AddLong(crc32, fsFile->GetModificationTime().GetAsDOS());

	if( deepUpdate==FALSE
	 && receiver->Callback_CheckTrackInfo(url, crc32) )
	{
		ret = TRUE; // success, the file is already in the database
		retTrackCount++;
		goto Cleanup;
	}

	// check if the current extension can be handled by the player
	if( !g_mainFrame->m_player.TestUrl(url) )
	{
		ret = TRUE; // error, but continue
		goto Cleanup;
	}

	// get track information
	trackInfo = new SjTrackInfo;
	if( !trackInfo )
	{
		ret = TRUE; // error, but continue
		goto Cleanup;
	}

	trackInfo->m_url        = url;
	trackInfo->m_updatecrc  = crc32;

	{
		SjResult result = SJ_ERROR;
		if( source->m_flags & SJ_FOLDERSCANNER_READID3 )
		{
			result = SjGetTrackInfoFromID3Etc(fsFile, *trackInfo, SJ_TI_FULLINFO);
			if( result == SJ_SUCCESS_BUT_NO_DATA )
			{
				ret = TRUE; // success
				goto Cleanup;
			}
		}

		if( result == SJ_ERROR
		 || trackInfo->m_trackName.IsEmpty()
		 || trackInfo->m_leadArtistName.IsEmpty() )
		{
			m_trackInfoMatcherObj.m_url = url;
			source->m_trackInfoMatcher.Match(m_trackInfoMatcherObj, *trackInfo);
		}

		if( trackInfo->m_trackName.IsEmpty() )
		{
			trackInfo->m_trackName = _("Unknown track");
		}

		if( trackInfo->m_leadArtistName.IsEmpty() )
		{
			trackInfo->m_leadArtistName = _("Unknown artist");
		}
	}

	// get fize size if not yet set
	if( trackInfo->m_dataBytes == 0 )
	{
		trackInfo->m_dataBytes = fileSize;
	}

	// append image list to the track information
	trackInfo->AddArt(arts);

	// give the track information object to the calling SjColModule object,
	// this function will delete the object if no longer needed
	if( !receiver->Callback_ReceiveTrackInfo(trackInfo) )
	{
		goto Cleanup; // user abort
	}
	trackInfo = NULL;

	// Success
	ret = TRUE;
	retTrackCount++;

	// Cleanup
Cleanup:

	if( trackInfo )
	{
		delete trackInfo;
	}

	if( fsFile )
	{
		delete fsFile;
	}

	return ret;
}


bool SjFolderScannerModule::IterateDir__(const wxString&        url, // may or may not terminate with a slash
                                         const wxString&        onlyThisFile,
                                         bool                   deepUpdate,
                                         SjFolderScannerSource* source,
                                         SjColModule*           receiver,
                                         long&                  retTrackCount )
{
	wxASSERT( url.Left(5) == "file:" );
	wxASSERT( onlyThisFile.Left(5) == "file:" || onlyThisFile.IsEmpty() );

	// progress information
	if( !SjBusyInfo::Set(url, false) )
	{
		return FALSE;
	}

	// get all files to "subdirEntries" and "fileEntries"
	wxArrayString subdirEntries;
	wxArrayString fileEntries;
	if( !onlyThisFile.IsEmpty() )
	{
		fileEntries.Add(onlyThisFile);
	}
	else
	{
		wxFileSystem    fileSystem;
		wxString        dirEntryStr;

		// prepare collecting
		fileSystem.ChangePathTo(url, TRUE/*is dir*/);

		// collect all FILES
		bool scanUsingFS = true;

		if( source->m_flags&SJ_FOLDERSCANNER_READHIDDENFILES )
		{
			wxFileName dirEntryFn  = wxFileSystem::URLToFileName(url);
			dirEntryStr = dirEntryFn.GetFullPath();
			if( dirEntryFn.IsOk() && wxDir::Exists(dirEntryStr) )
			{
				// ... collect FILES using wxDir (allows us to read HIDDEN files)
				wxDir::GetAllFiles(dirEntryStr, &fileEntries, "*",
								   wxDIR_FILES
								   |   ((source->m_flags&SJ_FOLDERSCANNER_READHIDDENFILES)? wxDIR_HIDDEN : 0));
				FileNamesToURLs(fileEntries);
				scanUsingFS = false;
			}
		}

		if( scanUsingFS )
		{
			// ... collect FILES using wxFileSystem
			dirEntryStr = fileSystem.FindFirst("*", wxFILE);
			while( !dirEntryStr.IsEmpty() )
			{
				fileEntries.Add(dirEntryStr);
				dirEntryStr = fileSystem.FindNext();
			}
		}


		// collect all DIRECTORIES
		scanUsingFS = true;

		if( source->m_flags&SJ_FOLDERSCANNER_READHIDDENDIRS )
		{
			// ... collect all DIRECTORIES using wxDir (allows us to read HIDDEN files - see http://www.silverjuke.net/forum/topic-3765.html)
			wxFileName dirEntryFn  = wxFileSystem::URLToFileName(url);
			wxString dirEntryStr = dirEntryFn.GetFullPath();
			if( dirEntryFn.IsOk() && wxDir::Exists(dirEntryStr) )
			{
				wxDir theDir(dirEntryStr);
				wxString theEntry;
				bool cont = theDir.GetFirst(&theEntry, "*",  wxDIR_DIRS
						|   ((source->m_flags&SJ_FOLDERSCANNER_READHIDDENDIRS)? wxDIR_HIDDEN : 0));
				while ( cont )
				{
					theEntry = SjTools::EnsureTrailingSlash(dirEntryStr) + theEntry;
					subdirEntries.Add(SjTools::EnsureTrailingSlash(theEntry));
					cont = theDir.GetNext(&theEntry);
				}
				FileNamesToURLs(subdirEntries);
				scanUsingFS = false;
			}
		}

		if( scanUsingFS )
		{
			// ... collect all DIRECTORIES using wxFileSystem
			dirEntryStr = fileSystem.FindFirst("*", wxDIR);
			while( !dirEntryStr.IsEmpty() )
			{
				dirEntryStr.Replace("tar:/", "tar:"); // THIS IS A HACK!!!
				dirEntryStr.Replace("zip:/", "zip:"); // THIS IS A HACK!!!
				// the files system returns directories as
				// "c:/bla/bla.zip#zip:/dir" which must be called
				// "c:/bla/bla.zip#zip:dir"
				subdirEntries.Add(dirEntryStr);
				dirEntryStr = fileSystem.FindNext();
			}
		}
	}

	// go through all art-files and collect them in "images"
	long            entriesCount = fileEntries.GetCount();
	long            entryIndex;
	wxString        currUrl;
	wxString        currExt;
	wxString        arts;
	uint32_t        crc32 = SjTools::Crc32Init();
	wxFileSystem    fileSystem;
	wxFSFile*       fsFile;
	for( entryIndex = 0; entryIndex < entriesCount; entryIndex++ )
	{
		currUrl = fileEntries.Item(entryIndex);
		currExt = SjTools::GetExt(currUrl);

		if( !::wxDirExists(currUrl)
		 && !source->m_ignoreExt.LookupExt(currExt)
		 &&  g_mainFrame->m_moduleSystem.FindImageHandlerByExt(currExt) )
		{
			if( !SjBusyInfo::Set(currUrl, false) )
			{
				return FALSE;
			}

			fsFile = fileSystem.OpenFile(currUrl);
			if( fsFile )
			{
				// add art
				if( !arts.IsEmpty() ) arts += "\n";
				arts += currUrl;

				// set crc
				crc32 = SjTools::Crc32AddLong(crc32, fsFile->GetModificationTime().GetAsDOS());
				delete fsFile;
			}
		}
	}
	crc32 = SjTools::Crc32AddString(crc32, arts);

	// go through all music-files
	for( entryIndex = 0; entryIndex < entriesCount; entryIndex++ )
	{
		currUrl = fileEntries.Item(entryIndex);
		currExt = SjTools::GetExt(currUrl);

		if( (source->m_flags&SJ_FOLDERSCANNER_READZIP) && currExt == wxT("zip") )
		{
			subdirEntries.Add(currUrl + wxT("#zip:/"));
		}
		else if( (source->m_flags&SJ_FOLDERSCANNER_READZIP) && currExt == wxT("tar") )
		{
			subdirEntries.Add(currUrl + wxT("#tar:/"));
		}
		else if( !source->m_ignoreExt.LookupExt(currExt) )
		{
			// the file is an understood media file
			if( !IterateFile__(currUrl, deepUpdate, arts, crc32, source, receiver, retTrackCount) )
			{
				return FALSE; // user abort
			}
		}
	}

	// go through all subdirectories - recursive call!
	entriesCount = subdirEntries.GetCount();
	for( entryIndex = 0; entryIndex < entriesCount; entryIndex++ )
	{
		currUrl = subdirEntries.Item(entryIndex);

		if( !IterateDir__(currUrl, "", deepUpdate, source, receiver, retTrackCount) )
		{
			return FALSE; // user abort
		}
	}

	// done
	return TRUE;
}


long SjFolderScannerModule::GetTrackCount__(SjFolderScannerSource* source)
{
	wxASSERT( source );

	// returns -1 for "don't know"
	wxSqlt sql;
	return sql.ConfigRead("folderscanner/trackCount/"+source->UrlPlusFile(), -1);
}


bool SjFolderScannerModule::IterateTrackInfo(SjColModule* receiver)
{
	wxSqltTransaction   transaction; // needed for SjTools::DbConfig*()
	bool                ret = TRUE;
	bool                deepUpdate, doIterateDir;
	wxString            onlyThisFile;

	// go through all sources
	SjFolderScannerSourceList::Node* currSourceNode = m_listOfSources.GetFirst();
	SjFolderScannerSource*           currSource;
	while( currSourceNode )
	{
		currSource = currSourceNode->GetData();
		wxASSERT(currSource);

		if( currSource->m_flags&SJ_FOLDERSCANNER_ENABLED )
		{
			// deep update current source?
			{
				wxSqlt sql;
				doIterateDir = TRUE;
				deepUpdate = sql.ConfigRead(wxT("folderscanner/deepupdate/")+currSource->UrlPlusFile(), 0)? TRUE : FALSE;
				if( deepUpdate )
				{
					sql.ConfigWrite(wxT("folderscanner/deepupdate/")+currSource->UrlPlusFile(), 0L);
				}
			}

			// iterate just a single file?
			onlyThisFile.Clear();
			if( !currSource->m_file.IsEmpty() )
			{
				// iterate just a single file.
				wxFileName fn(currSource->m_url, currSource->m_file);
				onlyThisFile = wxFileSystem::FileNameToURL(fn);
			}
			else if( !deepUpdate && !(currSource->m_flags&SJ_FOLDERSCANNER_DOUPDATE) )
			{
				// this folder should not be updated automatically;
				// however, the column module may also decide that this is neccessary
				wxFileName fn(currSource->m_url);
				wxString urlBegin = wxFileSystem::FileNameToURL(fn);
				if( urlBegin.Last()!='/' ) urlBegin += '/';
				if( receiver->Callback_MarkAsUpdated(urlBegin, GetTrackCount__(currSource)) )
				{
					doIterateDir = FALSE;
				}
			}

			// start source iteration
			if( doIterateDir )
			{
				wxFileName fn(currSource->m_url);
				long trackCount = 0;
				if( !IterateDir__(wxFileSystem::FileNameToURL(fn), onlyThisFile, deepUpdate, currSource, receiver, trackCount) )
				{
					ret = FALSE;  // user abort
					break;
				}

				if( GetTrackCount__(currSource) != trackCount )
				{
					wxSqlt sql;
					sql.ConfigWrite("folderscanner/trackCount/"+currSource->UrlPlusFile(), trackCount);
				}
			}
		}

		// next source
		currSourceNode = currSourceNode->GetNext();
	}

	// commit data?
	if( ret )
	{
		transaction.Commit();
	}

	return ret;
}


/*******************************************************************************
 * Get / Set Track Information and Data
 ******************************************************************************/


bool SjFolderScannerModule::SetTrackInfo(const wxString& url, SjTrackInfo& trackInfo)
{
	return SjSetTrackInfoToID3Etc(url, trackInfo);
}



