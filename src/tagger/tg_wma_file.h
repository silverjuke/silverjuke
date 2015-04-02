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



#ifndef TAGGER_WMAFILE_H
#define TAGGER_WMAFILE_H

#include "tg_tagger_base.h"
#include "tg_wma_properties.h"



class WMA_Tag;

//! A WMA file class with some useful methods specific to WMA
//! Most of the real parsing is ripped from MPlayer

/*!
 * This implements the generic WMA_File API
 */

class WMA_File : public Tagger_File
{
public:
	/*!
	 * Contructs a WMA file from \a file.  If \a readProperties is true the
	 * file's audio properties will also be read using \a propertiesStyle.  If
	 * false, \a propertiesStyle is ignored.
	 *
	 * \deprecated This constructor will be dropped in favor of the one below
	 * in a future version.
	 */
	WMA_File(const wxFSFile* file, wxInputStream* inputStream = NULL, bool readProperties = true);

	/*!
	 * Destroys this instance of the File.
	 */
	virtual ~WMA_File();

	/*!
	 * Returns a pointer to the WMATag class, which provides the
	 * basic Tagger_Tag fields
	 *
	 * \see WMATag()
	 */
	virtual Tagger_Tag *tag() const;

	/*!
	 * Returns the WMA::Properties for this file.  If no audio properties
	 * were read then this will return a null pointer.
	 */
	virtual Tagger_AudioProperties *audioProperties() const;

	/*!
	 * Save the file.
	 * This is the same as calling save(AllTags);
	 *
	 * \note As of now, saving WMA tags is not supported.
	 */
	virtual bool save();

	/*!
	 * Returns a pointer to the WMATag of the file.
	 *
	 * \note The Tag <b>is still</b> owned by the WMA::File and should not be
	 * deleted by the user.  It will be deleted when the file (object) is
	 * destroyed.
	 */
	WMA_Tag *getWMATag() const;

protected:
#ifdef __WXDEBUG__
	WMA_File &operator=(const WMA_File &) { wxASSERT_MSG(0, wxT("do not copy MPEG_Header objects this way!")); return *this; }
#endif

	void    getWavHeader    (int size);
	int     asfReadHeader   ();
	void    read            (bool readProperties);

	WMA_Tag*        wmaTag;
	WMA_Properties* wmaProperties;
};



#endif
