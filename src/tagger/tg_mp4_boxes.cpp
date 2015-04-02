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



/*******************************************************************************
 * MP4_IsoBox
 ******************************************************************************/



MP4_IsoBox* MP4_IsoBox::createInstance(MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset)
{
	if(!file)
		return 0;

	switch( fourcc )
	{
		case 0x6d6f6f76: /* 'moov' */   return new MP4_MoovBox      (file, fourcc, size, offset);   break;
		case 0x6d766864: /* 'mvhd' */   return new MP4_MvhdBox      (file, fourcc, size, offset);   break;
		case 0x7472616b: /* 'trak' */   return new MP4_TrakBox      (file, fourcc, size, offset);   break;
		case 0x6d646961: /* 'mdia' */   return new MP4_MdiaBox      (file, fourcc, size, offset);   break;
		case 0x6d696e66: /* 'minf' */   return new MP4_MinfBox      (file, fourcc, size, offset);   break;
		case 0x7374626c: /* 'stbl' */   return new MP4_StblBox      (file, fourcc, size, offset);   break;
		case 0x73747364: /* 'stsd' */   return new MP4_StsdBox      (file, fourcc, size, offset);   break;
		case 0x68646c72: /* 'hdlr' */   return new MP4_HdlrBox      (file, fourcc, size, offset);   break;
		case 0x75647461: /* 'udta' */   return new MP4_UdtaBox      (file, fourcc, size, offset);   break;
		case 0x6d657461: /* 'meta' */   return new MP4_MetaBox      (file, fourcc, size, offset);   break;
		case 0x696c7374: /* 'ilst' */   return new MP4_IlstBox      (file, fourcc, size, offset);   break;
		case 0xa96e616d: /* '_nam' */   return new MP4_ITunesNamBox (file, fourcc, size, offset);   break;
		case 0xa9415254: /* '_ART' */   return new MP4_ITunesArtBox (file, fourcc, size, offset);   break;
		case 0xa9616c62: /* '_alb' */   return new MP4_ITunesAlbBox (file, fourcc, size, offset);   break;
		case 0xa967656e: /* '_gen' */   return new MP4_ITunesGenBox (file, fourcc, size, offset);   break;
		case 0x676e7265: /* 'gnre' */   return new MP4_ITunesGenBox (file, fourcc, size, offset);   break;
		case 0xa9646179: /* '_day' */   return new MP4_ITunesDayBox (file, fourcc, size, offset);   break;
		case 0x74726b6e: /* 'trkn' */   return new MP4_ITunesTrknBox(file, fourcc, size, offset);   break;
		case 0xa9636d74: /* '_cmt' */   return new MP4_ITunesCmtBox (file, fourcc, size, offset);   break;
		case 0xa9677270: /* '_grp' */   return new MP4_ITunesGrpBox (file, fourcc, size, offset);   break;
		case 0xa9777274: /* '_wrt' */   return new MP4_ITunesWrtBox (file, fourcc, size, offset);   break;
		case 0x6469736b: /* 'disk' */   return new MP4_ITunesDiskBox(file, fourcc, size, offset);   break;
		case 0x746d706f: /* 'tmpo' */   return new MP4_ITunesTmpoBox(file, fourcc, size, offset);   break;
		case 0x636f7672: /* 'covr' */   return new MP4_ITunesCvrBox (file, fourcc, size, offset);   break;
		case 0x64616461: /* 'data' */   return new MP4_ITunesDataBox(file, fourcc, size, offset);   break;
		default:                        return new MP4_SkipBox      (file, fourcc, size, offset);   break;
	}
}



MP4_IsoBox::MP4_IsoBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
{
	m_file   = file;
	m_fourcc = fourcc;
	m_size   = size;
	m_offset = offset;
}



void MP4_IsoBox::parsebox()
{
	// seek to offset
	file()->Seek( offset(), SJ_SEEK_BEG );
	// simply call parse method of sub class
	parse();
}



MP4_ListOfIsoBoxes::~MP4_ListOfIsoBoxes()
{
	int i, iCount = m_arr.GetCount();
	for ( i = 0; i < iCount; i++ )
	{
		MP4_IsoBox* curbox = (MP4_IsoBox*)m_arr.Item(i);
		delete curbox;
	}
}



