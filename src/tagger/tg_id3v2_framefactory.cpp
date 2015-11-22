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
#include "tg_id3v2_framefactory.h"
#include "tg_id3v2_knownframes.h"



////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

static ID3v2_FrameFactory* s_factory = NULL;

ID3v2_FrameFactory *ID3v2_FrameFactory::instance()
{
	if( s_factory == NULL )
	{
		s_factory = new ID3v2_FrameFactory;
	}

	return s_factory;
}

void ID3v2_FrameFactory::exitLibrary()
{
	// if we're not in debug mode and we're in shutdown,
	// just let the OS free the memory, there is no advantage
	// to do it here (but some disadvantages,  eg. speed)

#ifdef __WXDEBUG__

	if( s_factory )
		delete s_factory;

#endif

	s_factory = NULL;
}


ID3v2_Frame *ID3v2_FrameFactory::createFrame(const SjByteVector &data, bool synchSafeInts) const
{
	return createFrame(data, (SjUint)(synchSafeInts ? 4 : 3));
}

ID3v2_Frame *ID3v2_FrameFactory::createFrame(const SjByteVector &data, SjUint version) const
{
	ID3v2_FrameHeader *header = new ID3v2_FrameHeader(data, version);
	SjByteVector frameID = header->frameID();

	// A quick sanity check -- make sure that the frameID is 4 uppercase Latin1
	// characters.  Also make sure that there is data in the frame.
	int frameIDchars = frameID.size();
	if( !(frameIDchars == (version < 3 ? 3 : 4)) || !header->IsOkay() ) {
		delete header;
		return 0;
	}

	/*for(ByteVector::ConstIterator it = frameID.begin(); it != frameID.end(); it++) {
	  if( (*it < 'A' || *it > 'Z') && (*it < '1' || *it > '9') ) {
	    delete header;
	    return 0;
	  }
	}*/
	for( int i = 0; i < frameIDchars; i++ )
	{
		unsigned char c = frameID[i];
		if( (c < 'A' || c > 'Z') && (c < '1' || c > '9') ) {
			delete header;
			return 0;
		}
	}

	// allow empty frames - although the id3 specification say, this is
	// not possible - each frame must have at least 1 byte data (11 bytes total),
	// there _are_ tags with empty frames :-(
	// modified by me
	//
	// note: the caller should check the frame size therefore!
	if( header->frameSize() <= 0 )
	{
		return new ID3v2_UnknownFrame(data, header);
	}

	// Tagger doesn't mess with encrypted frames, so just treat them
	// as unknown frames.

	/*
	if(header->compression()) {
	  wxLogDebug(wxT("Compressed frames are currently not supported."));
	  return new UnknownFrame(data, header);
	}
	*/

	if(header->encryption()) {
		wxLogDebug(wxT("Encrypted frames are currently not supported."));
		return new ID3v2_UnknownFrame(data, header);
	}

	if(!updateFrame(header)) {
		header->setTagAlterPreservation(true);
		return new ID3v2_UnknownFrame(data, header);
	}

	// updateFrame() might have updated the frame ID.

	frameID = header->frameID();

	// This is where things get necissarily nasty.  Here we determine which
	// Frame subclass (or if none is found simply an Frame) based
	// on the frame ID.  Since there are a lot of possibilities, that means
	// a lot of if blocks.

	// Text Identification (frames 4.2)

	if(frameID.startsWith((unsigned char*)"T")) {
		ID3v2_TextIdentificationFrame *f = frameID != "TXXX"
		                                   ? new ID3v2_TextIdentificationFrame(data, header)
		                                   : new ID3v2_UserTextIdentificationFrame(data, header);

		if(m_useDefaultEncoding)
			f->setTextEncoding(m_defaultEncoding);
		return f;
	}

	// Comments (frames 4.10)

	if(frameID == "COMM") {
		ID3v2_CommentsFrame *f = new ID3v2_CommentsFrame(data, header);
		if(m_useDefaultEncoding)
			f->setTextEncoding(m_defaultEncoding);
		return f;
	}

	// Attached Picture (frames 4.14)

	if(frameID == "APIC") {
		ID3v2_AttachedPictureFrame *f = new ID3v2_AttachedPictureFrame(data, header);
		if(m_useDefaultEncoding)
			f->setTextEncoding(m_defaultEncoding);
		return f;
	}

	// Popularimeter
	if( frameID == "POPM" )
	{
		ID3v2_PopularimeterFrame* f = new ID3v2_PopularimeterFrame(data);
		return f;
	}

	// Relative Volume Adjustment (frames 4.11) -- not implemented
	/*
	  if(frameID == "RVA2")
	    return new ID3v2_RelativeVolumeFrame(data, header);
	*/

	// Unique File Identifier (frames 4.1)
#ifdef TAGGER_USE_UNIQUE_FILE_IDENTIFIER_FRAME
	if(frameID == "UFID")
		return new ID3v2_UniqueFileIdentifierFrame(data, header);
#endif

	return new ID3v2_UnknownFrame(data, header);
}

