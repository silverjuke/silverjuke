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
#include "tg_ogg_xiphcomment.h"


wxString Ogg_XiphComment::m_fieldListMapFirstString(const wxString& key) const
{
	wxArrayString* array = m_fieldListMapArray(key);
	if( array && !array->IsEmpty() )
	{
		return array->Item(0);
	}
	return wxT("");
}



Ogg_XiphComment::Ogg_XiphComment() : Tagger_Tag()
{
}

Ogg_XiphComment::Ogg_XiphComment(const SjByteVector &data) : Tagger_Tag()
{
	parse(data);
}

Ogg_XiphComment::~Ogg_XiphComment()
{
	SjHashIterator iterator;
	wxString key;
	wxArrayString* a;
	while( (a = (wxArrayString*)m_fieldListMap.Iterate(iterator, key)) )
	{
		delete a;
	}
}



wxString Ogg_XiphComment::comment() const
{
	/*if(!d->fieldListMap["DESCRIPTION"].isEmpty()) {
	  d->commentField = "DESCRIPTION";
	  return d->fieldListMap["DESCRIPTION"].front();
	}

	if(!d->fieldListMap["COMMENT"].isEmpty()) {
	  d->commentField = "COMMENT";
	  return d->fieldListMap["COMMENT"].front();
	}*/

	wxArrayString* a = m_fieldListMapArray(wxT("DESCRIPTION"));
	if( a && !a->IsEmpty() )
	{
		m_commentField = wxT("DESCRIPTION");
		return a->Item(0);
	}

	a = m_fieldListMapArray(wxT("COMMENT"));
	if( a && !a->IsEmpty() )
	{
		m_commentField = wxT("COMMENT");
		return a->Item(0);
	}

	return wxT("");
}

long Ogg_XiphComment::year() const
{
	wxString str = m_fieldListMapFirstString(wxT("DATE"));
	if( str.Len() > 4 )
	{
		str = str.Right(4);
	}
	long ret = 0;
	if( !str.ToLong(&ret) )
		ret = 0;
	return ret;
}

long Ogg_XiphComment::beatsPerMinute() const
{
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	wxString str = m_fieldListMapFirstString(wxT("BPM"));
	long ret = 0;
	if( !str.ToLong(&ret) )
		ret = 0;
	return ret;
}

void Ogg_XiphComment::track(long& nr, long& count) const
{
	explodeNrAndCount(m_fieldListMapFirstString(wxT("TRACKNUMBER")), nr, count);
}

void Ogg_XiphComment::disk(long& nr, long& count) const
{
	// see http://age.hobba.nl/audio/tag_frame_reference.html
	explodeNrAndCount(m_fieldListMapFirstString(wxT("DISCNUMBER")), nr, count);
}

void Ogg_XiphComment::setTitle(const wxString &s)
{
	addField(wxT("TITLE"), s);
}

void Ogg_XiphComment::setLeadArtist(const wxString &s)
{
	addField(wxT("ARTIST"), s);
}

void Ogg_XiphComment::setOrgArtist(const wxString &s)
{
	// "AUTHOR" is documented but not the same as the original artist;
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	// see http://age.hobba.nl/audio/tag_frame_reference.html
	addField(wxT("ORIGARTIST"), s);
}

void Ogg_XiphComment::setComposer(const wxString &s)
{
	// see http://age.hobba.nl/audio/tag_frame_reference.html
	addField(wxT("COMPOSER"), s);
}

void Ogg_XiphComment::setAlbum(const wxString &s)
{
	addField(wxT("ALBUM"), s);
}

void Ogg_XiphComment::setComment(const wxString &s)
{
	addField(m_commentField.IsEmpty() ? wxT("DESCRIPTION") : m_commentField, s);
}

void Ogg_XiphComment::setGenre(const wxString &s)
{
	addField(wxT("GENRE"), s);
}

void Ogg_XiphComment::setGroup(const wxString &s)
{
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	addField(wxT("CONTENTGROUP"), s);
}

void Ogg_XiphComment::setYear(long i)
{
	if(i <= 0)
	{
		removeField(wxT("DATE"));
	}
	else
	{
		addField(wxT("DATE"), wxString::Format(wxT("%i"), (int)i));
	}
}

void Ogg_XiphComment::setBeatsPerMinute(long i)
{
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	if( i <= 0 )
	{
		removeField(wxT("BPM"));
	}
	else
	{
		addField(wxT("BPM"), wxString::Format(wxT("%i"), (int)i));
	}
}

void Ogg_XiphComment::setTrack(long nr, long count)
{
	if( nr <= 0 )
	{
		removeField(wxT("TRACKNUMBER"));
	}
	else
	{
		addField(wxT("TRACKNUMBER"), implodeNrAndCount(nr, 0 /*don't know if it is okay to add the count here*/));
	}
}

void Ogg_XiphComment::setDisk(long nr, long count)
{
	// see http://age.hobba.nl/audio/tag_frame_reference.html
	if( nr <= 0 )
	{
		removeField(wxT("DISCNUMBER"));
	}
	else
	{
		addField(wxT("DISCNUMBER"), implodeNrAndCount(nr, 0 /*don't know if it is okay to add the count here*/));
	}
}

