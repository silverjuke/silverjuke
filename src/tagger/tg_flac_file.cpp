/***************************************************************************
    copyright            : (C) 2003-2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.org
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
#include "tg_id3v2_header_footer.h"
#include "tg_id3v2_tag.h"
#include "tg_id3v1_tag.h"
#include "tg_ogg_xiphcomment.h"
#include "tg_flac_file.h"
#include "tg_combined_tag.h"



/*******************************************************************************
 *  FLAC_Properties
 ******************************************************************************/



FLAC_Properties::FLAC_Properties(const SjByteVector& data, long streamLength)
	: Tagger_AudioProperties()
{
	m_streamLength  = streamLength;
	m_length        = 0;
	m_bitrate       = 0;
	m_sampleRate    = 0;
	m_sampleWidth   = 0;
	m_channels      = 0;

	// read ...
	if(data.size() < 18)
		return;

	int pos = 0;

	// Minimum block size (in samples)
	pos += 2;

	// Maximum block size (in samples)
	pos += 2;

	// Minimum frame size (in bytes)
	pos += 3;

	// Maximum frame size (in bytes)
	pos += 3;

	SjUint flags = data.mid(pos, 4).toUInt(true);
	m_sampleRate = flags >> 12;
	m_channels = ((flags >> 9) & 7) + 1;
	m_sampleWidth = ((flags >> 4) & 31) + 1;

	// The last 4 bits are the most significant 4 bits for the 36 bit
	// stream length in samples. (Audio files measured in days)
	SjUint highLength =m_sampleRate > 0 ? (((flags & 0xf) << 28) / m_sampleRate) << 4 : 0;
	pos += 4;

	m_length = m_sampleRate > 0 ? (data.mid(pos, 4).toUInt(true)) / m_sampleRate + highLength : 0;
	pos += 4;

	// bitrate
	// (Uncompressed bitrate would be "((m_sampleRate * m_channels) / 1000) * m_sampleWidth")
	m_bitrate = m_length > 0 ? ((m_streamLength * 8L) / m_length) / 1000 : 0;
}



/*******************************************************************************
 *  FLAC_File
 ******************************************************************************/


enum FLAC_BLOCK_TYPE
{
    FLAC_STREAMINFO = 0,
    FLAC_PADDING,
    FLAC_APPLICATION,
    FLAC_SEEKTABLE,
    FLAC_VORBISCOMMENT,
    FLAC_CUESHEET
};



FLAC_File::FLAC_File(const wxString& url, wxInputStream* inputStream, bool readProperties, ID3v2_FrameFactory* frameFactory)
	: Tagger_File(url, inputStream)
{
	m_ID3v2Tag          = 0;
	m_ID3v2Location     = -1;
	m_ID3v2OriginalSize = 0;
	m_ID3v1Tag          = 0;
	m_ID3v1Location     = -1;
	m_tag               = 0; // fixed bug
	m_comment           = 0;
	m_properties        = 0;
	m_flacStart         = 0;
	m_streamStart       = 0;
	m_streamLength      = 0;
	m_scanned           = false;
	m_hasXiphComment    = false;
	m_hasID3v2          = false;
	m_hasID3v1          = false;
	m_ID3v2FrameFactory = frameFactory? frameFactory : ID3v2_FrameFactory::instance();

	read(readProperties);
}



FLAC_File::~FLAC_File()
{
	if( m_ID3v2Tag )    delete m_ID3v2Tag;
	if( m_ID3v1Tag)     delete m_ID3v1Tag;
	if( m_comment )     delete m_comment;
	if( m_properties )  delete m_properties;
	if( m_tag )         delete m_tag;
}