void ID3v2_FrameFactory::setDefaultTextEncoding(SjStringType encoding)
{
	m_useDefaultEncoding = true;
	m_defaultEncoding = encoding;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

ID3v2_FrameFactory::ID3v2_FrameFactory()
{
	m_defaultEncoding = SJ_LATIN1;
	m_useDefaultEncoding = false;
}

bool ID3v2_FrameFactory::updateFrame(ID3v2_FrameHeader *header) const
{
	SjByteVector frameID = header->frameID();

	switch(header->version()) {

		case 2: // ID3v2.2
		{
			if(frameID == "CRM" ||
			        frameID == "EQU" ||
			        frameID == "LNK" ||
			        frameID == "RVA" ||
			        frameID == "TIM" ||
			        frameID == "TSI")
			{
				wxLogDebug(wxT("ID3v2.4 no longer supports the frame type ") + frameID.toString(SJ_LATIN1) +
				           wxT(".  It will be discarded from the tag."));
				return false;
			}

			// ID3v2.2 only used 3 bytes for the frame ID, so we need to convert all of
			// the frames to their 4 byte ID3v2.4 equivalent.

			convertFrame("BUF", "RBUF", header);
			convertFrame("CNT", "PCNT", header);
			convertFrame("COM", "COMM", header);
			convertFrame("CRA", "AENC", header);
			convertFrame("ETC", "ETCO", header);
			convertFrame("GEO", "GEOB", header);
			convertFrame("IPL", "TIPL", header);
			convertFrame("MCI", "MCDI", header);
			convertFrame("MLL", "MLLT", header);
			convertFrame("PIC", "APIC", header);
			convertFrame("POP", "POPM", header);
			convertFrame("REV", "RVRB", header);
			convertFrame("SLT", "SYLT", header);
			convertFrame("STC", "SYTC", header);
			convertFrame("TAL", "TALB", header);
			convertFrame("TBP", "TBPM", header);
			convertFrame("TCM", "TCOM", header);
			convertFrame("TCO", "TCON", header);
			convertFrame("TCR", "TCOP", header);
			convertFrame("TDA", "TDRC", header);
			convertFrame("TDY", "TDLY", header);
			convertFrame("TEN", "TENC", header);
			convertFrame("TFT", "TFLT", header);
			convertFrame("TKE", "TKEY", header);
			convertFrame("TLA", "TLAN", header);
			convertFrame("TLE", "TLEN", header);
			convertFrame("TMT", "TMED", header);
			convertFrame("TOA", "TOAL", header);
			convertFrame("TOF", "TOFN", header);
			convertFrame("TOL", "TOLY", header);
			convertFrame("TOR", "TDOR", header);
			convertFrame("TOT", "TOAL", header);
			convertFrame("TP1", "TPE1", header);
			convertFrame("TP2", "TPE2", header);
			convertFrame("TP3", "TPE3", header);
			convertFrame("TP4", "TPE4", header);
			convertFrame("TPA", "TPOS", header);
			convertFrame("TPB", "TPUB", header);
			convertFrame("TRC", "TSRC", header);
			convertFrame("TRD", "TDRC", header);
			convertFrame("TRK", "TRCK", header);
			convertFrame("TSS", "TSSE", header);
			convertFrame("TT1", "TIT1", header);
			convertFrame("TT2", "TIT2", header);
			convertFrame("TT3", "TIT3", header);
			convertFrame("TXT", "TOLY", header);
			convertFrame("TXX", "TXXX", header);
			convertFrame("TYE", "TDRC", header);
			convertFrame("UFI", "UFID", header);
			convertFrame("ULT", "USLT", header);
			convertFrame("WAF", "WOAF", header);
			convertFrame("WAR", "WOAR", header);
			convertFrame("WAS", "WOAS", header);
			convertFrame("WCM", "WCOM", header);
			convertFrame("WCP", "WCOP", header);
			convertFrame("WPB", "WPUB", header);
			convertFrame("WXX", "WXXX", header);

			break;
		}

		case 3: // ID3v2.3
		{
			if(frameID == "EQUA" ||
			        frameID == "RVAD" ||
			        frameID == "TIME" ||
			        frameID == "TRDA" ||
			        frameID == "TSIZ")
			{
				wxLogDebug(wxT("ID3v2.4 no longer supports the frame type ") + frameID.toString(SJ_LATIN1) +
				           wxT(".  It will be discarded from the tag."));
				return false;
			}

			// -- no, we should not do this: TDAT is the date in addition to the year (TYER) in
			// -- ID3v2.3; TDAT has the format MMDD
			//convertFrame("TDAT", "TDRC", header);

			convertFrame("TORY", "TDOR", header);
			convertFrame("TYER", "TDRC", header);

			break;
		}

		default:

			// This should catch a typo that existed in Tagger up to and including
			// version 1.1 where TRDC was used for the year rather than TDRC.

			convertFrame("TRDC", "TDRC", header);
			break;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void ID3v2_FrameFactory::convertFrame(const char *from, const char *to,
                                      ID3v2_FrameHeader *header)
{
	if(header->frameID() != from)
		return;

	// debug("ID3v2.4 no longer supports the frame type " + String(from) + "  It has" +
	//       "been converted to the type " + String(to) + ".");

	header->setFrameID((const unsigned char*)to);
}
