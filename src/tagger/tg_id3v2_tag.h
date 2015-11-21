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



#ifndef TAGGER_ID3V2TAG_H
#define TAGGER_ID3V2TAG_H

#include "tg_tagger_base.h"
#include "tg_id3v2_header_footer.h"
#include "tg_id3v2_frame.h"
#include "tg_id3v2_framefactory.h"



//typedef List<Frame *> FrameList;
//typedef Map<ByteVector, FrameList> FrameListMap;



//! An ID3v2 implementation

/*!
 * This is a relatively complete and flexible framework for working with ID3v2
 * tags.
 *
 * \see ID3v2::Tag
 */

//! The main class in the ID3v2 implementation

/*!
 * This is the main class in the ID3v2 implementation.  It serves two
 * functions.  This first, as is obvious from the public API, is to provide a
 * container for the other ID3v2 related classes.  In addition, through the
 * read() and parse() protected methods, it provides the most basic level of
 * parsing.  In these methods the ID3v2 tag is extracted from the file and
 * split into data components.
 *
 * ID3v2 tags have several parts, Tagger attempts to provide an interface
 * for them all.  header(), footer() and extendedHeader() corespond to those
 * data structures in the ID3v2 standard and the APIs for the classes that
 * they return attempt to reflect this.
 *
 * Also ID3v2 tags are built up from a list of frames, which are in turn
 * have a header and a list of fields.  Tagger provides two ways of accessing
 * the list of frames that are in a given ID3v2 tag.  The first is simply
 * via the frameList() method.  This is just a list of pointers to the frames.
 * The second is a map from the frame type -- i.e. "COMM" for comments -- and
 * a list of frames of that type.  (In some cases ID3v2 allows for multiple
 * frames of the same type, hence this being a map to a list rather than just
 * a map to an individual frame.)
 *
 * More information on the structure of frames can be found in the ID3v2::Frame
 * class.
 *
 * read() and parse() pass binary data to the other ID3v2 class structures,
 * they do not handle parsing of flags or fields, for instace.  Those are
 * handled by similar functions within those classes.
 *
 * \note All pointers to data structures within the tag will become invalid
 * when the tag is destroyed.
 *
 * \warning Dealing with the nasty details of ID3v2 is not for the faint of
 * heart and should not be done without much meditation on the spec.  It's
 * rather long, but if you're planning on messing with this class and others
 * that deal with the details of ID3v2 (rather than the nice, safe, abstract
 * Tagger_Tag and friends), it's worth your time to familiarize yourself
 * with said spec (which is distrubuted with the Tagger sources).  Tagger
 * tries to do most of the work, but with a little luck, you can still
 * convince it to generate invalid ID3v2 tags.  The APIs for ID3v2 assume a
 * working knowledge of ID3v2 structure.  You're been warned.
 */

class ID3v2_CommentsFrame;

class ID3v2_Tag : public Tagger_Tag
{
public:
	/*!
	 * Constructs an ID3v2 tag read from file starting at tagOffset.
	 * factory specifies which FrameFactory will be used for the
	 * construction of new frames.
	 *
	 * note You should be able to ignore the  factory parameter in almost
	 * all situations.  You would want to specify your own FrameFactory
	 * subclass in the case that you are extending Tagger to support additional
	 * frame types, which would be incorperated into your factory.
	 *
	 * \note For empty tags, you must create at least one frame for this tag to be valid.
	 *
	 * see FrameFactory
	 */
	ID3v2_Tag(Tagger_File* file = NULL, long tagOffset = -1,
	          const ID3v2_FrameFactory *factory = ID3v2_FrameFactory::instance());

	/*!
	 * Destroys this Tag instance.
	 */
	virtual ~ID3v2_Tag();

	// Reimplementations.

	wxString simpleFrame(const wxString& frameId) const;
	virtual wxString title() const;
	virtual wxString leadArtist() const;
	virtual wxString orgArtist() const;
	virtual wxString composer() const;
	virtual wxString album() const;
	virtual wxString comment() const;
	virtual wxString genre() const;
	virtual wxString group() const;
	virtual long year() const;
	virtual long beatsPerMinute() const;
	virtual long rating() const;
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
	virtual void setRating(long i);
	virtual void setTrack(long nr, long count);
	virtual void setDisk(long nr, long count);

	virtual bool isEmpty() const;

	/*!
	 * Returns a pointer to the tag's header.
	 */
	ID3v2_Header* header() { return &(m_header); }

	/*!
	 * Returns a pointer to the tag's extended header or null if there is no
	 * extended header.
	 */
	ID3v2_ExtendedHeader *extendedHeader() const { return m_extendedHeader; }