/*******************************************************************************
 * MP4_IsoFullBox
 ******************************************************************************/




MP4_IsoFullBox::MP4_IsoFullBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}

void  MP4_IsoFullBox::parsebox()
{
	// seek to offset
	MP4_IsoBox::file()->Seek(MP4_IsoBox::offset(), SJ_SEEK_BEG );
	// parse version and flags
	SjByteVector version_flags = MP4_IsoBox::file()->ReadBlock(4);
	m_version = version_flags[0];
	m_flags   = version_flags[1] << 16 || version_flags[2] << 8 || version_flags[3];
	// call parse method of subclass
	parse();
}




/*******************************************************************************
 * MP4_SkipBox
 ******************************************************************************/



MP4_SkipBox::MP4_SkipBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	:MP4_IsoBox(file, fourcc, size, offset)
{
}

//! parse the content of the box
void MP4_SkipBox::parse()
{
	// skip contents
	file()->Seek( size() - 8, SJ_SEEK_CUR );
}




/*******************************************************************************
 * MP4_MoovBox
 ******************************************************************************/




MP4_MoovBox::MP4_MoovBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}

void MP4_MoovBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 8;
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;

	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		wxLogDebug(wxT("MP4_MoovBox::parse(): %i bytes, %s"), (int)size, fourcc.toString().c_str());

		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file: moov box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );
		wxASSERT( curbox );

		curbox->parsebox();
		m_moovBoxes.Add( curbox );

		// check for end of moov box
		if( totalsize == MP4_IsoBox::size() )
			break;
	}
}




/*******************************************************************************
 * MP4_MvhdBox
 ******************************************************************************/



MP4_MvhdBox::MP4_MvhdBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoFullBox( file, fourcc, size, offset )
{
	m_creationTime = 0;
	m_modificationTime = 0;
	m_timescale = 0;
	m_duration = 0;
	m_rate = 0;
	m_volume = 0;
	m_nextTrackID = 0;
}

void MP4_MvhdBox::parse()
{
	MP4_File* mp4file = file();

	if( version() == 1 )
	{
		if( !mp4file->readLongLong( m_creationTime ) )
			return;
		if( !mp4file->readLongLong( m_modificationTime ) )
			return;
		if( !mp4file->readInt( m_timescale ) )
			return;
		if( !mp4file->readLongLong( m_duration ) )
			return;
	}
	else
	{
		unsigned int creationTime_tmp, modificationTime_tmp, duration_tmp;

		if( !mp4file->readInt( creationTime_tmp ) )
			return;
		if( !mp4file->readInt( modificationTime_tmp ) )
			return;
		if( !mp4file->readInt( m_timescale ) )
			return;
		if( !mp4file->readInt( duration_tmp ) )
			return;

		m_creationTime     = creationTime_tmp;
		m_modificationTime = modificationTime_tmp;
		m_duration         = duration_tmp;
	}
	if( !mp4file->readInt( m_rate ) )
		return;
	if( !mp4file->readInt( m_volume ) )
		return;
	// jump over unused fields
	mp4file->Seek( 68, SJ_SEEK_CUR );

	if( !mp4file->readInt( m_nextTrackID ) )
		return;
	// register at proxy
	mp4file->propProxy()->registerMvhd( this );
}




/*******************************************************************************
 * MP4_TrakBox
 ******************************************************************************/



MP4_TrakBox::MP4_TrakBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}

void MP4_TrakBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 8;
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;

	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file trak box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );
		curbox->parsebox();
		m_trakBoxes.Add( curbox );

		// check for end of trak box
		if( totalsize == MP4_IsoBox::size() )
			break;
	}
}



/*******************************************************************************
 * MP4_MdiaBox
 ******************************************************************************/



MP4_MdiaBox::MP4_MdiaBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}

void MP4_MdiaBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 8;
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;

	// stores the current handler type
	MP4_Fourcc hdlrtype;

	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file mdia box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );
		if( static_cast<unsigned int>( fourcc ) == 0x6d696e66 /*"minf"*/ )
		{
			// cast to minf
			MP4_MinfBox* minfbox = (MP4_MinfBox*)curbox;

			// set handler type
			minfbox->setHandlerType( hdlrtype );
		}

		curbox->parsebox();
		m_mdiaBoxes.Add( curbox );

		if(static_cast<unsigned int>( fourcc ) == 0x68646c72 /*"hdlr"*/ )
		{
			// cast to hdlr box
			MP4_HdlrBox* hdlrbox = (MP4_HdlrBox*)curbox;

			// get handler type
			hdlrtype = hdlrbox->hdlr_type();
		}
		// check for end of mdia box
		if( totalsize == MP4_IsoBox::size() )
			break;

	}
}