bool FLAC_File::save()
{
	if(ReadOnly())
	{
		return false;
	}

	// Create new vorbis comments

	if(!m_comment)
	{
		m_comment = new Ogg_XiphComment;
		if(m_tag)
		{
			Tagger_Tag::duplicate(m_tag, m_comment, true);
		}
	}

	m_xiphCommentData = m_comment->render(false);

	SjByteVector v = SjByteVector::fromUint(m_xiphCommentData.size());

	// Set the type of the comment to be a Xiph / Vorbis comment
	// (See scan() for comments on header-format)
	v[0] = 4;
	v.append(m_xiphCommentData);


	// If file already have comment => find and update it
	//                       if not => insert one
	// TODO: Search for padding and use that

	if(m_hasXiphComment)
	{
		long nextPageOffset = m_flacStart;
		Seek(nextPageOffset);
		SjByteVector header = ReadBlock(4);
		SjUint length = header.mid(1, 3).toUInt();

		nextPageOffset += length + 4;

		// Search through the remaining metadata
		char blockType = header[0] & 0x7f;
		bool lastBlock = (header[0] & 0x80)!=0;

		while(!lastBlock)
		{
			Seek(nextPageOffset);

			header = ReadBlock(4);
			blockType = header[0] & 0x7f;
			lastBlock = (header[0] & 0x80)!=0;
			length = header.mid(1, 3).toUInt();

			// Type is vorbiscomment
			if(blockType == 4)
			{
				v[0] = header[0];
				Insert(v, nextPageOffset, length + 4);
				break;
			}

			nextPageOffset += length + 4;
		}
	}
	else
	{
		long nextPageOffset = m_flacStart;

		Seek(nextPageOffset);

		SjByteVector header = ReadBlock(4);
		// char blockType = header[0] & 0x7f;
		bool lastBlock = (header[0] & 0x80)!=0;
		SjUint length = header.mid(1, 3).toUInt();

		// If last block was last, make this one last
		if(lastBlock)
		{
			// Copy the bottom seven bits into the new value
			SjByteVector h((unsigned char)(header[0] & 0x7F));
			Insert(h, nextPageOffset, 1);

			// Set the last bit
			v[0] |= 0x80;
		}

		Insert(v, nextPageOffset + length + 4, 0);
		m_hasXiphComment = true;
	}

	// Update ID3 tags
	if(m_ID3v2Tag)
	{
		if(m_hasID3v2)
			Insert(m_ID3v2Tag->render(), m_ID3v2Location, m_ID3v2OriginalSize);
		else
			Insert(m_ID3v2Tag->render(), 0, 0);
	}

	if(m_ID3v1Tag)
	{
		if(m_hasID3v1)
			Seek(-128, SJ_SEEK_END);
		else
			Seek(0, SJ_SEEK_END);
		WriteBlock(m_ID3v1Tag->render());
	}

	return true;
}



ID3v2_Tag *FLAC_File::ID3v2Tag(bool create)
{
	if(!create || m_ID3v2Tag)
		return m_ID3v2Tag;

	// no ID3v2 tag exists and we've been asked to create one
	m_ID3v2Tag = new ID3v2_Tag;
	return m_ID3v2Tag;
}



ID3v1_Tag *FLAC_File::ID3v1Tag(bool create)
{
	if(!create || m_ID3v1Tag)
		return m_ID3v1Tag;

	// no ID3v1 tag exists and we've been asked to create one
	m_ID3v1Tag = new ID3v1_Tag;
	return m_ID3v1Tag;
}



Ogg_XiphComment *FLAC_File::xiphComment(bool create)
{
	if(!create || m_comment)
		return m_comment;

	// no XiphComment exists and we've been asked to create one

	m_comment = new Ogg_XiphComment;
	return m_comment;
}



