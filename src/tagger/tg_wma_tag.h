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




#ifndef TAGGER_WMATAG_H
#define TAGGER_WMATAG_H

#include "tg_tagger_base.h"
#include "tg_wma_file.h"


/*!
 * This implements the generic Tagger::Tag API
 */
class WMA_Tag : public Tagger_Tag
{
public:
	WMA_Tag();

	/*!
	 * Detroys this WMA_Tag instance.
	 */
	virtual ~WMA_Tag();

	/*!
	 * Returns the track name; if no track name is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString title() const { return m_title; }

	/*!
	 * Returns the artist name; if no artist name is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString leadArtist() const { return m_leadArtist; }
	virtual wxString orgArtist() const { return wxEmptyString; }
	virtual wxString composer() const { return wxEmptyString; }

	/*!
	 * Returns the album name; if no album name is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString album() const { return m_album; }

	/*!
	 * Returns the track comment; if no comment is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString comment() const { return m_comment; }

	/*!
	 * Returns the genre name; if no genre is present in the tag String::null
	 * will be returned.
	 */
	virtual wxString genre() const { return m_genre; }
	virtual wxString group() const { return wxEmptyString; }

	/*!
	 * Returns the year; if there is no year set, this will return 0.
	 */
	virtual long year() const { return m_year; }
	virtual long beatsPerMinute() const { return 0; }
	virtual long rating() const { return 0; }

	/*!
	 * Returns the track number; if there is no track number set, this will
	 * return 0.
	 */
	virtual void track(long& nr, long& count) const { nr = m_trackNr; count = 0; }
	virtual void disk(long& nr, long& count) const { nr = 0; count = 0; }

	/*!
	 * Sets the title to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setTitle(const wxString &s) { m_title = s; }

	/*!
	 * Sets the artist to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setLeadArtist(const wxString &s) { m_leadArtist = s; }
	virtual void setOrgArtist(const wxString &s) { }
	virtual void setComposer(const wxString &s) { }

	/*!
	 * Sets the album to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setAlbum(const wxString &s) { m_album = s; }

	/*!
	 * Sets the album to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setComment(const wxString &s) { m_comment = s; }

	/*!
	 * Sets the genre to \a s.  If \a s is String::null then this value will be
	 * cleared.  For tag formats that use a fixed set of genres, the appropriate
	 * value will be selected based on a string comparison.  A list of available
	 * genres for those formats should be available in that type's
	 * implementation.
	 */
	virtual void setGenre(const wxString &s) { m_genre = s; }
	virtual void setGroup(const wxString &s) { }

	/*!
	 * Sets the year to \a i.  If \a s is 0 then this value will be cleared.
	 */
	virtual void setYear(long i) { m_year = i; }
	virtual void setBeatsPerMinute(long i) { }
	virtual void setRating(long i) { }

	/*!
	 * Sets the track to \a i.  If \a s is 0 then this value will be cleared.
	 */
	virtual void setTrack(long nr, long count) { m_trackNr = nr; }
	virtual void setDisk(long nr, long count) { }

	/*!
	 * Returns true if the tag does not contain any data.  This should be
	 * reimplemented in subclasses that provide more than the basic tagging
	 * abilities in this class.
	 */
	virtual bool isEmpty() const;

	/*!
	 * Copies the generic data from one tag to another.
	 *
	 * \note This will no affect any of the lower level details of the tag.  For
	 * instance if any of the tag type specific data (maybe a URL for a band) is
	 * set, this will not modify or copy that.  This just copies using the API
	 * in this class.
	 *
	 * If \a overwrite is true then the values will be unconditionally copied.
	 * If false only empty values will be overwritten.
	 */
	//static void duplicate(const Tagger_Tag *source, Tagger_Tag *target, bool overwrite = true);

protected:
	wxString m_title;
	wxString m_leadArtist;
	wxString m_album;
	wxString m_comment;
	wxString m_genre;
	long m_year;
	long m_trackNr;

	// The WMA::File class does the reading of the tag itself

};

#endif