/*******************************************************************************
 * MP4_MinfBox
 ******************************************************************************/


MP4_MinfBox::MP4_MinfBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}

void MP4_MinfBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 8;
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;

	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file minf box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );
		if(static_cast<unsigned int>( fourcc ) == 0x7374626c /*stbl*/ )
		{
			// cast to hdlr box
			MP4_StblBox* stblbox = (MP4_StblBox*)curbox;

			// set handler type
			stblbox->setHandlerType( m_handler_type );
		}

		curbox->parsebox();
		m_minfBoxes.Add( curbox );

		// check for end of minf box
		if( totalsize == MP4_IsoBox::size() )
			break;
	}
}


/*******************************************************************************
 * MP4_StblBox
 ******************************************************************************/



MP4_StblBox::MP4_StblBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}

void MP4_StblBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 8;
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;

	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file stbl box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );

		// check for stsd
		if( static_cast<unsigned int>(fourcc) == 0x73747364 /*'stsd'*/ )
		{
			// cast to stsd box
			MP4_StsdBox* stsdbox = (MP4_StsdBox*)curbox;
			// set the handler type
			stsdbox->setHandlerType( m_handler_type );
		}
		curbox->parsebox();
		m_stblBoxes.Add( curbox );

		// check for end of stbl box
		if( totalsize == MP4_IsoBox::size() )
			break;
	}
}



/*******************************************************************************
 * MP4_StsdBox
 ******************************************************************************/


MP4_StsdBox::MP4_StsdBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoFullBox( file, fourcc, size, offset )
{
	m_audioSampleEntry = NULL;
}

MP4_StsdBox::~MP4_StsdBox()
{
	if( m_audioSampleEntry )
		delete m_audioSampleEntry;
}

void MP4_StsdBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 12; // initial size of box

	// check for handler type - only parse if 'soun':
	if( static_cast<unsigned int>(m_handler_type) == 0x736f756e )
	{
		// read entry count
		unsigned int entry_count;
		if(!mp4file->readInt( entry_count ))
			return;

		// simply read first entry and skip all following
		// read size and type
		unsigned int cursize;
		MP4_Fourcc  fourcc;
		if( !mp4file->readSizeAndType( cursize, fourcc ))
			return;

		totalsize += 12;
		// alocate an AudioSampleEntry
		m_audioSampleEntry = new MP4_AudioSampleEntry( mp4file, fourcc, cursize, mp4file->Tell() );
		// parse the AudioSampleEntry
		m_audioSampleEntry->parse();
		totalsize += cursize-8;
		// skip the remaining box contents
		mp4file->Seek( size()-totalsize, SJ_SEEK_CUR );
	}
	else
	{
		mp4file->Seek( size()-totalsize, SJ_SEEK_CUR );
	}
}


/*******************************************************************************
 * MP4_HdlrBox
 ******************************************************************************/




MP4_HdlrBox::MP4_HdlrBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoFullBox( file, fourcc, size, offset )
{
}

void MP4_HdlrBox::parse()
{
	unsigned int totalread = 12+20;

	MP4_File* mp4file = file();
	if( mp4file->readInt( m_pre_defined ) == false )
		return;
	if( mp4file->readFourcc( m_handler_type ) == false )
		return;

	// read reserved into trash
	mp4file->Seek( 3*4, SJ_SEEK_CUR );

	// check if there are bytes remaining - used for hdlr string
	if( size() - totalread != 0 )
		m_hdlr_string = mp4file->ReadBlock( size()-totalread ).toString(SJ_UTF8);
}



/*******************************************************************************
 * MP4_UdtaBox
 ******************************************************************************/





MP4_UdtaBox::MP4_UdtaBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}


void MP4_UdtaBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 8;
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;

	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file udta box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );
		curbox->parsebox();
		m_udtaBoxes.Add( curbox );

		// check for end of udta box
		if( totalsize == MP4_IsoBox::size() )
			break;
	}
}



