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
#include "tg_ogg_file.h"
#include "tg_oggvorbis_properties.h"
#include "tg_oggvorbis_file.h"


class OggVorbis_PropertiesPrivate
{
public:
	OggVorbis_PropertiesPrivate(OggVorbis_File *f) :
		file(f),
		length(0),
		bitrate(0),
		sampleRate(0),
		channels(0),
		vorbisVersion(0),
		bitrateMaximum(0),
		bitrateNominal(0),
		bitrateMinimum(0) {}

	OggVorbis_File *file;
	int length;
	int bitrate;
	int sampleRate;
	int channels;
	int vorbisVersion;
	int bitrateMaximum;
	int bitrateNominal;
	int bitrateMinimum;
};


/*!
 * Vorbis headers can be found with one type ID byte and the string "vorbis" in
 * an Ogg stream.  0x01 indicates the setup header.
 */
static const unsigned char vorbisSetupHeaderID[] = { 0x01, 'v', 'o', 'r', 'b', 'i', 's', 0 };


////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

OggVorbis_Properties::OggVorbis_Properties(OggVorbis_File *file) : Tagger_AudioProperties()
{
	d = new OggVorbis_PropertiesPrivate(file);
	read();
}

OggVorbis_Properties::~OggVorbis_Properties()
{
	delete d;
}

int OggVorbis_Properties::length() const
{
	return d->length;
}

int OggVorbis_Properties::bitrate() const
{
	return int(float(d->bitrate) / float(1000) + 0.5);
}

int OggVorbis_Properties::sampleRate() const
{
	return d->sampleRate;
}

int OggVorbis_Properties::channels() const
{
	return d->channels;
}

int OggVorbis_Properties::vorbisVersion() const
{
	return d->vorbisVersion;
}

int OggVorbis_Properties::bitrateMaximum() const
{
	return d->bitrateMaximum;
}

int OggVorbis_Properties::bitrateNominal() const
{
	return d->bitrateNominal;
}

int OggVorbis_Properties::bitrateMinimum() const
{
	return d->bitrateMinimum;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void OggVorbis_Properties::read()
{
	// Get the identification header from the Ogg implementation.

	SjByteVector data = d->file->packet(0);

	int pos = 0;

	if(data.mid(pos, 7) != vorbisSetupHeaderID) {
		wxLogDebug(wxT("Vorbis::Properties::read() -- invalid Vorbis identification header"));
		return;
	}

	pos += 7;

	d->vorbisVersion = data.mid(pos, 4).toUInt(false);
	pos += 4;

	d->channels = (unsigned char)(data[pos]);
	pos += 1;

	d->sampleRate = data.mid(pos, 4).toUInt(false);
	pos += 4;

	d->bitrateMaximum = data.mid(pos, 4).toUInt(false);
	pos += 4;

	d->bitrateNominal = data.mid(pos, 4).toUInt(false);
	pos += 4;

	d->bitrateMinimum = data.mid(pos, 4).toUInt(false);

	// TODO: Later this should be only the "fast" mode.
	d->bitrate = d->bitrateNominal;

	// Find the length of the file.  See http://wiki.xiph.org/VorbisStreamLength/
	// for my notes on the topic.

	const Ogg_PageHeader *first = d->file->firstPageHeader();
	const Ogg_PageHeader *last = d->file->lastPageHeader();

	if(first && last) {
		wxLongLong_t start = first->absoluteGranularPosition();
		wxLongLong_t end = last->absoluteGranularPosition();

		if(start >= 0 && end >= 0 && d->sampleRate > 0)
			d->length = (end - start) / (wxLongLong_t) d->sampleRate;
		else
			wxLogDebug(wxT("Vorbis::Properties::read() -- Either the PCM values for the start or end of this file was incorrect or the sample rate is zero."));
	}
	else
		wxLogDebug(wxT("Vorbis::Properties::read() -- Could not find valid first and last Ogg pages."));
}
