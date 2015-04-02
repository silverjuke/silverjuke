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



static ID3v1_StringHandler* stringHandler = NULL;



ID3v1_Tag::ID3v1_Tag(Tagger_File *file, long tagOffset)
	: Tagger_Tag()
{
	if( stringHandler == NULL )
	{
		stringHandler = new ID3v1_StringHandler;
	}

	m_majorVersion = 0;
	m_track = 0;
	m_genre = 255;

	if( file )
	{
		m_file = file;
		m_tagOffset = tagOffset;
		read();
	}
	else
	{
		m_file = NULL;
		m_tagOffset = -1;
	}
}



SjByteVector ID3v1_Tag::render() const
{
	SjByteVector data;

	data.append(fileIdentifier());
	data.append(stringHandler->render(m_title).resize(30));
	data.append(stringHandler->render(m_leadArtist).resize(30));
	data.append(stringHandler->render(m_album).resize(30));
	data.append(stringHandler->render(m_year).resize(4));
	data.append(stringHandler->render(m_comment).resize(28));
	data.append((unsigned char)(0));
	data.append((unsigned char)(m_track));
	data.append((unsigned char)(m_genre));

	return data;
}



SjByteVector ID3v1_Tag::fileIdentifier()
{
	return SjByteVector::fromCString("TAG");
}



wxString ID3v1_Tag::genre() const
{
	return lookupGenreName(m_genre);
}



long ID3v1_Tag::year() const
{
	long l;
	m_year.ToLong(&l);
	return l;
}



void ID3v1_Tag::setGenre(const wxString &s)
{
	m_genre = lookupGenreIndex(s);
}



void ID3v1_Tag::setYear(long i)
{
	m_year = wxString::Format(wxT("%i"), (int)i);
}



void ID3v1_Tag::setTrack(long nr, long count)
{
	m_track = (nr>0 && nr<256) ? nr : 0;
}



void ID3v1_Tag::setStringHandler(ID3v1_StringHandler* handler)
{
	if( stringHandler )
		delete stringHandler;

	stringHandler = handler;
}


void ID3v1_Tag::read()
{
	if(m_file && m_file->IsValid())
	{
		m_file->Seek(m_tagOffset);
		// read the tag -- always 128 bytes
		SjByteVector data = m_file->ReadBlock(128);

		// some initial sanity checking
		if(data.size() == 128 && data.startsWith((unsigned char*)"TAG"))
		{
			parse(data);
		}
		else
		{
			wxLogDebug(wxT("ID3v1 tag is not valid or could not be read at the specified offset."));
		}
	}
}

void ID3v1_Tag::parse(const SjByteVector &data)
{
	int offset = 3;

	m_title = stringHandler->parse(data.mid(offset, 30));
	offset += 30;

	m_leadArtist = stringHandler->parse(data.mid(offset, 30));
	offset += 30;

	m_album = stringHandler->parse(data.mid(offset, 30));
	offset += 30;

	m_year = stringHandler->parse(data.mid(offset, 4));
	offset += 4;

	// Check for ID3v1.1 -- Note that ID3v1 *does not* support "track zero" -- this
	// is not a bug in Tagger.  Since a zeroed byte is what we would expect to
	// indicate the end of a C-String, specifically the comment string, a value of
	// zero must be assumed to be just that.

	if(data[offset + 28] == 0 && data[offset + 29] != 0)
	{
		// ID3v1.1 detected

		m_comment = stringHandler->parse(data.mid(offset, 28));
		m_track = (unsigned char)(data[offset + 29]);
		m_majorVersion = 1;
	}
	else
	{
		m_comment = stringHandler->parse(data.mid(offset, 30));
		m_majorVersion = 0;
	}

	offset += 30;

	m_genre = (unsigned char)(data[offset]);
}