/*******************************************************************************
 * MP4_MetaBox
 ******************************************************************************/



MP4_MetaBox::MP4_MetaBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoFullBox( file, fourcc, size, offset )
{
}

void MP4_MetaBox::parse()
{
	MP4_File* mp4file = file();

	unsigned int totalsize = 12; // initial size of box
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;

	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file meta box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );
		curbox->parsebox();
		m_metaBoxes.Add( curbox );

		// check for end of meta box
		if( totalsize == MP4_IsoBox::size() )
			break;
	}
}




/*******************************************************************************
 * MP4_IlstBox
 ******************************************************************************/



MP4_IlstBox::MP4_IlstBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox( file, fourcc, size, offset )
{
}


void MP4_IlstBox::parse()
{


	MP4_File* mp4file = file();

	unsigned int totalsize = 8;
	// parse all contained boxes
	unsigned int size;
	MP4_Fourcc  fourcc;


	while( (mp4file->readSizeAndType( size, fourcc ) == true)  )
	{
		totalsize += size;

		// check for errors
		if( totalsize > MP4_IsoBox::size() )
		{
			wxLogDebug(wxT("Error in mp4 file ilst box contains bad box with name: %s"), fourcc.toString().c_str());
			return;
		}

		// create the appropriate subclass and parse it
		MP4_IsoBox* curbox = createInstance( mp4file, fourcc, size, mp4file->Tell() );
		curbox->parsebox();
		m_ilstBoxes.Add( curbox );

		// check for end of ilst box
		if( totalsize == MP4_IsoBox::size() )
			break;


	}
}



/*******************************************************************************
 * MP4_ITunesDataBox
 ******************************************************************************/



MP4_ITunesDataBox::MP4_ITunesDataBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	:MP4_IsoFullBox(file, fourcc, size, offset)
{
}


//! parse the content of the box
void MP4_ITunesDataBox::parse()
{
	// skip first 4 byte - don't know what they are supposed to be for - simply 4 zeros
	file()->Seek( 4, SJ_SEEK_CUR );
	// read contents - remaining size is box_size-12-4 (12:fullbox header, 4:starting zeros of data box)
	m_data = file()->ReadBlock( size()-12-4 );
}




/*******************************************************************************
 * MP4_ITunesNamBox
 ******************************************************************************/




MP4_ITunesNamBox::MP4_ITunesNamBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesNamBox::~MP4_ITunesNamBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesNamBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesNamBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_title, m_dataBox );

}


/*******************************************************************************
 * MP4_ITunesArtBox
 ******************************************************************************/



MP4_ITunesArtBox::MP4_ITunesArtBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesArtBox::~MP4_ITunesArtBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesArtBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesArtBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_artist, m_dataBox );

}




/*******************************************************************************
 * MP4_ITunesAlbBox
 ******************************************************************************/



MP4_ITunesAlbBox::MP4_ITunesAlbBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesAlbBox::~MP4_ITunesAlbBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesAlbBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesAlbBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_album, m_dataBox );

}


/*******************************************************************************
 * MP4_ITunesGenBox
 ******************************************************************************/




MP4_ITunesGenBox::MP4_ITunesGenBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesGenBox::~MP4_ITunesGenBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesGenBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesGenBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_genre, m_dataBox );

}



/*******************************************************************************
 * MP4_ITunesDayBox
 ******************************************************************************/



MP4_ITunesDayBox::MP4_ITunesDayBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesDayBox::~MP4_ITunesDayBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesDayBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesDayBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_year, m_dataBox );

}



/*******************************************************************************
 * MP4_ITunesTrknBox
 ******************************************************************************/



MP4_ITunesTrknBox::MP4_ITunesTrknBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesTrknBox::~MP4_ITunesTrknBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesTrknBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesTrknBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_trackno, m_dataBox );

}



/*******************************************************************************
 * MP4_ITunesCmtBox
 ******************************************************************************/



MP4_ITunesCmtBox::MP4_ITunesCmtBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesCmtBox::~MP4_ITunesCmtBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesCmtBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesCmtBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_comment, m_dataBox );

}




/*******************************************************************************
 * MP4_ITunesGrpBox
 ******************************************************************************/



MP4_ITunesGrpBox::MP4_ITunesGrpBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesGrpBox::~MP4_ITunesGrpBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesGrpBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesGrpBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_grouping, m_dataBox );

}




