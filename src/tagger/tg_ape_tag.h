/***************************************************************************
    copyright            : (C) 2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.org
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



#ifndef __SJ_APE_TAG_H__
#define __SJ_APE_TAG_H__



#include "tg_tagger_base.h"



enum APE_ItemType
{
    APE_ItemText = 0,           // Item contains text information coded in UTF-8
    APE_ItemBinary = 1,         // Item contains binary information
    APE_ItemLocator = 2         // Item is a locator of external stored information
                      // The value of 3 is reserved.
};




class APE_Item
{
public:
	APE_Item            ();
	APE_Item            (const wxString &key, const wxString &value);
	APE_Item            (const wxString &key, const wxArrayString &values);
	APE_Item            (const APE_Item &item);
	APE_Item&           operator=           (const APE_Item& o) { copyFrom(o); return *this; }
	wxString            key                 () const {return m_key;}
	SjByteVector        getBinary           () const {return m_binary;}
	int                 size                () const; // Returns the size of the full item.
	wxString            toString            () const; // In case of multiple strings, the first is returned.
	wxArrayString       toStringList        () const {return m_stringList;}
	SjByteVector        render              () const;
	void                parse               (const SjByteVector& data);
	void                setReadOnly         (bool readOnly) {m_readOnly = readOnly;}
	bool                isReadOnly          () const {return m_readOnly;}
	void                setType             (APE_ItemType type) {m_type=type;}
	APE_ItemType        type                () const {return m_type;}
	bool                isEmpty             () const;

private:
	APE_ItemType        m_type;
	wxString            m_key;
	SjByteVector        m_binary;
	wxArrayString       m_stringList;
	bool                m_readOnly;
	void                init                (); // this function should be called from within the constructor, it does not clear all fields!
	void                copyFrom            (const APE_Item& o);
};



class APE_Footer
{
public:
	// This class implements APE footers (and headers). It attempts to follow, both
	// semantically and programatically, the structure specified in
	// the APE v2.0 standard.  The API is based on the properties of APE footer and
	// headers specified there.
#define             APE_FOOTER_SIZE     32
	APE_Footer          (const SjByteVector* data = NULL); // parse() is called immediately.
	virtual             ~APE_Footer         () {}
	SjUint              version             () const { return m_version; } // Returns the version number, This is the 1000 or 2000
	bool                headerPresent       () const { return m_headerPresent; }
	bool                footerPresent       () const { return m_footerPresent; }
	bool                isHeader            () const { return m_isHeader; }
	void                setHeaderPresent    (bool b) { m_headerPresent = b; }
	SjUint              itemCount           () const { return m_itemCount; }
	void                setItemCount        (SjUint s) { m_itemCount = s; }
	SjUint              tagSize             () const { return m_tagSize; } // This is the size of the frame content and footer. The size of the entire tag will be this plus the header size, if present.
	SjUint              completeTagSize     () const; // Returns the tag size, including if present, the header size.
	void                setTagSize          (SjUint s) { m_tagSize = s; }
	static SjByteVector fileIdentifier      (); // Returns the string used to identify an APE tag inside of a file, this is always "APETAGEX".
	void                setData             (const SjByteVector &data) { parse(data); } // Sets the data that will be used as the footer.  32 bytes, starting from data will be used.
	SjByteVector        renderFooter        () const; // Renders the footer back to binary format.
	SjByteVector        renderHeader        () const; // Renders the header corresponding to the footer. If headerPresent is set to false, it returns an empty ByteVector.

protected:
	void                parse               (const SjByteVector &data); // Called by setData() to parse the footer data.  It makes this information available through the public API.
	SjByteVector        render              (bool isHeader) const; // Called by renderFooter and renderHeader

private:
#ifdef __WXDEBUG__
	APE_Footer&         operator=           (const APE_Footer &) { wxLogWarning(wxT("do not copy APE footer objects this way!")); return *this; }
#endif
	SjUint              m_version;
	bool                m_footerPresent;
	bool                m_headerPresent;
	bool                m_isHeader;
	SjUint              m_itemCount;
	SjUint              m_tagSize;
};



class APE_Tag : public Tagger_Tag
{
public:
	APE_Tag             (Tagger_File *file = NULL, long tagOffset = 0); // Create an APE tag and parse the data in file with APE footer at tagOffset.
	virtual             ~APE_Tag            ();
	SjByteVector        render              () const;

	static SjByteVector fileIdentifier      (); // Returns the string "APETAGEX" suitable for usage in locating the tag in a file.
	APE_Footer*         footer              () const { return &m_footer; } // Returns a pointer to the tag's footer.
	void                removeItem          (const wxString &key); // Removes the \a key item from the tag
	void                addValue            (const wxString &key, const wxString &value, bool replace = true);
	void                setItem             (const wxString &key, const APE_Item &item);
	APE_Item*           lookupItem          (const wxString& key) const { return (APE_Item*)m_itemListMap.Lookup(key); }
	wxString            lookupString        (const wxString& key) const;

	// Returns a reference to the item list map.
	// You should not modify this data structure directly, instead
	// use setItem() and removeItem().
	const SjSPHash*     itemListMap         () const { return &m_itemListMap; }

	// Reimplementations
	virtual wxString    title               () const { return lookupString(wxT("TITLE")); }
	virtual wxString    leadArtist          () const { return lookupString(wxT("ARTIST")); }
	virtual wxString    orgArtist           () const { return lookupString(wxT("ORIGARTIST")); }
	virtual wxString    composer            () const { return lookupString(wxT("COMPOSER")); }
	virtual wxString    album               () const { return lookupString(wxT("ALBUM")); }
	virtual wxString    comment             () const { return lookupString(wxT("COMMENT")); }
	virtual wxString    genre               () const { return lookupString(wxT("GENRE")); }
	virtual wxString    group               () const { return lookupString(wxT("CONTENTGROUP")); }
	virtual long        year                () const;
	virtual long        beatsPerMinute      () const;
	virtual void        track               (long& nr, long& count) const;
	virtual void        disk                (long& nr, long& count) const;
	virtual void        setTitle            (const wxString &s);
	virtual void        setLeadArtist       (const wxString &s);
	virtual void        setOrgArtist        (const wxString &s);
	virtual void        setComposer         (const wxString &s);
	virtual void        setAlbum            (const wxString &s);
	virtual void        setComment          (const wxString &s);
	virtual void        setGenre            (const wxString &s);
	virtual void        setGroup            (const wxString &s);
	virtual void        setYear             (long i);
	virtual void        setBeatsPerMinute   (long i);
	virtual void        setTrack            (long nr, long count);
	virtual void        setDisk             (long nr, long count);

protected:

	void                read                (Tagger_File*, long tagOffset); // Reads from the file specified in the constructor.
	void                parse               (const SjByteVector &data); // Parses the body of the tag in data.

private:
#ifdef __WXDEBUG__
	APE_Tag&            operator=           (const APE_Tag &) { wxLogWarning(wxT("do not copy APE_Tag objects this way!")); return *this; }
#endif
	mutable APE_Footer  m_footer;
	SjSPHash            m_itemListMap;
};



#endif // __SJ_APE_TAG_H__
