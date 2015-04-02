/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/



#include "tg_tagger_base.h"
#include "tg_wma_properties.h"
#include "tg_wma_file.h"




////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

WMA_Properties::WMA_Properties() : Tagger_AudioProperties()
{
	m_length = 0;
	m_bitrate = 0;
	m_sampleRate = 0;
	m_channels = 0;
}

WMA_Properties::~WMA_Properties()
{
}

int WMA_Properties::length() const
{
	return m_length;
}

int WMA_Properties::bitrate() const
{
	return m_bitrate;
}

int WMA_Properties::sampleRate() const
{
	return m_sampleRate;
}

int WMA_Properties::channels() const
{
	return m_channels;
}
