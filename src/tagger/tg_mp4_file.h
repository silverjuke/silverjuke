/**************************************************************************
    copyright            : (C) 2007 by Lukáš Lalinský
    email                : lalinsky@gmail.com
 **************************************************************************/

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



#ifndef TAGGER_MP4FILE_H
#define TAGGER_MP4FILE_H

#include "tg_tagger_base.h"
#include "tg_mp4_boxes.h"


class MP4_FilePrivate;
class MP4_File;





/*******************************************************************************
 * MP4_Tag
 ******************************************************************************/



class MP4_Tag : public Tagger_Tag
{
public:
	/*!
	 * Constructs an empty MP4 iTunes tag.
	 */
	MP4_Tag( ) {
		m_isEmpty=true;
		m_year = 0;
		m_beatsPerMinute = 0;
		m_trackNr=0;  m_trackCnt = 0;
		m_diskNr=0; m_diskCnt = 0;
	}

	/*!
	 * Destroys this Tag instance.
	 */
	virtual ~MP4_Tag() {}

	// Reimplementations.

	virtual wxString title() const {return m_title;}
	virtual wxString leadArtist() const {return m_leadArtist;}
	virtual wxString orgArtist() const {return wxT("");}
	virtual wxString composer() const {return m_composer;}
	virtual wxString album() const {return m_album;}
	virtual wxString comment() const {return m_comment;}
	virtual wxString genre() const {return m_genre;}
	virtual wxString group() const {return m_group;}
	virtual long year() const {return m_year;}
	virtual long beatsPerMinute() const {return m_beatsPerMinute;}
	virtual long rating() const {return 0;}
	virtual void track(long& nr, long& count) const {nr=m_trackNr; count=m_trackCnt;}
	virtual void disk(long& nr, long& count) const {nr=m_diskNr; count=m_diskCnt;}

	virtual void setTitle(const wxString &s) {m_title=s; m_isEmpty=false;}
	virtual void setLeadArtist(const wxString &s) {m_leadArtist=s; m_isEmpty=false;}
	virtual void setOrgArtist(const wxString &s) {}
	virtual void setComposer(const wxString &s) {m_composer=s; m_isEmpty=false;}
	virtual void setAlbum(const wxString &s) {m_album=s; m_isEmpty=false;}
	virtual void setComment(const wxString &s) {m_comment=s; m_isEmpty=false;}
	virtual void setGenre(const wxString &s) {m_genre=s; m_isEmpty=false;}
	virtual void setGroup(const wxString &s) {m_group=s; m_isEmpty=false;}
	virtual void setYear(long i) {m_year=i; m_isEmpty=false;}
	virtual void setBeatsPerMinute(long i) {m_beatsPerMinute=i; m_isEmpty=false;}
	virtual void setRating(long i) {}
	virtual void setTrack(long nr, long cnt) {m_trackNr=nr; m_trackCnt=cnt; m_isEmpty=false;}
	virtual void setDisk(long nr, long cnt) {m_diskNr=nr; m_diskCnt=cnt; m_isEmpty=false;}

	// MP4 specific fields

	SjByteVector cover() const {return m_cover;}
	void setCover( const SjByteVector& cover ) {m_cover=cover; m_isEmpty=false;}


	virtual bool isEmpty() const {return m_isEmpty;}

private:
#ifdef __WXDEBUG__
	MP4_Tag(const MP4_Tag &) { wxASSERT_MSG(0, wxT("do not construct MP4_Tag objects this way!"));  }
	MP4_Tag &operator=(const MP4_Tag &) { wxASSERT_MSG(0, wxT("do not copy MP4_Tag objects this way!")); return *this; }
#endif

	wxString    m_title;
	wxString    m_leadArtist;
	wxString    m_composer;
	wxString    m_album;
	wxString    m_comment;
	wxString    m_genre;
	wxString    m_group;
	long        m_year;
	long        m_beatsPerMinute;
	long        m_trackNr, m_trackCnt;
	long        m_diskNr, m_diskCnt;
	bool        m_isEmpty;
	SjByteVector m_cover;
};





/*******************************************************************************
 * MP4_TagsProxy
 ******************************************************************************/