#define SJ_GENRE_COUNT 148
static const char* s_genres[SJ_GENRE_COUNT] = {
	/* 0   */   "Blues",                    "Classic Rock",         "Country",          "Dance",
	/* 4   */   "Disco",                    "Funk",                 "Grunge",           "Hip-Hop",
	/* 8   */   "Jazz",                     "Metal",                "New Age",          "Oldies",
	/* 12  */   "Other",                    "Pop",                  "R&B",              "Rap",
	/* 16  */   "Reggae",                   "Rock",                 "Techno",           "Industrial",
	/* 20  */   "Alternative",              "Ska",                  "Death Metal",      "Pranks",
	/* 24  */   "Soundtrack",               "Euro-Techno",          "Ambient",          "Trip-Hop",
	/* 28  */   "Vocal",                    "Jazz+Funk",            "Fusion",           "Trance",
	/* 32  */   "Classical",                "Instrumental",         "Acid",             "House",
	/* 36  */   "Game",                     "Sound Clip",           "Gospel",           "Noise",
	/* 40  */   "Alternative Rock",         "Bass",                 "Soul",             "Punk",
	/* 44  */   "Space",                    "Meditative",           "Instrumental Pop", "Instrumental Rock",
	/* 48  */   "Ethnic",                   "Gothic",               "Darkwave",         "Techno-Industrial",
	/* 52  */   "Electronic",               "Pop-Folk",             "Eurodance",        "Dream",
	/* 56  */   "Southern Rock",            "Comedy",               "Cult",             "Gangsta",
	/* 60  */   "Top 40",                   "Christian Rap",        "Pop/Funk",         "Jungle",
	/* 64  */   "Native American",          "Cabaret",              "New Wave",         "Psychedelic",
	/* 68  */   "Rave",                     "Showtunes",            "Trailer",          "Lo-Fi",
	/* 72  */   "Tribal",                   "Acid Punk",            "Acid Jazz",        "Polka",
	/* 76  */   "Retro",                    "Musical",              "Rock & Roll",      "Hard Rock",
	/* 80  */   "Folk",                     "Folk/Rock",            "National Folk",    "Swing",
	/* 84  */   "Fusion",                   "Bebob",                "Latin",            "Revival",
	/* 88  */   "Celtic",                   "Bluegrass",            "Avantgarde",       "Gothic Rock",
	/* 92  */   "Progressive Rock",         "Psychedelic Rock",     "Symphonic Rock",   "Slow Rock",
	/* 96  */   "Big Band",                 "Chorus",               "Easy Listening",   "Acoustic",
	/* 100 */   "Humour",                   "Speech",               "Chanson",          "Opera",
	/* 104 */   "Chamber Music",            "Sonata",               "Symphony",         "Booty Bass",
	/* 108 */   "Primus",                   "Porn Groove",          "Satire",           "Slow Jam",
	/* 112 */   "Club",                     "Tango",                "Samba",            "Folklore",
	/* 116 */   "Ballad",                   "Power Ballad",         "Rhythmic Soul",    "Freestyle",
	/* 120 */   "Duet",                     "Punk Rock",            "Drum Solo",        "A Cappella",
	/* 124 */   "Euro-House",               "Dance Hall",           "Goa",              "Drum & Bass",
	/* 128 */   "Club-House",               "Hardcore",             "Terror",           "Indie",
	/* 132 */   "BritPop",                  "Negerpunk",            "Polsk Punk",       "Beat",
	/* 136 */   "Christian Gangsta Rap",    "Heavy Metal",          "Black Metal",      "Crossover",
	/* 140 */   "Contemporary Christian",   "Christian Rock",       "Merengue",         "Salsa",
	/* 144 */   "Thrash Metal",             "Anime",                "Jpop",             "Synthpop"
};



SjSLHash* ID3v1_Tag::getGenreMap()
{
	static SjSLHash m;
	if(m.GetCount()==0)
	{
		for(int i = 0; i < SJ_GENRE_COUNT; i++)
		{
			m.Insert(
#if wxUSE_UNICODE
			    wxString(s_genres[i], wxConvISO8859_1),
#else
			    s_genres[i],
#endif
			    i+1 /*null-values cannot be inserted to SjSLHash!*/);
		}
	}
	return &m;
}



int ID3v1_Tag::lookupGenreIndex(const wxString &name)
{
	SjSLHash* genreMap = getGenreMap(); // create map
	long i = genreMap->Lookup(name);
	if( i )
	{
		return i-1; // we've added 1 as null-values cannot be inserted to SjSLHash!
	}
	return 255;
}



wxString ID3v1_Tag::lookupGenreName(int i)
{
	if( i >= 0 && i < SJ_GENRE_COUNT )
	{
#if wxUSE_UNICODE
		return wxString(s_genres[i], wxConvISO8859_1);
#else
		return s_genres[i];
#endif
	}
	return wxEmptyString;
}
