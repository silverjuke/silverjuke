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
 * File:    vidout_module.h
 * Authors: Björn Petersen
 * Purpose: The video output module (not: the decoder module)
 *
 ******************************************************************************/


#ifndef __SJ_VIDOUT_MODULE_H__
#define __SJ_VIDOUT_MODULE_H__
#if SJ_USE_VIDEO


class SjVidoutModule : public SjVisRendererModule
{
public:
	                SjVidoutModule      (SjInterfaceBase* interf);
	                ~SjVidoutModule     ();

	bool            Start               (SjVisImpl*, bool justContinue);
	void            Stop                ();

	void            ReceiveMsg          (int);
	void            AddMenuOptions      (SjMenu&);
	void            OnMenuOption        (int);
	void            PleaseUpdateSize    (SjVisImpl*);

//	static void     SetRecentVidCh      (DWORD ch);

private:
	SjVisImpl*      m_impl;

//	static void     SetProperVideoSize  (DWORD ch);

	friend class SjVidoutWindow;
};


#endif // SJ_USE_VIDEO
#endif // __SJ_VIDOUT_MODULE_H__

