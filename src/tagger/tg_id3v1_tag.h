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



#ifndef TAGGER_ID3V1TAG_H
#define TAGGER_ID3V1TAG_H

#include "tg_tagger_base.h"




class ID3v1_StringHandler
{
public:
	// An abstraction for the string to data encoding in ID3v1 tags.
	//
	// ID3v1 should in theory always contain ISO-8859-1 (Latin1) data.  In
	// practice it does not.  Tagger by default only supports ISO-8859-1 data
	// in ID3v1 tags.
	// However by subclassing this class and reimplementing parse() and render()
	// and setting your reimplementation as the default with
	// ID3v1::Tag::setStringHandler() you can define how you would like these
	// transformations to be done.
	//
	// It is advisable not to write non-ISO-8859-1 data to ID3v1
	// tags.  Please consider disabling the writing of ID3v1 tags in the case
	// that the data is ISO-8859-1.

	// Decode a string from data.  The default implementation assumes that
	// data is an ISO-8859-1 (Latin1) character array.
	virtual wxString parse(const SjByteVector& data) const
	{
		return data.toString(SJ_LATIN1);
	}

	// Encode a ByteVector with the data from s.  The default implementation
	// assumes that s is an ISO-8859-1 (Latin1) string.
	virtual SjByteVector render(const wxString& s) const
	{
		SjByteVector ret;
		ret.appendString(s, SJ_LATIN1);
		return ret;
	}

	// missing virtual constructors may cause warnings otherwise ("deleting object of polymorphic class type X which has non-virtual destructor might cause undefined behaviour [-Wdelete-non-virtual-dtor]")
	virtual ~ID3v1_StringHandler()
	{
	}
};



class ID3v1_Tag : public Tagger_Tag
{
public:

	ID3v1_Tag           (Tagger_File *file = NULL, long tagOffset = 0); // Create an ID3v1 tag and parse the data in file starting at tagOffset.
	virtual             ~ID3v1_Tag          () {}
	static void         setStringHandler    (ID3v1_StringHandler* handler);

	// get the major version, this is the "x" in "ID3v1.x", either 0 or 1
	int                 majorVersion        () const { return m_majorVersion; }

	// Renders the in memory values to a ByteVector suitable for writing to
	// the file. Note: Most fields are truncated to a maximum of 28-30 bytes.
	// The truncation happens automatically when the tag is rendered.
	SjByteVector        render              () const;

	// Returns the string "TAG" suitable for usage in locating the tag in a
	// file.
	static SjByteVector fileIdentifier      ();

	// Genre Handling: Map from the canonical ID3v1 genre names to the
	// respective genre number and the other way round.
	// Unknown genres are returned as empty string or as the number 255.
	static SjSLHash*    getGenreMap         ();
	static wxString     lookupGenreName     (int index);
	static int          lookupGenreIndex    (const wxString &name);

	// Reimplementations
	virtual wxString    title               () const { return m_title; }
	virtual wxString    leadArtist          () const { return m_leadArtist; }
	virtual wxString    orgArtist           () const { return wxEmptyString; }
	virtual wxString    composer            () const { return wxEmptyString; }
	virtual wxString    album               () const { return m_album; }
	virtual wxString    comment             () const { return m_comment; }
	virtual wxString    genre               () const;
	virtual wxString    group               () const { return wxEmptyString; }
	virtual long        year                () const;
	virtual long        beatsPerMinute      () const { return 0; }
	virtual void        track               (long& nr, long& count) const { nr = m_track; count = 0; }
	virtual void        disk                (long& nr, long& count) const { nr = 0; count = 0; }
	virtual void        setTitle            (const wxString &s) { m_title = s; }
	virtual void        setLeadArtist       (const wxString &s) { m_leadArtist = s; }
	virtual void        setOrgArtist        (const wxString &s) {}
	virtual void        setComposer         (const wxString &s) {}
	virtual void        setAlbum            (const wxString &s) { m_album = s; }
	virtual void        setComment          (const wxString &s) { m_comment = s; }
	virtual void        setGenre            (const wxString &s);
	virtual void        setGroup            (const wxString &s) {}
	virtual void        setYear             (long i);
	virtual void        setBeatsPerMinute   (long i) {}
	virtual void        setTrack            (long nr, long count);
	virtual void        setDisk             (long nr, long count) {}

protected:
	void                read                ();
	void                parse               (const SjByteVector &data);

private:
	Tagger_File*        m_file;
	long                m_tagOffset;

	int                 m_majorVersion;

	wxString            m_title;
	wxString            m_leadArtist;
	wxString            m_album;
	wxString            m_year;
	wxString            m_comment;
	unsigned char       m_track;
	unsigned char       m_genre;
};



#endif
