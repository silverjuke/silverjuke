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



#include "tg_tagger_base.h"
#include "tg_mp4_file.h"
#include "tg_mp4_boxes.h"
#include "tg_id3v1_tag.h"



/*******************************************************************************
 * MP4_TagsProxy
 ******************************************************************************/




MP4_TagsProxy::MP4_TagsProxy()
{
	m_titleData    = 0;
	m_artistData   = 0;
	m_albumData    = 0;
	m_coverData    = 0;
	m_genreData    = 0;
	m_yearData     = 0;
	m_trknData     = 0;
	m_commentData  = 0;
	m_groupingData = 0;
	m_composerData = 0;
	m_diskData     = 0;
	m_bpmData      = 0;
}


void MP4_TagsProxy::registerBox( MP4_EBoxType boxtype, MP4_ITunesDataBox* databox )
{
	switch( boxtype )
	{
		case MP4_EBoxType_title:
			m_titleData = databox;
			break;
		case MP4_EBoxType_artist:
			m_artistData = databox;
			break;
		case MP4_EBoxType_album:
			m_albumData = databox;
			break;
		case MP4_EBoxType_cover:
			m_coverData = databox;
			break;
		case MP4_EBoxType_genre:
			m_genreData = databox;
			break;
		case MP4_EBoxType_year:
			m_yearData = databox;
			break;
		case MP4_EBoxType_trackno:
			m_trknData = databox;
			break;
		case MP4_EBoxType_comment:
			m_commentData = databox;
			break;
		case MP4_EBoxType_grouping:
			m_groupingData = databox;
			break;
		case MP4_EBoxType_composer:
			m_composerData = databox;
			break;
		case MP4_EBoxType_disk:
			m_diskData = databox;
			break;
		case MP4_EBoxType_bpm:
			m_bpmData = databox;
			break;
	}
}



/*******************************************************************************
 * MP4_PropsProxy
 ******************************************************************************/



////////////// MP4_SampleEntry //////////////


