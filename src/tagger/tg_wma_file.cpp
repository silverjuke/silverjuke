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
#include "tg_wma_tag.h"


////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

WMA_File::WMA_File(const wxFSFile* fsFile, wxInputStream* inputStream, bool readProperties)
	: Tagger_File(fsFile, inputStream)
{
	wmaTag = NULL;
	wmaProperties = NULL;

	wmaTag = new WMA_Tag();
	read(readProperties);
}

WMA_File::~WMA_File()
{
	if( wmaTag ) delete wmaTag;
	if( wmaProperties ) delete wmaProperties;
}

Tagger_Tag *WMA_File::tag() const
{
	return wmaTag;
}

WMA_Tag *WMA_File::getWMATag() const
{
	return wmaTag;
}

Tagger_AudioProperties *WMA_File::audioProperties() const
{
	return wmaProperties;
}

bool WMA_File::save()
{
	wxLogDebug (wxT("WMA::File: Saving not supported yet."));
	return false;
}


////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void WMA_File::read(bool readProperties)
{
	if(readProperties)
		wmaProperties = new WMA_Properties();

	asfReadHeader();
}
