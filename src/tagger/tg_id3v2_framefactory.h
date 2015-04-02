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




#ifndef TAGGER_ID3V2FRAMEFACTORY_H
#define TAGGER_ID3V2FRAMEFACTORY_H

#include "tg_id3v2_frame.h"


//! A factory for creating ID3v2 frames

/*!
 * This factory abstracts away the frame creation process and instantiates
 * the appropriate ID3v2::Frame subclasses based on the contents of the
 * data.
 *
 * Reimplementing this factory is the key to adding support for frame types
 * not directly supported by Tagger to your application.  To do so you would
 * subclass this factory reimplement createFrame().  Then by setting your
 * factory to be the default factory in ID3v2::Tag constructor or with
 * MPEG::File::setID3v2FrameFactory() you can implement behavior that will
 * allow for new ID3v2::Frame subclasses (also provided by you) to be used.
 *
 * This implements both <i>abstract factory</i> and <i>singleton</i> patterns
 * of which more information is available on the web and in software design
 * textbooks (Notably <i>Design Patters</i>).
 */
class ID3v2_FrameFactory
{
public:
	static ID3v2_FrameFactory *instance();
	static void exitLibrary();
	/*!
	 * Create a frame based on \a data.  \a synchSafeInts should only be set
	 * false if we are parsing an old tag (v2.3 or older) that does not support
	 * synchsafe ints.
	 *
	 * \deprecated Please use the method below that accepts an ID3 version
	 * number in new code.
	 */
	ID3v2_Frame *createFrame(const SjByteVector &data, bool synchSafeInts) const;

	/*!
	 * Create a frame based on \a data.  \a version should indicate the ID3v2
	 * version of the tag.  As ID3v2.4 is the most current version of the
	 * standard 4 is the default.
	 */
	// BIC: make virtual
	ID3v2_Frame *createFrame(const SjByteVector &data, SjUint version = 4) const;

	/*!
	 * Returns the default text encoding for text frames.  If setTextEncoding()
	 * has not been explicitly called this will only be used for new text
	 * frames.  However, if this value has been set explicitly all frames will be
	 * converted to this type (unless it's explitly set differently for the
	 * individual frame) when being rendered.
	 *
	 * \see setDefaultTextEncoding()
	 */
	SjStringType defaultTextEncoding() const { return m_defaultEncoding; }

	/*!
	 * Set the default text encoding for all text frames that are created to
	 * \a encoding.  If no value is set the frames with either default to the
	 * encoding type that was parsed and new frames default to Latin1.
	 *
	 * \see defaultTextEncoding()
	 */
	void setDefaultTextEncoding(SjStringType encoding);

protected:
	/*!
	 * Constructs a frame factory.  Because this is a singleton this method is
	 * protected, but may be used for subclasses.
	 */
	ID3v2_FrameFactory();

	/*!
	 * Destroys the frame factory.  In most cases this will never be called (as
	 * is typical of singletons).
	 */
	virtual ~ID3v2_FrameFactory() {}

	/*!
	 * This method checks for compliance to the current ID3v2 standard (2.4)
	 * and does nothing in the common case.  However if a frame is found that
	 * is not compatible with the current standard, this method either updates
	 * the frame or indicates that it should be discarded.
	 *
	 * This method with return true (with or without changes to the frame) if
	 * this frame should be kept or false if it should be discarded.
	 *
	 * See the id3v2.4.0-changes.txt document for further information.
	 */
	virtual bool updateFrame(ID3v2_FrameHeader *header) const;

private:
	ID3v2_FrameFactory(const ID3v2_FrameFactory &);
	ID3v2_FrameFactory &operator=(const ID3v2_FrameFactory &);

	/*!
	 * This method is used internally to convert a frame from ID \a from to ID
	 * \a to.  If the frame matches the \a from pattern and converts the frame
	 * ID in the \a header or simply does nothing if the frame ID does not match.
	 */
	static void convertFrame(const char *from, const char *to,
	                         ID3v2_FrameHeader *header);

	SjStringType  m_defaultEncoding;
	bool          m_useDefaultEncoding;
};


#endif
