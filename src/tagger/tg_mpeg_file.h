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




#ifndef TAGGER_MPEGFILE_H
#define TAGGER_MPEGFILE_H

#include "tg_tagger_base.h"
#include "tg_mpeg_properties.h"


class ID3v2_Tag;
class ID3v1_Tag;
class APE_Tag;
class MPEG_CombinedTag;

//! An implementation of Tagger_File with MPEG (MP3) specific methods

/*!
* This set of flags is used for various operations and is suitable for
* being OR-ed together.
*/
enum MPEG_TagTypes
{
    //! Empty set.  Matches no tag types.
    MPEG_NoTags  = 0x0000,
    //! Matches ID3v1 tags.
    MPEG_ID3v1   = 0x0001,
    //! Matches ID3v2 tags.
    MPEG_ID3v2   = 0x0002,
    //! Matches APE tags.
    MPEG_APE     = 0x0004,
    //! Matches all tag types.
    MPEG_AllTags = 0xffff
};


//! An MPEG file class with some useful methods specific to MPEG

/*!
 * This implements the generic Tagger_File API and additionally provides
 * access to properties that are distinct to MPEG files, notably access
 * to the different ID3 tags.
 */
class MPEG_File : public Tagger_File
{
public:
	/*!
	 * Contructs an MPEG file from \a file.  If \a readProperties is true the
	 * file's audio properties will also be read using \a propertiesStyle.  If
	 * false, \a propertiesStyle is ignored.
	 */
	MPEG_File(const wxString& url, wxInputStream*, int readStyle = Tagger_ReadTags|Tagger_ReadAudioProperties);

	/*!
	 * Destroys this instance of the File.
	 */
	virtual ~MPEG_File();

	/*!
	 * Returns a pointer to a tag that is the union of the ID3v2 and ID3v1
	 * tags. The ID3v2 tag is given priority in reading the information -- if
	 * requested information exists in both the ID3v2 tag and the ID3v1 tag,
	 * the information from the ID3v2 tag will be returned.
	 *
	 * If you would like more granular control over the content of the tags,
	 * with the concession of generality, use the tag-type specific calls.
	 *
	 * \note As this tag is not implemented as an ID3v2 tag or an ID3v1 tag,
	 * but a union of the two this pointer may not be cast to the specific
	 * tag types.
	 *
	 * \see ID3v1Tag()
	 * \see ID3v2Tag()
	 * \see APETag()
	 */
	virtual Tagger_Tag *tag() const;

	/*!
	 * Returns the MPEG::Properties for this file.  If no audio properties
	 * were read then this will return a null pointer.
	 */
	virtual Tagger_AudioProperties *audioProperties() const { return m_properties; }

	/*!
	 * Save the file.  If at least one tag -- ID3v1 or ID3v2 -- exists this
	 * will duplicate its content into the other tag.  This returns true
	 * if saving was successful.
	 *
	 * If neither exists or if both tags are empty, this will strip the tags
	 * from the file.
	 *
	 * This is the same as calling save(AllTags);
	 *
	 * If you would like more granular control over the content of the tags,
	 * with the concession of generality, use paramaterized save call below.
	 *
	 * \see save(int tags)
	 */
	virtual bool save() { return save(MPEG_AllTags); }

	/*!
	 * Save the file.  This will attempt to save all of the tag types that are
	 * specified by OR-ing together TagTypes values.  The save() method above
	 * uses AllTags.  This returns true if saving was successful.
	 *
	 * This strips all tags not included in the mask, but does not modify them
	 * in memory, so later calls to save() which make use of these tags will
	 * remain valid.  This also strips empty tags.
	 */
	bool save(int tags) { return save(tags, true); }

	/*!
	 * Save the file.  This will attempt to save all of the tag types that are
	 * specified by OR-ing together TagTypes values.  The save() method above
	 * uses AllTags.  This returns true if saving was successful.
	 *
	 * If \a stripOthers is true this strips all tags not included in the mask,
	 * but does not modify them in memory, so later calls to save() which make
	 * use of these tags will remain valid.  This also strips empty tags.
	 */
	// BIC: combine with the above method
	bool save(int tags, bool stripOthers);

