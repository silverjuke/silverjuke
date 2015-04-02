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




#ifndef TAGGER_VORBISCOMMENT_H
#define TAGGER_VORBISCOMMENT_H

#include "tg_tagger_base.h"




/*!
 * A mapping between a list of field names, or keys, and a list of values
 * associated with that field.
 *
 * \see XiphComment::fieldListMap()
 */
/*
typedef Map<String, StringList> FieldListMap;
*/


//! Ogg Vorbis comment implementation

/*!
 * This class is an implementation of the Ogg Vorbis comment specification,
 * to be found in section 5 of the Ogg Vorbis specification.  Because this
 * format is also used in other (currently unsupported) Xiph.org formats, it
 * has been made part of a generic implementation rather than being limited
 * to strictly Vorbis.
 *
 * Vorbis comments are a simple vector of keys and values, called fields.
 * Multiple values for a given key are supported.
 *
 * \see fieldListMap()
 */
class Ogg_XiphCommentPrivate;
class Ogg_XiphComment : public Tagger_Tag
{
public:
	/*!
	 * Constructs an empty Vorbis comment.
	 */
	Ogg_XiphComment();

	/*!
	 * Constructs a Vorbis comment from \a data.
	 */
	Ogg_XiphComment(const SjByteVector &data);

	/*!
	 * Destroys this instance of the XiphComment.
	 */
	virtual ~Ogg_XiphComment();

	virtual wxString title() const { return m_fieldListMapFirstString(wxT("TITLE")); }
	virtual wxString leadArtist() const { return m_fieldListMapFirstString(wxT("ARTIST")); }
	virtual wxString orgArtist() const { return m_fieldListMapFirstString(wxT("ORIGARTIST")); }
	virtual wxString composer() const { return m_fieldListMapFirstString(wxT("COMPOSER")); }
	virtual wxString album() const { return m_fieldListMapFirstString(wxT("ALBUM")); }
	virtual wxString comment() const;
	virtual wxString genre() const { return m_fieldListMapFirstString(wxT("GENRE")); }
	virtual wxString group() const { return m_fieldListMapFirstString(wxT("CONTENTGROUP")); }
	virtual long year() const;
	virtual long beatsPerMinute() const;
	virtual void track(long& nr, long& count) const;
	virtual void disk(long& nr, long& count) const;

	virtual void setTitle(const wxString &s);
	virtual void setLeadArtist(const wxString &s);
	virtual void setOrgArtist(const wxString &s);
	virtual void setComposer(const wxString &s);
	virtual void setAlbum(const wxString &s);
	virtual void setComment(const wxString &s);
	virtual void setGenre(const wxString &s);
	virtual void setGroup(const wxString &s);
	virtual void setYear(long i);
	virtual void setBeatsPerMinute(long i);
	virtual void setTrack(long nr, long count);
	virtual void setDisk(long nr, long count);

	virtual bool isEmpty() const;

	/*!
	 * Returns the number of fields present in the comment.
	 */
	SjUint fieldCount() const;

	/*!
	 * Returns a reference to the map of field lists.  Because Xiph comments
	 * support multiple fields with the same key, a pure Map would not work.
	 * As such this is a Map of string lists, keyed on the comment field name.
	 *
	 * The standard set of Xiph/Vorbis fields (which may or may not be
	 * contained in any specific comment) is:
	 *
	 * <ul>
	 *   <li>TITLE</li>
	 *   <li>VERSION</li>
	 *   <li>ALBUM</li>
	 *   <li>ARTIST</li>
	 *   <li>PERFORMER</li>
	 *   <li>COPYRIGHT</li>
	 *   <li>ORGANIZATION</li>
	 *   <li>DESCRIPTION</li>
	 *   <li>GENRE</li>
	 *   <li>DATE</li>
	 *   <li>LOCATION</li>
	 *   <li>CONTACT</li>
	 *   <li>ISRC</li>
	 * </ul>
	 *
	 * For a more detailed description of these fields, please see the Ogg
	 * Vorbis specification, section 5.2.2.1.
	 *
	 * \note The Ogg Vorbis comment specification does allow these key values
	 * to be either upper or lower case.  However, it is conventional for them
	 * to be upper case.  As such, Tagger, when parsing a Xiph/Vorbis comment,
	 * converts all fields to uppercase.  When you are using this data
	 * structure, you will need to specify the field name in upper case.
	 *
	 * \warning You should not modify this data structure directly, instead
	 * use addField() and removeField().
	 */
	const SjSPHash* fieldListMap() const { return &(m_fieldListMap); }

	/*!
	 * Returns the vendor ID of the Ogg Vorbis encoder.  libvorbis 1.0 as the
	 * most common case always returns "Xiph.Org libVorbis I 20020717".
	 */
	wxString vendorID() const { return m_vendorID; }

	/*!
	 * Add the field specified by \a key with the data \a value.  If \a replace
	 * is true, then all of the other fields with the same key will be removed
	 * first.
	 *
	 * If the field value is empty, the field will be removed.
	 */
	void addField(const wxString &key, const wxString &value, bool replace = true);

	/*!
	 * Remove the field specified by \a key with the data \a value.  If
	 * \a value is null, all of the fields with the given key will be removed.
	 */
	void removeField(const wxString &key, const wxString &value = wxT(""));

	/*!
	 * Renders the comment to a ByteVector suitable for inserting into a file.
	 */
	SjByteVector render() const; // BIC: remove and merge with below

	/*!
	 * Renders the comment to a ByteVector suitable for inserting into a file.
	 *
	 * If \a addFramingBit is true the standard Vorbis comment framing bit will
	 * be appended.  However some formats (notably FLAC) do not work with this
	 * in place.
	 */
	SjByteVector render(bool addFramingBit) const;

protected:
	/*!
	 * Reads the tag from the file specified in the constructor and fills the
	 * FieldListMap.
	 */
	void parse(const SjByteVector &data);

private:
	Ogg_XiphComment(const Ogg_XiphComment &);
	Ogg_XiphComment &operator=(const Ogg_XiphComment &);


	SjSPHash m_fieldListMap; // contains elements of type wxArrayString
	wxArrayString* m_fieldListMapArray(const wxString& key) const { return (wxArrayString*)m_fieldListMap.Lookup(key); }
	wxString m_fieldListMapFirstString(const wxString& key) const;

	wxString m_vendorID;
	mutable wxString m_commentField;
};


#endif
