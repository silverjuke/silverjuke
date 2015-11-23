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




#include "tg_tagger_base.h"
#include "tg_id3v1_tag.h"
#include "tg_id3v2_framefactory.h"




/*******************************************************************************
 *  Tagger_File
 ******************************************************************************/



Tagger_File::Tagger_File(const wxString& url, wxInputStream* inputStream)
	: SjByteFile(url, inputStream)
{
}


void Tagger_File::exitLibrary()
{
	// ID3v1: free the string handler
	ID3v1_Tag::setStringHandler(NULL);
	ID3v2_FrameFactory::exitLibrary();
}



/*******************************************************************************
 *  Tagger_Tag
 ******************************************************************************/


bool Tagger_Tag::isEmpty() const
{
	long trackNr, trackCount;
	track(trackNr, trackCount);

	long diskNr, diskCount;
	disk(diskNr, diskCount);

	return (title().IsEmpty() &&
	        leadArtist().IsEmpty() &&
	        orgArtist().IsEmpty() &&
	        composer().IsEmpty() &&
	        album().IsEmpty() &&
	        comment().IsEmpty() &&
	        genre().IsEmpty() &&
	        group().IsEmpty() &&
	        year() == 0 &&
	        trackNr <= 0 &&
	        diskNr <=0);
}

void Tagger_Tag::duplicate(const Tagger_Tag *source, Tagger_Tag *target, bool overwrite) // static
{
	long nr, count;
	if(overwrite)
	{
		target->setTitle(source->title());
		target->setLeadArtist(source->leadArtist());
		target->setOrgArtist(source->orgArtist());
		target->setComposer(source->composer());
		target->setAlbum(source->album());
		target->setComment(source->comment());
		target->setGenre(source->genre());
		target->setGroup(source->group());
		target->setYear(source->year());

		source->track(nr, count);
		target->setTrack(nr, count);

		source->disk(nr, count);
		target->setDisk(nr, count);
	}
	else
	{
		if(target->title().IsEmpty())
			target->setTitle(source->title());

		if(target->leadArtist().IsEmpty())
			target->setLeadArtist(source->leadArtist());

		if(target->orgArtist().IsEmpty())
			target->setOrgArtist(source->orgArtist());

		if(target->composer().IsEmpty())
			target->setComposer(source->composer());

		if(target->album().IsEmpty())
			target->setAlbum(source->album());

		if(target->comment().IsEmpty())
			target->setComment(source->comment());

		if(target->genre().IsEmpty())
			target->setGenre(source->genre());

		if(target->group().IsEmpty())
			target->setGroup(source->group());

		if(target->year() <= 0)
			target->setYear(source->year());

		target->track(nr, count);
		if( nr <= 0 )
		{
			source->track(nr, count);
			target->setTrack(nr, count);
		}

		target->disk(nr, count);
		if( nr <= 0 )
		{
			source->disk(nr, count);
			target->setDisk(nr, count);
		}
	}
}



void Tagger_Tag::explodeNrAndCount(const wxString& str, long& nr, long& count)
{
	if( str.Find('/') != -1 )
	{
		if( !str.BeforeFirst('/').ToLong(&nr) )
			nr = 0;

		if( !str.AfterFirst('/').ToLong(&count) )
			count = 0;
	}
	else
	{
		if( !str.ToLong(&nr) )
			nr = 0;

		count = 0;
	}

	if( nr < 0 ) nr = 0;
	if( count < 0 ) count = 0;
}

wxString Tagger_Tag::implodeNrAndCount(long nr, long count)
{
	if( nr > 0 )
	{
		if( count > 0 )
		{
			return wxString::Format(wxT("%i/%i"), (int)nr, (int)count);
		}

		return wxString::Format(wxT("%i"), (int)nr);
	}

	return wxEmptyString;
}



