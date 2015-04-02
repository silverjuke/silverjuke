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



#ifndef __SJ_COMBINES_TAG_H__
#define __SJ_COMBINES_TAG_H__


#include "tg_tagger_base.h"



class Combined_Tag : public Tagger_Tag
{
public:
#define Tagger_LookupCommentForFirstTagOnly 0x0001
	Combined_Tag(Tagger_Tag* tag1 = NULL, Tagger_Tag* tag2 = NULL, Tagger_Tag* tag3 = NULL, long flags = 0)
		: Tagger_Tag()
	{
		m_tag1 = tag1;
		m_tag2 = tag2;
		m_tag3 = tag3;
		m_flags = flags;
	}

	virtual wxString comment() const
	{
		wxString ret;
		if( m_tag1 )
		{
			ret = m_tag1->comment();
			if( !ret.IsEmpty() || m_flags&Tagger_LookupCommentForFirstTagOnly ) return ret;
		}
		if( m_tag2 )
		{
			ret = m_tag2->comment();
			if( !ret.IsEmpty() ) return ret;
		}
		if( m_tag3 )
		{
			return m_tag3->comment();
		}
		return wxT("");
	}

#define IMPLEMENT_GET_STR_FUNC(func) \
          virtual wxString func() const \
          { \
              wxString ret; \
              if( m_tag1 ) \
              { \
                  ret = m_tag1->func(); \
                  if( !ret.IsEmpty() ) return ret; \
              } \
              if( m_tag2 ) \
              { \
                  ret = m_tag2->func(); \
                  if( !ret.IsEmpty() ) return ret; \
              } \
              if( m_tag3 ) \
              { \
                  return m_tag3->func(); \
              } \
              return wxT(""); \
          }

	IMPLEMENT_GET_STR_FUNC(title)
	IMPLEMENT_GET_STR_FUNC(leadArtist)
	IMPLEMENT_GET_STR_FUNC(orgArtist)
	IMPLEMENT_GET_STR_FUNC(composer)
	IMPLEMENT_GET_STR_FUNC(album)
	IMPLEMENT_GET_STR_FUNC(genre)
	IMPLEMENT_GET_STR_FUNC(group)

#define IMPLEMENT_GET_LONG_FUNC(func) \
          virtual long func() const \
          { \
              long ret; \
              if( m_tag1 ) \
              { \
                  ret = m_tag1->func(); \
                  if( ret > 0 ) return ret; \
              } \
              if( m_tag2 ) \
              { \
                  ret = m_tag2->func(); \
                  if( ret > 0 ) return ret; \
              } \
              if( m_tag3 ) \
              { \
                  return m_tag3->func(); \
              } \
              return 0; \
          }

	IMPLEMENT_GET_LONG_FUNC(year)
	IMPLEMENT_GET_LONG_FUNC(beatsPerMinute)

#define IMPLEMENT_GET_LONG2_FUNC(func) \
          virtual void func(long& nr, long& count) const \
          { \
              nr = 0; count = 0; \
              if( m_tag1 ) \
              { \
                  m_tag1->func(nr, count); \
                  if( nr > 0 ) return; \
              } \
              if( m_tag2 ) \
              { \
                  m_tag2->func(nr, count); \
                  if( nr > 0 ) return; \
              } \
              if( m_tag3 ) \
              { \
                  m_tag3->func(nr, count); \
              } \
          }

	IMPLEMENT_GET_LONG2_FUNC(track)
	IMPLEMENT_GET_LONG2_FUNC(disk)

#define IMPLEMENT_SET_STR_FUNC(func) \
          virtual void func(const wxString& s) \
          { \
              prepareForWrite(); \
              if( m_tag1 ) m_tag1->func(s); \
              if( m_tag2 ) m_tag2->func(s); \
              if( m_tag3 ) m_tag3->func(s); \
          }

	IMPLEMENT_SET_STR_FUNC(setTitle);
	IMPLEMENT_SET_STR_FUNC(setLeadArtist);
	IMPLEMENT_SET_STR_FUNC(setOrgArtist);
	IMPLEMENT_SET_STR_FUNC(setComposer);
	IMPLEMENT_SET_STR_FUNC(setAlbum);
	IMPLEMENT_SET_STR_FUNC(setComment);
	IMPLEMENT_SET_STR_FUNC(setGenre);
	IMPLEMENT_SET_STR_FUNC(setGroup);

#define IMPLEMENT_SET_LONG_FUNC(func) \
          virtual void func(long i) \
          { \
              prepareForWrite(); \
              if( m_tag1 ) m_tag1->func(i); \
              if( m_tag2 ) m_tag2->func(i); \
              if( m_tag3 ) m_tag3->func(i); \
          }

	IMPLEMENT_SET_LONG_FUNC(setYear);
	IMPLEMENT_SET_LONG_FUNC(setBeatsPerMinute);

#define IMPLEMENT_SET_LONG2_FUNC(func) \
          virtual void func(long nr, long count) \
          { \
              prepareForWrite(); \
              if( m_tag1 ) m_tag1->func(nr, count); \
              if( m_tag2 ) m_tag2->func(nr, count); \
              if( m_tag3 ) m_tag3->func(nr, count); \
          }


	IMPLEMENT_SET_LONG2_FUNC(setTrack);
	IMPLEMENT_SET_LONG2_FUNC(setDisk);

protected:

	// prepareForWrite() may be implemtented in derived classes to
	// create missing tags as needed
	virtual void prepareForWrite() {}

	Tagger_Tag* m_tag1;
	Tagger_Tag* m_tag2;
	Tagger_Tag* m_tag3;
	int         m_flags;
};



#endif // __SJ_COMBINES_TAG_H__
