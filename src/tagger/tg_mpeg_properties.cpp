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
#include "tg_mpeg_properties.h"
#include "tg_mpeg_file.h"
#include "tg_mpeg_header.h"




MPEG_Properties::MPEG_Properties(MPEG_File *file) : Tagger_AudioProperties()
{
	m_length = 0;
	m_bitrate = 0;
	m_sampleRate = 0;
	m_isValid = FALSE;

	if(file )
		read(file);
}



void MPEG_Properties::read(MPEG_File *file)
{
	// Since we've likely just looked for the ID3v1 tag, start at the end of the
	// file where we're least likely to have to have to move the disk head.

	long last = file->lastFrameOffset();

	if( last < 0 )
	{
		wxLogDebug(wxT("MPEG::Properties::read() -- Could not find a valid last MPEG frame in the stream."));
		return;
	}

	file->Seek(last);
	MPEG_Header lastHeader(file->ReadBlock(4));

	long first = file->firstFrameOffset();

	if(first < 0)
	{
		wxLogDebug(wxT("MPEG::Properties::read() -- Could not find a valid first MPEG frame in the stream."));
		return;
	}

	if( !lastHeader.IsValid() )
	{
		long pos = last;
		while(pos > first)
		{
			pos = file->previousFrameOffset(pos);
			if( pos < 0 )
			{
				break;
			}

			file->Seek(pos);
			lastHeader.parse(file->ReadBlock(4));
			if( lastHeader.IsValid() )
			{
				last = pos;
				break;
			}
		}
	}

	// Now jump back to the front of the file and read what we need from there.

	file->Seek(first);
	MPEG_Header firstHeader(file->ReadBlock(4));

	if(!firstHeader.IsValid() || !lastHeader.IsValid())
	{
		wxLogDebug(wxT("MPEG::Properties::read() -- Page headers were invalid."));
		return;
	}

	// Check for a Xing header that will help us in gathering information about a
	// VBR stream.

	int xingHeaderOffset = MPEG_XingHeader::xingHeaderOffset(firstHeader.version(),
	                       firstHeader.channelMode());

	file->Seek(first + xingHeaderOffset);
	MPEG_XingHeader xingHeader(file->ReadBlock(16));

	// Read the length and the bitrate from the Xing header.

	if( firstHeader.frameLength() > 0 )
	{
		m_frameCount = (last - first) / firstHeader.frameLength() + 1;
	}
	else
	{
		m_frameCount = 0;
	}

	if( xingHeader.IsValid()
	        && firstHeader.sampleRate() > 0
	        && xingHeader.totalFrames() > 0 )
	{
		static const int blockSize[] = { 0, 384, 1152, 1152 };

		double timePerFrame = blockSize[firstHeader.layer()];
		timePerFrame = firstHeader.sampleRate() > 0 ? timePerFrame / firstHeader.sampleRate() : 0;
		m_length = int(timePerFrame * xingHeader.totalFrames());
		m_bitrate = m_length > 0 ? xingHeader.totalSize() * 8 / m_length / 1000 : 0;
	}
	else if(firstHeader.frameLength() > 0 && firstHeader.bitrate() > 0)
	{
		// Since there was no valid Xing header found, we hope that we're in a constant
		// bitrate file.

		// TODO: Make this more robust with audio property detection for VBR without a
		// Xing header.

		m_length = int(float(firstHeader.frameLength() * m_frameCount) /
		               float(firstHeader.bitrate() * 125) + 0.5);
		m_bitrate = firstHeader.bitrate();
	}

	m_sampleRate        = firstHeader.sampleRate();
	m_version           = firstHeader.version();
	m_layer             = firstHeader.layer();
	m_channelMode       = firstHeader.channelMode();
	m_isCopyrighted     = firstHeader.isCopyrighted();
	m_isOriginal        = firstHeader.isOriginal();
	m_isPadded          = firstHeader.isPadded();
	m_protectionEnabled = firstHeader.protectionEnabled();
	m_frameLength       = firstHeader.frameLength();
	m_emphasis          = firstHeader.emphasis();

	m_isValid           = TRUE;
}
