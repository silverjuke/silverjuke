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


#define BUFFER_SIZE 1024


SjByteFile::SjByteFile(const wxString& url, wxInputStream* inputStream)
{
	if( inputStream )
	{
		m_inputStream__     = inputStream;
		m_file__ = NULL;

		// seek to position 0 (this may be needed if the stream was used before;
		// I'm not sure, if Tagger does a seekback in all cases, so we do it here)

		wxASSERT( m_inputStream__->IsSeekable() );

		off_t newpos = m_inputStream__->SeekI(0, wxFromStart);

		wxASSERT( newpos != wxInvalidOffset ); // this may happen if wxFS_SEEKABLE was not given to wxFileSystem::OpenFile()
	}
	else
	{
		m_inputStream__     = NULL;
		m_file__ = NULL;

		wxString localFileName(url);
		if( localFileName.StartsWith("file:") )
		{
			localFileName = wxFileSystem::URLToFileName(localFileName).GetFullPath();
		}

		if( !localFileName.IsEmpty() )
		{
			#if defined(__WXMSW__) && wxUSE_UNICODE
				m_file__ = _wfopen(static_cast<const wxChar*>(localFileName.c_str()), wxT("rb+")); // the "b" for BINARY is really important!!! at least on MSW
			#else
				m_file__ = fopen(localFileName.fn_str(), "rb+"); // the "b" for BINARY is really important!!! at least on MSW - is fn_str() wxConvLibc?
			#endif
			if( m_file__ )
			{
				if( fseek(m_file__, (long)0, SEEK_SET) == -1 )
				{
					fclose(m_file__);
					m_file__ = NULL;
				}
			}
		}
	}

	m_valid = true;
	m_size = 0;
}


SjByteFile::~SjByteFile()
{
	if( m_file__ )
	{
		fclose(m_file__);
	}

	// m_inputStream__ is owned by the caller and must not be freed here!
}


SjByteVector SjByteFile::ReadBlock(unsigned long length)
{
	if(length > BUFFER_SIZE &&
	        length > (unsigned long)SjByteFile::Length())
	{
		length = SjByteFile::Length();
	}

	SjByteVector v((SjUint)length);
	int count = 0;

	if( m_file__ )
	{
		count = fread(v.getWriteableData(), sizeof(char), length, m_file__);
	}
	else if( m_inputStream__ )
	{
		count = m_inputStream__->Read(v.getWriteableData(), length).LastRead();
	}

	v.resize(count);
	return v;
}


bool SjByteFile::WriteBlock(const SjByteVector &data)
{
	if( m_file__ == NULL )
	{
		wxLogError("SjByteFile::WriteBlock(): File not ready.");
		return false;
	}

	size_t towrite = data.size();
	size_t written = fwrite(data.getReadableData(), sizeof(char), towrite, m_file__);
	if( written != towrite )
	{
		wxLogError("SjByteFile::WriteBlock(): fwrite() wrote %i instead of %i bytes, ferror() returns %i", written, towrite, ferror(m_file__));
		return false;
	}
	return true;
}


long SjByteFile::Find(const SjByteVector &pattern, long fromOffset, const SjByteVector &before)
{
	if(!m_inputStream__ || pattern.size() > BUFFER_SIZE)
	{
		return -1;
	}

	// The position in the file that the current buffer starts at.
	long bufferOffset = fromOffset;
	SjByteVector buffer;

	// These variables are used to keep track of a partial match that happens at
	// the end of a buffer.
	int previousPartialMatch = -1;
	int beforePreviousPartialMatch = -1;

	// Save the location of the current read pointer.  We will restore the
	// position using seek() before all returns.
	long originalPosition = Tell();

	// Start the search at the offset.
	Seek(fromOffset);

	// The search loop
	for(buffer = ReadBlock(BUFFER_SIZE); buffer.size() > 0; buffer = ReadBlock(BUFFER_SIZE))
	{
		// (1) previous partial match
		if(previousPartialMatch >= 0 && int(BUFFER_SIZE) > previousPartialMatch)
		{
			const int patternOffset = (BUFFER_SIZE - previousPartialMatch);
			if(buffer.containsAt(pattern, 0, patternOffset))
			{
				Seek(originalPosition);
				return bufferOffset - BUFFER_SIZE + previousPartialMatch;
			}
		}

		if(!before.isNull() && beforePreviousPartialMatch >= 0 && int(BUFFER_SIZE) > beforePreviousPartialMatch)
		{
			const int beforeOffset = (BUFFER_SIZE - beforePreviousPartialMatch);
			if(buffer.containsAt(before, 0, beforeOffset))
			{
				Seek(originalPosition);
				return -1;
			}
		}

		// (2) pattern contained in current buffer
		long location = buffer.find(pattern);
		if(location >= 0)
		{
			Seek(originalPosition);
			return bufferOffset + location;
		}

		if(!before.isNull() && buffer.find(before) >= 0)
		{
			Seek(originalPosition);
			return -1;
		}

		// (3) partial match
		previousPartialMatch = buffer.endsWithPartialMatch(pattern);

		if(!before.isNull())
		{
			beforePreviousPartialMatch = buffer.endsWithPartialMatch(before);
		}

		bufferOffset += BUFFER_SIZE;
	}

	// Since we hit the end of the file, reset the status before continuing.
	Clear();
	Seek(originalPosition);
	return -1;
}


