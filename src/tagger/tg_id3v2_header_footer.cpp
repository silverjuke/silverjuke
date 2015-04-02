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
#include "tg_id3v2_header_footer.h"



#define HEADER_SIZE 10


////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

SjUint ID3v2_Header::size()
{
	return HEADER_SIZE;
}

SjByteVector ID3v2_Header::fileIdentifier()
{
	return SjByteVector::fromCString("ID3");
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


ID3v2_Header::ID3v2_Header(const SjByteVector* data)
{
	m_majorVersion = 0;
	m_revisionNumber = 0;
	m_unsynchronisation = false;
	m_extendedHeader = false;
	m_experimentalIndicator = false;
	m_footerPresent = false;
	m_tagSize = 0;

	if( data )
	{
		parse(*data);
	}
}

SjUint ID3v2_Header::completeTagSize() const
{
	if(m_footerPresent)
		return m_tagSize + HEADER_SIZE + ID3v2_Footer::size();
	else
		return m_tagSize + HEADER_SIZE;
}

SjByteVector ID3v2_Header::render() const
{
	SjByteVector v;

	// add the file identifier -- "ID3"
	v.append(fileIdentifier());

	// add the version number -- we always render a 2.4.0 tag regardless of what
	// the tag originally was.

	v.append((unsigned char)(4));
	v.append((unsigned char)(0));

	// render and add the flags
	/*std::bitset<8> flags;
	flags[7] = d->unsynchronisation;
	flags[6] = d->extendedHeader;
	flags[5] = d->experimentalIndicator;
	flags[4] = d->footerPresent;*/
	unsigned char flags = 0;
	if( m_unsynchronisation       ) { flags |= (1<<7); }
	if( m_extendedHeader          ) { flags |= (1<<6); }
	if( m_experimentalIndicator   ) { flags |= (1<<5); }
	if( m_footerPresent           ) { flags |= (1<<4); }

	v.append(flags);

	// add the size
	v.append(ID3v2_SynchDataFromUInt(m_tagSize));

	return v;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void ID3v2_Header::parse(const SjByteVector &data)
{
	if(data.size() < size())
		return;


	// do some sanity checking -- even in ID3v2.3.0 and less the tag size is a
	// synch-safe integer, so all bytes must be less than 128.  If this is not
	// true then this is an invalid tag.

	// note that we're doing things a little out of order here -- the size is
	// later in the bytestream than the version

	SjByteVector sizeData = data.mid(6, 4);

	if(sizeData.size() != 4) {
		m_tagSize = 0;
		wxLogDebug(wxT("Error: D3v2 header parse: The tag size as read was 0 bytes!"));
		return;
	}

	/*for(SjByteVector::Iterator it = sizeData.begin(); it != sizeData.end(); it++) {
	  if(uchar(*it) >= 128) {
	    d->tagSize = 0;
	    wxLogDebug("Tag'Lib::ID3v2::Header::parse() - One of the size bytes in the id3v2 header was greater than the allowed 128.");
	    return;
	  }
	}*/
	int i, iCount = sizeData.size();
	const unsigned char* it = sizeData.getReadableData();
	for( i = 0; i < iCount; i++ )
	{
		if( it[i] >= 128 )
		{
			m_tagSize = 0;
			wxLogDebug(wxT("Tagger ID3v2 header parse: One of the size bytes in the id3v2 header was greater than the allowed 128."));
			return;
		}
	}


	// The first three bytes, data[0..2], are the File Identifier, "ID3". (structure 3.1 "file identifier")

	// Read the version number from the fourth and fifth bytes.
	m_majorVersion = data[3];   // (structure 3.1 "major version")
	m_revisionNumber = data[4]; // (structure 3.1 "revision number")

	// Read the flags, the first four bits of the sixth byte.
	/*std::bitset<8> flags(data[5]);
	d->unsynchronisation     = flags[7]; // (structure 3.1.a)
	d->extendedHeader        = flags[6]; // (structure 3.1.b)
	d->experimentalIndicator = flags[5]; // (structure 3.1.c)
	d->footerPresent         = flags[4]; // (structure 3.1.d)*/
	unsigned char flags = data[5];
	m_unsynchronisation       = (flags & (1<<7)) != 0;
	m_extendedHeader          = (flags & (1<<6)) != 0;
	m_experimentalIndicator   = (flags & (1<<5)) != 0;
	m_footerPresent           = (flags & (1<<4)) != 0;

	// Get the size from the remaining four bytes (read above)

	m_tagSize = ID3v2_SynchDataToUInt(sizeData); // (structure 3.1 "size")
}





/*******************************************************************************
 * extended header
 ******************************************************************************/


ID3v2_ExtendedHeader::ID3v2_ExtendedHeader()
{
	m_size = 0;
}

void ID3v2_ExtendedHeader::parse(const SjByteVector &data)
{
	m_size = ID3v2_SynchDataToUInt(data.mid(0, 4)); // (structure 3.2 "Extended header size")
}




/*******************************************************************************
 * footer
 ******************************************************************************/


static const SjUint ID3v2_FooterPrivateSize = 10;

const unsigned int ID3v2_Footer::size()
{
	return ID3v2_FooterPrivateSize;
}

SjByteVector ID3v2_Footer::render(const ID3v2_Header *header) const
{
	SjByteVector headerData = header->render();
	headerData[0] = '3';
	headerData[1] = 'D';
	headerData[2] = 'I';
	return headerData;
}



/*******************************************************************************
 * SynchData
 ******************************************************************************/



SjUint ID3v2_SynchDataToUInt(const SjByteVector &data)
{
	SjUint sum = 0;
	int last = data.size() > 4 ? 3 : data.size() - 1;

	for(int i = 0; i <= last; i++)
		sum |= (data[i] & 0x7f) << ((last - i) * 7);

	return sum;
}

SjByteVector ID3v2_SynchDataFromUInt(SjUint value)
{
	SjByteVector v(4, 0);

	for(int i = 0; i < 4; i++)
		v[i] = (unsigned char)(value >> ((3 - i) * 7) & 0x7f);

	return v;
}
