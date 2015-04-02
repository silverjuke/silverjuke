/***************************************************************************
    copyright            : (C) 2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.com
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
#include "tg_ape_tag.h"



/*******************************************************************************
 *  APE_Item
 ******************************************************************************/




void APE_Item::init()
{
	// this function should be called from within the constructor, it does not clear all fields!
	m_type      = APE_ItemText;
	m_readOnly  = false;
}



void APE_Item::copyFrom(const APE_Item& o)
{
	m_type      = o.m_type;
	m_readOnly  = o.m_readOnly;
	m_key       = o.m_key;
	m_binary    = o.m_binary;
	m_stringList= o.m_stringList;
}



APE_Item::APE_Item()
{
	init();
}



APE_Item::APE_Item(const wxString &key, const wxString &value)
{
	init();
	m_key = key;
	m_stringList.Add(value);
}



APE_Item::APE_Item(const wxString &key, const wxArrayString &values)
{
	init();
	m_key = key;
	m_stringList = values;
}



APE_Item::APE_Item(const APE_Item& o)
{
	copyFrom(o);
}



int APE_Item::size() const
{
	return 8 + m_key.size() + 1 + m_binary.size();
}



wxString APE_Item::toString() const
{
	return isEmpty() ? wxString(wxT("")) : m_stringList[0];
}



bool APE_Item::isEmpty() const
{
	switch(m_type)
	{
		case APE_ItemText: // 0
		case APE_ItemBinary: // 1
			if(m_stringList.IsEmpty())
				return true;
			if(m_stringList.GetCount() == 1 && m_stringList.Item(0).IsEmpty())
				return true;
			return false;

		case APE_ItemLocator: // 2
			return m_binary.isEmpty();

		default:
			return false;
	}
}



void APE_Item::parse(const SjByteVector &data)
{
	// 11 bytes is the minimum size for an APE item
	if(data.size() < 11)
	{
		wxLogDebug(wxT("APE::Item::parse() -- no data in item"));
		return;
	}

	SjUint valueLength  = data.mid(0, 4).toUInt(false);
	SjUint flags        = data.mid(4, 4).toUInt(false);

	m_key = data.mid(8).toString(SJ_UTF8); // data.mid(8) contains more than just the string -- but SjBytevector only converts up to the first null-byte at (***)
	m_binary = data.mid(8 + m_key.size() + 1, valueLength);

	setReadOnly(flags & 1);
	setType((APE_ItemType)((flags >> 1) & 3));

	if(int(m_type) < 2)
	{
		m_stringList = m_binary.splitToStrings((unsigned char)'\0', SJ_UTF8);
	}
}



SjByteVector APE_Item::render() const
{
	SjByteVector data;
	SjUint flags = ((m_readOnly) ? 1 : 0) | (m_type << 1);
	SjByteVector value;

	if(isEmpty())
		return data;

	if(m_type != APE_ItemBinary)
	{
		int i, iCount = m_stringList.GetCount();
		if( iCount>0 )
		{
			value.appendString(m_stringList.Item(0), SJ_UTF8);
			for( i = 1; i < iCount; i++ )
			{
				value.append((unsigned char)'\0');
				value.appendString(m_stringList.Item(i), SJ_UTF8);
			}
		}

		// there should be no need to set back m_binary
	}
	else
	{
		value.append(m_binary);
	}

	data.append(SjByteVector::fromUint(value.size(), false));
	data.append(SjByteVector::fromUint(flags, false));
	data.appendString(m_key, SJ_UTF8);
	data.append(SjByteVector((unsigned char)'\0'));
	data.append(value);

	return data;
}



/*******************************************************************************
 *  APE_Footer
 ******************************************************************************/



SjByteVector APE_Footer::fileIdentifier()
{
	return SjByteVector::fromCString("APETAGEX");
}



APE_Footer::APE_Footer(const SjByteVector* data)
{
	m_version = 0;
	m_footerPresent = true;
	m_headerPresent = false;
	m_isHeader = false;
	m_itemCount = 0;
	m_tagSize = 0;
	if( data )
	{
		parse(*data);
	}
}



