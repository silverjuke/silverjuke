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
#include "tg_mpeg_header.h"



/***************************************************************************
 * MPEG Header
 ***************************************************************************/



void MPEG_Header::parse(const SjByteVector &data)
{
	// see http://www.mp3-tech.org/programmer/frame_header.html

	m_isValid           = false;
	m_version           = MPEG_Version1;
	m_layer             = 0;
	m_protectionEnabled = false;
	m_sampleRate        = 0;
	m_isPadded          = false;
	m_channelMode       = MPEG_Stereo;
	m_isCopyrighted     = false;
	m_isOriginal        = false;
	m_emphasis          = 0;
	m_frameLength       = 0;

	// check for the size and for the first synch byte

	if(data.size() < 4 || (unsigned char)(data[0]) != 0xff)
	{
		wxLogDebug(wxT("MPEG::Header::parse() -- First byte did not mactch MPEG synch."));
		return;
	}

	unsigned long flags = data.toUInt();

	// Check for the second byte's part of the MPEG synch

	if( !(flags&(1<<23)) || !(flags&(1<<22)) || !(flags&(1<<21)) )
	{
		wxLogDebug(wxT("MPEG::Header::parse() -- Second byte did not mactch MPEG synch."));
		return;
	}

	// Set the MPEG version

	if( !(flags&(1<<20)) && !(flags&(1<<19)) )
	{
		m_version = MPEG_Version2_5;
	}
	else if( (flags&(1<<20)) && !(flags&(1<<19)) )
	{
		m_version = MPEG_Version2;
	}
	else if( (flags&(1<<20)) && (flags&(1<<19)) )
	{
		m_version = MPEG_Version1;
	}

	// Set the MPEG layer

	if( !(flags&(1<<18)) && (flags&(1<<17)) )
	{
		m_layer = 3;
	}
	else if( (flags&(1<<18)) && !(flags&(1<<17)) )
	{
		m_layer = 2;
	}
	else if( (flags&(1<<18)) && (flags&(1<<17)) )
	{
		m_layer = 1;
	}


	// set protection flags

	m_protectionEnabled = !(flags&(1<<16));

	// Set the bitrate

	static const int bitrates[2][3][16] =
	{
		{	// Version 1
			{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }, // layer 1
			{ 0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384, 0 }, // layer 2
			{ 0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 0 }  // layer 3
		},
		{	// Version 2 or 2.5
			{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }, // layer 1
			{ 0, 8,  16, 24, 32, 40, 48, 56,  64,  80,  96,  112, 128, 144, 160, 0 }, // layer 2
			{ 0, 8,  16, 24, 32, 40, 48, 56,  64,  80,  96,  112, 128, 144, 160, 0 }  // layer 3
		}
	};

	const int versionIndex = m_version == MPEG_Version1 ? 0 : 1;
	const int layerIndex = m_layer > 0 ? m_layer - 1 : 0;

	// The bitrate index is encoded as the first 4 bits of the 3rd byte,
	// i.e. 1111xxxx

	int i = (unsigned char)(data[2]) >> 4;

	m_bitrate = bitrates[versionIndex][layerIndex][i];

	// Set the sample rate

	static const int sampleRates[3][4] =
	{
		{ 44100, 48000, 32000, 0 }, // Version 1
		{ 22050, 24000, 16000, 0 }, // Version 2
		{ 11025, 12000, 8000,  0 }  // Version 2.5
	};

	// The sample rate index is encoded as two bits in the 3nd byte, i.e. xxxx11xx

	i = (unsigned char)(data[2]) >> 2 & 0x03;

	m_sampleRate = sampleRates[m_version][i];

	if(m_sampleRate == 0)
	{
		wxLogDebug(wxT("MPEG::Header::parse() -- Invalid sample rate."));
		return;
	}

	// The channel mode is encoded as a 2 bit value at the end of the 3nd byte,
	// i.e. xxxxxx11 - stimmt nicht! s. http://www.mp3-tech.org/programmer/frame_header.html

	m_channelMode = (MPEG_HeaderChannelMode)((flags&0xC0) >> 6);

	// TODO: Add mode extension for completeness

	m_isCopyrighted = (flags&(1<<3)) != 0;
	m_isOriginal = (flags&(1<<2)) != 0;
	m_emphasis = (unsigned char)flags&3;

	// Calculate the frame length

	if( m_layer == 1 )
	{
		m_frameLength = 24000 * 2 * m_bitrate / m_sampleRate + int(m_isPadded);
	}
	else
	{
		m_frameLength = 72000 * m_bitrate / m_sampleRate + int(m_isPadded);
	}

	// Now that we're done parsing, set this to be a valid header

	m_isValid = true;
}




/***************************************************************************
 * XINGheader
 ***************************************************************************/



MPEG_XingHeader::MPEG_XingHeader(const SjByteVector &data)
{
	m_frames = 0;
	m_size = 0;
	m_valid = 0;
	parse(data);
}



int MPEG_XingHeader::xingHeaderOffset(MPEG_HeaderVersion v,
                                      MPEG_HeaderChannelMode c)
{
	if(v == MPEG_Version1)
	{
		if( c == MPEG_SingleChannel )
			return 0x15;
		else
			return 0x24;
	}
	else
	{
		if( c == MPEG_SingleChannel )
			return 0x0D;
		else
			return 0x15;
	}
}



void MPEG_XingHeader::parse(const SjByteVector &data)
{
	// Check to see if a valid Xing header is available.

	if(!data.startsWith((unsigned char*)"Xing"))
	{
		return;
	}

	// If the XingHeader doesn't contain the number of frames and the total stream
	// info it's invalid.

	if(!(data[7] & 0x02))
	{
		wxLogDebug(wxT("MPEG::XingHeader::parse() -- Xing header doesn't contain the total number of frames."));
		return;
	}

	if(!(data[7] & 0x04))
	{
		wxLogDebug(wxT("MPEG::XingHeader::parse() -- Xing header doesn't contain the total stream size."));
		return;
	}

	m_frames = data.mid(8, 4).toUInt();
	m_size = data.mid(12, 4).toUInt();

	m_valid = true;
}


