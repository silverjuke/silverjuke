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

#include "tg_id3v2_tag.h"
#include "tg_id3v1_tag.h"
#include "tg_id3v2_knownframes.h"



ID3v2_Tag::ID3v2_Tag(Tagger_File *file, long tagOffset, const ID3v2_FrameFactory *factory) :
	Tagger_Tag()
{
	m_file = file;
	m_tagOffset = tagOffset;
	m_extendedHeader = NULL;
	m_footer = NULL;
	m_paddingSize = 0;
	m_factory = factory;

	if( file )
	{
		read();
	}
}



ID3v2_Tag::~ID3v2_Tag()
{
	if( m_extendedHeader) delete m_extendedHeader;
	if( m_footer ) delete m_footer;

	// delete all frame list contents, the frame list itself is destroyed automatically
	ID3v2_FrameList::Node* node = m_frameList.GetFirst();
	for( ; node; node = node->GetNext() )
	{
		delete node->GetData();
	}

	// delete all frame hash values, the hash itself is destroyed automatically
	SjHashIterator iterator;
	ID3v2_FrameList* frameList;
	wxString frameId;
	while( (frameList=(ID3v2_FrameList*)m_frameListMap.Iterate(iterator, frameId)) )
	{
		delete frameList;
	}
}



wxString ID3v2_Tag::simpleFrame(const wxString& frameId) const
{
	ID3v2_FrameList* list = frameList(frameId);
	if( list )
	{
		ID3v2_FrameList::Node* node = list->GetFirst();
		if( node )
		{
			return node->GetData()->toString();
		}
	}
	return wxEmptyString;
}




wxString ID3v2_Tag::title() const
{
	return simpleFrame(wxT("TIT2"));
	/*if(!d->frameListMap["TIT2"].isEmpty())
	  return d->frameListMap["TIT2"].front()->toString();
	return String::null;*/
}



wxString ID3v2_Tag::leadArtist() const
{
	wxString ret;
	ret =  simpleFrame(wxT("TPE1")); if( !ret.IsEmpty() ) { return ret; }
	ret =  simpleFrame(wxT("TPE2")); if( !ret.IsEmpty() ) { return ret; }
	ret =  simpleFrame(wxT("TPE3")); if( !ret.IsEmpty() ) { return ret; }
	ret =  simpleFrame(wxT("TPE4")); if( !ret.IsEmpty() ) { return ret; }
	ret =  simpleFrame(wxT("TOPE")); if( !ret.IsEmpty() ) { return ret; }
	return simpleFrame(wxT("TCOM"));
	/*
	  if(!d->frameListMap["TPE1"].isEmpty())
	    return d->frameListMap["TPE1"].front()->toString();
	  return String::null;
	  */
}



wxString ID3v2_Tag::orgArtist() const
{
	return simpleFrame(wxT("TOPE"));
}



wxString ID3v2_Tag::composer() const
{
	return simpleFrame(wxT("TCOM"));
}



wxString ID3v2_Tag::album() const
{
	return simpleFrame(wxT("TALB"));
	/*if(!d->frameListMap["TALB"].isEmpty())
	  return d->frameListMap["TALB"].front()->toString();
	return String::null;*/
}


ID3v2_CommentsFrame* ID3v2_Tag::findCommentFrame() const
{
	ID3v2_FrameList*        list = frameList(wxT("COMM"));
	ID3v2_CommentsFrame*    frame;
	if( list )
	{
		// prefer comments without description, start searching with the last (mostly the newest)
		ID3v2_FrameList::Node* node = list->GetLast();
		while( node )
		{
			frame = (ID3v2_CommentsFrame*)node->GetData();
			if(  frame->description().IsEmpty()
			        && !frame->text().IsEmpty() )
			{
				return frame;
			}
			node = node->GetPrevious();
		}

		// try to return empty frames without description
		list->GetLast();
		while( node )
		{
			frame = (ID3v2_CommentsFrame*)node->GetData();
			if(  frame->description().IsEmpty() )
			{
				return frame;
			}
			node = node->GetPrevious();
		}

		/*
		if( !forWrite ) // do not overwrite eg. MusicMatch comments
		{
		    // okay, no comment so far, check all comment frames and return the first non-empty comment
		    node = list->GetFirst();
		    while( node )
		    {
		        frame = (ID3v2_CommentsFrame*)node->GetData();
		        if( !frame->text().IsEmpty() )
		        {
		            return frame;
		        }
		        node = node->GetNext();
		    }

		    // just use the first frame
		    node = list->GetFirst();
		    if( node )
		    {
		        return (ID3v2_CommentsFrame*)node->GetData();
		    }
		}*/
	}
	return NULL;
}

