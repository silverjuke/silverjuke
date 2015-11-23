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



#ifndef __SJ_FLAC_FILE_H__
#define __SJ_FLAC_FILE_H__



#include "tg_tagger_base.h"



class Tagger_Tag;
class ID3v2_FrameFactory;
class ID3v2_Tag;
class ID3v1_Tag;
class Ogg_XiphComment;
class Combined_Tag;



class FLAC_Properties : public Tagger_AudioProperties
{
public:
	FLAC_Properties     (const SjByteVector& data, long streamLength);

	// Reimplementations.
	virtual int     length              () const {return m_length;}
	virtual int     bitrate             () const {return m_bitrate;}
	virtual int     sampleRate          () const {return m_sampleRate;}
	virtual int     channels            () const {return m_channels;}

	// Returns the sample width as read from the FLAC identification header.
	int             sampleWidth         () const {return m_sampleWidth;}

private:
	long            m_streamLength;
	int             m_length;
	int             m_bitrate;
	int             m_sampleRate;
	int             m_sampleWidth;
	int             m_channels;
};



class FLAC_File : public Tagger_File
{
public:
	                    FLAC_File           (const wxString& url, wxInputStream*, bool readProperties = true, ID3v2_FrameFactory* frameFactory = NULL);
	virtual             ~FLAC_File          ();
	void                setID3v2FrameFactory(const ID3v2_FrameFactory *factory) {m_ID3v2FrameFactory = factory;}

	// Reimplemenations
	virtual Tagger_Tag* tag                 () const {return (Tagger_Tag*)m_tag;} // This will be a union of XiphComment, ID3v1 and ID3v2 tags.
	virtual Tagger_AudioProperties*
	audioProperties     () const {return m_properties;} // If no audio properties were read then this will return a null pointer.

	// Reimplemenation: This will save the XiphComment and
	// keep any old ID3-tags up to date. If the file
	// has no XiphComment, one will be constructed from the ID3-tags.
	virtual bool        save                ();

	// xiphComment, ID3v2Tag() and ID3v1Tag() returns a pointer to the different tag of the file.
	// If create is false this will return a null pointer there is no valid tag of the requested type.
	// The Tag is always owned by the FLAC_File and should not be
	Ogg_XiphComment*    xiphComment         (bool create = false);
	ID3v2_Tag*          ID3v2Tag            (bool create = false);
	ID3v1_Tag*          ID3v1Tag            (bool create = false);

	// Returns the block of data used by FLAC_Properties for parsing the stream properties;
	// used by FLAC_Properties
	SjByteVector        streamInfoData      ();

	// Returns the length of the audio-stream, used by FLAC_Properties for
	// calculating the bitrate.
	long                streamLength        () {return m_streamLength;}

private:
	void                read                (bool readProperties);
	void                scan                ();
	long                findID3v2           ();
	long                findID3v1           ();
	SjByteVector        xiphCommentData     ();
	const ID3v2_FrameFactory*
	m_ID3v2FrameFactory;
	ID3v2_Tag*          m_ID3v2Tag;
	long                m_ID3v2Location;
	SjUint              m_ID3v2OriginalSize;

	ID3v1_Tag*          m_ID3v1Tag;
	long                m_ID3v1Location;

	Ogg_XiphComment*    m_comment;

	Combined_Tag*       m_tag;

	FLAC_Properties*    m_properties;
	SjByteVector        m_streamInfoData;
	SjByteVector        m_xiphCommentData;
	long                m_flacStart;
	long                m_streamStart;
	long                m_streamLength;
	bool                m_scanned;

	bool                m_hasXiphComment;
	bool                m_hasID3v2;
	bool                m_hasID3v1;
};



#endif // __SJ_FLAC_FILE_H__
