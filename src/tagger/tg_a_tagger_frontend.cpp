/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2015 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    tg_a_tagger_frontend.cpp
 * Authors: Björn Petersen
 * Purpose: The Virtual ID3 File System for reading Images eg.
 *          directly from MP3 files
 *
 ******************************************************************************/


#include "tg_tagger_base.h"
#include <wx/mstream.h>
#include "tg_a_tagger_frontend.h"
#include "tg_mpeg_file.h"
#include "tg_wma_file.h"
#include "tg_oggvorbis_file.h"
#include "tg_flac_file.h"
#include "tg_mpc_file.h"
#include "tg_monkeys_file.h"
#include "tg_wavpack_file.h"
#include "tg_mp4_file.h"
#include "tg_id3v1_tag.h"
#include "tg_id3v2_tag.h"
#include "tg_id3v2_knownframes.h"
#include "tg_ape_tag.h"


/*******************************************************************************
 * Open a Tagger file
 ******************************************************************************/


enum SjFileType
{
    SJ_FT_UNKNOWN = 0
                    ,   SJ_FT_KNOWN_UNSUPPORTED
    ,   SJ_FT_MP1_2_3
    ,   SJ_FT_OGG_VORBIS
    ,   SJ_FT_FLAC
    ,   SJ_FT_WMA
    ,   SJ_FT_MUSEPACK
    ,   SJ_FT_MONKEYS_AUDIO
    ,   SJ_FT_WAVPACK
    ,   SJ_FT_MP4
};


Tagger_Options* g_taggerOptions = NULL;


static Tagger_File* getTaggerFile(const wxString& url, wxFSFile* fsFile, SjFileType& ftOut)
{
	// create globals
	if( g_taggerOptions == NULL )
	{
		g_taggerOptions = new Tagger_Options();
		g_taggerOptions->m_flags = g_tools->m_config->Read("tageditor/tagflags",  SJTF_DEFAULTS);
		g_taggerOptions->m_ratingUser = g_tools->m_config->Read("tageditor/ratinguser",  "r@silverjuke.net");
	}

	// create file
	Tagger_File* file = NULL;

	// get file type by extension
	bool mpegTried = FALSE;
	ftOut = SJ_FT_UNKNOWN;
	wxString ext = SjTools::GetExt(url);
	if( ext == wxT("mp1") || ext == wxT("mp2") || ext == wxT("mp3") || ext == wxT("mp3pro") )
	{
		file = new MPEG_File(fsFile, NULL, Tagger_ReadTags|Tagger_ReadAudioProperties);
		ftOut = SJ_FT_MP1_2_3;
		mpegTried = TRUE;
	}
	else if( ext == wxT("ogg") )
	{
		file = new OggVorbis_File(fsFile);
		ftOut = SJ_FT_OGG_VORBIS;
	}
	else if( ext == wxT("fla") || ext == wxT("flac") )
	{
		file = new FLAC_File(fsFile);
		ftOut = SJ_FT_FLAC;
	}
	else if( ext == wxT("wma") || ext == wxT("wmv") || ext == wxT("asf") )
	{
		file = new WMA_File(fsFile);
		ftOut = SJ_FT_WMA;
	}
	else if( ext == wxT("mp+") || ext == wxT("mpc") || ext == wxT("mpp") )
	{
		file = new MPC_File(fsFile);
		ftOut = SJ_FT_MUSEPACK;
	}
	else if( ext == wxT("ape") )
	{
		file = new Monkeys_File(fsFile);
		ftOut = SJ_FT_MONKEYS_AUDIO;
	}
	else if( ext == wxT("wv") )
	{
		file = new WavPack_File(fsFile);
		ftOut = SJ_FT_WAVPACK;
	}
	else if( ext == wxT("m4a") )
	{
		file = new MP4_File(fsFile);
		ftOut = SJ_FT_MP4;
	}

	if( file != NULL && !file->IsValid() )
	{
		delete file;
		file = NULL;
	}

	// final try with MPEG type
	if( file == NULL && !mpegTried )
	{
		file = new MPEG_File(fsFile, NULL, Tagger_ReadTags|Tagger_ReadAudioProperties);
		if( file->IsValid()
		 && file->audioProperties()
		 && ((MPEG_Properties*)file->audioProperties())->IsValid() )
		{
			ftOut = SJ_FT_MP1_2_3;
		}
		else
		{
			delete file;
			file = NULL;
		}
	}

	// done
	return file;
}



