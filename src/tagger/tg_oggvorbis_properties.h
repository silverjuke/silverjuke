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




#ifndef TAGGER_VORBISPROPERTIES_H
#define TAGGER_VORBISPROPERTIES_H

#include "tg_tagger_base.h"



class OggVorbis_File;

//! An implementation of audio property reading for Ogg Vorbis

/*!
 * This reads the data from an Ogg Vorbis stream found in the AudioProperties
 * API.
 */

class OggVorbis_PropertiesPrivate;
class OggVorbis_Properties : public Tagger_AudioProperties
{
public:
	/*!
	 * Create an instance of Vorbis::Properties with the data read from the
	 * Vorbis::File \a file.
	 */
	OggVorbis_Properties(OggVorbis_File *file);

	/*!
	 * Destroys this VorbisProperties instance.
	 */
	virtual ~OggVorbis_Properties();

	// Reimplementations.

	virtual int length() const;
	virtual int bitrate() const;
	virtual int sampleRate() const;
	virtual int channels() const;

	/*!
	 * Returns the Vorbis version, currently "0" (as specified by the spec).
	 */
	int vorbisVersion() const;

	/*!
	 * Returns the maximum bitrate as read from the Vorbis identification
	 * header.
	 */
	int bitrateMaximum() const;

	/*!
	 * Returns the nominal bitrate as read from the Vorbis identification
	 * header.
	 */
	int bitrateNominal() const;

	/*!
	 * Returns the minimum bitrate as read from the Vorbis identification
	 * header.
	 */
	int bitrateMinimum() const;

private:
	OggVorbis_Properties(const OggVorbis_Properties &);
	OggVorbis_Properties &operator=(const OggVorbis_Properties &);

	void read();


	OggVorbis_PropertiesPrivate *d;
};



#endif