wxString ID3v2_Tag::comment() const
{
	// edit by me: if there are several
	// COMM frames, id3lib and Winamp use the last frame as the current comment.
	// so do we.
	ID3v2_Frame* f = findCommentFrame();
	if( f )
	{
		return f->toString();
	}


	return wxEmptyString;


	//return simpleFrame("COMM");

	/*if(!d->frameListMap["COMM"].isEmpty())
	  return d->frameListMap["COMM"].front()->toString();
	return String::null;*/
}



wxString ID3v2_Tag::genre() const
{
	// TODO: In the next major version (Tagger 2.0) a list of multiple genres
	// should be separated by " / " instead of " ".  For the moment to keep
	// the behavior the same as released versions it is being left with " ".

	/*if(!d->frameListMap["TCON"].isEmpty() &&
	    dynamic_cast<TextIdentificationFrame *>(d->frameListMap["TCON"].front()))
	begin
	    Frame *frame = d->frameListMap["TCON"].front();*/
	ID3v2_FrameList* it = frameList(wxT("TCON"));
	if( it && it->GetFirst() )
	{
		ID3v2_Frame* frame = it->GetFirst()->GetData();

		// ID3v2.4 lists genres as the fields in its frames field list.  If the field
		// is simply a number it can be assumed that it is an ID3v1 genre number.
		// Here was assume that if an ID3v1 string is present that it should be
		// appended to the genre string.  Multiple fields will be appended as the
		// string is built.

		if(m_header.majorVersion() == 4)
		{
			ID3v2_TextIdentificationFrame *f = (ID3v2_TextIdentificationFrame *)(frame);
			wxArrayString fields = f->fieldList();

			wxString genreString;
			bool hasNumber = false;

			//for(StringList::ConstIterator it = fields.begin(); it != fields.end(); ++it)
			int i, iCount = fields.GetCount();
			for( i = 0; i < iCount; i++ )
			{
				wxString itStr = fields.Item(i);
				int j, jCount = itStr.Len();
				if( jCount > 0 ) // skip empty strings as they're converted to "Blues" otherwise
				{
					bool isNumber = true;
					/*for(String::ConstIterator charIt = (*it).begin();
					    isNumber && charIt != (*it).end();
					    ++charIt)
					{
					    isNumber = *charIt >= '0' && *charIt <= '9';
					}*/

					for( j = 0; j < jCount; j++ )
					{
						char charIt = itStr.GetChar(j);
						if( charIt < '0' || charIt > '9' )
						{
							isNumber = false;
							break;
						}
					}

					if(!genreString.IsEmpty())
						genreString.Append(' ');

					if(isNumber)
					{
						//int number = (*itStr).toInt();
						long number;
						itStr.ToLong(&number);
						if(number >= 0 && number <= 255)
						{
							hasNumber = true;
							genreString.Append(ID3v1_Tag::lookupGenreName(number));
						}
					}
					else
					{
						genreString.append(itStr);
					}
				}
			} // for

			if(hasNumber)
				return genreString;
		}

		wxString s = frame->toString();

		// ID3v2.3 "content type" can contain a ID3v1 genre number in parenthesis at
		// the beginning of the field.  If this is all that the field contains, do a
		// translation from that number to the name and return that.  If there is a
		// string folloing the ID3v1 genre number, that is considered to be
		// authoritative and we return that instead.  Or finally, the field may
		// simply be free text, in which case we just return the value.

		int closing = s.find(wxT(")"));
		if(s.substr(0, 1) == wxT("(") && closing > 0)
		{
			if(closing == int(s.size() - 1))
			{
				long lng;
				wxString temp = s.substr(1, s.size() - 2);
				temp.ToLong(&lng);
				return ID3v1_Tag::lookupGenreName(lng);
			}
			else
				return s.substr(closing + 1);
		}

		return s;
	}

	return wxT("");
}



wxString ID3v2_Tag::group() const
{
	return simpleFrame(wxT("TIT1"));
}



long ID3v2_Tag::year() const
{
	/*if(!d->frameListMap["TDRC"].isEmpty())
	return d->frameListMap["TDRC"].front()->toString().substr(0, 4).toInt();*/
	wxString ret = simpleFrame(wxT("TDRC"));
	if( ret.IsEmpty() )
	{
		ret = simpleFrame(wxT("TYER"));
	}

	if( !ret.IsEmpty() ) {
		if(ret.Len()>4) ret = ret.Left(4);
		long l = 0;
		ret.ToLong(&l);
		return l;
	}
	return 0;
}



long ID3v2_Tag::beatsPerMinute() const
{
	wxString ret = simpleFrame(wxT("TBPM"));
	if( !ret.IsEmpty() ) {
		long l = 0;
		ret.ToLong(&l);
		return l;
	}
	return 0;
}



void ID3v2_Tag::track(long& nr, long& count) const
{
	explodeNrAndCount(simpleFrame(wxT("TRCK")), nr, count);
}