/*******************************************************************************
 *  SjTaggerFsHandler
 ******************************************************************************/



class SjTaggerInputStream : public wxMemoryInputStream
{
public:
	// a normal memory input stream which will delete the buffer on close

	SjTaggerInputStream(SjByteVector* fileData)
		: wxMemoryInputStream(fileData->getReadableData(), fileData->size())
	{
		wxASSERT( fileData );
		m_fileData = fileData;
	}

	~SjTaggerInputStream()
	{
		wxASSERT( m_fileData );
		delete m_fileData;
	}

private:
	SjByteVector* m_fileData;
};



class SjTaggerFsHandler : public wxFileSystemHandler
{
public:
	bool            CanOpen(const wxString& location) ;
	wxFSFile*       OpenFile(wxFileSystem& fs, const wxString& location);
};



bool SjTaggerFsHandler::CanOpen(const wxString& location)
{
	wxString protocol = GetProtocol(location);
	return (protocol == wxT("id3") || protocol == wxT("mp4"));
}



wxFSFile* SjTaggerFsHandler::OpenFile(wxFileSystem& fs, const wxString& location)
{	// location is eg. "c:/files/any.mp3#id3:image2" ...
	wxString        left  = GetLeftLocation(location);      // ... then left is "c:/files/any.mp3"
	wxString        right = GetRightLocation(location);     // ... and right is "image2.jpg" or eg. "cover.jpg"
	wxString        protocol = GetProtocol(location);       // ... and the protocol is "id3" or "mp4"
	wxInputStream*  inputStream = NULL;
	wxDateTime      modTime;

	if( protocol == wxT("mp4") )
	{
		// Get Cover from MP4Tag
		///////////////////////

		wxFSFile* fsFile = fs.OpenFile(left, wxFS_READ|wxFS_SEEKABLE);
		if( fsFile == NULL ) { return NULL; }
		MP4_File   mp4File(fsFile, NULL);
		modTime = fsFile->GetModificationTime();
		delete fsFile;

		MP4_Tag* mp4tag = mp4File.MP4Tag();
		if( mp4tag == NULL ) { return NULL; }

		// create input stream, the image is just read from memory
		inputStream = new SjTaggerInputStream(new SjByteVector(mp4tag->cover()));
	}
	else
	{
		// Get Cover from ID3v2
		///////////////////////

		// Using wxFSFile and wxID3Reader makes it possible
		// to read tags eg. from zipped files.
		wxFSFile* fsFile = fs.OpenFile(left, wxFS_READ|wxFS_SEEKABLE);
		if( fsFile == NULL ) { return NULL; }
		MPEG_File   mpegFile(fsFile, NULL, Tagger_ReadTags);
		modTime = fsFile->GetModificationTime();
		delete fsFile;

		ID3v2_Tag* id3v2 = mpegFile.ID3v2Tag();
		if( id3v2 == NULL ) { return NULL; }


		// get correct picture frame
		long index;
		right.Replace(wxT("image"), wxT(""));
		if( !right.BeforeFirst('.').ToLong(&index) ) { index = 0; }
		index--; // the filenames start with "1", not with "0"

		const ID3v2_FrameList* frameList = id3v2->frameList(wxT("APIC"));
		if( frameList == NULL ) { return NULL; }

		ID3v2_FrameList::Node* node = frameList->GetFirst();
		ID3v2_AttachedPictureFrame* frame = NULL;
		for( ; node; node = node->GetNext() )
		{
			if( index == 0 )
			{
				frame = (ID3v2_AttachedPictureFrame*)node->GetData();
			}
			index--;
		}
		if( frame == NULL ) { return NULL; }

		// create input stream, the image is just read from memory
		inputStream = new SjTaggerInputStream(new SjByteVector(frame->picture()));
	}

	// any error?
	if( inputStream == NULL )
		return NULL;

	// done so far
	return new wxFSFile(    inputStream,
	                        location,
	                        GetMimeTypeFromExt(location), // we open the images using wxBITMAP_TYPE_ANY, so mime or ext. are cosmetically only
	                        GetAnchor(location),
	                        modTime  );
}



/*******************************************************************************
 *  globally init the usage of ID3Etc
 ******************************************************************************/



void SjInitID3Etc(bool initFsHandler)
{
	if( initFsHandler )
	{
		wxFileSystem::AddHandler(new SjTaggerFsHandler);
	}
}



