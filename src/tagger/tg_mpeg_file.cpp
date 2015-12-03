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
#include "tg_id3v2_tag.h"
#include "tg_id3v2_header_footer.h"
#include "tg_id3v1_tag.h"
#include "tg_ape_tag.h"
#include "tg_mpeg_file.h"
#include "tg_mpeg_header.h"
#include "tg_combined_tag.h"


class MPEG_CombinedTag : public Combined_Tag
{
public:
	MPEG_CombinedTag(MPEG_File* file)
		: Combined_Tag(NULL, NULL, NULL,
		               Tagger_LookupCommentForFirstTagOnly)
		// do not hassle around with ID3v1 comments if an ID3v2 tag is present;
		// most times the ID3v1 tags are just truncated.
		// moreover, playing around with ID3v1 or APE comments is irritating
		// as it may seem as if we cannot read ID3v2 comments (in fact, we do not
		// read any comment here, eg. we skip music match etc.)
	{
		m_file = file;
	}

	void setID3v2Tag(ID3v2_Tag* id3v2Tag)
	{
		m_tag1 = id3v2Tag;
	}

	void setID3v1Tag(ID3v1_Tag* id3v1Tag)
	{
		m_tag2 = id3v1Tag;
	}

	void setAPETag(APE_Tag* apeTag)
	{
		m_tag3 = apeTag;
	}

protected:
	virtual void prepareForWrite()
	{
		// Create ID3v2 and ID3v1 tags but do not automatically create APE tags.

		m_file->ID3v2Tag(true); // Calling ID3v2Tag(true) automatically calls setID3v2Tag() here,
		m_file->ID3v1Tag(true); // same for ID3v1Tag(true)
	}

private:
	MPEG_File* m_file;
};



MPEG_File::MPEG_File(const wxString& url, wxInputStream* inputStream, int readStyle) :
	Tagger_File(url, inputStream)
{
	m_id3v2Tag          = NULL;
	m_id3v2Location     = -1;
	m_id3v2OriginalSize = 0;

	m_apeTag            = NULL;
	m_apeLocation       = -1;
	m_apeOriginalSize   = 0;

	m_id3v1Tag          = NULL;
	m_id3v1Location     = -1;

	m_combinedTag       = new MPEG_CombinedTag(this); // always needed to avoid too much checking for null-pointers!
	m_hasId3v2          = false;
	m_hasId3v1          = false;
	m_hasApe            = false;
	m_properties        = 0;

	read(readStyle);
}



MPEG_File::~MPEG_File()
{
	if( m_id3v2Tag )    delete m_id3v2Tag;
	if( m_id3v1Tag )    delete m_id3v1Tag;
	if( m_apeTag )      delete m_apeTag;
	if( m_properties )  delete m_properties;

	delete m_combinedTag; // always created, no null-check needed
}



Tagger_Tag* MPEG_File::tag() const
{
	return m_combinedTag;
}



