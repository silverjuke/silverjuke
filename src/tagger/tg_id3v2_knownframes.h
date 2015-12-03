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




#ifndef __ID3V2_KNOWNFRAMES_H__
#define __ID3V2_KNOWNFRAMES_H__


#include "tg_tagger_base.h"
#include "tg_id3v2_frame.h"
#include "tg_id3v2_header_footer.h"

class ID3v2_Tag;



/*******************************************************************************
 *  ID3v2_AttachedPictureFrame
 ******************************************************************************/



//! An ID3v2 attached picture frame implementation


/*!
* This describes the function or content of the picture.
*/
enum ID3v2_AttachedPictureType
{
    //! A type not enumerated below
    ID3v2_Other              = 0x00,
    //! 32x32 PNG image that should be used as the file icon
    ID3v2_FileIcon           = 0x01,
    //! File icon of a different size or format
    ID3v2_OtherFileIcon      = 0x02,
    //! Front cover image of the album
    ID3v2_FrontCover         = 0x03,
    //! Back cover image of the album
    ID3v2_BackCover          = 0x04,
    //! Inside leaflet page of the album
    ID3v2_LeafletPage        = 0x05,
    //! Image from the album itself
    ID3v2_Media              = 0x06,
    //! Picture of the lead artist or soloist
    ID3v2_LeadArtist         = 0x07,
    //! Picture of the artist or performer
    ID3v2_Artist             = 0x08,
    //! Picture of the conductor
    ID3v2_Conductor          = 0x09,
    //! Picture of the band or orchestra
    ID3v2_Band               = 0x0A,
    //! Picture of the composer
    ID3v2_Composer           = 0x0B,
    //! Picture of the lyricist or text writer
    ID3v2_Lyricist           = 0x0C,
    //! Picture of the recording location or studio
    ID3v2_RecordingLocation  = 0x0D,
    //! Picture of the artists during recording
    ID3v2_DuringRecording    = 0x0E,
    //! Picture of the artists during performance
    ID3v2_DuringPerformance  = 0x0F,
    //! Picture from a movie or video related to the track
    ID3v2_MovieScreenCapture = 0x10,
    //! Picture of a large, coloured fish
    ID3v2_ColouredFish       = 0x11,
    //! Illustration related to the track
    ID3v2_Illustration       = 0x12,
    //! Logo of the band or performer
    ID3v2_BandLogo           = 0x13,
    //! Logo of the publisher (record company)
    ID3v2_PublisherLogo      = 0x14
};

/*!
 * This is an implementation of ID3v2 attached pictures.  Pictures may be
 * included in tags, one per APIC frame (but there may be multiple APIC
 * frames in a single tag).  These pictures are usually in either JPEG or
 * PNG format.
 */
class ID3v2_AttachedPictureFrame : public ID3v2_Frame
{
public:
	/*!
	 * Constructs an empty picture frame.  The description, content and text
	 * encoding should be set manually.
	 */
	ID3v2_AttachedPictureFrame();

	/*!
	 * Constructs an AttachedPicture frame based on \a data.
	 */
	explicit ID3v2_AttachedPictureFrame(const SjByteVector &data);

	/*!
	 * Destroys the AttahcedPictureFrame instance.
	 */
	virtual ~ID3v2_AttachedPictureFrame() {}

	/*!
	 * Returns a string containing the description and mime-type
	 */
	virtual wxString toString() const { return wxT(""); }

	/*!
	 * Returns the text encoding used for the description.
	 *
	 * \see setTextEncoding()
	 * \see description()
	 */
	SjStringType textEncoding() const { return m_textEncoding; }

	/*!
	 * Set the text encoding used for the description.
	 *
	 * \see description()
	 */
	void setTextEncoding(SjStringType t) { m_textEncoding = t; }

	/*!
	 * Returns the mime type of the image.  This should in most cases be
	 * "image/png" or "image/jpeg".
	 */
	wxString mimeType() const { return m_mimeType; }

	/*!
	 * Sets the mime type of the image.  This should in most cases be
	 * "image/png" or "image/jpeg".
	 */
	void setMimeType(const wxString &m) { m_mimeType = m; }

	/*!
	 * Returns the type of the image.
	 *
	 * \see Type
	 * \see setType()
	 */
	ID3v2_AttachedPictureType type() const { return m_type; }

	/*!
	 * Sets the type for the image.
	 *
	 * \see Type
	 * \see type()
	 */
	void setType(ID3v2_AttachedPictureType t) { m_type = t; }

