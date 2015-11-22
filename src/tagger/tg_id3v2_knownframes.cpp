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
#include "tg_id3v2_knownframes.h"
#include "tg_id3v2_tag.h"




/*******************************************************************************
 *  ID3v2_AttachedPictureFrame
 ******************************************************************************/


ID3v2_AttachedPictureFrame::ID3v2_AttachedPictureFrame() : ID3v2_Frame((unsigned char*)"APIC")
{
	m_textEncoding = SJ_LATIN1;
	m_type = ID3v2_Other;
}

ID3v2_AttachedPictureFrame::ID3v2_AttachedPictureFrame(const SjByteVector &data) : ID3v2_Frame(data)
{
	m_textEncoding = SJ_LATIN1;
	m_type = ID3v2_Other;
	setData(data);
}

ID3v2_AttachedPictureFrame::ID3v2_AttachedPictureFrame(const SjByteVector &data, ID3v2_FrameHeader *h)
	: ID3v2_Frame(h)
{
	m_textEncoding = SJ_LATIN1;
	m_type = ID3v2_Other;
	parseFields(fieldData(data));
}

/*
wxString ID3v2_AttachedPictureFrame::toString() const
{
  wxString s = "[" + m_mimeType + "]";
  return m_description.IsEmpty() ? s : m_description + " " + s;
}
*/

void ID3v2_AttachedPictureFrame::parseFields(const SjByteVector &data)
{
	if(data.size() < 5) {
		wxLogDebug(wxT("A picture frame must contain at least 5 bytes."));
		return;
	}

	int pos = 0, offset;

	// read text encoding
	m_textEncoding = (SjStringType)(data[pos]);
	pos += 1;

	if( header()->version() <= 2 )
	{
		// read image format (3 characters), valid for ID3V2_2_1 or older
		m_mimeType = data.mid(pos, 3).toString(SJ_LATIN1);
		pos += 3;
	}
	else
	{
		// read mime type (null-terminated), valid for newer specs
		offset = data.find(textDelimiter(SJ_LATIN1), pos);

		if(offset < pos)
			return;

		m_mimeType = data.mid(pos, offset - pos).toString(SJ_LATIN1);
		pos = offset + 1;
	}

	// read type
	m_type = (ID3v2_AttachedPictureType)(data[pos]);
	pos += 1;

	// read description
	offset = data.find(textDelimiter(m_textEncoding), pos);

	if(offset < pos)
		return;

	m_description = data.mid(pos, offset - pos).toString(m_textEncoding);
	pos = offset + 1;

	// read image data
	m_data = data.mid(pos);
}

SjByteVector ID3v2_AttachedPictureFrame::renderFields() const
{
	SjByteVector data;

	data.append((unsigned char)(m_textEncoding));
	data.appendString(m_mimeType, SJ_LATIN1);
	data.append(textDelimiter(SJ_LATIN1));
	data.append((unsigned char)(m_type));
	data.appendString(m_description, m_textEncoding);
	data.append(textDelimiter(m_textEncoding));
	data.append(m_data);

	return data;
}



/*******************************************************************************
 *  ID3v2_CommentsFrame
 ******************************************************************************/



ID3v2_CommentsFrame::ID3v2_CommentsFrame(SjStringType encoding) : ID3v2_Frame((unsigned char*)"COMM")
{
	m_textEncoding = encoding;
}

ID3v2_CommentsFrame::ID3v2_CommentsFrame(const SjByteVector &data) : ID3v2_Frame(data)
{
	m_textEncoding = SJ_LATIN1;
	setData(data);
}

ID3v2_CommentsFrame::ID3v2_CommentsFrame(const SjByteVector &data, ID3v2_FrameHeader *h) : ID3v2_Frame(h)
{
	m_textEncoding = SJ_LATIN1;
	parseFields(fieldData(data));
}

void ID3v2_CommentsFrame::setLanguage(const SjByteVector &languageEncoding)
{
	m_language = languageEncoding.mid(0, 3);
}