bool MPEG_File::save(int tags, bool stripOthers)
{
	if(tags == MPEG_NoTags && stripOthers)
		return strip(MPEG_AllTags);

	if( !m_id3v2Tag && !m_id3v1Tag && !m_apeTag )
	{
		if( (m_hasId3v1 || m_hasId3v2 || m_hasApe) && stripOthers)
			return strip(MPEG_AllTags);

		return true;
	}

	if( ReadOnly() )
	{
		wxLogDebug(wxT("MPEG::File::save() -- File is read only."));
		return false;
	}

	// Create the tags if we've been asked to.  Copy the values from the tag that
	// does exist into the new tag.

	if( (tags & MPEG_ID3v2) && m_id3v1Tag )
	{
		Tagger_Tag::duplicate(m_id3v1Tag, ID3v2Tag(true), false);
	}

	if( (tags & MPEG_ID3v1) && m_id3v2Tag )
	{
		Tagger_Tag::duplicate(m_id3v2Tag, ID3v1Tag(true), false);
	}

	bool success = true;

	if(MPEG_ID3v2 & tags)
	{
		if (m_id3v2Tag && !m_id3v2Tag->isEmpty() )
		{
			if(!m_hasId3v2)
			{
				m_id3v2Location = 0;
			}

			success = Insert(m_id3v2Tag->render(), m_id3v2Location, m_id3v2OriginalSize) && success;
		}
		else if(stripOthers)
		{
			success = strip(MPEG_ID3v2, false) && success;
		}
	}
	else if(m_hasId3v2 && stripOthers)
	{
		success = strip(MPEG_ID3v2) && success;
	}

	if(MPEG_ID3v1 & tags)
	{
		if(m_id3v1Tag && !m_id3v1Tag->isEmpty())
		{
			int offset = m_hasId3v1 ? -128 : 0;
			Seek(offset, SJ_SEEK_END);
			success = WriteBlock(m_id3v1Tag->render()) && success;
		}
		else if(stripOthers)
		{
			success = strip(MPEG_ID3v1) && success;
		}
	}
	else if(m_hasId3v1 && stripOthers)
	{
		success = strip(MPEG_ID3v1, false) && success;
	}

	// Dont save an APE-tag unless one has been created

	if((MPEG_APE & tags) && m_apeTag)
	{
		if(m_hasApe)
		{
			success = Insert(m_apeTag->render(), m_apeLocation, m_apeOriginalSize) && success;
		}
		else
		{
			if(m_hasId3v1)
			{
				success = Insert(m_apeTag->render(), m_id3v1Location, 0) && success;
				m_apeOriginalSize = m_apeTag->footer()->completeTagSize();
				m_hasApe = true;
				m_apeLocation = m_id3v1Location;
				m_id3v1Location += m_apeOriginalSize;
			}
			else
			{
				Seek(0, SJ_SEEK_END);
				m_apeLocation = Tell();
				success = WriteBlock(m_apeTag->render()) && success;
				m_apeOriginalSize = m_apeTag->footer()->completeTagSize();
				m_hasApe = true;
			}
		}
	}
	else if(m_hasApe && stripOthers)
	{
		success = strip(MPEG_APE, false) && success;
	}

	return success;
}



ID3v2_Tag* MPEG_File::ID3v2Tag(bool create)
{
	if( !create || m_id3v2Tag )
	{
		return m_id3v2Tag;
	}

	// no ID3v2 tag exists and we've been asked to create one
	m_id3v2Tag = new ID3v2_Tag;
	m_combinedTag->setID3v2Tag(m_id3v2Tag);
	return m_id3v2Tag;
}



ID3v1_Tag* MPEG_File::ID3v1Tag(bool create)
{
	if(!create || m_id3v1Tag)
	{
		return m_id3v1Tag;
	}

	// no ID3v1 tag exists and we've been asked to create one
	m_id3v1Tag = new ID3v1_Tag;
	m_combinedTag->setID3v1Tag(m_id3v1Tag);
	return m_id3v1Tag;
}



APE_Tag* MPEG_File::APETag(bool create)
{
	if( !create || m_apeTag )
	{
		return m_apeTag;
	}

	// no APE tag exists and we've been asked to create one
	m_apeTag = new APE_Tag;
	m_combinedTag->setAPETag(m_apeTag);
	return m_apeTag;
}



bool MPEG_File::strip(int tags, bool freeMemory)
{
	if( ReadOnly() )
	{
		wxLogDebug(wxT("MPEG::File::strip() - Cannot strip tags from a read only file."));
		return false;
	}

	if((tags & MPEG_ID3v2) && m_hasId3v2)
	{
		if( !RemoveBlock(m_id3v2Location, m_id3v2OriginalSize) )
			return false;
		m_id3v2Location = -1;
		m_id3v2OriginalSize = 0;
		m_hasId3v2 = false;
		if(freeMemory)
		{
			delete m_id3v2Tag;
			m_id3v2Tag = NULL;
			m_combinedTag->setID3v2Tag(NULL);
		}

		// v1 tag location has changed, update if it exists
		if(m_id3v1Tag)
		{
			m_id3v1Location = findID3v1();
		}
	}

	if((tags & MPEG_ID3v1) && m_hasId3v1)
	{
		Truncate(m_id3v1Location);
		m_id3v1Location = -1;
		m_hasId3v1 = false;
		if(freeMemory)
		{
			delete m_id3v1Tag;
			m_id3v1Tag = NULL;
			m_combinedTag->setID3v1Tag(NULL);
		}
	}

	if((tags & MPEG_APE) && m_hasApe)
	{
		if( !RemoveBlock(m_apeLocation, m_apeOriginalSize) )
			return false;
		m_apeLocation = -1;
		m_hasApe = false;
		if(m_hasId3v1)
		{
			if (m_id3v1Location > m_apeLocation)
				m_id3v1Location -= m_apeOriginalSize;
		}
		if(freeMemory)
		{
			delete m_apeTag;
			m_apeTag = NULL;
			m_combinedTag->setAPETag(NULL);
		}
	}

	return true;
}