long SjByteFile::RFind(const SjByteVector &pattern, long fromOffset, const SjByteVector &before)
{
	if(!m_inputStream__ || pattern.size() > BUFFER_SIZE)
	{
		return -1;
	}

	// The position in the file that the current buffer starts at.
	SjByteVector buffer;

	// Save the location of the current read pointer.  We will restore the
	// position using seek() before return.
	long originalPosition = Tell();

	// Start the search at the offset.
	long bufferOffset;
	if(fromOffset == 0)
	{
		Seek(-1 * int(BUFFER_SIZE), SJ_SEEK_END);
		bufferOffset = Tell();
	}
	else
	{
		Seek(fromOffset + -1 * int(BUFFER_SIZE), SJ_SEEK_BEG);
		bufferOffset = Tell();
	}

	// the loop
	for(buffer = ReadBlock(BUFFER_SIZE); buffer.size() > 0; buffer = ReadBlock(BUFFER_SIZE))
	{
		long location = buffer.rfind(pattern);
		if(location >= 0) {
			Seek(originalPosition);
			return bufferOffset + location;
		}

		if(!before.isNull() && buffer.find(before) >= 0) {
			Seek(originalPosition);
			return -1;
		}

		bufferOffset -= BUFFER_SIZE;
		Seek(bufferOffset);
	}

	// Since we hit the end of the file, reset the status before continuing.
	Clear();
	Seek(originalPosition);
	return -1;
}


bool SjByteFile::Insert(const SjByteVector &data, unsigned long start, unsigned long replace)
{
	if( m_file__==NULL )
	{
		wxLogError("SjByteFile::Insert(): File not ready.");
		return false;
	}

	if(data.size() == replace) {
		Seek(start);
		return WriteBlock(data);
	}
	else if(data.size() < replace) {
		Seek(start);
		if( !WriteBlock(data) ) {
			return false;
		}
		return RemoveBlock(start + data.size(), replace - data.size());
	}

	// Woohoo!  Faster (about 20%) than id3lib at last.  I had to get hardcore
	// and avoid Tagger's high level API for rendering just copying parts of
	// the file that don't contain tag data.
	//
	// Now I'll explain the steps in this ugliness:

	// First, make sure that we're working with a buffer that is longer than
	// the *differnce* in the tag sizes.  We want to avoid overwriting parts
	// that aren't yet in memory, so this is necessary.

	unsigned long bufferLength = BufferSize();
	while(data.size() - replace > bufferLength)
		bufferLength += BufferSize();

	// Set where to start the reading and writing.

	long readPosition = start + replace;
	long writePosition = start;

	SjByteVector buffer;
	SjByteVector aboutToOverwrite((SjUint)bufferLength);

	// This is basically a special case of the loop below.  Here we're just
	// doing the same steps as below, but since we aren't using the same buffer
	// size -- instead we're using the tag size -- this has to be handled as a
	// special case.  We're also using File::writeBlock() just for the tag.
	// That's a bit slower than using char *'s so, we're only doing it here.

	Seek(readPosition);
	int bytesRead = fread(aboutToOverwrite.getWriteableData(), sizeof(char), bufferLength, m_file__);
	readPosition += bufferLength;

	Seek(writePosition);
	if( !WriteBlock(data) ) {
		return false;
	}
	writePosition += data.size();

	buffer = aboutToOverwrite;

	// Ok, here's the main loop.  We want to loop until the read fails, which
	// means that we hit the end of the file.

	while(bytesRead != 0) {

		// Seek to the current read position and read the data that we're about
		// to overwrite.  Appropriately increment the readPosition.

		Seek(readPosition);
		bytesRead = fread(aboutToOverwrite.getWriteableData(), sizeof(char), bufferLength, m_file__);
		aboutToOverwrite.resize(bytesRead);
		readPosition += bufferLength;

		// Check to see if we just read the last block.  We need to call clear()
		// if we did so that the last write succeeds.

		if((unsigned long)bytesRead < bufferLength)
			Clear();

		// Seek to the write position and write our buffer.  Increment the
		// writePosition.

		Seek(writePosition);
		size_t written = fwrite(buffer.getReadableData(), sizeof(char), bufferLength, m_file__);
		if( written != bufferLength ) {
			wxLogError("SjByteFile::Insert(): fwrite() wrote %i instead of %i bytes, ferror() returns %i", written, bufferLength, ferror(m_file__));
			return false;
		}
		writePosition += bufferLength;

		// Make the current buffer the data that we read in the beginning.

		buffer = aboutToOverwrite;

		// Again, we need this for the last write.  We don't want to write garbage
		// at the end of our file, so we need to set the buffer size to the amount
		// that we actually read.

		bufferLength = bytesRead;
	}

	return true;
}


