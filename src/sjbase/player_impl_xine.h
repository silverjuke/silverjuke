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
 * File:    player_impl_xine.h
 * Authors: Björn Petersen
 * Purpose: Player Xine implementation
 *
 ******************************************************************************/



#ifndef __SJ_PLAYER_XINE_H__
#define __SJ_PLAYER_XINE_H__


#include <xine.h>


class SjXineStream;


class SjPlayerImpl
{
public:
	SjPlayer*           m_player;

	xine_t*             m_xine;
	xine_audio_port_t*  m_ao_port;

	SjXineStream*		m_currStream;

	SjExtList           m_extList;
	bool                m_extListInitialized;

	wxString            m_iniDevice;

	bool                InitXine    ();
};


#endif // __SJ_PLAYER_XINE_H__

