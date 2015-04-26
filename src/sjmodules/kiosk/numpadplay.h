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
 * File:    numpadplay.h
 * Authors: Björn Petersen
 * Purpose: The numpad handler for selecting and playing tracks
 *
 ******************************************************************************/


#ifndef __SJ_NUMPADPLAY_H___
#define __SJ_NUMPADPLAY_H___


enum SjNumpadPlayReturn
{
     SJ_NUMPAD_WAITING
    ,SJ_NUMPAD_ENTER_MENU
    ,SJ_NUMPAD_ERROR
    ,SJ_NUMPAD_URL_PRESSED
};


class SjNumpadPlayHandler : public SjNumpadHandler
{
public:
	                   SjNumpadPlayHandler  (SjNumpadInput*);
	SjNumpadHandlerId  KeyPressed           (int idt);

private:
	SjNumpadPlayReturn GetPressedDigitsInfo (wxString& digits, wxArrayString& urls) const;
	wxArrayLong        m_pressedDigits;
};


#endif // __SJ_NUMPADPLAY_H___