	/*!
	 * Returns a text description of the image.
	 *
	 * \see setDescription()
	 * \see textEncoding()
	 * \see setTextEncoding()
	 */
	wxString description() const { return m_description; }

	/*!
	 * Sets a textual description of the image to \a desc.
	 *
	 * \see description()
	 * \see textEncoding()
	 * \see setTextEncoding()
	 */
	void setDescription(const wxString &desc) { m_description = desc; }

	/*!
	 * Returns the image data as a SjByteVector.
	 *
	 * \note SjByteVector has a data() method that returns a const char * which
	 * should make it easy to export this data to external programs.
	 *
	 * \see setPicture()
	 * \see mimeType()
	 */
	SjByteVector picture() const { return m_data; }

	/*!
	 * Sets the image data to \a p.  \a p should be of the type specified in
	 * this frame's mime-type specification.
	 *
	 * \see picture()
	 * \see mimeType()
	 * \see setMimeType()
	 */
	void setPicture(const SjByteVector &p) { m_data = p; }

	virtual SjByteVector renderFields() const;

protected:
	virtual void parseFields(const SjByteVector &data);

private:
	ID3v2_AttachedPictureFrame(const SjByteVector &data, ID3v2_FrameHeader *h);
	ID3v2_AttachedPictureFrame(const ID3v2_AttachedPictureFrame &);
	ID3v2_AttachedPictureFrame &operator=(const ID3v2_AttachedPictureFrame &);

	SjStringType                m_textEncoding;
	wxString                    m_mimeType;
	ID3v2_AttachedPictureType   m_type;
	wxString                    m_description;
	SjByteVector                m_data;

	friend class ID3v2_FrameFactory;
};



/*******************************************************************************
 *  ID3v2_CommentsFrame
 ******************************************************************************/



//! An implementation of ID3v2 comments
/*!
 * This implements the ID3v2 comment format.  An ID3v2 comment concists of
 * a language encoding, a description and a single text field.
 */

class ID3v2_CommentsFrame : public ID3v2_Frame
{
public:
	/*!
	 * Construct an empty comment frame that will use the text encoding
	 * \a encoding.
	 */
	explicit ID3v2_CommentsFrame(SjStringType encoding = SJ_LATIN1);

	/*!
	 * Construct a comment based on the data in \a data.
	 */
	explicit ID3v2_CommentsFrame(const SjByteVector &data);

	/*!
	 * Destroys this CommentFrame instance.
	 */
	virtual ~ID3v2_CommentsFrame() {}

	/*!
	 * Returns the text of this comment.
	 *
	 * \see text()
	 */
	virtual wxString toString() const { return m_text; }

	/*!
	 * Returns the language encoding as a 3 byte encoding as specified by
	 * <a href="http://en.wikipedia.org/wiki/ISO_639">ISO-639-2</a>.
	 *
	 * \note Most taggers simply ignore this value.
	 *
	 * \see setLanguage()
	 */
	SjByteVector language() const { return m_language; }

	/*!
	 * Returns the description of this comment.
	 *
	 * \note Most taggers simply ignore this value.
	 *
	 * \see setDescription()
	 */
	wxString description() const { return m_description; }

	/*!
	 * Returns the text of this comment.
	 *
	 * \see setText()
	 */
	wxString text() const { return m_text; }

	/*!
	 * Set the language using the 3 byte language code from
	 * <a href="http://en.wikipedia.org/wiki/ISO_639">ISO-639-2</a> to
	 * \a languageCode.
	 *
	 * \see language()
	 */
	void setLanguage(const SjByteVector &languageCode);

	/*!
	 * Sets the description of the comment to \a s.
	 *
	 * \see decription()
	 */
	void setDescription(const wxString &s) { m_description = s; }

	/*!
	 * Sets the text portion of the comment to \a s.
	 *
	 * \see text()
	 */
	virtual void setText(const wxString &s) { m_text = s; }

	/*!
	 * Returns the text encoding that will be used in rendering this frame.
	 * This defaults to the type that was either specified in the constructor
	 * or read from the frame when parsed.
	 *
	 * \see setTextEncoding()
	 * \see render()
	 */
	SjStringType textEncoding() const { return m_textEncoding; }

	/*!
	 * Sets the text encoding to be used when rendering this frame to
	 * \a encoding.
	 *
	 * \see textEncoding()
	 * \see render()
	 */
	void setTextEncoding(SjStringType encoding) { m_textEncoding = encoding; }

	virtual SjByteVector renderFields() const;

protected:
	// Reimplementations.