typedef enum
{
    MP4_EBoxType_title = 0,
    MP4_EBoxType_artist,
    MP4_EBoxType_album,
    MP4_EBoxType_cover,
    MP4_EBoxType_genre,
    MP4_EBoxType_year,
    MP4_EBoxType_trackno,
    MP4_EBoxType_comment,
    MP4_EBoxType_grouping,
    MP4_EBoxType_composer,
    MP4_EBoxType_disk,
    MP4_EBoxType_bpm
} MP4_EBoxType;

// forward declaration(s)
class MP4_ITunesDataBox;
/*! proxy for mp4 itunes tag relevant boxes
 *
 *  this class works as a proxy for the specific tag boxes
 *  in an mp4 itunes file. the boxes are mired in
 *  the mp4 file structure and stepping through all box layers
 *  is avoided by registration at the proxy object.
 */
class MP4_TagsProxy
{
public:
	/*! enum for all supported box types */

	//! constructor
	MP4_TagsProxy();
	//! destructor
	~MP4_TagsProxy() {}

	//! function to get the data box for the title
	MP4_ITunesDataBox* titleData() const {return m_titleData;}
	//! function to get the data box for the artist
	MP4_ITunesDataBox* artistData() const {return m_artistData;}
	//! function to get the data box for the album
	MP4_ITunesDataBox* albumData() const {return m_albumData;}
	//! function to get the data box for the genre
	MP4_ITunesDataBox* genreData() const {return m_genreData;}
	//! function to get the data box for the year
	MP4_ITunesDataBox* yearData() const {return m_yearData;}
	//! function to get the data box for the track number
	MP4_ITunesDataBox* trknData() const {return m_trknData;}
	//! function to get the data box for the comment
	MP4_ITunesDataBox* commentData() const {return m_commentData;}
	//! function to get the data box for the grouping
	MP4_ITunesDataBox* groupingData() const {return m_groupingData;}
	//! function to get the data box for the composer
	MP4_ITunesDataBox* composerData() const {return m_composerData;}
	//! function to get the data box for the disk number
	MP4_ITunesDataBox* diskData() const {return m_diskData;}
	//! function to get the data box for the bpm
	MP4_ITunesDataBox* bpmData() const {return m_bpmData;}
	//! function to get the data box for the cover
	MP4_ITunesDataBox* coverData() const {return m_coverData;}

	//! function to register a data box for a certain box type
	void registerBox( MP4_EBoxType boxtype, MP4_ITunesDataBox* databox );

private:
	//! private data of tags proxy
	MP4_ITunesDataBox* m_titleData;
	MP4_ITunesDataBox* m_artistData;
	MP4_ITunesDataBox* m_albumData;
	MP4_ITunesDataBox* m_coverData;
	MP4_ITunesDataBox* m_genreData;
	MP4_ITunesDataBox* m_yearData;
	MP4_ITunesDataBox* m_trknData;
	MP4_ITunesDataBox* m_commentData;
	MP4_ITunesDataBox* m_groupingData;
	MP4_ITunesDataBox* m_composerData;
	MP4_ITunesDataBox* m_diskData;
	MP4_ITunesDataBox* m_bpmData;
}; // class Mp4TagsProxy




/*******************************************************************************
 * MP4_PropsProxy
 ******************************************************************************/


class MP4_SampleEntry: public MP4_IsoBox
{
public:
	MP4_SampleEntry( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_SampleEntry() {}

public:
	//! parse the content of the box
	virtual void parse();

private:
	//! function to be implemented in subclass
	virtual void parseEntry() = 0;

private:
	unsigned int m_data_reference_index;
}; // class Mp4SampleEntry


class MP4_AudioSampleEntry: public MP4_SampleEntry
{
public:
	MP4_AudioSampleEntry( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_AudioSampleEntry() {}

	//! function to get the number of channels
	unsigned int channels() const { return m_channelcount; }
	//! function to get the sample rate
	unsigned int samplerate() const { return m_samplerate; }
	//! function to get the average bitrate of the audio stream
	unsigned int bitrate() const { return m_bitrate; }

private:
	//! parse the content of the box
	void parseEntry();

private:
	unsigned int m_channelcount;
	unsigned int m_samplerate;
	unsigned int m_bitrate;
}; // class Mp4AudioSampleEntry



class MP4_PropsProxy
{
public:
	//! constructor for properties proxy
	MP4_PropsProxy();
	//! destructor
	~MP4_PropsProxy() {}

