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
 * File:    msgboxsj.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke tools
 *
 ******************************************************************************/


#ifndef __SJ_MSGBOX_H__
#define __SJ_MSGBOX_H__


#define SJ_MSG_BOX_WRAP 400

int SjMessageBox        (const wxString& message, const wxString& caption,
                         int style,
                         wxWindow *parent,
                         const wxArrayString* options=NULL, int* selOption=NULL,
                         const wxString& yesTitle=wxT(""), const wxString& noTitle=wxT(""),
                         wxBitmap* bitmap=NULL);



#endif // __SJ_MSGBOX_H__