	virtual void parseFields(const SjByteVector &data);

private:
	/*!
	 * The constructor used by the FrameFactory.
	 */
	ID3v2_CommentsFrame(const SjByteVector &data, ID3v2_FrameHeader *h);
	ID3v2_CommentsFrame(const ID3v2_CommentsFrame &);
	ID3v2_CommentsFrame &operator=(const ID3v2_CommentsFrame &);

	SjStringType    m_textEncoding;
	SjByteVector    m_language;
	wxString        m_description;
	wxString        m_text;

	friend class ID3v2_FrameFactory;
};




/*******************************************************************************
 *  ID3v2_TextIdentificationFrame
 ******************************************************************************/



//! An ID3v2 text identification frame implementation

/*!
 * This is an implementation of the most common type of ID3v2 frame -- text
 * identification frames.  There are a number of variations on this.  Those
 * enumerated in the ID3v2.4 standard are:
 *
 * <ul>
 *   <li><b>TALB</b> Album/Movie/Show title</li>
 *   <li><b>TBPM</b> BPM (beats per minute)</li>
 *   <li><b>TCOM</b> Composer</li>
 *   <li><b>TCON</b> Content type</li>
 *   <li><b>TCOP</b> Copyright message</li>
 *   <li><b>TDEN</b> Encoding time</li>
 *   <li><b>TDLY</b> Playlist delay</li>
 *   <li><b>TDOR</b> Original release time</li>
 *   <li><b>TDRC</b> Recording time</li>
 *   <li><b>TDRL</b> Release time</li>
 *   <li><b>TDTG</b> Tagging time</li>
 *   <li><b>TENC</b> Encoded by</li>
 *   <li><b>TEXT</b> Lyricist/Text writer</li>
 *   <li><b>TFLT</b> File type</li>
 *   <li><b>TIPL</b> Involved people list</li>
 *   <li><b>TIT1</b> Content group description</li>
 *   <li><b>TIT2</b> Title/songname/content description</li>
 *   <li><b>TIT3</b> Subtitle/Description refinement</li>
 *   <li><b>TKEY</b> Initial key</li>
 *   <li><b>TLAN</b> Language(s)</li>
 *   <li><b>TLEN</b> Length</li>
 *   <li><b>TMCL</b> Musician credits list</li>
 *   <li><b>TMED</b> Media type</li>
 *   <li><b>TMOO</b> Mood</li>
 *   <li><b>TOAL</b> Original album/movie/show title</li>
 *   <li><b>TOFN</b> Original filename</li>
 *   <li><b>TOLY</b> Original lyricist(s)/text writer(s)</li>
 *   <li><b>TOPE</b> Original artist(s)/performer(s)</li>
 *   <li><b>TOWN</b> File owner/licensee</li>
 *   <li><b>TPE1</b> Lead performer(s)/Soloist(s)</li>
 *   <li><b>TPE2</b> Band/orchestra/accompaniment</li>
 *   <li><b>TPE3</b> Conductor/performer refinement</li>
 *   <li><b>TPE4</b> Interpreted, remixed, or otherwise modified by</li>
 *   <li><b>TPOS</b> Part of a set</li>
 *   <li><b>TPRO</b> Produced notice</li>
 *   <li><b>TPUB</b> Publisher</li>
 *   <li><b>TRCK</b> Track number/Position in set</li>
 *   <li><b>TRSN</b> Internet radio station name</li>
 *   <li><b>TRSO</b> Internet radio station owner</li>
 *   <li><b>TSOA</b> Album sort order</li>
 *   <li><b>TSOP</b> Performer sort order</li>
 *   <li><b>TSOT</b> Title sort order</li>
 *   <li><b>TSRC</b> ISRC (international standard recording code)</li>
 *   <li><b>TSSE</b> Software/Hardware and settings used for encoding</li>
 *   <li><b>TSST</b> Set subtitle</li>
 * </ul>
 *
 * The ID3v2 Frames document gives a description of each of these formats
 * and the expected order of strings in each.  ID3v2::Header::frameID() can
 * be used to determine the frame type.
 */
class ID3v2_TextIdentificationFrame : public ID3v2_Frame
{
	friend class ID3v2_FrameFactory;

public:
	/*!
	 * Construct an empty frame of type \a type.  Uses \a encoding as the
	 * default text encoding.
	 *
	 * \note In this case you must specify the text encoding as it
	 * resolves the ambiguity between constructors.
	 */
	ID3v2_TextIdentificationFrame(const SjByteVector &type, SjStringType encoding);