void SjExitID3Etc()
{
	Tagger_File::exitLibrary();
}




/*******************************************************************************
 *  get track information to a stream defined by a wxFSFile object
 ******************************************************************************/



SjResult SjGetTrackInfoFromID3Etc(wxFSFile* fsFile, SjTrackInfo& ti, long flags)
{
	wxASSERT(fsFile);
	wxString url = fsFile->GetLocation();

	// crash handling
	wxLogDebug(wxT("reading tags from %s..."), url.c_str());

	if( !g_tools->CrashPrecaution(wxT("Tagger"), wxT("Read"), url) )
	{
		wxLogDebug(wxT("%s: ID3-Tags not read due possible crash."), url.c_str());
		return SJ_SUCCESS_BUT_NO_DATA;
	}

	// set URL
	ti.m_url = url;

	// get tag...
	SjFileType fileTypeOut;
	Tagger_File* file = getTaggerFile(url, fsFile, fileTypeOut);
	if( file )
	{
		Tagger_Tag* tag = file->tag();
		if( tag )
		{
			// get lead artist name
			ti.m_leadArtistName = tag->leadArtist().Trim(TRUE).Trim(FALSE);
			if( !ti.m_leadArtistName.IsEmpty() )
				ti.m_validFields |= SJ_TI_LEADARTISTNAME;

			// get track name
			ti.m_trackName = tag->title().Trim(TRUE).Trim(FALSE);
			if( !ti.m_trackName.IsEmpty() )
				ti.m_validFields |= SJ_TI_TRACKNAME;

			// get album name
			ti.m_albumName = tag->album().Trim(TRUE).Trim(FALSE);
			if( !ti.m_albumName.IsEmpty() )
				ti.m_validFields |= SJ_TI_ALBUMNAME;
		}

		Tagger_AudioProperties* prop = file->audioProperties();
		if( prop )
		{
			// get playing time, samplerate etc.
			ti.m_playtimeMs  = prop->length()*1000;
			ti.m_samplerate  = prop->sampleRate();
			ti.m_bitrate     = prop->bitrate()*1000;
			ti.m_channels    = prop->channels();
		}

		if( flags & SJ_TI_QUICKINFO )
		{
			// for "quickinfo", we're done here
			delete file;
			return SJ_SUCCESS;
		}

		if( tag )
		{
			// get the original artist and the composer
			ti.m_orgArtistName = tag->orgArtist().Trim(TRUE).Trim(FALSE);
			if( !ti.m_orgArtistName.IsEmpty() ) ti.m_validFields |= SJ_TI_ORGARTISTNAME;

			ti.m_composerName = tag->composer().Trim(TRUE).Trim(FALSE);
			if( !ti.m_composerName.IsEmpty() ) ti.m_validFields |= SJ_TI_COMPOSERNAME;

			// get the genre, the group and the comment
			ti.m_genreName = tag->genre().Trim(TRUE).Trim(FALSE);
			if( !ti.m_genreName.IsEmpty() ) ti.m_validFields |= SJ_TI_GENRENAME;

			ti.m_groupName = tag->group().Trim(TRUE).Trim(FALSE);
			if( !ti.m_groupName.IsEmpty() ) ti.m_validFields |= SJ_TI_GROUPNAME;

			ti.m_comment = tag->comment().Trim(TRUE).Trim(FALSE);
			if( !ti.m_comment.IsEmpty() ) ti.m_validFields |= SJ_TI_COMMENT;

			// get the year
			ti.m_year = tag->year();
			if( ti.m_year > 0 ) ti.m_validFields |= SJ_TI_YEAR;

			// get the track and the disk numbers
			tag->track(ti.m_trackNr, ti.m_trackCount);
			if( ti.m_trackNr > 0 ) ti.m_validFields |= SJ_TI_TRACKNR;
			if( ti.m_trackCount > 0 ) ti.m_validFields |= SJ_TI_TRACKCOUNT;

			tag->disk(ti.m_diskNr, ti.m_diskCount);
			if( ti.m_diskNr > 0 ) ti.m_validFields |= SJ_TI_DISKNR;
			if( ti.m_diskCount > 0 ) ti.m_validFields |= SJ_TI_DISKCOUNT;

			// get the beats per minute
			ti.m_beatsPerMinute = tag->beatsPerMinute();
			if( ti.m_beatsPerMinute > 0 ) ti.m_validFields |= SJ_TI_BEATSPERMINUTE;

			// get the rating
			if( g_taggerOptions->m_flags&SJTF_READ_RATINGS )
			{
				ti.m_rating = tag->rating();
				wxASSERT( ti.m_rating >= 0 && ti.m_rating <= 5 );
				if( ti.m_rating > 0 ) ti.m_validFields |= SJ_TI_RATING;
			}
		}

		// check for special tags (ID3v2, MP4)
		ID3v2_Tag*  id3v2 = NULL;
		MP4_Tag*    mp4tag = NULL;
		if( fileTypeOut == SJ_FT_MP1_2_3 )
		{
			id3v2 = ((MPEG_File*)file)->ID3v2Tag();
		}
		else if( fileTypeOut == SJ_FT_FLAC )
		{
			id3v2 = ((FLAC_File*)file)->ID3v2Tag();
		}
		else if( fileTypeOut == SJ_FT_MP4 )
		{
			mp4tag = ((MP4_File*)file)->MP4Tag();
		}

		// check for embedded pictures
		if( id3v2 )
		{
			const ID3v2_FrameList*  frameList = id3v2->frameList(wxT("APIC"));
			if( frameList != NULL )
			{
				int                         index = 0;
				ID3v2_FrameList::Node*      node = frameList->GetFirst();
				ID3v2_AttachedPictureFrame* frame;
				wxString                    ext;
				for( ; node; node = node->GetNext() )
				{
					frame = (ID3v2_AttachedPictureFrame*)node->GetData();
					if( frame->picture().size() > 0 )
					{
						ext = frame->mimeType().Trim(TRUE).Trim(FALSE);
						ext = ext.AfterLast('/');
						if( ext.IsEmpty() || ext == wxT("jpeg") ) ext = wxT("jpg");

						// add to list of arts
						ti.AddArt(wxString::Format(wxT("%s#id3:image%i.%s"), url.c_str(), (int)(index+1)/*let the name start with "1"*/, ext.c_str()));
					}

					index++;
				}
			}
		}
		else if( mp4tag )
		{
			if( mp4tag->cover().size() > 128 )
			{
				// add cover
				ti.AddArt(wxString::Format(wxT("%s#mp4:cover.jpg"), url.c_str()));
			}
		}

		delete file;
	}


	return SJ_SUCCESS;
}




