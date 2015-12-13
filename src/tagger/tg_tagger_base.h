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





#ifndef __TAGGER_BASE_H__
#define __TAGGER_BASE_H__


#include <sjbase/base.h> // just to include the most recent wxWidgets files
#include "tg_bytefile.h"



class Tagger_Tag;
class Tagger_AudioProperties;
class Tagger_FilePrivate;


class Tagger_Options
{
	public:
		#define   SJTF_READ_RATINGS          0x0001
		#define   SJTF_WRITE_RATINGS         0x0002
		#define   SJTF_READ_OTHERS_RATINGS   0x0004
		#define   SJTF_CHANGE_OTHERS_RATINGS 0x0008
		#define   SJTF_REMOVE_OTHERS_RATINGS 0x0010
		#define   SJTF_ADD_LEADING_ZEROS     0x0020
		#define   SJTF_DEFAULTS              0x000F // by default, we read ratings from any frame and do not change the user if not necessary
		int       m_flags;
		wxString  m_ratingUser; // the user to write ratings for
};

extern Tagger_Options* g_taggerOptions;


/*!
 * Read Style
 */
#define Tagger_ReadTags                 0x0001
#define Tagger_ReadAudioProperties      0x0002
#define Tagger_ReadAccurate             0x0004 //! Read as much of the file as needed to report accurate values




//! A file class with some useful methods for tag manipulation
/*!
 * This class is a basic file class with some methods that are particularly
 * useful for tag editors.  It has methods to take advantage of
 * ByteVector and a binary search method for finding patterns in a file.
 */
class Tagger_File : public SjByteFile
{
public: // to the user ...

	/*!
	 * Destroys this File instance.
	 */
	virtual ~Tagger_File() {}

	/*!
	 * Returns a pointer to this file's tag.  This should be reimplemented in
	 * the concrete subclasses.
	 */
	virtual Tagger_Tag *tag() const = 0;

	/*!
	 * Returns a pointer to this file's audio properties.  This should be
	 * reimplemented in the concrete subclasses.  If no audio properties were
	 * read then this will return a null pointer.
	 */
	virtual Tagger_AudioProperties *audioProperties() const = 0;

	/*!
	 * Save the file and its associated tags.  This should be reimplemented in
	 * the concrete subclasses.  Returns true if the save succeeds.
	 */
	virtual bool save() = 0;

	/* !
	 * Exit the library.
	 */
	static void exitLibrary();


protected:
	/*!
	 * Construct a File object and opens the file.
	 *
	 * \note Constructor is protected since this class should only be
	 * instantiated through subclasses.
	 */
	Tagger_File(const wxString& url, wxInputStream*);

private:
#ifdef __WXDEBUG__
	Tagger_File &operator=(const Tagger_File &) { wxLogWarning(wxT("do not copy Tagger_File objects this way!")); return *this; }
#endif
};





//! A simple, generic interface to common audio meta data fields
/*!
 * This is an attempt to abstract away the difference in the meta data formats
 * of various audio codecs and tagging schemes.  As such it is generally a
 * subset of what is available in the specific formats but should be suitable
 * for most applications.  This is meant to complient the generic APIs found
 * in Tagger_AudioProperties, Tagger_File and Tagger_FileRef.
 */
class Tagger_Tag
{
public:

	/*!
	 * Detroys this Tagger_Tag instance.
	 */
	virtual ~Tagger_Tag() {}

	/*!
	 * Returns the track name; if no track name is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString title() const = 0;

	/*!
	 * Returns the artist name; if no artist name is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString leadArtist() const = 0;
	virtual wxString orgArtist() const = 0;
	virtual wxString composer() const = 0;

	/*!
	 * Returns the album name; if no album name is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString album() const = 0;

	/*!
	 * Returns the track comment; if no comment is present in the tag
	 * String::null will be returned.
	 */
	virtual wxString comment() const = 0;

	/*!
	 * Returns the genre name; if no genre is present in the tag String::null
	 * will be returned.
	 */
	virtual wxString genre() const = 0;
	virtual wxString group() const = 0;

