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




#ifndef TAGGER_WMAPROPERTIES_H
#define TAGGER_WMAPROPERTIES_H

#include "tg_tagger_base.h"



class File;

/*!
 * This reads the data from a WMA stream to support the
 * AudioProperties API.
 */

class WMA_Properties : public Tagger_AudioProperties
{
public:
	/*!
	 * Initialize this structure
	 */
	WMA_Properties();

	/*!
	 * Destroys this WMA_Properties instance.
	 */
	virtual ~WMA_Properties();

	// Reimplementations.

	virtual int length() const;
	virtual int bitrate() const;
	virtual int sampleRate() const;
	virtual int channels() const;

private:
	friend class WMA_File;

	int m_length;
	int m_bitrate;
	int m_sampleRate;
	int m_channels;

	WMA_Properties(const WMA_Properties &);
	WMA_Properties &operator=(const WMA_Properties &);

	void read();
};


#endif
