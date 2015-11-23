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



#ifndef TAGGER_MPCFILE_H
#define TAGGER_MPCFILE_H



#include "tg_tagger_base.h"



class Tagger_Tag;
class ID3v1_Tag;
class ID3v2_Header;
class APE_Tag;




class MPC_Properties : public Tagger_AudioProperties
{
public:
	MPC_Properties      (const SjByteVector& data, long streamLength);

	// Returns the version of the bitstream (SV4-SV7)
#define         MPC_HeaderSize      (8*7)
	int             mpcVersion          () const {return m_version;}

	// Reimplementations
	virtual int     length              () const {return m_length;}
	virtual int     bitrate             () const {return m_bitrate;}
	virtual int     sampleRate          () const {return m_sampleRate;}
	virtual int     channels            () const {return m_channels;}

private:
	int             m_version;
	int             m_length;
	int             m_bitrate;
	int             m_sampleRate;
	int             m_channels;
};



enum MPC_TagType
{
    MPC_NoTags  = 0x0000,   // Empty set.  Matches no tag types.
    MPC_ID3v1   = 0x0001,   // Matches ID3v1 tags.
    MPC_ID3v2   = 0x0002,   // Matches ID3v2 tags.
    MPC_APE     = 0x0004,   // Matches APE tags.
    MPC_AllTags = 0xffff    // Matches all tag types.
};



class MPC_FilePrivate;
class MPC_File : public Tagger_File
{
public:

	                    MPC_File            (const wxString& url, wxInputStream*, bool readProperties = true);
	virtual             ~MPC_File           ();


	// reimplementations
	virtual Tagger_Tag* tag             () const {return m_tag;} // This will be an APE tag, an ID3v1 tag or a combination of the two.
	virtual Tagger_AudioProperties*
	audioProperties () const {return m_properties;} // If no audio properties were read then this will return a null pointer.
	virtual bool        save            ();

	// ID3v1Tag() and APETag() returns a pointer to the different tag of the file.
	// If create is false this will return a null pointer there is no valid tag of the requested type.
	// The Tag is always owned by the FLAC_File and should not be
	ID3v1_Tag*          ID3v1Tag        (bool create = false);
	APE_Tag*            APETag          (bool create = false);

	// Remove the tags that match the OR-ed together type from the file.
	// This will also invalidate pointers to the tags.
	// In order to make the removal permanent save() still needs to be called.
	void                remove          (int tags = MPC_AllTags);

private:
	void                read            (bool readProperties);
	void                scan            ();
	long                findAPE         ();
	long                findID3v1       ();
	long                findID3v2       ();

	APE_Tag*            m_APETag;
	long                m_APELocation;
	SjUint              m_APESize;

	ID3v1_Tag*          m_ID3v1Tag;
	long                m_ID3v1Location;
	ID3v2_Header*       m_ID3v2Header;
	long                m_ID3v2Location;
	SjUint              m_ID3v2Size;

	Tagger_Tag*         m_tag;
	MPC_Properties*     m_properties;
	bool                m_scanned;

	// These indicate whether the file *on disk* has these tags, not if
	// this data structure does.  This is used in computing offsets.
	bool                m_hasAPE;
	bool                m_hasID3v1;
	bool                m_hasID3v2;
};



#endif