	/*!
	 * Returns the year/bpm/rating; if not year set, this will return 0.
	 */
	virtual long year() const = 0;
	virtual long beatsPerMinute() const = 0;
	virtual long rating() const = 0;

	/*!
	 * Returns the track number; if there is no track number set, this will
	 * return 0.
	 */
	virtual void track(long& nr, long& count) const = 0;
	virtual void disk(long& nr, long& count) const = 0;

	/*!
	 * Sets the title to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setTitle(const wxString &s) = 0;

	/*!
	 * Sets the artist to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setLeadArtist(const wxString &s) = 0;
	virtual void setOrgArtist(const wxString &s) = 0;
	virtual void setComposer(const wxString &s) = 0;

	/*!
	 * Sets the album to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setAlbum(const wxString &s) = 0;

	/*!
	 * Sets the album to \a s.  If \a s is String::null then this value will be
	 * cleared.
	 */
	virtual void setComment(const wxString &s) = 0;

	/*!
	 * Sets the genre to \a s.  If \a s is String::null then this value will be
	 * cleared.  For tag formats that use a fixed set of genres, the appropriate
	 * value will be selected based on a string comparison.  A list of available
	 * genres for those formats should be available in that type's
	 * implementation.
	 */
	virtual void setGenre(const wxString &s) = 0;
	virtual void setGroup(const wxString &s) = 0;

	/*!
	 * Sets the year/bpm/rating to \a i.  If \a s is 0 then this value will be cleared.
	 */
	virtual void setYear(long i) = 0;
	virtual void setBeatsPerMinute(long i) = 0;
	virtual void setRating(long i) = 0;

	/*!
	 * Sets the track to \a i.  If \a s is 0 then this value will be cleared.
	 */
	virtual void setTrack(long nr, long count) = 0;
	virtual void setDisk(long nr, long count) = 0;

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
	static void duplicate(const Tagger_Tag *source, Tagger_Tag *target, bool overwrite = true);

protected:
	/*!
	 * Construct a Tagger_Tag.  This is protected since tags should only be instantiated
	 * through subclasses.
	 */
	Tagger_Tag() {}

	/*!
	 * Interprets a string like "2/10" as "2 of 10" and returns the given numbers.
	 * If the character "/" is not present in the string, only the number is returned and
	 * the count is set to null.
	 */
	static void explodeNrAndCount(const wxString&, long& nr, long& count);
	static wxString implodeNrAndCount(long nr, long count);

private:
#ifdef __WXDEBUG__
	Tagger_Tag &operator=(const Tagger_Tag &) { wxLogWarning(wxT("do not copy Tagger_Tag objects this way!")); return *this; }
#endif
};



//! A simple, abstract interface to common audio properties
/*!
 * The values here are common to most audio formats.  For more specific, codec
 * dependant values, please see see the subclasses APIs.  This is meant to
 * compliment the Tagger_File and Tagger_Tag APIs in providing a simple
 * interface that is sufficient for most applications.
 */
class Tagger_AudioProperties
{
public:

	/*!
	 * Destroys this Tagger_AudioProperties instance.
	 */
	virtual ~Tagger_AudioProperties() {}

	/*!
	 * Returns the lenght of the file in seconds.
	 */
	virtual int length() const = 0;

	/*!
	 * Returns the most appropriate bit rate for the file in kb/s.  For constant
	 * bitrate formats this is simply the bitrate of the file.  For variable
	 * bitrate formats this is either the average or nominal bitrate.
	 */
	virtual int bitrate() const = 0;

	/*!
	 * Returns the sample rate in Hz.
	 */
	virtual int sampleRate() const = 0;

	/*!
	 * Returns the number of audio channels.
	 */
	virtual int channels() const = 0;

protected:

	/*!
	 * Construct an audio properties instance.  This is protected as this class
	 * should not be instantiated directly, but should be instantiated via its
	 * subclasses and can be fetched from the FileRef or File APIs.
	 */
	Tagger_AudioProperties() {}

private:
#ifdef __WXDEBUG__
	Tagger_AudioProperties &operator=(const Tagger_AudioProperties &) { wxLogWarning(wxT("do not copy AudioProperties objects this way!")); return *this; }
#endif
};









#endif // __TAGGER_BASE_H__

