/***************************************************************************
    copyright            : (C) 2006 by Lukáš Lalinský
    email                : lalinsky@gmail.com

    copyright            : (C) 2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.org
                           (original MPC implementation)
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



#ifndef TAGGER_WAVPACKFILE_H
#define TAGGER_WAVPACKFILE_H

#include "tg_mpeg_file.h"


class WavPack_File : public MPEG_File
{
public:
	WavPack_File(
	    const wxFSFile* file,                // you should give either fsFile
	    wxInputStream* inputStream = NULL)   // in inputStream, not both!
		: MPEG_File(file, inputStream, Tagger_ReadTags)
	{
	}

	virtual ~WavPack_File() {}

	// for writing tags or for reading the audio properties, we should
	// know some more details about the WavPack Audio file format,
	// currently, we only know that the tags seem to be the same than
	// for MPEG files; however, I'm also not sure if the format is worth more programming.
	virtual Tagger_AudioProperties *audioProperties() const { return NULL; }
	virtual bool save() { return FALSE; }

private:
};



#endif
