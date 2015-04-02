/***************************************************************************
    copyright            : (C) 2004 by Allan Sandfeld Jensen
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
#include "tg_mpc_file.h"
#include "tg_id3v1_tag.h"
#include "tg_id3v2_header_footer.h"
#include "tg_ape_tag.h"
#include "tg_combined_tag.h"



/*******************************************************************************
 *  MPC_Properties
 ******************************************************************************/



MPC_Properties::MPC_Properties(const SjByteVector& data, long streamLength)
	: Tagger_AudioProperties()
{
	m_version       = 0;
	m_length        = 0;
	m_bitrate       = 0;
	m_sampleRate    = 0;
	m_channels      = 0;

	// read ...
	if(!data.startsWith((unsigned char*)"MP+"))
		return;

	m_version = data[3] & 15;

	unsigned int frames;
	static const unsigned short sftable [4] = { 44100, 48000, 37800, 32000 };

	if(m_version >= 7)
	{
		frames = data.mid(4, 4).toUInt(false);

		unsigned long flags = data.mid(8, 4).toUInt(false);
		int flags17 = (flags&(1<<17))? 1 : 0;
		int flags16 = (flags&(1<<16))? 1 : 0;
		m_sampleRate = sftable[flags17 * 2 + flags16];

		m_channels = 2;
	}
	else
	{
		unsigned int headerData = data.mid(0, 4).toUInt(false);
		m_bitrate = (headerData >> 23) & 0x01ff;
		m_version = (headerData >> 11) & 0x03ff;
		m_sampleRate = 44100;
		m_channels = 2;
		if(m_version >= 5)
			frames = data.mid(4, 4).toUInt(false);
		else
			frames = data.mid(4, 2).toUInt(false);
	}

	unsigned int samples = frames * 1152 - 576;
	m_length = m_sampleRate > 0 ? (samples + (m_sampleRate / 2)) / m_sampleRate : 0;

	if(!m_bitrate)
		m_bitrate = m_length > 0 ? ((streamLength * 8L) / m_length) / 1000 : 0;
}



/*******************************************************************************
 *  MPC_File
 ******************************************************************************/



MPC_File::MPC_File(const wxFSFile* fsFile, wxInputStream* inputStream, bool readProperties)
	: Tagger_File(fsFile, inputStream)
{
	m_APETag        = 0;
	m_APELocation   = -1;
	m_APESize       = 0;
	m_ID3v1Tag      = 0;
	m_ID3v1Location = -1;
	m_ID3v2Header   = 0;
	m_ID3v2Location = -1;
	m_ID3v2Size     = 0;
	m_tag           = 0;
	m_properties    = 0;
	m_scanned       = false;
	m_hasAPE        = false;
	m_hasID3v1      = false;
	m_hasID3v2      = false;

	read(readProperties);
}



MPC_File::~MPC_File()
{
	if( m_tag && m_tag != m_ID3v1Tag && m_tag != m_APETag ) delete m_tag;
	if( m_ID3v1Tag )                                        delete m_ID3v1Tag;
	if( m_APETag )                                          delete m_APETag;
	if( m_ID3v2Header )                                     delete m_ID3v2Header;
	if( m_properties )                                      delete m_properties;
}



bool MPC_File::save()
{
	if(ReadOnly())
		return false;

	// Possibly strip ID3v2 tag
	if(m_hasID3v2 && !m_ID3v2Header)
	{
		RemoveBlock(m_ID3v2Location, m_ID3v2Size);
		m_hasID3v2 = false;
		if(m_hasID3v1)
			m_ID3v1Location -= m_ID3v2Size;

		if(m_hasAPE)
			m_APELocation -= m_ID3v2Size;
	}

	// Update ID3v1 tag
	if(m_ID3v1Tag)
	{
		if(m_hasID3v1)
		{
			Seek(m_ID3v1Location);
			WriteBlock(m_ID3v1Tag->render());
		}
		else
		{
			Seek(0, SJ_SEEK_END);
			m_ID3v1Location = Tell();
			WriteBlock(m_ID3v1Tag->render());
			m_hasID3v1 = true;
		}
	}
	else if(m_hasID3v1)
	{
		RemoveBlock(m_ID3v1Location, 128);
		m_hasID3v1 = false;
		if(m_hasAPE)
		{
			if(m_APELocation > m_ID3v1Location)
				m_APELocation -= 128;
		}
	}

	// Update APE tag
	if(m_APETag)
	{
		if(m_hasAPE)
		{
			Insert(m_APETag->render(), m_APELocation, m_APESize);
		}
		else
		{
			if(m_hasID3v1)
			{
				Insert(m_APETag->render(), m_ID3v1Location, 0);
				m_APESize = m_APETag->footer()->completeTagSize();
				m_hasAPE = true;
				m_APELocation = m_ID3v1Location;
				m_ID3v1Location += m_APESize;
			}
			else
			{
				Seek(0, SJ_SEEK_END);
				m_APELocation = Tell();
				WriteBlock(m_APETag->render());
				m_APESize = m_APETag->footer()->completeTagSize();
				m_hasAPE = true;
			}
		}
	}
	else if(m_hasAPE)
	{
		RemoveBlock(m_APELocation, m_APESize);
		m_hasAPE = false;
		if(m_hasID3v1)
		{
			if (m_ID3v1Location > m_APELocation)
				m_ID3v1Location -= m_APESize;
		}
	}

	return true;
}



