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
 * File:    openfiles.h
 * Authors: Björn Petersen
 * Purpose: The open files & more dialog
 *
 ******************************************************************************/



#ifndef __SJ_OPENFILES_H__
#define __SJ_OPENFILES_H__



class SjOpenFilesDialog;



class SjOpenFilesModule : public SjCommonModule
{
public:
	SjOpenFilesModule   (SjInterfaceBase*);

	void            OpenDialog          (bool checkAppend /*if set to FALSE, the checkbox stays as it is*/);
	void            CloseDialog         ();
	bool            IsDialogOpened      () const { return (m_dlg!=NULL); }

protected:
	bool            FirstLoad           ();
	void            LastUnload          ();
	void            ReceiveMsg          (int msg);

private:
	SjOpenFilesDialog* m_dlg;

	bool            m_settingsLoaded;
	wxArrayString   m_settingsHistory;
	bool            m_settingsAppend;

	friend class    SjOpenFilesDialog;
};



extern SjOpenFilesModule* g_openFilesModule;



#endif // __SJ_OPENFILES_H__
