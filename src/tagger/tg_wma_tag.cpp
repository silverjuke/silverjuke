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




WMA_Tag::WMA_Tag() : Tagger_Tag() {
	/*m_title = String::null;
	m_artist = String::null;
	m_album = String::null;
	m_comment = String::null;
	m_genre = String::null;*/
	m_year = 0;
	m_trackNr = 0;
}

WMA_Tag::~WMA_Tag() {
}

bool WMA_Tag::isEmpty() const {
	return  m_title.IsEmpty() &&
	        m_leadArtist.IsEmpty() &&
	        m_album.IsEmpty() &&
	        m_comment.IsEmpty() &&
	        m_genre.IsEmpty() &&
	        m_year == 0 &&
	        m_trackNr == 0;
}

/*void WMA_Tag::duplicate(const Tagger_Tag *source, Tagger_Tag *target, bool overwrite) {
  // No nonstandard information stored yet
  Tagger_Tag::duplicate(source, target, overwrite);
}*/

