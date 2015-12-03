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





#ifndef __SJ_MP4_BOXES__
#define __SJ_MP4_BOXES__


#include "tg_tagger_base.h"

class MP4_File;


/*******************************************************************************
 * tools
 ******************************************************************************/



/*! union for easy fourcc / type handling */
class MP4_Fourcc
{
public:
	//! std constructor
	MP4_Fourcc();
	//! string constructor
	MP4_Fourcc(const wxString& fourcc);


	//! function to get a string version of the fourcc
	wxString toString() const;

	//! cast operator to unsigned int
	operator unsigned int() const;
	//! comparison operator
	bool operator == (unsigned int fourccB ) const;
	//! comparison operator
	bool operator != (unsigned int fourccB ) const;
	//! assigment operator for unsigned int
	MP4_Fourcc& operator = (unsigned int fourcc );
	//! assigment operator for character string
	MP4_Fourcc& operator = (const unsigned char fourcc[4]);

private:
	unsigned int m_fourcc;     /*!< integer code of the fourcc */
};




/*******************************************************************************
 * box base class
 ******************************************************************************/



class MP4_IsoBox
{
public:
	//! factory function
	static MP4_IsoBox* createInstance( MP4_File* anyfile, MP4_Fourcc fourcc, unsigned int size, long offset );

	//! constructor for base class
	MP4_IsoBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	//! destructor - simply freeing private ptr
	virtual ~MP4_IsoBox() {}

	//! function to get the fourcc code
	MP4_Fourcc fourcc() const {return m_fourcc;}
	//! function to get the size of tha atom/box
	unsigned int size() const {return m_size;}
	//! function to get the offset of the atom in the mp4 file
	long offset() const {return m_offset;}

	//! parse wrapper to get common interface for both box and fullbox
	virtual void  parsebox();
	//! pure virtual function for all subclasses to implement
	virtual void parse() = 0;

protected:
	//! function to get the file pointer
	MP4_File* file() const {return m_file;}

private:
	MP4_Fourcc   m_fourcc;
	unsigned int  m_size;
	long          m_offset;
	MP4_File*     m_file;
};


class MP4_ListOfIsoBoxes
{
public:
	MP4_ListOfIsoBoxes() {}
	~MP4_ListOfIsoBoxes();
	void Add(MP4_IsoBox* b) { m_arr.Add(b); }

private:
	wxArrayPtrVoid m_arr;
};



class MP4_IsoFullBox : public MP4_IsoBox
{
public:
	//! constructor for full box
	MP4_IsoFullBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	//! destructor for mp4 iso full box
	virtual ~MP4_IsoFullBox() {}

	//! function to get the version of box
	unsigned char version() {return m_version;}
	//! function to get the flag map
	unsigned int  flags() {return m_flags;}

	//! parse wrapper to get common interface for both box and fullbox
	virtual void  parsebox();

private:
	unsigned char m_version;
	unsigned int  m_flags;
};



/*******************************************************************************
 * concrete box classes
 ******************************************************************************/



class MP4_SkipBox: public MP4_IsoBox
{
public:
	MP4_SkipBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_SkipBox() {}

private:
	//! parse the content of the box
	virtual void parse();

protected:
}; // class Mp4SkipBox






class MP4_MoovBox: public MP4_IsoBox
{
public:
	MP4_MoovBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_MoovBox() {}

	//! parse moov contents
	void parse();

private:
	//! container for all boxes in moov box (type: MP4_IsoBox)
	MP4_ListOfIsoBoxes    m_moovBoxes;
}; // Mp4MoovBox


class MP4_MvhdBox: public MP4_IsoFullBox
{
public:
	MP4_MvhdBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_MvhdBox() {}

	//! function to get the creation time of the mp4 file
	unsigned wxLongLong_t creationTime() const {return m_creationTime;}
	//! function to get the modification time of the mp4 file
	unsigned wxLongLong_t modificationTime() const {return m_modificationTime;}
	//! function to get the timescale referenced by the above timestamps
	unsigned int timescale() const {return m_timescale;}
	//! function to get the presentation duration in the mp4 file
	unsigned wxLongLong_t duration() const {return m_duration;}
	//! function to get the rate (playout speed) - typically 1.0;
	unsigned int rate() const {return m_rate;}
	//! function to get volume level for presentation - typically 1.0;
	unsigned int volume() const {return m_volume;}
	//! function to get the track ID for adding new tracks - useless for this lib
	unsigned int nextTrackID() const {return m_nextTrackID;}

	//! parse mvhd contents
	void parse();

private:
	//! creation time of the file
	unsigned wxLongLong_t m_creationTime;
	//! modification time of the file - since midnight, Jan. 1, 1904, UTC-time
	unsigned wxLongLong_t m_modificationTime;
	//! timescale for the file - referred by all time specifications in this box
	unsigned int      m_timescale;
	//! duration of presentation
	unsigned wxLongLong_t m_duration;
	//! playout speed
	unsigned int      m_rate;
	//! volume for entire presentation
	unsigned int      m_volume;
	//! track ID for an additional track (next new track)
	unsigned int      m_nextTrackID;
}; // Mp4MvhdBox



class MP4_TrakBox: public MP4_IsoBox
{
public:
	MP4_TrakBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_TrakBox() {}

	//! parse trak contents
	void parse();

private:
	//! container for all boxes in trak box (type: MP4_IsoBox)
	MP4_ListOfIsoBoxes m_trakBoxes;
}; // Mp4TrakBox



class MP4_MdiaBox: public MP4_IsoBox
{
public:
	MP4_MdiaBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_MdiaBox() {}

