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



#ifndef __SJ_BYTEFILE_H__
#define __SJ_BYTEFILE_H__


#include "tg_bytevector.h"


enum SjByteFileSeek
{
    SJ_SEEK_BEG,    // Seek from the beginning of the file.
    SJ_SEEK_CUR,    // Seek from the current position in the file.
    SJ_SEEK_END     // Seek from the end of the file.
};



class SjByteFile
{
public:
	// constructor / destructor
	SjByteFile          (const wxFSFile* fsFile, wxInputStream* inputStream);
	virtual         ~SjByteFile         ();

	// IsValid() Returns true if the file is open and readable and/or valid information were found
	bool            IsValid             () const;


	// Reads a block of the given length at the current pointer.
	SjByteVector    ReadBlock           (unsigned long length);

	// Attempts to write the block at the current pointer.  If the
	// file is currently only opened read only, this attempts to reopen the file in read/write mode.
	void            WriteBlock          (const SjByteVector &data);

	// Find() returns the offset in the file where "pattern" occurs at or -1 if it can not be found.
	// If "before" is set, the search will only continue until the pattern "before" is found.
	// Searching starts at "fromOffset", which defaults to the beginning of the file.
	// Note: This has the practial limitation that "pattern" cannot be longer than the buffer size used by ReadBlock().  Currently this is 1024 bytes.
	long            Find                (const SjByteVector &pattern, long fromOffset = 0,
	                                     const SjByteVector &before = SjByteVector::null);

	// RFind() returns the offset in the file where "pattern" occurs at or -1 if it can not be found.
	// If "before" is set, the search will only continue until the pattern "before" is found.
	// Searching starts at "fromOffset" to the beginning of the file; the default is the end of the file.
	// Note: This has the practial limitation that "pattern" cannot be longer than the buffer size used by ReadBlock().  Currently this is 1024 bytes.
	long            RFind               (const SjByteVector &pattern, long fromOffset = 0,
	                                     const SjByteVector &before = SjByteVector::null);

	// Insert "data" at position "start" in the file overwriting "replace" bytes of the original content.
	// Note: This method is slow since it requires rewriting all of the file after the insertion point.
	void            Insert              (const SjByteVector &data, unsigned long start = 0, unsigned long replace = 0);

	// Removes a block of the file starting at "start" and continuing for "length" bytes.
	// Note: This method is slow since it involves rewriting all of the file after the removed portion.
	void            RemoveBlock         (unsigned long start = 0, unsigned long length = 0);

	// Returns true if the file is read only (or if the file can not be opened).
	bool            ReadOnly            ();

	// Move the I/O pointer to "offset" in the file from position "p".
	void            Seek                (long offset, SjByteFileSeek p = SJ_SEEK_BEG);

	// Reset the end-of-file and error flags on the file.
	void            Clear               ();

	// Returns the current offset withing the file.
	long            Tell                () const;

	// Returns the length of the file.
	long            Length              ();

	// Truncates the file.
	bool            Truncate            (long length);

protected:
	// SetValid() marks the file as valid or invalid.
	void            SetValid            (bool valid) {m_valid = valid;}

	// Returns the buffer size that is used for internal buffering.
	static SjUint   BufferSize          ();

private:
#ifdef __WXDEBUG__
	SjByteFile &operator=(const SjByteFile &) { wxLogWarning(wxT("do not copy SjByteFile objects this way!")); return *this; }
#endif

	wxString            getLocalFileName() const;
	bool                openForWriting();

	const wxFSFile*     m_fsFile__;

	FILE*               m_file__;
	bool                m_file__triedOpening;

	wxInputStream*      m_inputStream__;

	bool                m_valid;
	unsigned long       m_size;
};



#endif // __SJ_BYTEFILE_H__