void ID3v2_CommentsFrame::parseFields(const SjByteVector &data)
{
	if(data.size() < 5) {
		wxLogDebug(wxT("A comment frame must contain at least 5 bytes."));
		return;
	}

	m_textEncoding = (SjStringType)(data[0]);
	m_language = data.mid(1, 3);

	int byteAlign = (m_textEncoding == SJ_LATIN1 || m_textEncoding == SJ_UTF8) ? 1 : 2;

	//ByteVectorList l = ByteVectorList::split(data.mid(4), textDelimiter(d->textEncoding), byteAlign, 2);
	SjArrayByteVector l = data.mid(4).splitToArray(textDelimiter(m_textEncoding), byteAlign, 2);

	if(l.GetCount() == 2) {
		/*d->description = String(l.front(), d->textEncoding);
		d->text = String(l.back(), d->textEncoding);
		*/
		m_description = l.Item(0).toString(m_textEncoding);
		m_text = l.Item(1).toString(m_textEncoding);
	}
}

SjByteVector ID3v2_CommentsFrame::renderFields() const
{
	SjByteVector v;

	v.append((unsigned char)(m_textEncoding));

	//v.append(d->language.size() == 3 ? d->language : "   ");
	if( m_language.size() == 3 )
	{
		v.append(m_language);
	}
	else
	{
		v.append((unsigned char*)"   ");
	}

	v.appendString(m_description, m_textEncoding);
	v.append(textDelimiter(m_textEncoding));
	v.appendString(m_text, m_textEncoding);

	return v;
}





/*******************************************************************************
 *  ID3v2_TextIdentificationFrame
 ******************************************************************************/




ID3v2_TextIdentificationFrame::ID3v2_TextIdentificationFrame(const SjByteVector &type, SjStringType encoding) :
	ID3v2_Frame(type)
{
	m_textEncoding = encoding;
}

ID3v2_TextIdentificationFrame::ID3v2_TextIdentificationFrame(const SjByteVector &data) :
	ID3v2_Frame(data)
{
	m_textEncoding = SJ_LATIN1;
	setData(data);
}

ID3v2_TextIdentificationFrame::ID3v2_TextIdentificationFrame(const SjByteVector &data, ID3v2_FrameHeader *h)
	: ID3v2_Frame(h)
{
	m_textEncoding = SJ_LATIN1;
	parseFields(fieldData(data));
}

wxString ID3v2_TextIdentificationFrame::toString() const
{
	// return SjTools::Implode(m_fieldList, wxT(" "));
	//
	// ^^^ this should only be done for frames we really know that multiple values are okay.
	//     currently this is true only for genres (where we use fieldList() explicitly for 2.4 tags)
	//
	// For other tags, simply return the first string as some taggers think,
	// the null byte ends the whole string (it does not; it introduces the next string in the field).
	// First we thought is is an good idea to return the first non-empty string,
	// but this will fail to catch the error, if the taggers mentioned above will write an empty string.
	if( m_fieldList.GetCount() > 0 )
		return m_fieldList[0];

	return wxEmptyString;
}

void ID3v2_TextIdentificationFrame::parseFields(const SjByteVector &data)
{
	// read the string data type (the first byte of the field data)

	m_textEncoding = (SjStringType)(data[0]);

	// split the byte array into chunks based on the string type (two byte delimiter
	// for unicode encodings)

	int byteAlign = m_textEncoding == SJ_LATIN1 || m_textEncoding == SJ_UTF8 ? 1 : 2;

	//ByteVectorList l = ByteVectorList::split(data.mid(1), textDelimiter(d->textEncoding), byteAlign);
	SjArrayByteVector l = data.mid(1).splitToArray(textDelimiter(m_textEncoding), byteAlign);

	m_fieldList.Empty();

	// append those split values to the list and make sure that the new string's
	// type is the same specified for this frame

	/*for(ByteVectorList::Iterator it = l.begin(); it != l.end(); it++) {
	  String s(*it, d->textEncoding);
	  d->fieldList.append(s);
	}
	*/
	int i, iCount = l.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		m_fieldList.Add(l.Item(i).toString(m_textEncoding));
	}
}