	//! parse mdia contents
	void parse();

private:
	//! container for all boxes in mdia box (type: MP4_IsoBox)
	MP4_ListOfIsoBoxes m_mdiaBoxes;
}; // Mp4MdiaBox


class MP4_MinfBox: public MP4_IsoBox
{
public:
	MP4_MinfBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_MinfBox() {}

	//! parse minf contents
	void parse();
	//! set the handler type - needed for stsd
	void setHandlerType( MP4_Fourcc fourcc ) {m_handler_type = fourcc;}

private:
	//! container for all boxes in minf box
	MP4_ListOfIsoBoxes m_minfBoxes;
	//! stores the handler type of the current trak
	MP4_Fourcc         m_handler_type;
}; // Mp4MinfBox



class MP4_StblBox: public MP4_IsoBox
{
public:
	MP4_StblBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_StblBox() {}

	//! parse stbl contents
	void parse();
	//! set the handler type - needed for stsd
	void setHandlerType( MP4_Fourcc fourcc ) {m_handler_type = fourcc;}

private:
	//! container for all boxes in stbl box
	MP4_ListOfIsoBoxes m_stblBoxes;
	//! the handler type for the current trak
	MP4_Fourcc         m_handler_type;
}; // Mp4StblBox


class MP4_AudioSampleEntry;
class MP4_StsdBox: public MP4_IsoFullBox
{
public:
	MP4_StsdBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_StsdBox();

	//! parse stsd contents
	void parse();
	//! set the handler type - needed for stsd
	void setHandlerType( MP4_Fourcc fourcc ) {m_handler_type = fourcc;}

private:
	//! the handler type for the current trak
	MP4_Fourcc   m_handler_type;
	//! the audio sample entry
	MP4_AudioSampleEntry* m_audioSampleEntry;
}; // Mp4StsdBox



class MP4_HdlrBox: public MP4_IsoFullBox
{
public:
	MP4_HdlrBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_HdlrBox() {}

	//! parse hdlr contents
	void parse();
	//! get the handler type
	MP4_Fourcc hdlr_type() const { return m_handler_type; }
	//! get the hdlr string
	wxString hdlr_string() const { return m_hdlr_string; }

private:
	unsigned int   m_pre_defined;
	MP4_Fourcc    m_handler_type;
	wxString m_hdlr_string;
}; // Mp4HdlrBox




class MP4_UdtaBox: public MP4_IsoBox
{
public:
	MP4_UdtaBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_UdtaBox() {}

	//! parse moov contents
	void parse();

private:
	//! container for all boxes in udta box
	MP4_ListOfIsoBoxes m_udtaBoxes;
}; // Mp4UdtaBox



class MP4_MetaBox: public MP4_IsoFullBox
{
public:
	MP4_MetaBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_MetaBox() {}

	//! parse meta contents
	void parse();

private:
	//! container for all boxes in meta box
	MP4_ListOfIsoBoxes m_metaBoxes;
}; // Mp4MetaBox



class MP4_IlstBox: public MP4_IsoBox
{
public:
	MP4_IlstBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_IlstBox() {}

	//! parse ilst contents
	void parse();

private:
	//! container for all boxes in ilst box
	MP4_ListOfIsoBoxes m_ilstBoxes;
}; // Mp4IlstBox



class MP4_ITunesDataBox: public MP4_IsoFullBox
{
public:
	MP4_ITunesDataBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesDataBox() {}

	//! get the internal data, which can be txt or binary data as well
	SjByteVector data() const {return m_data;}

private:
	//! parse the content of the box
	virtual void parse();

private:
	SjByteVector m_data;
}; // class ITunesDataBox



class MP4_ITunesNamBox: public MP4_IsoBox
{
public:
	MP4_ITunesNamBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesNamBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesNamBox




class MP4_ITunesArtBox: public MP4_IsoBox
{
public:
	MP4_ITunesArtBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesArtBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesArtBox





class MP4_ITunesAlbBox: public MP4_IsoBox
{
public:
	MP4_ITunesAlbBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesAlbBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesAlbBox




class MP4_ITunesGenBox: public MP4_IsoBox
{
public:
	MP4_ITunesGenBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesGenBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesGenBox



class MP4_ITunesDayBox: public MP4_IsoBox
{
public:
	MP4_ITunesDayBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesDayBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesDayBox



class MP4_ITunesTrknBox: public MP4_IsoBox
{
public:
	MP4_ITunesTrknBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesTrknBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesTrknBox



class MP4_ITunesCmtBox: public MP4_IsoBox
{
public:
	MP4_ITunesCmtBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesCmtBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesCmtBox



class MP4_ITunesGrpBox: public MP4_IsoBox
{
public:
	MP4_ITunesGrpBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesGrpBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesGrpBox





class MP4_ITunesWrtBox: public MP4_IsoBox
{
public:
	MP4_ITunesWrtBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesWrtBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesWrtBox




class MP4_ITunesDiskBox: public MP4_IsoBox
{
public:
	MP4_ITunesDiskBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesDiskBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesDiskBox



class MP4_ITunesTmpoBox: public MP4_IsoBox
{
public:
	MP4_ITunesTmpoBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesTmpoBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesTmpoBox



class MP4_ITunesCvrBox: public MP4_IsoBox
{
public:
	MP4_ITunesCvrBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset );
	~MP4_ITunesCvrBox();

private:
	//! parse the content of the box
	virtual void parse();

private:
	MP4_ITunesDataBox* m_dataBox;
}; // class ITunesCvrBox




#endif // __SJ_MP4_BOXES__