	/*!
	 * Returns a pointer to the tag's footer or null if there is no footer.
	 *
	 * \deprecated I don't see any reason to keep this around since there's
	 * nothing useful to be retrieved from the footer, but well, again, I'm
	 * prone to change my mind, so this gets to stay around until near a
	 * release.
	 */
	ID3v2_Footer *footer() const { return m_footer; }

	/*!
	 * Returns a reference to the frame list map.  This is an FrameListMap of
	 * all of the frames in the tag.
	 *
	 * This is the most convenient structure for accessing the tag's frames.
	 * Many frame types allow multiple instances of the same frame type so this
	 * is a map of lists.  In most cases however there will only be a single
	 * frame of a certain type.
	 *
	 * Let's say for instance that you wanted to access the frame for total
	 * beats per minute -- the TBPM frame.
	 *
	 * \code
	 * Tagger_MPEG::File f("foo.mp3");
	 *
	 * // Check to make sure that it has an ID3v2 tag
	 *
	 * if(f.ID3v2Tag()) {
	 *
	 *   // Get the list of frames for a specific frame type
	 *
	 *   Tagger_ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()["TBPM"];
	 *
	 *   if(!l.isEmpty())
	 *     std::cout << l.front().toString() << std::endl;
	 * }
	 *
	 * \endcode
	 *
	 * \warning You should not modify this data structure directly, instead
	 * use addFrame() and removeFrame().
	 *
	 * \see frameList()
	 */
	const SjSPHash* frameListMap() const { return &m_frameListMap; }

	/*!
	 * Returns a reference to the frame list.  This is an FrameList of all of
	 * the frames in the tag in the order that they were parsed.
	 *
	 * This can be useful if for example you want iterate over the tag's frames
	 * in the order that they occur in the tag.
	 *
	 * \warning You should not modify this data structure directly, instead
	 * use addFrame() and removeFrame().
	 */
	const ID3v2_FrameList &frameList() const { return m_frameList; }

	/*!
	 * Returns the frame list for frames with the id \a frameID or an empty
	 * list if there are no frames of that type.  This is just a convenience
	 * and is equivalent to:
	 *
	 * \code
	 * frameListMap()[frameID];
	 * \endcode
	 *
	 * \see frameListMap()
	 */
	ID3v2_FrameList* frameList(const wxString& frameID) const;

	/*!
	 * Add a frame to the tag.  At this point the tag takes ownership of
	 * the frame and will handle freeing its memory.
	 *
	 * \note Using this method will invalidate any pointers on the list
	 * returned by frameList()
	 */
	void addFrame(ID3v2_Frame *frame);

	/*!
	 * Remove a frame from the tag.  If \a del is true the frame's memory
	 * will be freed; if it is false, it must be deleted by the user.
	 *
	 * \note Using this method will invalidate any pointers on the list
	 * returned by frameList()
	 */
	void removeFrame(ID3v2_Frame *frame, bool del = true);

	/*!
	 * Remove all frames of type \a id from the tag and free their memory.
	 *
	 * \note Using this method will invalidate any pointers on the list
	 * returned by frameList()
	 */
	void removeFrames(const wxString &id);

	/*!
	 * Render the tag back to binary data, suitable to be written to disk.
	 */
	SjByteVector render();

protected:
	/*!
	 * Reads data from the file specified in the constructor.  It does basic
	 * parsing of the data in the largest chunks.  It partitions the tag into
	 * the Header, the body of the tag  (which contains the ExtendedHeader and
	 * frames) and Footer.
	 */
	void read();

	/*!
	 * This is called by read to parse the body of the tag.  It determines if an
	 * extended header exists and adds frames to the FrameListMap.
	 */
	void parse(const SjByteVector &data);

	/*!
	 * Sets the value of the text frame with the Frame ID \a id to \a value.
	 * If the frame does not exist, it is created.
	 */
	void setTextFrame(const wxString &id, const wxString &value);

private:
#ifdef __WXDEBUG__
	ID3v2_Tag &operator=(const ID3v2_Tag &) { wxLogWarning(wxT("do not copy ID3v2_Tag objects this way!")); return *this; }
#endif

	Tagger_File*                m_file;
	long                        m_tagOffset;
	const ID3v2_FrameFactory*   m_factory;

	ID3v2_Header                m_header;
	ID3v2_ExtendedHeader*       m_extendedHeader;
	ID3v2_Footer*               m_footer;

	ID3v2_CommentsFrame*        findCommentFrame() const;

	int                         m_paddingSize;

	SjSPHash                    m_frameListMap; // contains ID3v2_FrameList's

	ID3v2_FrameList             m_frameList;
};

#endif
