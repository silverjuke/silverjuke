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
#include "tg_oggvorbis_file.h"

class OggVorbis_FilePrivate
{
public:
	OggVorbis_FilePrivate() :
		comment(0),
		properties(0) {}

	~OggVorbis_FilePrivate()
	{
		if( comment ) delete comment;
		if( properties ) delete properties;
	}

	Ogg_XiphComment *comment;
	OggVorbis_Properties *properties;
};

/*!
 * Vorbis headers can be found with one type ID byte and the string "vorbis" in
 * an Ogg stream.  0x03 indicates the comment header.
 */
static const unsigned char vorbisCommentHeaderID[] = { 0x03, 'v', 'o', 'r', 'b', 'i', 's', 0 };

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

OggVorbis_File::OggVorbis_File(const wxString& url, wxInputStream* inputStream, bool readProperties)
	: Ogg_File(url, inputStream)
{
	d = new OggVorbis_FilePrivate;
	read(readProperties);
}

OggVorbis_File::~OggVorbis_File()
{
	delete d;
}

Tagger_Tag *OggVorbis_File::tag() const
{
	return d->comment;
}

Tagger_AudioProperties *OggVorbis_File::audioProperties() const
{
	return d->properties;
}

bool OggVorbis_File::save()
{
	SjByteVector v(vorbisCommentHeaderID);

	if(!d->comment)
		d->comment = new Ogg_XiphComment;
	v.append(d->comment->render());

	setPacket(1, v);

	return Ogg_File::save();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void OggVorbis_File::read(bool readProperties)
{
	SjByteVector commentHeaderData = packet(1);

	if(commentHeaderData.mid(0, 7) != vorbisCommentHeaderID) {
		wxLogDebug(wxT("Vorbis::File::read() - Could not find the Vorbis comment header."));
		SetValid(false);
		return;
	}

	d->comment = new Ogg_XiphComment(commentHeaderData.mid(7));

	if(readProperties)
		d->properties = new OggVorbis_Properties(this);
}
