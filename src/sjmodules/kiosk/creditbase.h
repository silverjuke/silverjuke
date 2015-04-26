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
 * File:    creditbase.h
 * Authors: Björn Petersen
 * Purpose: Kiosk credit handling
 *
 ******************************************************************************/


#ifndef __SJ_CREDITBASE_H__
#define __SJ_CREDITBASE_H__


#define SJ_ADDCREDIT_SET_TO_NULL_BEFORE_ADD 0x01
#define SJ_ADDCREDIT_FROM_DDE               0x02

#define SJ_CREDITBASEF_ENABLED          0x00010000L
#define SJ_CREDITBASEF_SAVE_CREDITS     0x00020000L
#define SJ_CREDITBASEF_LISTEN_TO_DDE    0x00000001L // +command line +TCP/IP +shortcuts
#define SJ_CREDITBASEF_DEFAULT          0x0000FFFFL


class SjCreditBase
{
public:
	                SjCreditBase            ();
	                ~SjCreditBase           ();

	long            GetCredit               () { return m_creditCount; }

	// CheckAndDecreaseCredit() should be called before enqueing titles,
	// normally, this function is only called by SjKioskModule::CanEnqueue()
	//
	// CheckAndDecreaseCredit() returns TRUE if adding the number of given
	// titles is okay;  in this case you should really add the titles to the
	// queue as the credit counter is decreased.
	//
	// If FALSE is returned, adding is not possible at the moment, an error is already
	// printed by CheckCredit() eg. using g_mainFrame->SetDisplayMsg()
	bool            CheckAndDecreaseCredit  (long requestedTitlesCount);


	// AddCredit() is called by SjCreditModule's eg. if a coin is inserted.
	void            AddCredit               (long titleCount, long flags /*SJ_ADDCREDIT_*/ );

	// Check flags
	long            GetFlags                () const { return m_flags; /*SJ_CREDITBASEF_*/ }
	void            SetFlag                 (long flag /*SJ_CREDITBASEF_*/, bool set=TRUE);

	// Is the credit system enabled? - this state may also be checked without the kiosk module loaded.
	static bool     IsCreditSystemEnabled   ();

	// Do listen to shortcuts?
	bool            ListenToShortcuts       () const { return (GetFlags()&SJ_CREDITBASEF_LISTEN_TO_DDE)!=0; }

	// Update the target in the skin
	void            UpdateSkinTarget        () const;

private:
	long            m_flags;
	long            m_creditCount;

	void            SaveCreditCount         ();
	bool            ShowCreditInDisplay     () const;
};


#endif // __SJ_CREDITBASE_H__