void ID3v2_Tag::disk(long& nr, long& count) const
{
	explodeNrAndCount(simpleFrame(wxT("TPOS")), nr, count);
}



void ID3v2_Tag::setTitle(const wxString &s)
{
	setTextFrame(wxT("TIT2"), s);
}



void ID3v2_Tag::setLeadArtist(const wxString &s)
{
	setTextFrame(wxT("TPE1"), s);
}



void ID3v2_Tag::setOrgArtist(const wxString &s)
{
	setTextFrame(wxT("TOPE"), s);
}



void ID3v2_Tag::setComposer(const wxString &s)
{
	setTextFrame(wxT("TCOM"), s);
}



void ID3v2_Tag::setAlbum(const wxString &s)
{
	setTextFrame(wxT("TALB"), s);
}



void ID3v2_Tag::setGenre(const wxString &s)
{
	if(s.IsEmpty())
	{
		removeFrames(wxT("TCON"));
		return;
	}

	int index = ID3v1_Tag::lookupGenreIndex(s);
	if(index != 255)
	{
		setTextFrame(wxT("TCON"), wxString::Format(wxT("%i"), index));
	}
	else
	{
		setTextFrame(wxT("TCON"), s);
	}
}



void ID3v2_Tag::setGroup(const wxString &s)
{
	setTextFrame(wxT("TIT1"), s);
}



void ID3v2_Tag::setYear(long i)
{
	if(i <= 0)
	{
		removeFrames(wxT("TDRC"));
		return;
	}

	setTextFrame(wxT("TDRC"), wxString::Format(wxT("%i"), (int)i));
}



void ID3v2_Tag::setBeatsPerMinute(long i)
{
	if(i <= 0)
	{
		removeFrames(wxT("TBPM"));
		return;
	}

	setTextFrame(wxT("TBPM"), wxString::Format(wxT("%i"), (int)i));
}



void ID3v2_Tag::setTrack(long nr, long count)
{
	if(nr <= 0)
	{
		removeFrames(wxT("TRCK"));
		return;
	}

	setTextFrame(wxT("TRCK"), implodeNrAndCount(nr, count));
}



void ID3v2_Tag::setDisk(long nr, long count)
{
	if(nr <= 0)
	{
		removeFrames(wxT("TPOS"));
		return;
	}

	setTextFrame(wxT("TPOS"), implodeNrAndCount(nr, count));
}



bool ID3v2_Tag::isEmpty() const
{
	return m_frameList.IsEmpty();
}



ID3v2_FrameList* ID3v2_Tag::frameList(const wxString &frameID) const
{
	return (ID3v2_FrameList*)m_frameListMap.Lookup(frameID);
}



void ID3v2_Tag::addFrame(ID3v2_Frame *frame)
{
	m_frameList.Append(frame);

	ID3v2_FrameList* l = frameList(frame->frameID().toString(SJ_LATIN1));
	if( l == NULL )
	{
		l = new ID3v2_FrameList;
		m_frameListMap.Insert(frame->frameID().toString(SJ_LATIN1), l);
	}
	l->Append(frame);
}

void ID3v2_Tag::removeFrame(ID3v2_Frame *frame, bool del)
{
	// remove the frame from the frame list
	/*FrameList::Iterator it = d->frameList.find(frame);
	d->frameList.erase(it);*/
	m_frameList.DeleteObject(frame); // this does not delete the object itself!


	// ...and from the frame list map
	/*it = d->frameListMap[frame->frameID()].find(frame);
	d->frameListMap[frame->frameID()].erase(it);*/
	ID3v2_FrameList* it = frameList(frame->frameID().toString(SJ_LATIN1));
	if( it )
	{
		it->DeleteObject(frame);
	}

	// ...and delete as desired
	if(del)
		delete frame;
}

void ID3v2_Tag::removeFrames(const wxString &id)
{
	/*FrameList l = d->frameListMap[id];
	for(FrameList::Iterator it = l.begin(); it != l.end(); ++it)
	  removeFrame(*it, true);*/
	ID3v2_FrameList* l = frameList(id);
	if( l )
	{
		ID3v2_FrameList::Node* first;
		while( (first=l->GetFirst()) != NULL )
		{
			removeFrame(first->GetData(), true);
		}
	}
}