	/*!
	 * This is a dual purpose constructor.  \a data can either be binary data
	 * that should be parsed or (at a minimum) the frame ID.
	 */
	explicit ID3v2_TextIdentificationFrame(const SjByteVector &data);

	/*!
	 * Destroys this TextIdentificationFrame instance.
	 */
	virtual ~ID3v2_TextIdentificationFrame() {}

	/*!
	 * Text identification frames are a list of string fields.
	 *
	 * This function will accept either a StringList or a String (using the
	 * StringList constructor that accepts a single String).
	 *
	 * \note This will not change the text encoding of the frame even if the
	 * strings passed in are not of the same encoding.  Please use
	 *  setEncoding(s.type()) if you wish to change the encoding of the frame.
	 */
	void setText(const wxArrayString &l) { m_fieldList = l; }

	// Reimplementations.

	virtual void setText(const wxString &s) { m_fieldList.Empty(); m_fieldList.Add(s); }
	virtual wxString toString() const;

	/*!
	 * Returns the text encoding that will be used in rendering this frame.
	 * This defaults to the type that was either specified in the constructor
	 * or read from the frame when parsed.
	 *
	 * \see setTextEncoding()
	 * \see render()
	 */
	SjStringType textEncoding() const { return m_textEncoding; }

	/*!
	 * Sets the text encoding to be used when rendering this frame to
	 * \a encoding.
	 *
	 * \see textEncoding()
	 * \see render()
	 */
	void setTextEncoding(SjStringType encoding) { m_textEncoding = encoding; }

	/*!
	 * Returns a list of the strings in this frame.
	 */
	wxArrayString fieldList() const { return m_fieldList; }

	virtual SjByteVector renderFields() const;

protected:
	// Reimplementations.

	virtual void parseFields(const SjByteVector &data);

	/*!
	 * The constructor used by the FrameFactory.
	 */
	ID3v2_TextIdentificationFrame(const SjByteVector &data, ID3v2_FrameHeader *h);

private:
	ID3v2_TextIdentificationFrame(const ID3v2_TextIdentificationFrame &);
	ID3v2_TextIdentificationFrame &operator=(const ID3v2_TextIdentificationFrame &);


	SjStringType    m_textEncoding;
	wxArrayString m_fieldList;
};



/*!
 * This is a specialization of text identification frames that allows for
 * user defined entries.  Each entry has a description in addition to the
 * normal list of fields that a text identification frame has.
 *
 * This description identifies the frame and must be unique.
 */
//! An ID3v2 custom text identification frame implementationx
class ID3v2_UserTextIdentificationFrame : public ID3v2_TextIdentificationFrame
{
	friend class ID3v2_FrameFactory;

public:
	/*!
	 * Constructs an empty user defined text identification frame.  For this to be
	 * a useful frame both a description and text must be set.
	 */
	explicit ID3v2_UserTextIdentificationFrame(SjStringType encoding = SJ_LATIN1);

	/*!
	 * Creates a frame based on \a data.
	 */
	explicit ID3v2_UserTextIdentificationFrame(const SjByteVector &data);

	// no special stuff
	//virtual wxString toString() const;

	/*!
	 * Returns the description for this frame.
	 */
	wxString description() const;

	/*!
	 * Sets the description of the frame to \a s.  \a s must be unique.  You can
	 * check for the presense of another user defined text frame of the same type
	 * using find() and testing for null.
	 */
	void setDescription(const wxString &s);

	wxArrayString fieldList() const;
	void setText(const wxString &text);
	void setText(const wxArrayString &fields);

	/*!
	 * Searches for the user defined text frame with the description \a description
	 * in \a tag.  This returns null if no matching frames were found.
	 */
	static ID3v2_UserTextIdentificationFrame *find(ID3v2_Tag *tag, const wxString &description);

private:
	ID3v2_UserTextIdentificationFrame(const SjByteVector &data, ID3v2_FrameHeader *h);
	ID3v2_UserTextIdentificationFrame(const ID3v2_TextIdentificationFrame &);
	ID3v2_UserTextIdentificationFrame &operator=(const ID3v2_UserTextIdentificationFrame &);
};



/*******************************************************************************
 *  ID3v2_UniquefileIdentifierFrame
 ******************************************************************************/



/*!
 * This is an implementation of ID3v2 unique file identifier frames.  This
 * frame is used to identify the file in an arbitrary database identified
 * by the owner field.
 */