ID3v1_Tag* MPC_File::ID3v1Tag(bool create)
{
	if(!create || m_ID3v1Tag)
		return m_ID3v1Tag;

	// no ID3v1 tag exists and we've been asked to create one
	m_ID3v1Tag = new ID3v1_Tag;

	if(m_APETag)
		m_tag = new Combined_Tag(m_APETag, m_ID3v1Tag);
	else
		m_tag = m_ID3v1Tag;

	return m_ID3v1Tag;
}



APE_Tag *MPC_File::APETag(bool create)
{
	if(!create || m_APETag)
		return m_APETag;

	// no APE tag exists and we've been asked to create one
	m_APETag = new APE_Tag;

	if(m_ID3v1Tag)
		m_tag = new Combined_Tag(m_APETag, m_ID3v1Tag);
	else
		m_tag = m_APETag;

	return m_APETag;
}



void MPC_File::remove(int tags)
{
	if(tags & MPC_ID3v1)
	{
		delete m_ID3v1Tag;
		m_ID3v1Tag = 0;

		if(m_APETag)
			m_tag = m_APETag;
		else
			m_tag = m_APETag = new APE_Tag;
	}

	if(tags & MPC_ID3v2)
	{
		delete m_ID3v2Header;
		m_ID3v2Header = 0;
	}

	if(tags & MPC_APE)
	{
		delete m_APETag;
		m_APETag = 0;

		if(m_ID3v1Tag)
			m_tag = m_ID3v1Tag;
		else
			m_tag = m_APETag = new APE_Tag;
	}
}



void MPC_File::read(bool readProperties)
{
	// Look for an ID3v1 tag
	m_ID3v1Location = findID3v1();

	if(m_ID3v1Location >= 0)
	{
		m_ID3v1Tag = new ID3v1_Tag(this, m_ID3v1Location);
		m_hasID3v1 = true;
	}

	// Look for an APE tag
	findAPE();

	m_APELocation = findAPE();

	if(m_APELocation >= 0)
	{
		m_APETag = new APE_Tag(this, m_APELocation);
		m_APESize = m_APETag->footer()->completeTagSize();
		m_APELocation = m_APELocation + APE_FOOTER_SIZE - m_APESize;
		m_hasAPE = true;
	}

	if(m_hasID3v1 && m_hasAPE)
	{
		m_tag = new Combined_Tag(m_APETag, m_ID3v1Tag);
	}
	else
	{
		if(m_hasID3v1)
		{
			m_tag = m_ID3v1Tag;
		}
		else
		{
			if(m_hasAPE)
				m_tag = m_APETag;
			else
				m_tag = m_APETag = new APE_Tag;
		}
	}

	// Look for and skip an ID3v2 tag
	m_ID3v2Location = findID3v2();

	if(m_ID3v2Location >= 0)
	{
		Seek(m_ID3v2Location);
		SjByteVector temp = ReadBlock(ID3v2_Header::size());
		m_ID3v2Header = new ID3v2_Header(&temp);
		m_ID3v2Size = m_ID3v2Header->completeTagSize();
		m_hasID3v2 = true;
	}

	if(m_hasID3v2)
		Seek(m_ID3v2Location + m_ID3v2Size);
	else
		Seek(0);

	// Look for MPC metadata

	if(readProperties)
	{
		m_properties = new MPC_Properties(ReadBlock(MPC_HeaderSize), Length() - m_ID3v2Size - m_APESize);
	}
}



long MPC_File::findAPE()
{
	if(!IsValid())
		return -1;

	if(m_hasID3v1)
		Seek(-160, SJ_SEEK_END);
	else
		Seek(-32, SJ_SEEK_END);

	long p = Tell();

	if(ReadBlock(8) == APE_Tag::fileIdentifier())
		return p;

	return -1;
}



long MPC_File::findID3v1()
{
	if(!IsValid())
		return -1;

	Seek(-128, SJ_SEEK_END);
	long p = Tell();

	if(ReadBlock(3) == ID3v1_Tag::fileIdentifier())
		return p;

	return -1;
}



long MPC_File::findID3v2()
{
	if(!IsValid())
		return -1;

	Seek(0);

	if(ReadBlock(3) == ID3v2_Header::fileIdentifier())
		return 0;

	return -1;
}