SjByteVector ID3v2_TextIdentificationFrame::renderFields() const
{
	SjByteVector v;

	if(m_fieldList.GetCount() > 0) {

		v.append((unsigned char)(m_textEncoding));

		/*for(StringList::Iterator it = d->fieldList.begin(); it != d->fieldList.end(); it++) {

		  // Since the field list is null delimited, if this is not the first
		  // element in the list, append the appropriate delimiter for this
		  // encoding.

		  if(it != d->fieldList.begin())
		    v.append(textDelimiter(d->textEncoding));

		  v.append((*it).data(d->textEncoding));
		}*/
		int i, iCount = m_fieldList.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			if( i )
				v.append(textDelimiter(m_textEncoding));

			v.appendString(m_fieldList.Item(i), m_textEncoding);
		}
	}

	return v;
}


ID3v2_UserTextIdentificationFrame::ID3v2_UserTextIdentificationFrame(SjStringType encoding) :
	ID3v2_TextIdentificationFrame((unsigned char*)"TXXX", encoding)
{
	wxArrayString l;
	l.Add(wxT(""));
	l.Add(wxT(""));
	setText(l);
}


ID3v2_UserTextIdentificationFrame::ID3v2_UserTextIdentificationFrame(const SjByteVector &data) :
	ID3v2_TextIdentificationFrame(data)
{
}
/*
wxString ID3v2_UserTextIdentificationFrame::toString() const
{
  return "[" + description() + "] " + SjTools::Implode(fieldList(), wxT(" "));
}
*/
wxString ID3v2_UserTextIdentificationFrame::description() const
{
	return !ID3v2_TextIdentificationFrame::fieldList().IsEmpty()
	       ? ID3v2_TextIdentificationFrame::fieldList().Item(0)
	       : wxT("");
}

wxArrayString ID3v2_UserTextIdentificationFrame::fieldList() const
{
	wxArrayString l = ID3v2_TextIdentificationFrame::fieldList();

	if(!l.IsEmpty()) {
		l.RemoveAt(0L, 1L);
	}

	return l;
}

void ID3v2_UserTextIdentificationFrame::setText(const wxString &text)
{
	if(description().IsEmpty())
		setDescription(wxT("")); // create the element!

	wxArrayString a;
	a.Add(description());
	a.Add(text);
	ID3v2_TextIdentificationFrame::setText(a);
}

void ID3v2_UserTextIdentificationFrame::setText(const wxArrayString &fields)
{
	if(description().IsEmpty())
		setDescription(wxT("")); // create the element!

	wxArrayString a = fields;
	a.Insert(description(), 0);
	ID3v2_TextIdentificationFrame::setText(a);
}

void ID3v2_UserTextIdentificationFrame::setDescription(const wxString &s)
{
	wxArrayString l = fieldList();

	if(l.IsEmpty())
		l.Add(s);
	else
		l[0] = s;

	ID3v2_TextIdentificationFrame::setText(l);
}

ID3v2_UserTextIdentificationFrame *find(ID3v2_Tag *tag, const wxString &description) // static
{
	const ID3v2_FrameList* l = tag->frameList(wxT("TXXX"));

	/*for(FrameList::Iterator it = l.begin(); it != l.end(); ++it) {
	  UserTextIdentificationFrame *f = dynamic_cast<UserTextIdentificationFrame *>(*it);
	  if(f && f->description() == description)
	    return f;
	}
	*/
	if( l )
	{
		for ( ID3v2_FrameList::Node *node = l->GetFirst(); node; node = node->GetNext() )
		{
			ID3v2_UserTextIdentificationFrame *f = (ID3v2_UserTextIdentificationFrame*)node->GetData();
			if( f && f->description() == description )
				return f;
		}
	}

	return 0;
}


ID3v2_UserTextIdentificationFrame::ID3v2_UserTextIdentificationFrame(const SjByteVector &data, ID3v2_FrameHeader *h) :
	ID3v2_TextIdentificationFrame(data, h)
{
}