long MPEG_File::nextFrameOffset(long position)
{
	// TODO: This will miss syncs spanning buffer read boundaries.

	SjByteVector buffer = ReadBlock(BufferSize());

	while(buffer.size() > 0)
	{
		Seek(position);
		SjByteVector buffer = ReadBlock(BufferSize());

		for(SjUint i = 0; i < buffer.size(); i++)
		{
			if((unsigned char)(buffer[i]) == 0xff && secondSynchByte(buffer[i + 1]))
			{
				return position + i;
			}
		}
		position += BufferSize();
	}

	return -1;
}



long MPEG_File::previousFrameOffset(long position)
{
	// TODO: This will miss syncs spanning buffer read boundaries.

	while(int(position - BufferSize()) > int(BufferSize()))
	{
		position -= BufferSize();
		Seek(position);
		SjByteVector buffer = ReadBlock(BufferSize());

		// If the amount of data is smaller than an MPEG header (4 bytes) there's no
		// chance of this being valid.

		if(buffer.size() < 4)
		{
			return -1;
		}

		for(int i = buffer.size() - 2; i >= 0; i--)
		{
			if((unsigned char)(buffer[i]) == 0xff && secondSynchByte(buffer[i + 1]))
			{
				return position + i;
			}
		}
	}

	return -1;
}



long MPEG_File::firstFrameOffset()
{
	long position = 0;

	if( m_id3v2Tag )
	{
		position = m_id3v2Location + m_id3v2Tag->header()->completeTagSize();
	}

	return nextFrameOffset(position);
}



long MPEG_File::lastFrameOffset()
{
	return previousFrameOffset(m_id3v1Tag ? m_id3v1Location - 1 : Length());
}



void MPEG_File::read(int readStyle)
{
	if( readStyle & Tagger_ReadTags )
	{
		// Look for an ID3v2 tag

		m_id3v2Location = findID3v2();

		if(m_id3v2Location >= 0)
		{
			m_id3v2Tag = new ID3v2_Tag(this, m_id3v2Location, ID3v2_FrameFactory::instance());

			m_id3v2OriginalSize = m_id3v2Tag->header()->completeTagSize();

			if(m_id3v2Tag->header()->tagSize() <= 0)
			{
				delete m_id3v2Tag;
				m_id3v2Tag = NULL;
			}
			else
			{
				m_hasId3v2 = true;
				m_combinedTag->setID3v2Tag(m_id3v2Tag);
			}
		}

		// Look for an ID3v1 tag

		m_id3v1Location = findID3v1();

		if(m_id3v1Location >= 0)
		{
			m_id3v1Tag = new ID3v1_Tag(this, m_id3v1Location);
			m_hasId3v1 = true;

			m_combinedTag->setID3v1Tag(m_id3v1Tag);
		}

		// Look for an APE tag

		m_apeLocation = findAPE();

		if(m_apeLocation >= 0)
		{
			m_apeTag = new APE_Tag(this, m_apeLocation);

			m_apeOriginalSize = m_apeTag->footer()->completeTagSize();

			m_apeLocation = m_apeLocation + APE_FOOTER_SIZE - m_apeOriginalSize;

			m_hasApe = true;

			m_combinedTag->setAPETag(m_apeTag);
		}
	}

	if( readStyle & Tagger_ReadAudioProperties )
	{
		// read headers etc.

		m_properties = new MPEG_Properties(this);
	}
}