/*******************************************************************************
 * MP4_ITunesWrtBox
 ******************************************************************************/



MP4_ITunesWrtBox::MP4_ITunesWrtBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesWrtBox::~MP4_ITunesWrtBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesWrtBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesWrtBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_composer, m_dataBox );

}


/*******************************************************************************
 * MP4_ITunesDiskBox
 ******************************************************************************/



MP4_ITunesDiskBox::MP4_ITunesDiskBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesDiskBox::~MP4_ITunesDiskBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesDiskBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesDiskBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_disk, m_dataBox );

}



/*******************************************************************************
 * MP4_ITunesTmpoBox
 ******************************************************************************/



MP4_ITunesTmpoBox::MP4_ITunesTmpoBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesTmpoBox::~MP4_ITunesTmpoBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesTmpoBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesTmpoBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_bpm, m_dataBox );

}


/*******************************************************************************
 * MP4_ITunesCvrBox
 ******************************************************************************/



MP4_ITunesCvrBox::MP4_ITunesCvrBox( MP4_File* file, MP4_Fourcc fourcc, unsigned int size, long offset )
	: MP4_IsoBox(file, fourcc, size, offset)
{
	m_dataBox = 0;
}

MP4_ITunesCvrBox::~MP4_ITunesCvrBox()
{
	if( m_dataBox != 0 )
		delete m_dataBox;

}

//! parse the content of the box
void MP4_ITunesCvrBox::parse()
{
	MP4_File* mp4file = file();

	// parse data box
	unsigned int size;
	MP4_Fourcc  fourcc;

	if(mp4file->readSizeAndType( size, fourcc ) == true)
	{
		// check for type - must be 'data'
		if( fourcc != MP4_Fourcc(wxT("data")) )
		{
			wxLogDebug(wxT("bad atom in itunes tag - skipping it."));
			// jump over data tag
			mp4file->Seek( size-8, SJ_SEEK_CUR );
			return;
		}
		m_dataBox = new MP4_ITunesDataBox( mp4file, fourcc, size, mp4file->Tell() );
		m_dataBox->parsebox();
	}
	else
	{
		// reading unsuccessful - serious error!
		wxLogDebug(wxT("Error in parsing ITunesCvrBox!"));
		return;
	}
	// register data box
	mp4file->tagProxy()->registerBox( MP4_EBoxType_cover, m_dataBox );

}



/*******************************************************************************
 * MP4_Fourcc
 ******************************************************************************/



MP4_Fourcc::MP4_Fourcc()
{
	m_fourcc = 0U;
}

MP4_Fourcc::MP4_Fourcc( const wxString& fourcc )
{
	m_fourcc = 0U;

	if( fourcc.Len() >= 4 )
		m_fourcc = (unsigned int)(fourcc[0]) << 24 |
		           (unsigned int)(fourcc[1]) << 16 |
		           (unsigned int)(fourcc[2]) <<  8 |
		           (unsigned int)(fourcc[3]);
}

wxString MP4_Fourcc::toString() const
{
	wxString fourcc;
	fourcc.Append(wxChar(m_fourcc >> 24 & 0xFF));
	fourcc.Append(wxChar(m_fourcc >> 16 & 0xFF));
	fourcc.Append(wxChar(m_fourcc >>  8 & 0xFF));
	fourcc.Append(wxChar(m_fourcc       & 0xFF));

	return fourcc;
}

MP4_Fourcc::operator unsigned int() const
{
	return m_fourcc;
}

bool MP4_Fourcc::operator == (unsigned int fourccB ) const
{
	return (m_fourcc==fourccB);
}

bool MP4_Fourcc::operator != (unsigned int fourccB ) const
{
	return (m_fourcc!=fourccB);
}

MP4_Fourcc& MP4_Fourcc::operator = (unsigned int fourcc )
{
	m_fourcc = fourcc;
	return *this;
}

MP4_Fourcc& MP4_Fourcc::operator = (const unsigned char fourcc[4])
{
	m_fourcc = (unsigned int)(fourcc[0]) << 24 |
	           (unsigned int)(fourcc[1]) << 16 |
	           (unsigned int)(fourcc[2]) <<  8 |
	           (unsigned int)(fourcc[3]);
	return *this;
}