/*******************************************************************************
 *  ID3v2_UniquefileIdentifierFrame
 ******************************************************************************/



#ifdef TAGGER_USE_UNIQUE_FILE_IDENTIFIER_FRAME
ID3v2_UniqueFileIdentifierFrame::ID3v2_UniqueFileIdentifierFrame(const SjByteVector &data) :
	ID3v2_Frame(data)
{
	setData(data);
}

ID3v2_UniqueFileIdentifierFrame::ID3v2_UniqueFileIdentifierFrame(const wxString &owner, const SjByteVector &id) :
	ID3v2_Frame((unsigned char*)"UFID")
{
	m_owner = owner;
	m_identifier = id;
}

void ID3v2_UniqueFileIdentifierFrame::parseFields(const SjByteVector &data)
{
	/*ByteVectorList fields = ByteVectorList::split(data, char(0));

	if(fields.size() != 2)
	    return;

	d->owner = fields.front();
	d->identifier = fields.back();*/
	SjArrayByteVector fields = data.splitToArray((unsigned char)"\0", 1L);

	if( fields.GetCount() != 2 )
		return;

	m_owner = fields.Item(0).toString(SJ_LATIN1);
	m_identifier = fields.Item(1);
}

SjByteVector ID3v2_UniqueFileIdentifierFrame::renderFields() const
{
	SjByteVector data;

	data.appendString(m_owner, SJ_LATIN1);
	data.append((unsigned char)(0));
	data.append(m_identifier);

	return data;
}

ID3v2_UniqueFileIdentifierFrame::ID3v2_UniqueFileIdentifierFrame(const SjByteVector &data, ID3v2_FrameHeader *h) :
	ID3v2_Frame(h)
{
	parseFields(fieldData(data));
}
#endif // TAGGER_USE_UNIQUE_FILE_IDENTIFIER_FRAME



/*******************************************************************************
 * ID3v2_PopularimeterFrame
 ******************************************************************************/

ID3v2_PopularimeterFrame::ID3v2_PopularimeterFrame()
	: ID3v2_Frame((unsigned char*)"POPM")
{
	m_rating255 = 0;
	m_counter = 0;
}

ID3v2_PopularimeterFrame::ID3v2_PopularimeterFrame(const SjByteVector &data)
	: ID3v2_Frame(data)
{
	setData(data);
}


void ID3v2_PopularimeterFrame::parseFields(const SjByteVector& data)
{
	m_email.Empty();
	m_rating255 = 0;
	m_counter = 0;

	int offset, pos = 0, size = (int)data.size();
	offset = data.find(textDelimiter(SJ_LATIN1), pos);
	if( offset < pos ) {
		return;
	}

	m_email = data.mid(pos, offset - pos).toString(SJ_LATIN1);
	pos = offset + 1;

	if(pos < size)
	{
		m_rating255 = (int)(data[pos]);
		pos++;

		if(pos < size)
		{
			m_counter = data.mid(pos, 4).toUInt();
		}
	}
}


wxString ID3v2_PopularimeterFrame::toString() const
{
	return wxString::Format("user=%s, rating=%i, cnt=%i", m_email.c_str(), (int)m_rating255, (int)m_counter);
}


SjByteVector ID3v2_PopularimeterFrame::renderFields() const
{
	SjByteVector data;

	data.appendString(m_email, SJ_LATIN1);
	data.append(textDelimiter(SJ_LATIN1));
	data.append((unsigned char)m_rating255);
	data.append(SjByteVector::fromUint((SjUint)m_counter));

	return data;
}


/*******************************************************************************
 *  ID3v2_UnknownFrame
 ******************************************************************************/



ID3v2_UnknownFrame::ID3v2_UnknownFrame(const SjByteVector &data) : ID3v2_Frame(data)
{
	setData(data);
}


ID3v2_UnknownFrame::ID3v2_UnknownFrame(const SjByteVector &data, ID3v2_FrameHeader *h) : ID3v2_Frame(h)
{
	parseFields(fieldData(data));
}