long MPEG_File::findID3v2()
{
	// This method is based on the contents of Tagger_File::find(), but because
	// of some subtlteies -- specifically the need to look for the bit pattern of
	// an MPEG sync, it has been modified for use here.

	if( IsValid()
	        && ID3v2_Header::fileIdentifier().size() <= BufferSize() )
	{
		// The position in the file that the current buffer starts at.

		long bufferOffset = 0;
		SjByteVector buffer;

		// These variables are used to keep track of a partial match that happens at
		// the end of a buffer.

		int previousPartialMatch = -1;
		bool previousPartialSynchMatch = false;

		// Save the location of the current read pointer.  We will restore the
		// position using seek() before all returns.

		long originalPosition = Tell();

		// Start the search at the beginning of the file.

		Seek(0);

		// This loop is the crux of the find method.  There are three cases that we
		// want to account for:
		// (1) The previously searched buffer contained a partial match of the search
		// pattern and we want to see if the next one starts with the remainder of
		// that pattern.
		//
		// (2) The search pattern is wholly contained within the current buffer.
		//
		// (3) The current buffer ends with a partial match of the pattern.  We will
		// note this for use in the next itteration, where we will check for the rest
		// of the pattern.

		for(buffer = ReadBlock(BufferSize()); buffer.size() > 0; buffer = ReadBlock(BufferSize()))
		{

			// (1) previous partial match

			if(previousPartialSynchMatch && secondSynchByte(buffer[0]))
			{
				return -1;
			}

			if(previousPartialMatch >= 0 && int(BufferSize()) > previousPartialMatch)
			{
				const int patternOffset = (BufferSize() - previousPartialMatch);
				if(buffer.containsAt(ID3v2_Header::fileIdentifier(), 0, patternOffset))
				{
					Seek(originalPosition);
					return bufferOffset - BufferSize() + previousPartialMatch;
				}
			}

			// (2) pattern contained in current buffer

			long location = buffer.find(ID3v2_Header::fileIdentifier());
			if(location >= 0)
			{
				Seek(originalPosition);
				return bufferOffset + location;
			}

			int firstSynchByte = buffer.find(/*(char)*/((unsigned char)(255)));

			// Here we have to loop because there could be several of the first
			// (11111111) byte, and we want to check all such instances until we find
			// a full match (11111111 111) or hit the end of the buffer.

			while(firstSynchByte >= 0)
			{

				// if this *is not* at the end of the buffer

				if(firstSynchByte < int(buffer.size()) - 1)
				{
					if(secondSynchByte(buffer[firstSynchByte + 1]))
					{
						// We've found the frame synch pattern.
						Seek(originalPosition);
						return -1;
					}
					else
					{

						// We found 11111111 at the end of the current buffer indicating a
						// partial match of the synch pattern.  The find() below should
						// return -1 and break out of the loop.

						previousPartialSynchMatch = true;
					}
				}

				// Check in the rest of the buffer.

				firstSynchByte = buffer.find(/*char*/((unsigned char)(255)), firstSynchByte + 1);
			}

			// (3) partial match

			previousPartialMatch = buffer.endsWithPartialMatch(ID3v2_Header::fileIdentifier());

			bufferOffset += BufferSize();

		} // for()

		// Since we hit the end of the file, reset the status before continuing.

		Clear();

		Seek(originalPosition);
	}

	return -1;
}



long MPEG_File::findID3v1()
{
	if( IsValid() )
	{
		Seek(-128, SJ_SEEK_END);
		long p = Tell();

		if( ReadBlock(3) == ID3v1_Tag::fileIdentifier() )
		{
			return p;
		}
	}

	return -1;
}



long MPEG_File::findAPE()
{
	if( IsValid() )
	{
		if (m_hasId3v1)
		{
			Seek(-160, SJ_SEEK_END);
		}
		else
		{
			Seek(-32, SJ_SEEK_END);
		}

		long p = Tell();

		if(ReadBlock(8) == APE_Tag::fileIdentifier())
		{
			return p;
		}
	}

	return -1;
}



bool MPEG_File::secondSynchByte(char byte)
{
	if( (unsigned char)(byte) == 0xff )
	{
		return false;
	}

	// check to see if the byte matches 111xxxxx
	return ( (((unsigned char)byte) & 0xE0) == 0xE0 );
}
