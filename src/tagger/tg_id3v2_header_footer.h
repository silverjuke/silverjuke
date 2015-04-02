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



#ifndef TAGGER_ID3V2HEADER_H
#define TAGGER_ID3V2HEADER_H

#include "tg_tagger_base.h"


//! An implementation of ID3v2 headers

/*!
 * This class implements ID3v2 headers.  It attempts to follow, both
 * semantically and programatically, the structure specified in
 * the ID3v2 standard.  The API is based on the properties of ID3v2 headers
 * specified there.  If any of the terms used in this documentation are
 * unclear please check the specification in the linked section.
 * (Structure, <a href="id3v2-structure.html#3.1">3.1</a>)
 */

class ID3v2_Header
{
public:

	/*!
	 * Constructs an ID3v2 header based on \a data.  parse() is called
	 * immediately.
	 */
	ID3v2_Header(const SjByteVector* data = NULL);

	/*!
	 * Destroys the header.
	 */
	virtual ~ID3v2_Header() {}

	/*!
	 * Returns the major version number.  (Note: This is the 4, not the 2 in
	 * ID3v2.4.0.  The 2 is implied.)
	 */
	SjUint majorVersion() const { return m_majorVersion; }

	/*!
	 * Returns the revision number.  (Note: This is the 0, not the 4 in
	 * ID3v2.4.0.  The 2 is implied.)
	 */
	SjUint revisionNumber() const { return m_revisionNumber; }

	/*!
	 * Returns true if unsynchronisation has been applied to all frames.
	 */
	bool unsynchronisation() const { return m_unsynchronisation; }

	/*!
	 * Returns true if an extended header is present in the tag.
	 */
	bool extendedHeader() const { return m_extendedHeader; }

	/*!
	 * Returns true if the experimental indicator flag is set.
	 */
	bool experimentalIndicator() const { return m_experimentalIndicator; }

	/*!
	 * Returns true if a footer is present in the tag.
	 */
	bool footerPresent() const { return m_footerPresent; }
	/*!
	 * Returns the tag size in bytes.  This is the size of the frame content.
	 * The size of the \e entire tag will be this plus the header size (10
	 * bytes) and, if present, the footer size (potentially another 10 bytes).
	 *
	 * \note This is the value as read from the header to which Tagger attempts
	 * to provide an API to; it was not a design decision on the part of Tagger
	 * to not include the mentioned portions of the tag in the \e size.
	 *
	 * \see completeTagSize()
	 */
	SjUint tagSize() const { return m_tagSize; }

	/*!
	 * Returns the tag size, including the header and, if present, the footer
	 * size.
	 *
	 * \see tagSize()
	 */
	SjUint completeTagSize() const;

	/*!
	 * Set the tag size to \a s.
	 * \see tagSize()
	 */
	void setTagSize(SjUint s) { m_tagSize = s; }

	/*!
	 * Returns the size of the header.  Presently this is always 10 bytes.
	 */
	static SjUint size();

	/*!
	 * Returns the string used to identify and ID3v2 tag inside of a file.
	 * Presently this is always "ID3".
	 */
	static SjByteVector fileIdentifier();

	/*!
	 * Sets the data that will be used as the extended header.  10 bytes,
	 * starting from \a data will be used.
	 */
	void setData(const SjByteVector &data) { parse(data); }

	/*!
	 * Renders the Header back to binary format.
	 */
	SjByteVector render() const;

protected:
	/*!
	 * Called by setData() to parse the header data.  It makes this information
	 * available through the public API.
	 */
	void parse(const SjByteVector &data);

private:
#ifdef __WXDEBUG__
	ID3v2_Header &operator=(const ID3v2_Header &) { wxLogWarning(wxT("do not copy ID3v2 headers this way!")); return *this; }
#endif

	SjUint m_majorVersion;
	SjUint m_revisionNumber;

	bool m_unsynchronisation;
	bool m_extendedHeader;
	bool m_experimentalIndicator;
	bool m_footerPresent;

	SjUint m_tagSize;


};






//! ID3v2 extended header implementation

/*!
 * This class implements ID3v2 extended headers.  It attempts to follow,
 * both  semantically and programatically, the structure specified in
 * the ID3v2 standard.  The API is based on the properties of ID3v2 extended
 * headers specified there.  If any of the terms used in this documentation
 * are unclear please check the specification in the linked section.
 * (Structure, <a href="id3v2-structure.html#3.2">3.2</a>)
 */


class ID3v2_ExtendedHeader
{
public:
	/*!
	 * Constructs an empty ID3v2 extended header.
	 */
	ID3v2_ExtendedHeader();

	/*!
	 * Destroys the extended header.
	 */
	virtual ~ID3v2_ExtendedHeader() {}

	/*!
	 * Returns the size of the extended header.  This is variable for the
	 * extended header.
	 */
	SjUint size() const { return m_size; }

	/*!
	 * Sets the data that will be used as the extended header.  Since the
	 * length is not known before the extended header has been parsed, this
	 * should just be a pointer to the first byte of the extended header.  It
	 * will determine the length internally and make that available through
	 * size().
	 */
	void setData(const SjByteVector &data) { parse(data); }

protected:
	/*!
	 * Called by setData() to parse the extended header data.  It makes this
	 * information available through the public API.
	 */
	void parse(const SjByteVector &data);

private:
#ifdef __WXDEBUG__
	ID3v2_ExtendedHeader &operator=(const ID3v2_ExtendedHeader &) { wxLogWarning(wxT("do not copy ID3v2 ext. headers this way!")); return *this; }
#endif

	SjUint m_size;
};




//! ID3v2 footer implementation

/*!
 * Per the ID3v2 specification, the tag's footer is just a copy of the
 * information in the header.  As such there is no API for reading the
 * data from the header, it can just as easily be done from the header.
 *
 * In fact, at this point, Tagger does not even parse the footer since
 * it is not useful internally.  However, if the flag to include a footer
 * has been set in the ID3v2::Tag, Tagger will render a footer.
 */
class ID3v2_Footer
{
public:
	/*!
	 * Constructs an empty ID3v2 footer.
	 */
	ID3v2_Footer() {}
	/*!
	 * Destroys the footer.
	 */
	virtual ~ID3v2_Footer() {}

	/*!
	 * Returns the size of the footer.  Presently this is always 10 bytes.
	 */
	static const unsigned int size();

	/*!
	 * Renders the footer based on the data in \a header.
	 */
	SjByteVector render(const ID3v2_Header *header) const;

private:
	ID3v2_Footer(const ID3v2_Footer &);
	ID3v2_Footer &operator=(const ID3v2_Footer &);
};




//! A few functions for ID3v2 synch safe integer conversion

/*!
 * In the ID3v2.4 standard most integer values are encoded as "synch safe"
 * integers which are encoded in such a way that they will not give false
 * MPEG syncs and confuse MPEG decoders.  This namespace provides some
 * methods for converting to and from these values to ByteVectors for
 * things rendering and parsing ID3v2 data.
 */

/*!
 * This returns the unsigned integer value of \a data where \a data is a
 * ByteVector that contains a \e synchsafe integer (Structure,
 * <a href="id3v2-structure.html#6.2">6.2</a>).  The default \a length of
 * 4 is used if another value is not specified.
 */
SjUint ID3v2_SynchDataToUInt(const SjByteVector &data);

/*!
 * Returns a 4 byte (32 bit) synchsafe integer based on \a value.
 */
SjByteVector ID3v2_SynchDataFromUInt(SjUint value);


#endif