	/*!
	 * Returns a pointer to the ID3v2 tag of the file.
	 *
	 * If \a create is false (the default) this will return a null pointer
	 * if there is no valid ID3v2 tag.  If \a create is true it will create
	 * an ID3v2 tag if one does not exist.
	 *
	 * \note The Tag <b>is still</b> owned by the MPEG::File and should not be
	 * deleted by the user.  It will be deleted when the file (object) is
	 * destroyed.
	 */
	ID3v2_Tag *ID3v2Tag(bool create = false);

	/*!
	 * Returns a pointer to the ID3v1 tag of the file.
	 *
	 * If \a create is false (the default) this will return a null pointer
	 * if there is no valid ID3v1 tag.  If \a create is true it will create
	 * an ID3v1 tag if one does not exist.
	 *
	 * \note The Tag <b>is still</b> owned by the MPEG::File and should not be
	 * deleted by the user.  It will be deleted when the file (object) is
	 * destroyed.
	 */
	ID3v1_Tag *ID3v1Tag(bool create = false);

	/*!
	 * Returns a pointer to the APE tag of the file.
	 *
	 * If \a create is false (the default) this will return a null pointer
	 * if there is no valid APE tag.  If \a create is true it will create
	 * an APE tag if one does not exist.
	 *
	 * \note The Tag <b>is still</b> owned by the MPEG::File and should not be
	 * deleted by the user.  It will be deleted when the file (object) is
	 * destroyed.
	 */
	APE_Tag *APETag(bool create = false);

	/*!
	 * This will strip the tags that match the OR-ed together TagTypes from the
	 * file.  By default it strips all tags.  It returns true if the tags are
	 * successfully stripped.
	 *
	 * This is equivalent to strip(tags, true)
	 *
	 * \note This will also invalidate pointers to the ID3 and APE tags
	 * as their memory will be freed.
	 */
	bool strip(int tags = MPEG_AllTags) { return strip(tags, true); }

	/*!
	 * This will strip the tags that match the OR-ed together TagTypes from the
	 * file.  By default it strips all tags.  It returns true if the tags are
	 * successfully stripped.
	 *
	 * If \a freeMemory is true the ID3 and APE tags will be deleted and
	 * pointers to them will be invalidated.
	 */
	// BIC: merge with the method above
	bool strip(int tags, bool freeMemory);

	/*!
	 * Returns the position in the file of the first MPEG frame.
	 */
	long firstFrameOffset();

	/*!
	 * Returns the position in the file of the next MPEG frame,
	 * using the current position as start
	 */
	long nextFrameOffset(long position);

	/*!
	 * Returns the position in the file of the previous MPEG frame,
	 * using the current position as start
	 */
	long previousFrameOffset(long position);

	/*!
	 * Returns the position in the file of the last MPEG frame.
	 */
	long lastFrameOffset();

private:
	MPEG_File(const MPEG_File &);
	MPEG_File &operator=(const MPEG_File &);

	void read(int readStyle);
	long findID3v2();
	long findID3v1();
	long findAPE();

	/*!
	 * MPEG frames can be recognized by the bit pattern 11111111 111, so the
	 * first byte is easy to check for, however checking to see if the second byte
	 * starts with \e 111 is a bit more tricky, hence this member function.
	 */
	static bool secondSynchByte(char byte);

	ID3v2_Tag*                  m_id3v2Tag;
	long                        m_id3v2Location;
	SjUint                      m_id3v2OriginalSize;

	APE_Tag*                    m_apeTag;
	long                        m_apeLocation;
	SjUint                      m_apeOriginalSize;

	ID3v1_Tag*                  m_id3v1Tag;
	long                        m_id3v1Location;

	MPEG_CombinedTag*           m_combinedTag;

	// These indicate whether the file *on disk* has these tags, not if
	// this data structure does.  This is used in computing offsets.

	bool                        m_hasId3v2;
	bool                        m_hasId3v1;
	bool                        m_hasApe;

	Tagger_AudioProperties*     m_properties;
};

#endif