bool Ogg_XiphComment::isEmpty() const
{
	/*FieldListMap::ConstIterator it = d->fieldListMap.begin();
	for(; it != d->fieldListMap.end(); ++it)
	  if(!(*it).second.isEmpty())
	    return false;*/
	SjHashIterator iterator;
	wxString key;
	wxArrayString* a;
	while( (a = (wxArrayString*)m_fieldListMap.Iterate(iterator, key)) )
	{
		if( !a->IsEmpty() )
		{
			return FALSE;
		}
	}

	return true;
}

SjUint Ogg_XiphComment::fieldCount() const
{
	SjUint count = 0;

	/*FieldListMap::ConstIterator it = d->fieldListMap.begin();
	for(; it != d->fieldListMap.end(); ++it)
	  count += (*it).second.size();
	*/
	SjHashIterator iterator;
	wxString key;
	wxArrayString* a;
	while( (a = (wxArrayString*)m_fieldListMap.Iterate(iterator, key)) )
	{
		count += a->GetCount();
	}

	return count;
}


void Ogg_XiphComment::addField(const wxString &key, const wxString &value, bool replace)
{
	if(replace)
	{
		removeField(key.Upper());
	}

	if(!key.IsEmpty())
	{
		//d->fieldListMap[key.Upper()].append(value);
		wxArrayString* a = m_fieldListMapArray(key.Upper()); // before 2.10beta5 we forgot the Upper()
		if( a==NULL )
		{
			a = new wxArrayString;
			m_fieldListMap.Insert(key.Upper(), a); // before 2.10beta5 we forgot the Upper()
		}

		a->Add(value);
	}
}

void Ogg_XiphComment::removeField(const wxString &key, const wxString &value)
{
	wxArrayString* a = m_fieldListMapArray(key);
	if( a )
	{
		if( !value.IsEmpty() )
		{
			/*StringList::Iterator it = d->fieldListMap[key].begin();
			for(; it != d->fieldListMap[key].end(); ++it) {
			  if(value == *it)
			    d->fieldListMap[key].erase(it);
			}*/
			int i, iCount = a->GetCount();
			for( i = 0; i < iCount; i++ )
			{
				if( a->Item(i) == value )
				{
					a->RemoveAt(i);
				}
			}
		}
		else
		{
			//d->fieldListMap[key].clear();
			a->Clear();
		}
	}
}

SjByteVector Ogg_XiphComment::render() const
{
	return render(true);
}

SjByteVector Ogg_XiphComment::render(bool addFramingBit) const
{
	SjByteVector data;

	// Add the vendor ID length and the vendor ID.  It's important to use the
	// lenght of the data(String::UTF8) rather than the lenght of the the string
	// since this is UTF8 text and there may be more characters in the data than
	// in the UTF16 string.

	//ByteVector vendorData = d->vendorID.data(String::UTF8);
	SjByteVector vendorData;
	vendorData.appendString(m_vendorID, SJ_UTF8);

	data.append(SjByteVector::fromUint(vendorData.size(), false));
	data.append(vendorData);

	// Add the number of fields.

	data.append(SjByteVector::fromUint(fieldCount(), false));

	// Iterate over the the field lists.  Our iterator returns a
	// std::pair<String, StringList> where the first String is the field name and
	// the StringList is the values associated with that field.

	/*FieldListMap::ConstIterator it = d->fieldListMap.begin();
	for(; it != d->fieldListMap.end(); ++it)*/
	wxArrayString* values;
	wxString fieldName;
	SjHashIterator iterator;
	while( (values=(wxArrayString*)m_fieldListMap.Iterate(iterator, fieldName)) )
	{

		// And now iterate over the values of the current list.

		/*String fieldName = (*it).first;
		StringList values = (*it).second;*/

		/*StringList::ConstIterator valuesIt = values.begin();
		for(; valuesIt != values.end(); ++valuesIt)*/
		int i, iCount = values->GetCount();
		for( i = 0; i < iCount; i++ )
		{
			SjByteVector fieldData;
			fieldData.appendString(fieldName, SJ_UTF8);
			fieldData.append((unsigned char)'=');
			fieldData.appendString(values->Item(i), SJ_UTF8);

			data.append(SjByteVector::fromUint(fieldData.size(), false));
			data.append(fieldData);
		}
	}

	// Append the "framing bit".

	if(addFramingBit)
		data.append((unsigned char)(1));

	return data;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void Ogg_XiphComment::parse(const SjByteVector &data)
{
	// The first thing in the comment data is the vendor ID length, followed by a
	// UTF8 string with the vendor ID.

	int pos = 0;

	int vendorLength = data.mid(0, 4).toUInt(false);
	pos += 4;

	m_vendorID = data.mid(pos, vendorLength).toString(SJ_UTF8);
	pos += vendorLength;

	// Next the number of fields in the comment vector.

	int commentFields = data.mid(pos, 4).toUInt(false);
	pos += 4;

	for(int i = 0; i < commentFields; i++) {

		// Each comment field is in the format "KEY=value" in a UTF8 string and has
		// 4 bytes before the text starts that gives the length.

		int commentLength = data.mid(pos, 4).toUInt(false);
		pos += 4;

		wxString comment = data.mid(pos, commentLength).toString(SJ_UTF8);
		pos += commentLength;

		int commentSeparatorPosition = comment.find(wxT("="));

		wxString key = comment.substr(0, commentSeparatorPosition);
		wxString value = comment.substr(commentSeparatorPosition + 1);

		addField(key, value, false);
	}
}
