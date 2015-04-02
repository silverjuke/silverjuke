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




#ifndef TAGGER_MPEGPROPERTIES_H
#define TAGGER_MPEGPROPERTIES_H

#include "tg_tagger_base.h"
#include "tg_mpeg_header.h"


class MPEG_File;



//! An implementation of audio property reading for MP3

/*!
 * This reads the data from an MPEG Layer III stream found in the
 * AudioProperties API.
 */
class MPEG_Properties : public Tagger_AudioProperties
{
public:
	/*!
	* Create an instance of MPEG::Properties with the data read from the
	* MPEG::File \a file.
	*/
	MPEG_Properties(MPEG_File *file);

	/*!
	* Destroys this MPEG Properties instance.
	*/
	virtual ~MPEG_Properties() {}

	bool IsValid() const { return m_isValid; }

	// Reimplementations.

	int length() const { return m_length; }
	int bitrate() const { return m_bitrate; }
	int sampleRate() const { return m_sampleRate; }
	int channels() const { return m_channelMode==MPEG_SingleChannel? 1 : 2; }

	/*!
	* Returns the MPEG Version of the file.
	*/
	MPEG_HeaderVersion version() const { return m_version; }

	/*!
	* Returns the layer version.  This will be between the values 1-3.
	*/
	int layer() const { return m_layer; }

	/*!
	* Returns the channel mode for this frame.
	*/
	MPEG_HeaderChannelMode channelMode() const { return m_channelMode; }

	/*!
	* Returns true if the copyrighted bit is set.
	*/
	bool isCopyrighted() const { return m_isCopyrighted; }

	/*!
	* Returns true if the "original" bit is set.
	*/
	bool isOriginal() const { return m_isOriginal; }

	/*!
	* added by me
	*/
	bool isPadded() const { return m_isPadded; }
	bool protectionEnabled() const { return m_protectionEnabled; }
	int frameLength() const { return m_frameLength; }
	int frameCount() const { return m_frameCount; }
	unsigned char emphasis() const { return m_emphasis; }

private:
#ifdef __WXDEBUG__
	MPEG_Properties &operator=(const MPEG_Properties &)  { wxLogWarning(wxT("do not copy MPEG_Properties objects this way!")); return *this; }
#endif

	void read(MPEG_File* file);

	int     m_length;
	int     m_bitrate;
	int     m_sampleRate;
	int     m_layer;
	bool    m_isCopyrighted;
	bool    m_isOriginal;
	bool    m_isPadded;
	bool    m_protectionEnabled;
	int     m_frameLength;
	int     m_frameCount;
	unsigned char m_emphasis;
	MPEG_HeaderVersion      m_version;
	MPEG_HeaderChannelMode  m_channelMode;

	bool    m_isValid;
};

#endif