SjUint APE_Footer::completeTagSize() const
{
	if(m_headerPresent)
	{
		return m_tagSize + APE_FOOTER_SIZE;
	}
	else
	{
		return m_tagSize;
	}
}



SjByteVector APE_Footer::renderFooter() const
{
	return render(false);
}



SjByteVector APE_Footer::renderHeader() const
{
	if (!m_headerPresent)
		return SjByteVector();

	return render(true);
}

void APE_Footer::parse(const SjByteVector &data)
{
	if( data.size() < APE_FOOTER_SIZE )
	{
		return;
	}

	// The first eight bytes, data[0..7], are the File Identifier, "APETAGEX".

	// Read the version number
	m_version = data.mid(8, 4).toUInt(false);

	// Read the tag size
	m_tagSize = data.mid(12, 4).toUInt(false);

	// Read the item count
	m_itemCount = data.mid(16, 4).toUInt(false);

	// Read the flags
	unsigned long flags = (unsigned long)data.mid(20, 4).toUInt(false);
	m_headerPresent =   (flags & (1<<31))!=0;
	m_footerPresent = !((flags & (1<<30))!=0);
	m_isHeader      =   (flags & (1<<29))!=0;
}



SjByteVector APE_Footer::render(bool isHeader) const
{
	SjByteVector v;

	// add the file identifier -- "APETAGEX"
	v.append(fileIdentifier());

	// add the version number -- we always render a 2.000 tag regardless of what
	// the tag originally was.
	v.append(SjByteVector::fromUint(2000, false));

	// add the tag size
	v.append(SjByteVector::fromUint(m_tagSize, false));

	// add the item count
	v.append(SjByteVector::fromUint(m_itemCount, false));

	// render and add the flags (footer is always present)
	unsigned long flags = 0;
	flags |= m_headerPresent? 0x80000000UL : 0UL;
	flags |= isHeader? 0x20000000UL : 0UL;

	v.append(SjByteVector::fromUint(flags, false));

	// add the reserved 64bit
	v.append(SjByteVector::fromLongLong(0));

	// done
	return v;
}



/*******************************************************************************
 *  APE_Tag
 ******************************************************************************/



APE_Tag::APE_Tag(Tagger_File *file, long tagOffset) : Tagger_Tag()
{
	if( file )
	{
		read(file, tagOffset);
	}
}



APE_Tag::~APE_Tag()
{
	SjHashIterator  iterator;
	APE_Item*       item;
	wxString        key;
	while( (item=(APE_Item*)m_itemListMap.Iterate(iterator, key)) )
	{
		delete item;
	}
}



SjByteVector APE_Tag::fileIdentifier()
{
	return SjByteVector::fromCString("APETAGEX");
}



wxString APE_Tag::lookupString(const wxString& key) const
{
	APE_Item* i = lookupItem(key);
	if(i)
	{
		return i->toString();
	}

	return wxT("");
}



long APE_Tag::year() const
{
	wxString s = lookupString(wxT("YEAR"));
	if( s.IsEmpty() ) return 0;

	long l;
	s.ToLong(&l);
	return l;
}



long APE_Tag::beatsPerMinute() const
{
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	wxString s = lookupString(wxT("BPM"));
	if( s.IsEmpty() ) return 0;

	long l;
	s.ToLong(&l);
	return l;
}



void APE_Tag::track(long& nr, long& count) const
{
	nr = 0; count = 0;
	wxString s = lookupString(wxT("TRACK"));
	if( s.IsEmpty() ) return;

	explodeNrAndCount(s, nr, count);
}



void APE_Tag::disk(long& nr, long& count) const
{
	// see http://age.hobba.nl/audio/mirroredpages/apekey.html
	nr = 0; count = 0;
	wxString s = lookupString(wxT("MEDIA"));
	if( s.IsEmpty() ) return;

	explodeNrAndCount(s, nr, count);
}



void APE_Tag::setTitle(const wxString &s)
{
	addValue(wxT("TITLE"), s, true);
}



void APE_Tag::setLeadArtist(const wxString &s)
{
	addValue(wxT("ARTIST"), s, true);
}



void APE_Tag::setOrgArtist(const wxString &s)
{
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	addValue(wxT("ORIGARTIST"), s, true);
}