	//! function to get length of media in seconds
	unsigned int seconds() const;
	//! function to get the nunmber of channels
	unsigned int channels() const;
	//! function to get the sample rate
	unsigned int sampleRate() const;
	//! function to get the bitrate rate
	unsigned int bitRate() const;

	//! function to register the movie header box - mvhd
	void registerMvhd( MP4_MvhdBox* mvhdbox );
	//! function to register the sample description box
	void registerAudioSampleEntry( MP4_AudioSampleEntry* audiosampleentry );

private:
	//! the movie header box
	MP4_MvhdBox* m_mvhdbox;
	//! the sample table box
	MP4_AudioSampleEntry* m_audiosampleentry;

}; // Mp4PropsProxy





/*******************************************************************************
 * MP4_AudioProperties
 ******************************************************************************/




class MP4_AudioProperties : public Tagger_AudioProperties
{
public:
	//! constructor
	MP4_AudioProperties();
	//! destructor
	~MP4_AudioProperties() {}

	//! function to set the proxy
	void setProxy( MP4_PropsProxy* proxy )  { m_propsproxy = proxy; }

	/*!
	 * Returns the length of the file in seconds.
	 */
	int length() const;

	/*!
	 * Returns the most appropriate bit rate for the file in kb/s.  For constant
	 * bitrate formats this is simply the bitrate of the file.  For variable
	 * bitrate formats this is either the average or nominal bitrate.
	 */
	int bitrate() const;

	/*!
	 * Returns the sample rate in Hz.
	 */
	int sampleRate() const;

	/*!
	 * Returns the number of audio channels.
	 */
	int channels() const;

private:
	MP4_PropsProxy* m_propsproxy;
};



/*******************************************************************************
 * MP4_File
 ******************************************************************************/



class MP4_File : public Tagger_File
{
public:
	MP4_File(
	    const wxFSFile* file,                // you should give either fsFile
	    wxInputStream* inputStream = NULL);  // in inputStream, not both!

	virtual ~MP4_File();

	/*!
	 * Returns the Tag for this file.  This will be an APE tag, an ID3v1 tag
	 * or a combination of the two.
	 */
	virtual Tagger_Tag *tag() const;
	MP4_Tag*  MP4Tag() const;

	/*!
	 * Returns the mp4 itunes::Properties for this file.  If no audio properties
	 * were read then this will return a null pointer.
	 */
	virtual Tagger_AudioProperties *audioProperties() const;

	/*!
	 * Saves the file. (not implemented)
	 */
	virtual bool save() { return FALSE; }

	/*!
	 * Helper function for parsing the MP4 file - reads the size and type of the next box.
	 * Returns true if read succeeded - not at EOF
	 */
	bool readSizeAndType( unsigned int& size, MP4_Fourcc& fourcc );

	/*!
	 * Helper function to read the length of an descriptor in systems manner
	 */
	unsigned int readSystemsLen();

	/*!
	 * Helper function for reading an unsigned int out of the file (big endian method)
	 */
	bool readInt( unsigned int& toRead );

	/*!
	 * Helper function for reading an unsigned short out of the file (big endian method)
	 */
	bool readShort( unsigned int& toRead );

	/*!
	 * Helper function for reading an unsigned long long (64bit) out of the file (big endian method)
	 */
	bool readLongLong( unsigned wxLongLong_t& toRead );

	/*!
	 *  Helper function to read a fourcc code
	 */
	bool readFourcc( MP4_Fourcc& fourcc );

	/*!
	 * Function to get the tags proxy for registration of the tags boxes.
	 * The proxy provides direct access to the data boxes of the certain tags - normally
	 * covered by several levels of subboxes
	 */
	MP4_TagsProxy* tagProxy() const;

	/*!
	 * Function to get the properties proxy for registration of the properties boxes.
	 * The proxy provides direct access to the needed boxes describing audio properties.
	 */
	MP4_PropsProxy* propProxy() const;


private:
	MP4_FilePrivate*    d;
};



#endif