MP4_SampleEntry::MP4_SampleEntry( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	:MP4_IsoBox(file, fourcc, size, offset)
{
}

void MP4_SampleEntry::parse()
{
	MP4_File* mp4file = file();
	if(!mp4file)
		return;

	// skip the first 6 bytes
	mp4file->Seek( 6, SJ_SEEK_CUR );
	// read data reference index
	if(!mp4file->readShort( m_data_reference_index))
		return;
	parseEntry();
}


////////////// MP4_AudioSampleEntry //////////////




MP4_AudioSampleEntry::MP4_AudioSampleEntry( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	:MP4_SampleEntry(file, fourcc, size, offset)
{
}

void MP4_AudioSampleEntry::parseEntry()
{
	MP4_File* mp4file = file();
	if(!mp4file)
		return;

	// read 8 reserved bytes
	mp4file->Seek( 8, SJ_SEEK_CUR );
	// read channelcount
	if(!mp4file->readShort( m_channelcount ))
		return;
	// seek over samplesize, pre_defined and reserved
	mp4file->Seek( 6, SJ_SEEK_CUR );
	// read samplerate
	if(!mp4file->readInt( m_samplerate ))
		return;

	// register box at proxy
	mp4file->propProxy()->registerAudioSampleEntry( this );


	//std::cout << "fourcc of audio sample entry: " << fourcc().toString() << std::endl;
	// check for both mp4a (plain files) and drms (encrypted files)
	if( (fourcc() == MP4_Fourcc(wxT("mp4a"))) ||
	        (fourcc() == MP4_Fourcc(wxT("drms")))  )
	{
		MP4_Fourcc fourcc;
		unsigned int        esds_size;

		mp4file->readSizeAndType( esds_size, fourcc );

		// read esds' main parts
		if( size()-48 > 0 )
			SjByteVector flags_version = mp4file->ReadBlock(4);
		else
			return;

		SjByteVector EsDescrTag = mp4file->ReadBlock(1);
		// first 4 bytes contain full box specifics (version & flags)
		// upcoming byte must be ESDescrTag (0x03)
		if( EsDescrTag[0] == 0x03 )
		{
			unsigned int descr_len = mp4file->readSystemsLen();
			unsigned int EsId;
			if( !mp4file->readShort( EsId ) )
				return;
			SjByteVector priority = mp4file->ReadBlock(1);
			if( descr_len < 20 )
				return;
		}
		else
		{
			unsigned int EsId;
			if( !mp4file->readShort( EsId ) )
				return;
		}
		// read decoder configuration tag (0x04)
		SjByteVector DecCfgTag = mp4file->ReadBlock(1);
		if( DecCfgTag[0] != 0x04 )
			return;
		// read decoder configuration length
		// unsigned int deccfg_len = mp4file->readSystemsLen();
		// read object type Id
		SjByteVector objId = mp4file->ReadBlock(1);
		// read stream type id
		SjByteVector strId = mp4file->ReadBlock(1);
		// read buffer Size DB
		SjByteVector bufferSizeDB = mp4file->ReadBlock(3);
		// read max bitrate
		unsigned int max_bitrate;
		if( !mp4file->readInt( max_bitrate ) )
			return;
		// read average bitrate
		if( !mp4file->readInt( m_bitrate ) )
			return;
		// skip the rest
		mp4file->Seek( offset()+size()-8, SJ_SEEK_BEG );
	}
	else
		mp4file->Seek( size()-36, SJ_SEEK_CUR );
}




////////////// MP4_PropsProxy //////////////



MP4_PropsProxy::MP4_PropsProxy()
{
	m_mvhdbox = 0;
	m_audiosampleentry = 0;
}

unsigned int MP4_PropsProxy::seconds() const
{
	if( m_mvhdbox )
	{
		unsigned int timescale = m_mvhdbox->timescale();
		if( timescale == 0 ) return 0;

		return static_cast<unsigned int>( m_mvhdbox->duration() / timescale );
	}
	else
	{
		return 0;
	}
}

unsigned int MP4_PropsProxy::channels() const
{
	if( m_audiosampleentry )
		return m_audiosampleentry->channels();
	else
		return 0;
}

unsigned int MP4_PropsProxy::sampleRate() const
{
	if( m_audiosampleentry )
		return (m_audiosampleentry->samplerate()>>16);
	else
		return 0;
}

unsigned int MP4_PropsProxy::bitRate() const
{
	if( m_audiosampleentry )
		return (m_audiosampleentry->bitrate());
	else
		return 0;
}

void MP4_PropsProxy::registerMvhd( MP4_MvhdBox* mvhdbox )
{
	m_mvhdbox = mvhdbox;
}

void MP4_PropsProxy::registerAudioSampleEntry( MP4_AudioSampleEntry* audioSampleEntry )
{
	m_audiosampleentry = audioSampleEntry;
}


/*******************************************************************************
 * MP4_AudioProperties
 ******************************************************************************/




MP4_AudioProperties::MP4_AudioProperties()
	: Tagger_AudioProperties()
{
	m_propsproxy  = 0;
}

int MP4_AudioProperties::length() const
{
	if( m_propsproxy == 0 )
		return 0;
	return m_propsproxy->seconds();
}

int MP4_AudioProperties::bitrate() const
{
	if( m_propsproxy == 0 )
	{
		return 0;
	}

	unsigned int bitsPerSecond = m_propsproxy->bitRate();
	if( bitsPerSecond <    1000
	        || bitsPerSecond > 1411200 /*bitrate of CD*/ )
	{
		return 0; // funny data - eg. the bitrate of ALAC tracks is not correct
	}

	return bitsPerSecond/1000; // we return kbit/seconds
}

int MP4_AudioProperties::sampleRate() const
{
	if( m_propsproxy == 0 )
		return 0;
	return m_propsproxy->sampleRate();
}

int MP4_AudioProperties::channels() const
{
	if( m_propsproxy == 0 )
		return 0;
	return m_propsproxy->channels();
}



/*******************************************************************************
 * MP4_File
 ******************************************************************************/




class MP4_FilePrivate
{
public:
	//! list for all boxes of the mp4 file (type: MP4_IsoBox)
	MP4_ListOfIsoBoxes            boxes;

	//! proxy for the tags is filled after parsing
	MP4_TagsProxy                 tagsProxy;

	//! the tag returned by tag() function
	MP4_Tag                       mp4tag;

	//! proxy for audio properties
	MP4_PropsProxy                propsProxy;

	//! container for the audio properties returned by properties() function
	MP4_AudioProperties           mp4audioproperties;
};



//! function to fill the tags with converted proxy data, which has been parsed out of the file previously
static void fillTagFromProxy( MP4_TagsProxy& proxy, MP4_Tag& mp4tag );



MP4_File::MP4_File(
    const wxFSFile* file,        // you should give either fsFile
    wxInputStream* inputStream)  // in inputStream, not both!
	:   Tagger_File(file, inputStream)
{
	// create member container
	d = new MP4_FilePrivate();

	unsigned int size;
	MP4_Fourcc  fourcc;

	while( readSizeAndType( size, fourcc ) == true )
	{
		// create the appropriate subclass and parse it
		wxLogDebug(wxT("MP4::readSizeAndType(): %i bytes, %s"), (int)size, fourcc.toString().c_str());

		MP4_IsoBox* curbox = MP4_IsoBox::createInstance( this, fourcc, size, Tell() );
		wxASSERT( curbox );

		curbox->parsebox();
		d->boxes.Add(curbox);
	}

	/*
	int i, iCount = d->boxes.GetCount();
	for ( i = 0; i < iCount; i++ )
	{
	    MP4_IsoBox* curbox = (MP4_IsoBox*)d->boxes.Item(i);
	    if( curbox->fourcc() == MP4_Fourcc(wxT("moov")) )
	    {
	      d->IsValid = true;
	      break;
	    }
	}
	*/

	// fill tags from proxy data
	fillTagFromProxy( d->tagsProxy, d->mp4tag );
}



MP4_File::~MP4_File()
{
	delete d;
}




Tagger_Tag* MP4_File::tag() const
{
	return &d->mp4tag;
}



MP4_Tag* MP4_File::MP4Tag() const
{
	return &d->mp4tag;
}



Tagger_AudioProperties * MP4_File::audioProperties() const
{
	d->mp4audioproperties.setProxy( &d->propsProxy );
	return &d->mp4audioproperties;
}




bool MP4_File::readSizeAndType( unsigned int& size, MP4_Fourcc& fourcc )
{
	// read the two blocks from file
	SjByteVector readsize = ReadBlock(4);
	SjByteVector readtype = ReadBlock(4);

	if( (readsize.size() != 4) || (readtype.size() != 4) )
		return false;

	// set size
	size = (unsigned int)(readsize[0]) << 24 |
	       (unsigned int)(readsize[1]) << 16 |
	       (unsigned int)(readsize[2]) <<  8 |
	       (unsigned int)(readsize[3]);

	// type and size seem to be part of the stored size
	if( size < 8 )
		return false;

	// set fourcc
	fourcc = readtype.getReadableData();

	return true;
}



bool MP4_File::readInt( unsigned int& toRead )
{
	SjByteVector readbuffer = ReadBlock(4);
	if( readbuffer.size() != 4 )
		return false;

	toRead = static_cast<unsigned char>(readbuffer[0]) << 24 |
	         static_cast<unsigned char>(readbuffer[1]) << 16 |
	         static_cast<unsigned char>(readbuffer[2]) <<  8 |
	         static_cast<unsigned char>(readbuffer[3]);
	return true;
}

bool MP4_File::readShort( unsigned int& toRead )
{
	SjByteVector readbuffer = ReadBlock(2);
	if( readbuffer.size() != 2 )
		return false;

	toRead = static_cast<unsigned char>(readbuffer[0]) <<  8 |
	         static_cast<unsigned char>(readbuffer[1]);
	return true;
}

bool MP4_File::readLongLong( unsigned wxLongLong_t& toRead )
{
	SjByteVector readbuffer = ReadBlock(8);
	if( readbuffer.size() != 8 )
		return false;

	toRead = static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[0])) << 56 |
	         static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[1])) << 48 |
	         static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[2])) << 40 |
	         static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[3])) << 32 |
	         static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[4])) << 24 |
	         static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[5])) << 16 |
	         static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[6])) <<  8 |
	         static_cast<unsigned wxLongLong_t>(static_cast<unsigned char>(readbuffer[7]));
	return true;
}



