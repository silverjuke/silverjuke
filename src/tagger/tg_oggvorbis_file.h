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




#ifndef TAGGER_VORBISFILE_H
#define TAGGER_VORBISFILE_H

#include "tg_ogg_file.h"
#include "tg_ogg_xiphcomment.h"

#include "tg_oggvorbis_properties.h"





//! An implementation of Ogg::File with Vorbis specific methods

/*!
 * This is the central class in the Ogg Vorbis metadata processing collection
 * of classes.  It's built upon Ogg::File which handles processing of the Ogg
 * logical bitstream and breaking it down into pages which are handled by
 * the codec implementations, in this case Vorbis specifically.
 */
class OggVorbis_FilePrivate;
class OggVorbis_File : public Ogg_File
{
public:
	/*!
	 * Contructs a Vorbis file from \a file.  If \a readProperties is true the
	 * file's audio properties will also be read using \a propertiesStyle.  If
	 * false, \a propertiesStyle is ignored.
	 */
	OggVorbis_File(const wxString& url, wxInputStream*, bool readProperties = true);

	/*!
	 * Destroys this instance of the File.
	 */
	virtual ~OggVorbis_File();

	/*!
	 * Returns the XiphComment for this file.  XiphComment implements the tag
	 * interface, so this serves as the reimplementation of
	 * Tagger_File::tag().
	 */
	virtual Tagger_Tag *tag() const;

	/*!
	 * Returns the Vorbis::Properties for this file.  If no audio properties
	 * were read then this will return a null pointer.
	 */
	virtual Tagger_AudioProperties *audioProperties() const;

	virtual bool save();

private:
	OggVorbis_File(const OggVorbis_File &);
	OggVorbis_File &operator=(const OggVorbis_File &);

	void read(bool readProperties);


	OggVorbis_FilePrivate *d;
};


#endif