void SjGetMoreInfoFromID3Etc(wxFSFile* fsFile, SjProp& prop)
{
	wxString url = fsFile->GetLocation(), temp;

	// crash handling
	if( !g_tools->CrashPrecaution(wxT("Tagger"), wxT("Read"), url) )
	{
		wxLogDebug(wxT("%s: ID3-Tags not read due possible crash."), url.c_str());
		return;
	}

	// get tag...
	SjFileType fileTypeOut;
	Tagger_File* file = getTaggerFile(url, fsFile, fileTypeOut);
	if( file )
	{
		// set up the different tags and properties
		ID3v1_Tag*              id3v1       = NULL;
		ID3v2_Tag*              id3v2       = NULL;
		APE_Tag*                ape         = NULL;
		Ogg_XiphComment*        xiph        = NULL;
		MP4_Tag*                mp4tag      = NULL;
		Tagger_AudioProperties* audioProp   = file->audioProperties();
		MPEG_Properties*        mpegProp    = NULL;
		MPC_Properties*         mpcProp     = NULL;
		OggVorbis_Properties*   oggvProp    = NULL;
		WMA_Properties*         wmaProp     = NULL;

		if( fileTypeOut == SJ_FT_MP1_2_3 )
		{
			id3v1       = ((MPEG_File*)file)->ID3v1Tag();
			id3v2       = ((MPEG_File*)file)->ID3v2Tag();
			ape         = ((MPEG_File*)file)->APETag();
			mpegProp    = (MPEG_Properties*)audioProp;
		}
		else if( fileTypeOut == SJ_FT_FLAC )
		{
			id3v1       = ((FLAC_File*)file)->ID3v1Tag();
			id3v2       = ((FLAC_File*)file)->ID3v2Tag();
			xiph        = ((FLAC_File*)file)->xiphComment();
		}
		else if( fileTypeOut == SJ_FT_MUSEPACK )
		{
			id3v1       = ((MPC_File*)file)->ID3v1Tag();
			ape         = ((MPC_File*)file)->APETag();
			mpcProp     = (MPC_Properties*)audioProp;
		}
		else if( fileTypeOut == SJ_FT_OGG_VORBIS )
		{
			xiph        = (Ogg_XiphComment*)((OggVorbis_File*)file)->tag();
			oggvProp    = (OggVorbis_Properties*)audioProp;
		}
		else if( fileTypeOut == SJ_FT_WMA )
		{
			wmaProp     = (WMA_Properties*)audioProp;
		}
		else if( fileTypeOut == SJ_FT_MONKEYS_AUDIO )
		{
			id3v1       = ((Monkeys_File*)file)->ID3v1Tag();
			id3v2       = ((Monkeys_File*)file)->ID3v2Tag();
			ape         = ((Monkeys_File*)file)->APETag();
		}
		else if( fileTypeOut == SJ_FT_WAVPACK )
		{
			id3v1       = ((WavPack_File*)file)->ID3v1Tag();
			id3v2       = ((WavPack_File*)file)->ID3v2Tag();
			ape         = ((WavPack_File*)file)->APETag();
		}
		else if( fileTypeOut == SJ_FT_MP4 )
		{
			mp4tag      = ((MP4_File*)file)->MP4Tag();
		}

		// write MPEG information
		if( mpegProp )
		{
			if( !mpegProp->IsValid() )
			{
				prop.Add(wxT("MPEG Version"), wxT("Invalid"));
			}
			else
			{
				// write MPEG version
				temp.Empty();
				switch( mpegProp->version() )
				{
					case MPEG_Version1:     temp = wxT("1.0");      break;
					case MPEG_Version2:     temp = wxT("2.0");      break;
					case MPEG_Version2_5:   temp = wxT("2.5");      break;
				}
				prop.Add(wxT("MPEG Version"), temp, SJ_PROP_EMPTYIFEMPTY);

				// write MPEG layer
				prop.Add(wxT("MPEG Layer"), mpegProp->layer(), SJ_PROP_EMPTYIFEMPTY);


				// write MPEG VBR info
				// ...

				// write MPEG channel mode
				temp.Empty();
				switch( mpegProp->channelMode() )
				{
					case MPEG_Stereo:           temp = wxT("Stereo");           break;
					case MPEG_JointStereo:      temp = wxT("Joint Stereo");     break;
					case MPEG_DualChannel:      temp = wxT("Dual Channel");     break;
					case MPEG_SingleChannel:    temp = wxT("Single Channel");   break;
				}
				prop.Add(wxT("Channel Mode"), temp, SJ_PROP_EMPTYIFEMPTY);

				// write MPEG emphasis (value #2 is reserved)
				temp.Empty();
				switch( mpegProp->emphasis() )
				{
					case 0:         temp = wxT("None");         break;
					case 1:         temp = wxT("50/15 ms");     break;
					case 3:         temp = wxT("CCIT J.17");    break;
				}
				prop.Add(wxT("Emphasis"), temp, SJ_PROP_EMPTYIFEMPTY);

				// write MPEG Misc.
				prop.Add(wxT("Copyrighted Flag"), mpegProp->isCopyrighted()? wxT("Set") : wxT("Not set"));
				prop.Add(wxT("Original Flag"), mpegProp->isOriginal()? wxT("Set") : wxT("Not set"));
				prop.Add(wxT("Padded Flag"), mpegProp->isPadded()? wxT("Set") : wxT("Not set"));
				prop.Add(wxT("Checksum Protected"), mpegProp->protectionEnabled()? wxT("Yes") : wxT("No"));
				prop.AddBytes(wxT("Frame Size"), mpegProp->frameLength(), SJ_PROP_EMPTYIFEMPTY);
				prop.Add(wxT("Frame Count"), mpegProp->frameCount(), SJ_PROP_EMPTYIFEMPTY);
			}
		}

		// write OGG vorbis information
		if( oggvProp )
		{
			prop.Add(wxT("OGG Vorbis Version"), oggvProp->vorbisVersion());

			temp.Empty();
			if( oggvProp->bitrateNominal() > 0 )
				temp = SjTools::FormatNumber(oggvProp->bitrateNominal()) + wxT(" Bit/s");
			prop.Add(wxT("Nominal Bitrate"), temp, SJ_PROP_EMPTYIFEMPTY);

			temp.Empty();
			if( oggvProp->bitrateMaximum() > 0 )
				temp = SjTools::FormatNumber(oggvProp->bitrateMaximum()) + wxT(" Bit/s");
			prop.Add(wxT("Maximal Bitrate"), temp, SJ_PROP_EMPTYIFEMPTY);

			temp.Empty();
			if( oggvProp->bitrateMinimum() > 0 )
				temp = SjTools::FormatNumber(oggvProp->bitrateMinimum()) + wxT(" Bit/s");
			prop.Add(wxT("Minimal Bitrate"), temp, SJ_PROP_EMPTYIFEMPTY);
		}

		// write MPC information
		if( mpcProp )
		{
			// write version
			prop.Add(wxT("MPC Version"), wxString::Format(wxT("SV%i"), (int)mpcProp->mpcVersion()));
		}

		// write tag types
		temp.Empty();
		if( id3v2 )         { temp += wxT(", ID3v2"); }
		if( id3v1 )         { temp += wxT(", ID3v1"); }
		if( ape )           { temp += wxT(", APE"); }
		if( xiph )          { temp += wxT(", Xiph"); }
		if( wmaProp )       { temp += wxT(", WMA"); }
		if( mp4tag )        { temp += wxT(", MP4"); }
		if( temp.Len() )    { temp = temp.Mid(2); /*strip leading comma*/ };
		prop.Add(wxT("Tags"), temp, SJ_PROP_EMPTYIFEMPTY);

		// Write out all frames of the ID3v2 tag ...
		if( id3v2 )
		{
			// ... headline
			ID3v2_Header* header = id3v2->header();
			temp = wxT("ID3v2");
			if( header )
			{
				temp += wxString::Format(wxT(".%i.%i"), (int)header->majorVersion(), (int)header->revisionNumber());
				if( header->experimentalIndicator() ) temp += wxT(" (experimental)");
			}
			temp += wxT(" Frames");
			prop.Add(temp, wxT(""), SJ_PROP_HEADLINE);

			// ... go through all frame ...
			ID3v2_FrameList::Node* node = id3v2->frameList().GetFirst();
			SjStringType encoding = SJ_UTF8; bool encodingSet = FALSE; bool encodingMixed = FALSE;
			for( ; node; node = node->GetNext() )
			{
				ID3v2_Frame* frame = node->GetData();
				wxString frameId = frame->frameID().toString(SJ_LATIN1);
				wxString frameContent = frame->toString().Trim(TRUE).Trim(FALSE);

				if( frameId == wxT("COMM") )
				{
					// ... comment frame specials
					ID3v2_CommentsFrame* COMM = (ID3v2_CommentsFrame*)frame;
					if( !COMM->description().IsEmpty() )
					{
						frameId += wxT(" [") + COMM->description() + wxT("]");
					}
				}
				else if( frameId == wxT("APIC") )
				{
					// ... picture frame specials
					ID3v2_AttachedPictureFrame* APIC = (ID3v2_AttachedPictureFrame*)frame;
					frameId += wxString::Format(wxT(" [%i]"), (int)APIC->type());
					frameContent = APIC->description();
					if( !frameContent.IsEmpty() )
					{
						frameContent += wxT(", ");
					}
					frameContent += wxString::Format(wxT("%s, "), APIC->mimeType().c_str());
					frameContent += SjTools::FormatBytes(APIC->picture().size(), SJ_FORMAT_ADDEXACT);
				}
				else if( frameId.StartsWith(wxT("T")) )
				{
					// ... text frame specials
					ID3v2_TextIdentificationFrame* TXXX = (ID3v2_TextIdentificationFrame*)frame;
					if( !encodingSet )
					{
						encoding = TXXX->textEncoding();
						encodingSet = TRUE;
					}
					else if( encoding != TXXX->textEncoding() )
					{
						encodingMixed = TRUE;
					}
				}

				if( frameContent.IsEmpty() )
				{
					// ... other (binary) frames specials
					SjByteVector data = frame->renderFields();
					if( data.size() > 0 )
					{
						frameContent = SjTools::FormatBytes(data.size(), SJ_FORMAT_ADDEXACT);
					}
				}

				// ... write frame information
				prop.Add(frameId, frameContent, SJ_PROP_EMPTYIFEMPTY);
			}

			// ... write encoding
			temp.Empty();
			if( encodingSet )
			{
				if( encodingMixed )
				{
					temp = wxT("Mixed");
				}
				else switch( encoding )
					{
						case SJ_UTF8:   temp = wxT("UTF-8"); break;
						case SJ_UTF16:
						case SJ_UTF16BE:
						case SJ_UTF16LE:temp = wxT("UTF-16"); break;
						case SJ_LATIN1: temp = wxT("Latin-1"); break;
					}
			}
			prop.Add(wxT("Encoding"), temp, SJ_PROP_EMPTYIFEMPTY);

			// ... write total size
			if( header )
			{
				prop.AddBytes(wxT("Total Size"), header->completeTagSize(), SJ_PROP_EMPTYIFEMPTY);
			}
		}

		// Write out all fields of the ID3v1 tag
		if( id3v1 )
		{
			prop.Add(wxString::Format(wxT("ID3v1.%i Fields"), (int)id3v1->majorVersion()), wxT(""), SJ_PROP_HEADLINE);

			prop.Add(wxT("Title"),  id3v1->title(),         SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Artist"), id3v1->leadArtist(),    SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Album"),  id3v1->album(),         SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Year"),   id3v1->year(),          SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Comment"),id3v1->comment(),       SJ_PROP_EMPTYIFEMPTY);
			if( id3v1->majorVersion() == 1 )
			{
				long nr, dummy;
				id3v1->track(nr, dummy);
				prop.Add(wxT("Track"), nr,                  SJ_PROP_EMPTYIFEMPTY);
			}
			prop.Add(wxT("Genre"),  id3v1->genre(),         SJ_PROP_EMPTYIFEMPTY);
		}

		// Write out all fields of an APE tag
		if( ape )
		{
			prop.Add(wxT("APE Items"), wxT(""), SJ_PROP_HEADLINE);

			const SjSPHash* items = ape->itemListMap();
			if( items )
			{
				SjHashIterator  iterator;
				wxString        id;
				APE_Item*       item;
				while( (item=(APE_Item*)items->Iterate(iterator, id)) )
				{
					prop.Add(id, item->toString(), SJ_PROP_EMPTYIFEMPTY);
				}
			}

			APE_Footer* footer = ape->footer();
			if( footer )
			{
				prop.AddBytes(wxT("Total Size"), footer->completeTagSize(), SJ_PROP_EMPTYIFEMPTY);
			}
		}

		// Write out all fields of an Xiph comment
		if( xiph )
		{
			prop.Add(wxT("Xiph Comment"), wxT(""), SJ_PROP_HEADLINE);

			const SjSPHash* fieldList = xiph->fieldListMap();
			if( fieldList )
			{
				SjHashIterator  iterator;
				wxString        id;
				wxArrayString*  values;
				while( (values=(wxArrayString*)fieldList->Iterate(iterator, id)) )
				{
					temp.Empty();
					for( size_t v = 0; v < values->GetCount(); v++ )
					{
						if( !values->Item(v).IsEmpty() )
						{
							if( !temp.IsEmpty() ) temp += wxT(", ");
							temp += values->Item(v);
						}
					}
					prop.Add(id, temp, SJ_PROP_EMPTYIFEMPTY);
				}
			}

			prop.Add(wxT("Vendor ID"), xiph->vendorID(), SJ_PROP_EMPTYIFEMPTY);
		}

		// Write out generic tag - debug mode only as this does not generate any additional information
		if( g_debug && mp4tag )
		{
			Tagger_Tag* anytag = mp4tag;

			prop.Add(wxT("Tag"),            wxT(""),                    SJ_PROP_HEADLINE);
			prop.Add(wxT("Title"),          anytag->title(),            SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Lead artist"),    anytag->leadArtist(),       SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Original artist"),anytag->orgArtist(),        SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Composer"),       anytag->composer(),         SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Album"),          anytag->album(),            SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Comment"),        anytag->comment(),          SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Genre"),          anytag->genre(),            SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Group"),          anytag->group(),            SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Year"),           anytag->year(),             SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("BPM"),            anytag->beatsPerMinute(),   SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Rating"),         anytag->rating(),           SJ_PROP_EMPTYIFEMPTY);
			long l1, l2;
			anytag->track(l1, l2);
			prop.Add(wxT("Track"),          wxString::Format(wxT("%i/%i"), (int)l1, (int)l2));
			anytag->disk(l1, l2);
			prop.Add(wxT("Disk"),           wxString::Format(wxT("%i/%i"), (int)l1, (int)l2));
			prop.Add(wxT("Seconds"),        audioProp->length(),        SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Bitrate"),        audioProp->bitrate(),       SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Samplerate"),     audioProp->sampleRate(),    SJ_PROP_EMPTYIFEMPTY);
			prop.Add(wxT("Channels"),       audioProp->channels(),      SJ_PROP_EMPTYIFEMPTY);
		}

		// done so far, deleting the file delete all related tags & Co.
		delete file;
	}
}