void FLAC_File::read(bool readProperties)
{
	// Look for an ID3v2 tag
	m_ID3v2Location = findID3v2();
	if( m_ID3v2Location >= 0 )
	{
		m_ID3v2Tag = new ID3v2_Tag(this, m_ID3v2Location, m_ID3v2FrameFactory);
		m_ID3v2OriginalSize = m_ID3v2Tag->header()->completeTagSize();
		if(m_ID3v2Tag->header()->tagSize() <= 0)
		{
			delete m_ID3v2Tag;
			m_ID3v2Tag = 0;
		}
		else
		{
			m_hasID3v2 = true;
		}
	}

	// Look for an ID3v1 tag
	m_ID3v1Location = findID3v1();
	if( m_ID3v1Location >= 0 )
	{
		m_ID3v1Tag = new ID3v1_Tag(this, m_ID3v1Location);
		m_hasID3v1 = true;
	}

	// Look for FLAC metadata, including vorbis comments
	scan();
	if( !IsValid() )
	{
		return;
	}

	if(m_hasXiphComment)
	{
		m_comment = new Ogg_XiphComment(xiphCommentData());
	}

	if(m_hasXiphComment || m_hasID3v2 || m_hasID3v1)
	{
		m_tag = new Combined_Tag(m_comment, m_ID3v2Tag, m_ID3v1Tag);
	}
	else
	{
		m_tag = new Combined_Tag(new Ogg_XiphComment);
	}

	if(readProperties)
	{
		m_properties = new FLAC_Properties(streamInfoData(), streamLength());
	}
}



SjByteVector FLAC_File::streamInfoData()
{
	if (IsValid())
		return m_streamInfoData;
	else
		return SjByteVector();
}



SjByteVector FLAC_File::xiphCommentData()
{
	if (IsValid() && m_hasXiphComment)
		return m_xiphCommentData;
	else
		return SjByteVector();
}



void FLAC_File::scan()
{
	// Scan the metadata pages
	if( m_scanned || !IsValid() )
		return;

	long nextPageOffset;
	long fileSize = Length();

	if (m_hasID3v2)
		nextPageOffset = Find((unsigned char*)"fLaC", m_ID3v2Location+m_ID3v2OriginalSize);
	else
		nextPageOffset = Find((unsigned char*)"fLaC");

	if(nextPageOffset < 0)
	{
		SetValid(false);
		return;
	}

	nextPageOffset += 4;
	m_flacStart = nextPageOffset;

	Seek(nextPageOffset);

	SjByteVector header = ReadBlock(4);

	// Header format (from spec):
	// <1> Last-metadata-block flag
	// <7> BLOCK_TYPE
	//  0 : STREAMINFO
	//    1 : PADDING
	//    ..
	//    4 : VORBIS_COMMENT
	//    ..
	// <24> Length of metadata to follow

	char blockType = header[0] & 0x7f;
	bool lastBlock = (header[0] & 0x80)!=0;
	SjUint length = header.mid(1, 3).toUInt();

	// First block should be the stream_info metadata
	if(blockType != 0)
	{
		SetValid(false);
		return;
	}
	m_streamInfoData = ReadBlock(length);
	nextPageOffset += length + 4;

	// Search through the remaining metadata
	while(!lastBlock)
	{
		header = ReadBlock(4);
		blockType = header[0] & 0x7f;
		lastBlock = (header[0] & 0x80)!=0;
		length = header.mid(1, 3).toUInt();

		if(blockType == 1)
		{
			; // padding found
		}
		else if(blockType == 4)
		{
			// Found the vorbis-comment
			m_xiphCommentData = ReadBlock(length);
			m_hasXiphComment = true;
		}

		nextPageOffset += length + 4;
		if (nextPageOffset >= fileSize)
		{
			// FLAC stream corrupted
			SetValid(false);
			return;
		}
		Seek(nextPageOffset);
	}

	// End of metadata, now comes the datastream
	m_streamStart = nextPageOffset;
	m_streamLength = Tagger_File::Length() - m_streamStart;
	if (m_hasID3v1)
		m_streamLength -= 128;

	m_scanned = true;
}



long FLAC_File::findID3v1()
{
	if(!IsValid())
		return -1;

	Seek(-128, SJ_SEEK_END);
	long p = Tell();

	if(ReadBlock(3) == ID3v1_Tag::fileIdentifier())
		return p;

	return -1;
}



long FLAC_File::findID3v2()
{
	if(!IsValid())
		return -1;

	Seek(0);

	if(ReadBlock(3) == ID3v2_Header::fileIdentifier())
		return 0;

	return -1;
}