void APE_Tag::setComposer(const wxString &s)
{
	addValue(wxT("COMPOSER"), s, true);
}



void APE_Tag::setAlbum(const wxString &s)
{
	addValue(wxT("ALBUM"), s, true);
}



void APE_Tag::setComment(const wxString &s)
{
	addValue(wxT("COMMENT"), s, true);
}



void APE_Tag::setGenre(const wxString &s)
{
	addValue(wxT("GENRE"), s, true);
}



void APE_Tag::setGroup(const wxString &s)
{
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	addValue(wxT("CONTENTGROUP"), s, true);
}



void APE_Tag::setYear(long i)
{
	if(i <= 0)
		removeItem(wxT("YEAR"));
	else
		addValue(wxT("YEAR"), wxString::Format(wxT("%i"), (int)i), true);
}



void APE_Tag::setBeatsPerMinute(long i)
{
	// I have not found anything standard-releated for this, so I use the same name as in ID3v2
	if(i <= 0)
		removeItem(wxT("BPM"));
	else
		addValue(wxT("BPM"), wxString::Format(wxT("%i"), (int)i), true);
}



void APE_Tag::setTrack(long nr, long count)
{
	if( nr <= 0 )
	{
		removeItem(wxT("TRACK"));
	}
	else
	{
		// "1/n" is okay, see http://age.hobba.nl/audio/mirroredpages/apekey.html
		addValue(wxT("TRACK"),  implodeNrAndCount(nr, count), true);
	}
}



void APE_Tag::setDisk(long nr, long count)
{
	if( nr <= 0 )
	{
		removeItem(wxT("MEDIA"));
	}
	else
	{
		// "1/n" is okay, see http://age.hobba.nl/audio/mirroredpages/apekey.html
		addValue(wxT("MEDIA"),  implodeNrAndCount(nr, count), true);
	}
}



void APE_Tag::removeItem(const wxString &key)
{
	APE_Item* i = (APE_Item*)m_itemListMap.Remove(key.Upper());
	if( i )
	{
		delete i;
	}
}



void APE_Tag::addValue(const wxString &key, const wxString &value, bool replace)
{
	if(replace)
	{
		removeItem(key);
	}

	if(!value.IsEmpty())
	{
		APE_Item* i = lookupItem(key.Upper());
		if( i )
		{
			i->toStringList().Add(value);
		}
		else
		{
			setItem(key, APE_Item(key, value));
		}
	}
}



void APE_Tag::setItem(const wxString& key, const APE_Item& item)
{
	APE_Item* oldItem = (APE_Item*)m_itemListMap.Insert(key.Upper(), new APE_Item(item));
	if( oldItem )
	{
		delete oldItem;
	}
}



void APE_Tag::read(Tagger_File* file, long tagOffset)
{
	if(file && file->IsValid())
	{
		file->Seek(tagOffset);
		m_footer.setData(file->ReadBlock(APE_FOOTER_SIZE));

		if( m_footer.tagSize() == 0 || m_footer.tagSize() > (SjUint)(file->Length()) )
		{
			return;
		}

		file->Seek(tagOffset + APE_FOOTER_SIZE - m_footer.tagSize());
		parse(file->ReadBlock(m_footer.tagSize() - APE_FOOTER_SIZE));
	}
}



SjByteVector APE_Tag::render() const
{
	SjByteVector data;
	SjUint itemCount = 0;

	{
		SjHashIterator iterator;
		APE_Item* item;
		wxString key;
		while( (item=(APE_Item*)m_itemListMap.Iterate(iterator, key)) )
		{
			data.append(item->render());
			itemCount++;
		}
	}

	m_footer.setItemCount(itemCount);
	m_footer.setTagSize(data.size()+APE_FOOTER_SIZE);
	m_footer.setHeaderPresent(true);

	return m_footer.renderHeader() + data + m_footer.renderFooter();
}



void APE_Tag::parse(const SjByteVector &data)
{
	SjUint pos = 0;

	// 11 bytes is the minimum size for an APE item
	for(SjUint i = 0; i < m_footer.itemCount() && pos <= data.size() - 11; i++)
	{
		APE_Item item;
		item.parse(data.mid(pos));

		setItem(item.key().Upper(), item);

		pos += item.size();
	}
}