bool MP4_File::readFourcc( MP4_Fourcc& fourcc )
{
	SjByteVector readtype = ReadBlock(4);

	if( readtype.size() != 4)
		return false;

	// set fourcc
	fourcc = readtype.getReadableData();

	return true;
}



unsigned int MP4_File::readSystemsLen()
{
	unsigned int length = 0;
	unsigned int nbytes = 0;
	SjByteVector   input;
	unsigned char tmp_input;

	do
	{
		input = ReadBlock(1);
		tmp_input = static_cast<unsigned char>(input[0]);
		nbytes++;
		length = (length<<7) | (tmp_input&0x7F);
	} while( (tmp_input&0x80) && (nbytes<4) );

	return length;
}



MP4_TagsProxy* MP4_File::tagProxy() const
{
	return &d->tagsProxy;
}




MP4_PropsProxy* MP4_File::propProxy() const
{
	return &d->propsProxy;
}



void fillTagFromProxy( MP4_TagsProxy& proxy, MP4_Tag& mp4tag )
{
	// tmp buffer for each tag
	MP4_ITunesDataBox* databox;

	databox = proxy.titleData();
	if( databox != 0 )
	{
		// convert data to string
		wxString datastring = databox->data().toString(SJ_UTF8);
		// check if string was set
		if( !datastring.IsEmpty() )
			mp4tag.setTitle( datastring );
	}

	databox = proxy.artistData();
	if( databox != 0 )
	{
		// convert data to string
		wxString datastring = databox->data().toString(SJ_UTF8);
		// check if string was set
		if( !datastring.IsEmpty() )
			mp4tag.setLeadArtist( datastring );
	}

	databox = proxy.albumData();
	if( databox != 0 )
	{
		// convert data to string
		wxString datastring = databox->data().toString(SJ_UTF8);
		// check if string was set
		if( !datastring.IsEmpty() )
			mp4tag.setAlbum( datastring );
	}

	databox = proxy.genreData();
	if( databox != 0 )
	{
		// convert data to string
		SjByteVector data = databox->data();
		wxString datastring;

		// try to set by string
		if( data[0] )
		{
			datastring = data.toString(SJ_UTF8);
		}

		// set by ID - the size is normally two bytes;
		// we assume the 2nd byte to contain an index (bp)
		if( datastring.IsEmpty() && data.size() == 2 )
		{
			int genreId3v1 = data[1] - 1;
			datastring = ID3v1_Tag::lookupGenreName(genreId3v1);
		}

		// check if string was set
		if( !datastring.IsEmpty() )
			mp4tag.setGenre( datastring );
	}

	databox = proxy.yearData();
	if( databox != 0 )
	{
		// convert data to string
		wxString datastring = databox->data().toString(SJ_UTF8);
		// check if string was set
		if( !datastring.IsEmpty() )
		{
			long l;
			if( datastring.ToLong(&l) )
				mp4tag.setYear( l );
		}
	}

	databox = proxy.trknData();
	if( databox != 0 )
	{
		// convert data to uint
		SjByteVector datavec = databox->data();
		if( datavec.size() >= 4 )
		{
			unsigned int trackno = ( /*unsigned char(datavec[0]) << 24 |  <-- The Tag'Lib Mp4 implementation
                                                                        uses 32 bits for the track number;
                                                                        however, the track count seems to
                                                                        be only 16 bit in the bytes 4+5;
                                                                        so I think, the track number should
                                                                        also use only 16 bits (makes no sense
                                                                        to have a larger track count than
                                                                        possible track numbers) and the
                                                                        bytes 0+1 are used for sth. else
                                                                        or a reserved. (bp)
                               unsigned char(datavec[1]) << 16 |*/
			                           (unsigned int)(datavec[2]) <<  8 |
			                           (unsigned int)(datavec[3]) );
			unsigned int trackct = ( (unsigned int)(datavec[4]) <<  8 |
			                         (unsigned int)(datavec[5]) );
			mp4tag.setTrack( trackno, trackct );
		}
		else
			mp4tag.setTrack( 0, 0 );
	}

	databox = proxy.commentData();
	if( databox != 0 )
	{
		// convert data to string
		wxString datastring = databox->data().toString(SJ_UTF8);
		// check if string was set
		if( !datastring.IsEmpty() )
			mp4tag.setComment( datastring );
	}

	databox = proxy.groupingData();
	if( databox != 0 )
	{
		// convert data to string
		wxString datastring = databox->data().toString(SJ_UTF8);
		// check if string was set
		if( !datastring.IsEmpty() )
			mp4tag.setGroup( datastring );
	}

	databox = proxy.composerData();
	if( databox != 0 )
	{
		// convert data to string
		wxString datastring = databox->data().toString(SJ_UTF8);
		// check if string was set
		if( !datastring.IsEmpty() )
			mp4tag.setComposer( datastring );
	}

	databox = proxy.diskData();
	if( databox != 0 )
	{
		// convert data to uint
		SjByteVector datavec = databox->data();
		if( datavec.size() >= 4 )
		{
			unsigned int discno = (   /*unsigned char(datavec[0]) << 24 | -- see commen for track number
                                unsigned char(datavec[1]) << 16 |*/
			                          (unsigned int)(datavec[2]) <<  8 |
			                          (unsigned int)(datavec[3]) );
			unsigned int discct =   ( (unsigned int)(datavec[4]) <<  8 |
			                          (unsigned int)(datavec[5]) );
			mp4tag.setDisk( discno, discct );
		}
		else
			mp4tag.setDisk( 0, 0 );
	}

	databox = proxy.bpmData();
	if( databox != 0 )
	{
		// convert data to uint
		SjByteVector datavec = databox->data();

		if( datavec.size() >= 2 )
		{
			unsigned int bpm = ( (unsigned int)(datavec[0]) <<  8 |
			                     (unsigned int)(datavec[1]) );
			mp4tag.setBeatsPerMinute( bpm );
		}
		else
			mp4tag.setBeatsPerMinute( 0 );
	}

	databox = proxy.coverData();
	if( databox != 0 )
	{
		// get byte vector
		mp4tag.setCover( databox->data() );
	}
}