SjByteVector ID3v2_Tag::render()
{
	// We need to render the "tag data" first so that we have to correct size to
	// render in the tag's header.  The "tag data" -- everything that is included
	// in ID3v2::Header::tagSize() -- includes the extended header, frames and
	// padding, but does not include the tag's header or footer.

	SjByteVector tagData;

	// TODO: Render the extended header.

	// Loop through the frames rendering them and adding them to the tagData.

	//for(FrameList::Iterator it = d->frameList.begin(); it != d->frameList.end(); it++)
	for ( ID3v2_FrameList::Node *node = m_frameList.GetFirst(); node; node = node->GetNext() )
	{
		ID3v2_Frame* it = node->GetData();
		if(!it->header()->tagAlterPreservation())
			tagData.append(it->render());
	}

	// Compute the amount of padding, and append that to tagData.

	SjUint paddingSize = 0;
	SjUint originalSize = m_header.tagSize();

	if(tagData.size() < originalSize)
		paddingSize = originalSize - tagData.size();
	else
		paddingSize = 1024;

	tagData.append(SjByteVector(paddingSize, char(0)));

	// Set the tag size.
	m_header.setTagSize(tagData.size());

	// TODO: This should eventually include d->footer->render().
	return m_header.render() + tagData;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void ID3v2_Tag::read()
{
	if(m_file ) {

		m_file->Seek(m_tagOffset);
		m_header.setData(m_file->ReadBlock(ID3v2_Header::size()));

		// if the tag size is 0, then this is an invalid tag (tags must contain at
		// least one frame)

		if(m_header.tagSize() == 0)
			return;

		parse(m_file->ReadBlock(m_header.tagSize()));
	}
}

void ID3v2_Tag::parse(const SjByteVector &data)
{
	SjUint frameDataPosition = 0;
	SjUint frameDataLength = data.size();

	// check for extended header

	if(m_header.extendedHeader()) {
		if(!m_extendedHeader)
			m_extendedHeader = new ID3v2_ExtendedHeader;
		m_extendedHeader->setData(data);
		if(m_extendedHeader->size() <= data.size()) {
			frameDataPosition += m_extendedHeader->size();
			frameDataLength -= m_extendedHeader->size();
		}
	}

	// check for footer -- we don't actually need to parse it, as it *must*
	// contain the same data as the header, but we do need to account for its
	// size.

	if(m_header.footerPresent() && ID3v2_Footer::size() <= frameDataLength)
		frameDataLength -= ID3v2_Footer::size();

	// parse frames

	// Make sure that there is at least enough room in the remaining frame data for
	// a frame header.

	while(frameDataPosition < frameDataLength - ID3v2_Frame::headerSize(m_header.majorVersion())) {

		// If the next data is position is 0, assume that we've hit the padding
		// portion of the frame data.

		if(data.at(frameDataPosition) == 0) {
			if(m_header.footerPresent())
				wxLogDebug(wxT("Padding *and* a footer found.  This is not allowed by the spec."));

			m_paddingSize = frameDataLength - frameDataPosition;
			return;
		}

		ID3v2_Frame *frame = m_factory->createFrame(data.mid(frameDataPosition),
		                     m_header.majorVersion());

		if(!frame)
			return;

		// get the next frame position

		frameDataPosition += frame->size() + ID3v2_Frame::headerSize(m_header.majorVersion());

		// add the frame if it has a size of at least 1 byte (smaller frames are not allowed
		// by the specification, but they're returned from createFrame() to allow seeking to the
		// next frame).
		// modification by me

		if(frame->size() <= 0) {
			delete frame;
		}
		else {
			addFrame(frame);
		}
	}
}

void ID3v2_Tag::setComment(const wxString &s)
{
	//if(s.IsEmpty()) {
	//  removeFrames("COMM");
	//  return;
	//}

	//if(!d->frameListMap["COMM"].isEmpty())
	//  d->frameListMap["COMM"].front()->setText(s);

	// we do not remove all frames as this will remove MusicMatch etc.

	ID3v2_CommentsFrame* f = findCommentFrame();
	if( f )
	{
		if( s.IsEmpty() )
		{
			removeFrame(f);
		}
		else
		{
			f->setText(s);
		}
	}
	else if( !s.IsEmpty() )
	{
		f = new ID3v2_CommentsFrame(m_factory->defaultTextEncoding());
		addFrame(f);
		f->setText(s);
	}
}


void ID3v2_Tag::setTextFrame(const wxString &id, const wxString &value)
{
	if(value.IsEmpty()) {
		removeFrames(id);
		return;
	}

	//if(!d->frameListMap[id].isEmpty())
	//  d->frameListMap[id].front()->setText(value);
	ID3v2_FrameList* it = frameList(id);
	if( it )
	{
		ID3v2_FrameList::Node* l = it->GetFirst();
		if( l )
		{
			l->GetData()->setText(value);
		}
	}
	else
	{
		const SjStringType encoding = m_factory->defaultTextEncoding();

		SjByteVector idBv;
		idBv.appendString(id, SJ_LATIN1);
		ID3v2_TextIdentificationFrame *f = new ID3v2_TextIdentificationFrame(idBv, encoding);

		addFrame(f);
		f->setText(value);
	}
}
