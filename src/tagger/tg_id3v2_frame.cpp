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
#include <zlib.h>
#include "tg_id3v2_frame.h"
#include "tg_id3v2_header_footer.h" // for SynchData

#include <wx/listimpl.cpp> // sic!
WX_DEFINE_LIST(ID3v2_FrameList);




////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

SjUint ID3v2_Frame::headerSize()
{
	return ID3v2_FrameHeader::size();
}

SjUint ID3v2_Frame::headerSize(SjUint version)
{
	return ID3v2_FrameHeader::size(version);
}

SjByteVector ID3v2_Frame::textDelimiter(SjStringType t)
{
	SjByteVector d = (unsigned char)(0);
	if(t == SJ_UTF16 || t == SJ_UTF16BE)
		d.append((unsigned char)(0));
	return d;
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

ID3v2_Frame::~ID3v2_Frame()
{
	if( m_header ) delete m_header;
}

SjByteVector ID3v2_Frame::frameID() const
{
	if(m_header)
		return m_header->frameID();
	else
		return SjByteVector::null;
}

SjUint ID3v2_Frame::size() const
{
	if(m_header)
		return m_header->frameSize();
	else
		return 0;
}

SjByteVector ID3v2_Frame::render() const
{
	SjByteVector fieldData = renderFields();
	m_header->setFrameSize(fieldData.size());
	SjByteVector headerData = m_header->render();

	return headerData + fieldData;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

ID3v2_Frame::ID3v2_Frame(const SjByteVector &data)
{
	m_header = new ID3v2_FrameHeader(data);
}

ID3v2_Frame::ID3v2_Frame(ID3v2_FrameHeader *h)
{
	m_header = h;
}

void ID3v2_Frame::setHeader(ID3v2_FrameHeader *h, bool deleteCurrent)
{
	if(deleteCurrent)
		delete m_header;

	m_header = h;
}

void ID3v2_Frame::parse(const SjByteVector &data)
{
	if(m_header)
		m_header->setData(data);
	else
		m_header = new ID3v2_FrameHeader(data);

	parseFields(fieldData(data));
}

SjByteVector ID3v2_Frame::fieldData(const SjByteVector &frameData) const
{
	SjUint headerSize = ID3v2_FrameHeader::size(m_header->version());

	SjUint frameDataOffset = headerSize;
	SjUint frameDataLength = size();

	if(m_header->compression() || m_header->dataLengthIndicator()) {
		frameDataLength = frameData.mid(headerSize, 4).toUInt();
		frameDataOffset += 4;
	}

	if(m_header->compression()) {
		SjByteVector data(frameDataLength);

		uLongf uLongTmp = data.size(); // normally, this should be same as frameDataLength - however, on allocation errors, this may be 0!
		::uncompress((Bytef *) data.getWriteableData(),
		             (uLongf *) &uLongTmp,
		             (Bytef *) frameData.getReadableData() + frameDataOffset,
		             size());
		return data;
	}
	else
		return frameData.mid(frameDataOffset, frameDataLength);
}

////////////////////////////////////////////////////////////////////////////////
// Frame::Header class
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// static members (Frame::Header)
////////////////////////////////////////////////////////////////////////////////

SjUint ID3v2_FrameHeader::size()
{
	return size(4);
}

SjUint ID3v2_FrameHeader::size(SjUint version)
{
	switch(version) {
		case 0:
		case 1:
		case 2:
			return 6;
		case 3:
		case 4:
		default:
			return 10;
	}
}

////////////////////////////////////////////////////////////////////////////////
// public members (Frame::Header)
////////////////////////////////////////////////////////////////////////////////

ID3v2_FrameHeader::ID3v2_FrameHeader(const SjByteVector &data, bool synchSafeInts)
{
	setData(data, synchSafeInts);
}

ID3v2_FrameHeader::ID3v2_FrameHeader(const SjByteVector &data, SjUint version)
{
	setData(data, version);
}

void ID3v2_FrameHeader::setData(const SjByteVector &data, bool synchSafeInts)
{
	setData(data, (SjUint)(synchSafeInts ? 4 : 3));
}

void ID3v2_FrameHeader::setData(const SjByteVector &data, SjUint version)
{
	// this was ID3v2_FrameHeaderPrivate
	m_isOkay = FALSE;
	m_frameSize = 0;
	m_version = 4;
	m_tagAlterPreservation = false;
	m_fileAlterPreservation = false;
	m_readOnly = false;
	m_groupingIdentity = false;
	m_compression = false;
	m_encryption = false;
	m_unsyncronisation = false;
	m_dataLengthIndicator = false;
	// /ID3v2_FrameHeaderPrivate


	m_version = version;

	switch(version) {
		case 0:
		case 1:
		case 2:
		{
			// ID3v2.2

			if(data.size() < 3) {
				wxLogDebug(wxT("You must at least specify a frame ID."));
				return;
			}

			// Set the frame ID -- the first three bytes

			m_frameID = data.mid(0, 3);

			// If the full header information was not passed in, do not continue to the
			// steps to parse the frame size and flags.

			if(data.size() < 6) {
				return;
			}

			m_frameSize = data.mid(3, 3).toUInt();

			break;
		}
		case 3:
		{
			// ID3v2.3 - see http://www.id3.org/id3v2.3.0.html#sec3.3

			if(data.size() < 4) {
				wxLogDebug(wxT("You must at least specify a frame ID."));
				return;
			}

			// Set the frame ID -- the first four bytes

			m_frameID = data.mid(0, 4);

			// If the full header information was not passed in, do not continue to the
			// steps to parse the frame size and flags.

			if(data.size() < 10) {
				return;
			}

			// Set the size -- the frame size is the four bytes starting at byte four in
			// the frame header (structure 4)

			m_frameSize = data.mid(4, 4).toUInt();

			{	// read the first byte of flags
				/*std::bitset<8> flags(data[8]);
				d->tagAlterPreservation  = flags[7]; // (structure 3.3.1.a)
				d->fileAlterPreservation = flags[6]; // (structure 3.3.1.b)
				d->readOnly              = flags[5]; // (structure 3.3.1.c)*/
				unsigned char flags = data[8];
				m_tagAlterPreservation    = (flags & (1<<7)) != 0;
				m_fileAlterPreservation   = (flags & (1<<6)) != 0;
				m_readOnly                = (flags & (1<<5)) != 0;
			}

			{	// read the second byte of flags
				/*std::bitset<8> flags(data[9]);
				d->compression         = flags[7]; // (structure 3.3.1.i)
				d->encryption          = flags[6]; // (structure 3.3.1.j)
				d->groupingIdentity    = flags[5]; // (structure 3.3.1.k)*/
				unsigned char flags = data[9];
				m_compression     = (flags & (1<<7)) != 0;
				m_encryption          = (flags & (1<<6)) != 0;
				m_groupingIdentity    = (flags & (1<<5)) != 0;
			}
			break;
		}
		case 4:
		default:
		{
			// ID3v2.4

			if(data.size() < 4) {
				wxLogDebug(wxT("You must at least specify a frame ID."));
				return;
			}

			// Set the frame ID -- the first four bytes

			m_frameID = data.mid(0, 4);

			// If the full header information was not passed in, do not continue to the
			// steps to parse the frame size and flags.

			if(data.size() < 10) {
				return;
			}

			// Set the size -- the frame size is the four bytes starting at byte four in
			// the frame header (structure 4)

			m_frameSize = ID3v2_SynchDataToUInt(data.mid(4, 4));

			{	// read the first byte of flags
				/*std::bitset<8> flags(data[8]);
				d->tagAlterPreservation  = flags[6]; // (structure 4.1.1.a)
				d->fileAlterPreservation = flags[5]; // (structure 4.1.1.b)
				d->readOnly              = flags[4]; // (structure 4.1.1.c)*/
				unsigned char flags = data[8];
				m_tagAlterPreservation    = (flags & (1<<6)) != 0;
				m_fileAlterPreservation   = (flags & (1<<5)) != 0;
				m_readOnly                = (flags & (1<<4)) != 0;
			}

			{	// read the second byte of flags
				/*std::bitset<8> flags(data[9]);
				d->groupingIdentity    = flags[6]; // (structure 4.1.2.h)
				d->compression         = flags[3]; // (structure 4.1.2.k)
				d->encryption          = flags[2]; // (structure 4.1.2.m)
				d->unsyncronisation    = flags[1]; // (structure 4.1.2.n)
				d->dataLengthIndicator = flags[0]; // (structure 4.1.2.p)*/
				unsigned char flags = data[9];
				m_groupingIdentity    = (flags & (1<<6)) != 0;
				m_compression         = (flags & (1<<3)) != 0;
				m_encryption          = (flags & (1<<2)) != 0;
				m_unsyncronisation    = (flags & (1<<1)) != 0;
				m_dataLengthIndicator = (flags & (1<<0)) != 0;
			}
			break;
		}
	}

	m_isOkay = TRUE;
}

void ID3v2_FrameHeader::setFrameID(const SjByteVector &id)
{
	m_frameID = id.mid(0, 4);
}

SjByteVector ID3v2_FrameHeader::render() const
{
	SjByteVector flags(2, char(0)); // just blank for the moment

	SjByteVector v = m_frameID + ID3v2_SynchDataFromUInt(m_frameSize) + flags;

	return v;
}