bool SjByteFile::RemoveBlock(unsigned long start, unsigned long length)
{
	if( m_file__==NULL )
	{
		wxLogError("SjByteFile::RemoveBlock(): File not ready.");
		return false;
	}

	unsigned long bufferLength = BufferSize();

	long readPosition = start + length;
	long writePosition = start;

	SjByteVector buffer((SjUint)bufferLength);

	unsigned long bytesRead = true;

	while(bytesRead != 0) {
		Seek(readPosition);
		bytesRead = fread(buffer.getWriteableData(), sizeof(char), bufferLength, m_file__);
		buffer.resize(bytesRead);
		readPosition += bytesRead;

		// Check to see if we just read the last block.  We need to call clear()
		// if we did so that the last write succeeds.

		if(bytesRead < bufferLength)
			Clear();

		Seek(writePosition);
		size_t written = fwrite(buffer.getReadableData(), sizeof(char), bytesRead, m_file__);
		if( written != bytesRead ) {
			wxLogError("SjByteFile::RemoveBlock(): fwrite() wrote %i instead of %i bytes, ferror() returns %i", written, bytesRead, ferror(m_file__));
			return false;
		}
		writePosition += bytesRead;
	}
	Truncate(writePosition);
	return true;
}


bool SjByteFile::ReadOnly()
{
	return m_file__? false : true;
}


bool SjByteFile::IsValid() const
{
	return (m_inputStream__||m_file__) && m_valid;
}

void SjByteFile::Seek(long offset, SjByteFileSeek p)
{
	if( m_file__ )
	{
		switch( p )
		{
			case SJ_SEEK_BEG:
				fseek(m_file__, offset, SEEK_SET);
				break;

			case SJ_SEEK_CUR:
				fseek(m_file__, offset, SEEK_CUR);
				break;

			case SJ_SEEK_END:
				fseek(m_file__, offset, SEEK_END);
				break;
		}
	}
	else if( m_inputStream__ )
	{
		switch( p )
		{
			case SJ_SEEK_BEG:
				m_inputStream__->SeekI(offset, wxFromStart);
				break;

			case SJ_SEEK_CUR:
				m_inputStream__->SeekI(offset, wxFromCurrent);
				break;

			case SJ_SEEK_END:
				m_inputStream__->SeekI(offset, wxFromEnd);
				break;
		}
	}
}


void SjByteFile::Clear()
{
	if( m_file__ )
	{
		clearerr(m_file__);
	}
}


long SjByteFile::Tell() const
{
	if( m_file__ )
	{
		return ftell(m_file__);
	}
	else if( m_inputStream__ )
	{
		return m_inputStream__->TellI();
	}
	else
	{
		return 0;
	}
}


long SjByteFile::Length()
{
	// Do some caching in case we do multiple calls.
	if(m_size > 0) {
		return m_size;
	}

	long curpos = Tell();

	Seek(0, SJ_SEEK_END);
	long endpos = Tell();

	Seek(curpos, SJ_SEEK_BEG);

	m_size = endpos;
	return endpos;
}


#ifdef __WXMSW__
#include <io.h>
#endif
bool SjByteFile::Truncate(long length)
{
	if( m_file__ )
	{
		#ifdef __WXMSW__
		if( _chsize(_fileno(m_file__), length) == 0 )
		{
			return true;
		}
		#else
		if( ftruncate(fileno(m_file__), length) == 0 )
		{
			return true;
		}
		#endif
	}

	return false;
}


SjUint SjByteFile::BufferSize()
{
	return BUFFER_SIZE;
}
