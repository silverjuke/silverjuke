/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    ringbuffer.cpp
 * Authors: Björn Petersen
 * Purpose: A simple ringbuffer
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/ringbuffer.h>


SjRingbuffer::SjRingbuffer()
{
	m_buffer				= NULL; //Free() checks this pointer
	m_totalBytes			= 0;
	m_waitingForResizeBytes = 0;

	Empty();
}


SjRingbuffer::~SjRingbuffer()
{
	Free();
}


bool SjRingbuffer::Alloc(long totalBytes)
{
	m_waitingForResizeBytes = 0;

    if( m_totalBytes == totalBytes )
    {
        Empty();
    }
    else
    {
        Free();

		if( totalBytes > 0 )
		{
			m_buffer = (unsigned char*)malloc(totalBytes);
			if( m_buffer == NULL )
			{
				return FALSE;
			}
		}

        m_totalBytes = totalBytes;
    }

    return TRUE;

}


bool SjRingbuffer::Resize(long totalBytes)
{
	m_waitingForResizeBytes = 0;

	if( m_buffer == NULL || totalBytes <= 0 )
	{
		// this function may not be used to allocate buffers, use Alloc() instead
		// or check if the buffer is allocted using IsAllocated()
		wxASSERT( m_buffer && m_totalBytes > 0 );
		return FALSE;
	}
	else if( m_totalBytes == totalBytes )
	{
		// same size - nothing to do
		return TRUE;
	}
	else if( m_totalBytes > totalBytes )
	{
		// shrink buffer
		if( m_validBytes <= totalBytes )
		{
			return ResizeDo(totalBytes);
		}
		else
		{
			m_waitingForResizeBytes = totalBytes;
			return TRUE;
		}
	}
	else
	{
		// enlarge buffer
		return ResizeDo(totalBytes);
	}
}


bool SjRingbuffer::ResizeDo(long totalBytes)
{
	wxASSERT( m_buffer && totalBytes > 0 );

	unsigned char* newBuffer = (unsigned char*)malloc(totalBytes);
	if( newBuffer == NULL )
	{
		return FALSE;
	}

	PeekFromBeg(newBuffer, 0, GetValidBytes());

	free(m_buffer);

	m_buffer				= newBuffer;
	m_totalBytes			= totalBytes;
	m_validPos				= 0;
	m_waitingForResizeBytes = 0;

	return TRUE;
}


void SjRingbuffer::Free()
{
    if( m_buffer )
    {
        free(m_buffer);
        m_buffer = NULL;
		m_totalBytes = 0;
		//m_totalMs = 0;
    }

	wxASSERT( m_buffer == NULL && m_totalBytes == 0 );
	Empty();
}


void SjRingbuffer::Empty()
{
    m_validPos							= 0;
    m_validBytes						= 0;
	ResizeCheck();
}


void SjRingbuffer::PushToEnd(const unsigned char* src, long bytesToCopy)
{
	// Function adds the given data to the end of the ringbuffer.
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().

	long i, destPos;

	// check param
    if( !m_buffer || !src || bytesToCopy <= 0 || m_validPos >= m_totalBytes ) {
		return;
    }

	// are there enough free bytes in the buffer?
	if( bytesToCopy > GetFreeBytes() )
	{
		bytesToCopy = GetFreeBytes();

		if( bytesToCopy <= 0 )
		{
			return; // buffer is full
		}
	}

	// add data to buffer
    destPos = (m_validPos + m_validBytes) % m_totalBytes;

    if( destPos + bytesToCopy > m_totalBytes )
    {
        i = m_totalBytes - destPos;

        memcpy(m_buffer + destPos,  src,       i);
        memcpy(m_buffer,            src + i,   bytesToCopy - i);
    }
    else
    {
        memcpy(m_buffer + destPos,  src,       bytesToCopy);
    }

	// correct ringbuffer pointers
    m_validBytes += bytesToCopy;
}


void SjRingbuffer::PeekFromBeg(unsigned char* dest, long offset, long bytesToCopy) const
{
	// Function gets a number of valid bytes from the ringbuffer.
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().

	long bytesAtEnd, validPos;

	if( !dest || !m_buffer || offset < 0 || bytesToCopy <= 0 ) {
		return;
	}

	// check if there are enough bytes to copy
	if( offset >= m_validBytes )
	{
		memset(dest, 0, bytesToCopy);
		return;
	}

	if( bytesToCopy > (m_validBytes-offset) )
	{
		if( dest )
		{
			memset(dest + (m_validBytes-offset), 0, bytesToCopy-(m_validBytes-offset));
		}

		bytesToCopy = (m_validBytes-offset);

		if( bytesToCopy <= 0 )
		{
			return;
		}
	}

    // copy data
	validPos = (m_validPos + offset) % m_totalBytes;

    if( validPos + bytesToCopy > m_totalBytes )
    {
        bytesAtEnd = m_totalBytes - validPos;

		if( dest )
		{
	        memcpy(dest,				m_buffer + validPos, bytesAtEnd);
		    memcpy(dest + bytesAtEnd,	m_buffer,			 bytesToCopy - bytesAtEnd);
		}
    }
    else
    {
		if( dest )
		{
			memcpy(dest,     m_buffer + validPos, bytesToCopy);
		}
    }
}


void SjRingbuffer::RemoveFromBeg(long bytesToRemove)
{
	// Remove some bytes from the beginning of the buffer.
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().
	//
	// It is okay to remove 0 bytes in which case the function does nothing.

	if( bytesToRemove > m_validBytes ) {
		bytesToRemove = m_validBytes;
	}

    // correct buffer
    m_validPos = (m_validPos + bytesToRemove) % m_totalBytes;
    m_validBytes -= bytesToRemove;

	// are the valid bytes small enough to perform a possibly waiting buffer shrinking?
	ResizeCheck();
}