/*******************************************************************************
 *  set track information to a stream defined by a wxFSFile object
 ******************************************************************************/



bool SjSetTrackInfoToID3Etc(const wxString& url, const SjTrackInfo& ti)
{
	// crash handling
	if( !g_tools->CrashPrecaution(wxT("Tagger"), wxT("Write"), url) )
	{
		return FALSE; // do not log any error as the user was asked on startup
	}

	// open file
	bool            success = FALSE;
	wxFileSystem    fs;
	wxFSFile*       fsFile = fs.OpenFile(url, wxFS_READ|wxFS_SEEKABLE);
	if( fsFile )
	{
		SjFileType fileTypeOut;
		Tagger_File* file = getTaggerFile(url, fsFile, fileTypeOut);
		if( file )
		{
			Tagger_Tag* tag = file->tag();
			if( tag )
			{
				// set track name etc.
				if( ti.m_validFields & SJ_TI_TRACKNAME )
				{
					tag->setTitle(ti.m_trackName);
				}

				if( ti.m_validFields & SJ_TI_LEADARTISTNAME )
				{
					tag->setLeadArtist(ti.m_leadArtistName);
				}

				if( ti.m_validFields & SJ_TI_ORGARTISTNAME )
				{
					tag->setOrgArtist(ti.m_orgArtistName);
				}

				if( ti.m_validFields & SJ_TI_COMPOSERNAME )
				{
					tag->setComposer(ti.m_composerName);
				}

				if( ti.m_validFields & SJ_TI_ALBUMNAME )
				{
					tag->setAlbum(ti.m_albumName);
				}

				if( ti.m_validFields & SJ_TI_COMMENT )
				{
					tag->setComment(ti.m_comment);
				}

				if( ti.m_validFields & SJ_TI_GENRENAME )
				{
					tag->setGenre(ti.m_genreName);
				}

				if( ti.m_validFields & SJ_TI_GROUPNAME )
				{
					tag->setGroup(ti.m_groupName);
				}

				// set track number
				long oldNr, oldCount;
				tag->track(oldNr, oldCount);
				if( ti.m_validFields & SJ_TI_TRACKNR )
				{
					if( ti.m_validFields & SJ_TI_TRACKCOUNT )
					{
						tag->setTrack(ti.m_trackNr, ti.m_trackCount);
					}
					else
					{
						tag->setTrack(ti.m_trackNr, oldCount);
					}
				}
				else if( ti.m_validFields & SJ_TI_TRACKCOUNT )
				{
					tag->setTrack(oldNr, ti.m_trackCount);
				}

				// set disk number
				tag->disk(oldNr, oldCount);
				if( ti.m_validFields & SJ_TI_DISKNR )
				{
					if( ti.m_validFields & SJ_TI_DISKCOUNT )
					{
						tag->setDisk(ti.m_diskNr, ti.m_diskCount);
					}
					else
					{
						tag->setDisk(ti.m_diskNr, oldCount);
					}
				}
				else if( ti.m_validFields & SJ_TI_DISKCOUNT )
				{
					tag->setDisk(oldNr, ti.m_diskCount);
				}

				// set year
				if( ti.m_validFields & SJ_TI_YEAR )
				{
					tag->setYear(ti.m_year);
				}

				// set BPM
				if( ti.m_validFields & SJ_TI_BEATSPERMINUTE )
				{
					tag->setBeatsPerMinute(ti.m_beatsPerMinute);
				}

				// set RATING
				if( ti.m_validFields & SJ_TI_RATING )
				{
					wxASSERT( ti.m_rating >= 0 && ti.m_rating <= 5 );
					if( g_taggerOptions->m_flags&SJTF_WRITE_RATINGS )
					{
						tag->setRating(ti.m_rating);
					}
				}

				// save the file
				if( file->save() )
				{
					success = TRUE;
				}
			}

			delete file;
		}

		delete fsFile;
	}

	// some so far
	if( !success )
	{
		wxLogError(_("Cannot write \"%s\".")/*n/t*/, url.c_str());
	}

	return success;
}


