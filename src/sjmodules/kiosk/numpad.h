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
 * File:    numpad.h
 * Authors: Björn Petersen
 * Purpose: Numpad handling
 *
 ******************************************************************************/


#ifndef __SJ_NUMPAD_H__
#define __SJ_NUMPAD_H__


class SjNumpadInput;


enum SjNumpadHandlerId
{
     SJ_NUMPAD_VOID         // can be returned from SjNumpadHandler::KeyPressed to leave the handler "as is"
    ,SJ_NUMPAD_DELETEME     // can be returned from SjNumpadHandler::KeyPressed
    ,SJ_NUMPAD_PLAY
    ,SJ_NUMPAD_MENU
    ,SJ_NUMPAD_SEARCH
    ,SJ_NUMPAD_PASSWORD
};



class SjNumpadHandler
{
public:
	// constructor
	SjNumpadHandler     (SjNumpadInput* numpadInput, SjNumpadHandlerId handlerId)
	{
		m_numpadInput = numpadInput;
		m_handlerId   = handlerId;
	}
	virtual         ~SjNumpadHandler    () {}

	// the handler id
	SjNumpadHandlerId
	m_handlerId;

	// this function is called for any key pressed,
	// if another handler should be started, the function returns its
	// id and the caller is responsible for creating it
	virtual SjNumpadHandlerId
	KeyPressed          (int idt) = 0;

protected:
	// may be used by derived classes
	SjNumpadInput*  m_numpadInput;
};



class SjNumpadInput
{
public:
	// constructor
	SjNumpadInput       ();
	~SjNumpadInput      ();

	// misc. usefull stuff for the handlers
	long            GetNumDigits        (long count) const;
	long            GetNumAlbumDigits   () const { return GetNumDigits(GetAlbumCount()); }
	long            GetAlbumCount       () const;
	long            DbAlbumIndex2Number (long dbAlbumIndex) const { return dbAlbumIndex+1; }
	wxString        DbAlbumIndex2String (long dbAlbumIndex) const;
	void            FinalizeTrackNumber (wxString& trackNumber, long maxTrackNumber);
	long            UnfinalizeTrackNumber(const wxString& trackNumber) const;
	void            Navigate            (int idt, bool wrapVscroll);

	// KeyPressed() is called from SjMainFrame if a numpad key is pressed
	// DisplayTimeout() is called from SjMainFrame when the "holdMs" are elapsed
	void            KeyPressed          (long id);
	void            DisplayTimeout      ();

	// Display hold settings
	#define         SJ_NUMPAD_HOLD_NORMAL       0L
	#define         SJ_NUMPAD_HOLD_ERROR        1L
	#define         SJ_NUMPAD_HOLD_TEXTENTRY    2L
	#define         SJ_NUMPAD_HOLD_CHARENTRY    3L
	#define         SJ_NUMPAD_HOLD_COUNT        4L
	long            GetHoldSeconds      (int i);
	long            GetHoldMs           (int i) { return GetHoldSeconds(i)*1000; }
	void            SetHoldSeconds      (int i, long);

private:
	// private stuff
	long            m_holdSeconds_DontUse[SJ_NUMPAD_HOLD_COUNT];
	SjNumpadHandler*
	m_currHandler;
};


#endif // __SJ_NUMPAD_H__