//! An implementation of ID3v2 unique identifier frames
//#define TAGGER_USE_UNIQUE_FILE_IDENTIFIER_FRAME
#ifdef TAGGER_USE_UNIQUE_FILE_IDENTIFIER_FRAME
class ID3v2_UniqueFileIdentifierFrame : public ID3v2_Frame
{
public:
	/*!
	 * Creates a uniqe file identifier frame based on \a data.
	 */
	ID3v2_UniqueFileIdentifierFrame(const SjByteVector &data);

	/*!
	 * Creates a unique file identifier frame with the owner \a owner and
	 * the identification \a id.
	 */
	ID3v2_UniqueFileIdentifierFrame(const wxString &owner, const SjByteVector &id);

	/*!
	 * Returns the owner for the frame; essentially this is the key for
	 * determining which identification scheme this key belongs to.  This
	 * will usually either be an email address or URL for the person or tool
	 * used to create the unique identifier.
	 *
	 * \see setOwner()
	 */
	wxString owner() const { return m_owner; }

	/*!
	 * Returns the unique identifier.  Though sometimes this is a text string
	 * it also may be binary data and as much should be assumed when handling
	 * it.
	 */
	SjByteVector identifier() const { return m_identifier; }

	/*!
	 * Sets the owner of the identification scheme to \a s.
	 *
	 * \see owner()
	 */
	void setOwner(const wxString &s) { m_owner = s; }

	/*!
	 * Sets the unique file identifier to \a v.
	 *
	 * \see identifier()
	 */
	void setIdentifier(const SjByteVector &v) { m_identifier = v; }

	virtual wxString toString() const { return ""; }

	virtual SjByteVector renderFields() const;

protected:
	virtual void parseFields(const SjByteVector &data);

private:
	ID3v2_UniqueFileIdentifierFrame(const SjByteVector &data, ID3v2_FrameHeader *h);

	wxString m_owner;
	SjByteVector m_identifier;

	friend class ID3v2_FrameFactory;
};
#endif


/*******************************************************************************
 *  ID3v2_PopularimeterFrame
 ******************************************************************************/


class ID3v2_PopularimeterFrame : public ID3v2_Frame
{
public:
	// constructor
	                     ID3v2_PopularimeterFrame();
	                     ID3v2_PopularimeterFrame(const SjByteVector& data);

	// get/set functions
	wxString             GetEmail                () const            { return m_email; }
	int                  GetRating255            () const            { return m_rating255; } // 0..255
	long                 GetCounter              () const            { return m_counter; }
	void                 SetEmail                (const wxString& e) { m_email = e; }
	void                 SetRating255            (int rating255)     { m_rating255 = rating255; } // 0..255
	void                 SetCounter              (long cnt)          { m_counter = cnt; }

	// methods for rendering the content
	virtual wxString     toString                () const;
	virtual SjByteVector renderFields            () const;

protected:
	void			     parseFields             (const SjByteVector& data);

private:
	wxString             m_email;
	int                  m_rating255; // 0..255
	long                 m_counter;
};


/*******************************************************************************
 *  ID3v2_UnknownFrame
 ******************************************************************************/



//! A frame type \e unknown to Tagger.
/*!
 * This class represents a frame type not known (or more often simply
 * unimplemented) in Tagger.  This is here provide a basic API for
 * manipulating the binary data of unknown frames and to provide a means
 * of rendering such \e unknown frames.
 *
 * Please note that a cleaner way of handling frame types that Tagger
 * does not understand is to subclass ID3v2::Frame and ID3v2::FrameFactory
 * to have your frame type supported through the standard ID3v2 mechanism.
 */
class ID3v2_UnknownFrame : public ID3v2_Frame
{
public:
	ID3v2_UnknownFrame(const SjByteVector &data);
	virtual ~ID3v2_UnknownFrame() {}

	virtual wxString toString() const { return wxT(""); }

	/*!
	 * Returns the field data (everything but the header) for this frame.
	 */
	SjByteVector data() const { return m_fieldData; }

	virtual SjByteVector renderFields() const { return m_fieldData; }

protected:
	virtual void parseFields(const SjByteVector &data) { m_fieldData = data; }

private:
	ID3v2_UnknownFrame(const SjByteVector &data, ID3v2_FrameHeader *h);
	ID3v2_UnknownFrame(const ID3v2_UnknownFrame &);
	ID3v2_UnknownFrame &operator=(const ID3v2_UnknownFrame &);

	SjByteVector m_fieldData;
	friend class ID3v2_FrameFactory;
};




#endif // __ID3V2_KNOWNFRAMES_H__

