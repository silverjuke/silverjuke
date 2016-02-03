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
	//m_totalMs				= 0;
	m_waitingForResizeBytes = 0;

	Empty();
}


SjRingbuffer::~SjRingbuffer()
{
	Free();
}


bool SjRingbuffer::Alloc(long totalBytes)
{
    wxASSERT( totalBytes >= 0 );
    wxASSERT( totalBytes % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );

	m_waitingForResizeBytes = 0;

    if( m_totalBytes == totalBytes )
    {
        Empty();
    }
    else
    {
        Free();

        m_buffer = (unsigned char*)malloc(totalBytes);
        if( m_buffer == NULL )
        {
            wxLogError("SjRingbuffer::Alloc: Out of memory."/*n/t*/);
            return FALSE;
        }

        m_totalBytes = totalBytes;
		//m_totalMs = ::SjBytes2MsExact(totalBytes);
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

	if( m_markerId )
	{
		m_markerPos -= m_validPos;
	}

	m_buffer				= newBuffer;
	m_totalBytes			= totalBytes;
	//m_totalMs				= SjBytes2MsExact(totalBytes);
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
    m_markerId							= 0;
	//m_removeSilenceOnPushBytesLeft	= 0;
	//m_crossfade.Init(NULL, 0);
	ResizeCheck();
}


void SjRingbuffer::PushToEnd(const unsigned char* src, long bytesToCopy)
{
	// Function adds the given data to the end of the ringbuffer.
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().

	long i, destPos;
	//long bytesToCrossfade;

	// check param
    wxASSERT( m_buffer );
    wxASSERT( src );
    wxASSERT( bytesToCopy >= 0 );
    wxASSERT( bytesToCopy % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );
    wxASSERT( m_validPos < m_totalBytes );

	// remove silence?
	/*
	if( m_removeSilenceOnPushBytesLeft > 0 )
	{
		// get the max. number of bytes to remove to i
		i = bytesToCopy;
		if( i > m_removeSilenceOnPushBytesLeft )
		{
			i = m_removeSilenceOnPushBytesLeft;
		}

		// get the number of silent bytes to i
		i = ::SjGetSilentBytes(src, i, FALSE, SILENCE_THRESHOLD_ON_BEG);
		wxASSERT( i >= 0 && i <= bytesToCopy );
		wxASSERT( i % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );

		if( i )
		{
			// buffer contains silent bytes on begin, remove them
			src += i;
			bytesToCopy -= i;
			m_removeSilenceOnPushBytesLeft -= i;
		}

		// buffer complety silent? wait for next buffer
		if( bytesToCopy <= 0 )
		{
			return;
		}

		// buffer is not completly silent, skip subsequent silence search
		m_removeSilenceOnPushBytesLeft = 0;
	}
	*/

	// are there enough free bytes in the buffer?
	if( bytesToCopy > GetFreeBytes() )
	{
		bytesToCopy = GetFreeBytes();
		wxASSERT( bytesToCopy >= 0 );

		if( bytesToCopy <= 0 )
		{
			return; // buffer is full
		}
	}

	// crossfading?
	/*
	bytesToCrossfade = m_crossfade.GetRemainingBytes();
	wxASSERT( bytesToCrossfade >= 0 );
	if( bytesToCrossfade > 0 )
	{
		if( bytesToCrossfade > bytesToCopy )
		{
			bytesToCrossfade = bytesToCopy;
		}

		// add crossfaded data to buffer
		destPos = (m_validPos + m_validBytes) % m_totalBytes;

		if( destPos + bytesToCrossfade > m_totalBytes )
		{
			i = m_totalBytes - destPos;

			m_crossfade.Step(m_buffer + destPos,  src,       i);
			m_crossfade.Step(m_buffer,            src + i,   bytesToCrossfade - i);
		}
		else
		{
			m_crossfade.Step(m_buffer + destPos,  src,       bytesToCrossfade);
		}

		// correct ringbuffer pointers
		m_validBytes += bytesToCrossfade;
		wxASSERT( m_crossfade.GetRemainingBytes() >= 0 );

		bytesToCopy -= bytesToCrossfade;
		wxASSERT( bytesToCopy >= 0 );
		if( bytesToCopy <= 0 )
		{
			return; // done, all crossfaded
		}
	}
	*/

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


long SjRingbuffer::PeekFromBeg(unsigned char* dest, long offset, long bytesToCopy) const
{
	// Function gets a number of valid bytes from the ringbuffer.
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().
	//
	// if "dest" is NULL, only the marker is returned.

	long bytesAtEnd, validPos, retMarkerId = 0;

    wxASSERT( m_buffer );
	wxASSERT( offset >= 0 );

	if( bytesToCopy <= 0 ) {
		return 0;
	}

	if( bytesToCopy % (SJ_WW_BYTERES*SJ_WW_CH) != 0 ) {
        bytesToCopy = (bytesToCopy/(SJ_WW_BYTERES*SJ_WW_CH))*(SJ_WW_BYTERES*SJ_WW_CH);
	}


	// check if there are enough bytes to copy
	if( offset >= m_validBytes )
	{
		if( dest )
		{
			memset(dest, 0, bytesToCopy);
		}

		wxLogDebug(" ++++ SjRingbuffer::PeekFromBeg(): not enough bytes to copy (1)");
		return retMarkerId;
	}

	wxASSERT(m_validBytes > offset);
	if( bytesToCopy > (m_validBytes-offset) )
	{
		if( dest )
		{
			memset(dest + (m_validBytes-offset), 0, bytesToCopy-(m_validBytes-offset));
		}

		bytesToCopy = (m_validBytes-offset);

		if( bytesToCopy <= 0 )
		{
			wxLogDebug(" ++++ SjRingbuffer::PeekFromBeg(): not enough bytes to copy (2)");
			return retMarkerId;
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

    // check for markers
    if( IsMarkerInRange(validPos, bytesToCopy) )
    {
		retMarkerId = m_markerId;

		#ifdef DEBUG_MARKERS
			wxLogDebug(" . . . . . . . (%s)  marker %i peeked at position %i",
				wxDateTime::Now().FormatISOTime().c_str(),
				m_markerId, m_markerPos);
		#endif
    }

	return retMarkerId;
}


void SjRingbuffer::RemoveFromBeg(long bytesToRemove, long offsetForMarkerRemovment)
{
	// Remove some bytes from the beginning of the buffer.
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().
	//
	// It is okay to remove 0 bytes in which case the function does nothing.

	if( bytesToRemove > m_validBytes ) {
		bytesToRemove = m_validBytes;
	}

	if( bytesToRemove % (SJ_WW_BYTERES*SJ_WW_CH) != 0 ) {
        bytesToRemove = (bytesToRemove/(SJ_WW_BYTERES*SJ_WW_CH))*(SJ_WW_BYTERES*SJ_WW_CH);
	}

	// remove markers
	if( IsMarkerInRange((m_validPos + offsetForMarkerRemovment) % m_totalBytes, bytesToRemove) )
    {
		#ifdef DEBUG_MARKERS
			wxLogDebug(" . . . . . . . (%s)  marker %i removed automatically from position %i",
				wxDateTime::Now().FormatISOTime().c_str(),
				m_markerId, m_markerPos);
		#endif

		m_markerId = 0;
    }

    // correct buffer
    m_validPos = (m_validPos + bytesToRemove) % m_totalBytes;
    m_validBytes -= bytesToRemove;

	// are the valid bytes small enough to perform a possibly waiting buffer shrinking?
	ResizeCheck();
}


void SjRingbuffer::SetMarker(long markerId, long offset)
{
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().

	// debugging stuff
	#if defined(__WXDEBUG__) && defined(DEBUG_MARKERS)
		long debug_oldMarkerId  = m_markerId,
			 debug_oldMarkerPos = m_markerPos;
	#endif

	// save marker id
	m_markerId = markerId;

	// save position for the marker
	if( m_totalBytes )
	{
		m_markerPos = (m_validPos + m_validBytes + offset) % m_totalBytes;
	}
	else
	{
		m_markerPos = 0;
	}

	// debugging stuff
	#if defined(__WXDEBUG__) && defined(DEBUG_MARKERS)
		if( m_markerId )
		{
			wxString debug_oldMarkerInfo;
			if( debug_oldMarkerId )
			{
				debug_oldMarkerInfo.Printf("(old marker %i at position %i overwritten)",
					debug_oldMarkerId, debug_oldMarkerPos);
			}

			wxLogDebug(" . . . . . . . (%s)  marker %i set to position %i (%i+%i%%%i) %s",
				wxDateTime::Now().FormatISOTime().c_str(),
				m_markerId, m_markerPos,
				m_validPos+m_validBytes, offset, m_totalBytes,
				debug_oldMarkerInfo.c_str());
		}
		else if( debug_oldMarkerId )
		{
			wxLogDebug(" . . . . . . . (%s)  marker %i removed manually from position %i",
				wxDateTime::Now().FormatISOTime().c_str(),
				debug_oldMarkerId, debug_oldMarkerPos);
		}
		else
		{
			wxLogDebug(" . . . . . . . (%s)  marker is already removed from position %i",
				wxDateTime::Now().FormatISOTime().c_str(),
				debug_oldMarkerPos);
		}
	#endif
}


bool SjRingbuffer::IsMarkerInRange(long pos, long bytes) const
{
	if( m_markerId )
	{
		wxASSERT( m_markerPos >= 0 && m_markerPos < m_totalBytes );

		pos = pos % m_totalBytes;

	    if( pos + bytes > m_totalBytes )
		{
			long bytesAtEnd = m_totalBytes - pos;

			if( m_markerPos >= 0
			 && m_markerPos <  bytes-bytesAtEnd )
			{
				#ifdef DEBUG_MARKERS
					//wxLogDebug(" . . . . . . . (%s)  marker %i at position %i is in range %i+%i",
					//	wxDateTime::Now().FormatISOTime().c_str(),
					//	m_markerId, m_markerPos, pos, bytes);
				#endif

				return TRUE;
			}

			if( m_markerPos >= pos
			 && m_markerPos <  pos+bytesAtEnd )
			{
				#ifdef DEBUG_MARKERS
					//wxLogDebug(" . . . . . . . (%s)  marker %i at position %i is in range %i+%i",
					//	wxDateTime::Now().FormatISOTime().c_str(),
					//	m_markerId, m_markerPos, pos, bytes);
				#endif

				return TRUE;
			}
		}
		else
		{
			if( m_markerPos >= pos
			 && m_markerPos <  pos+bytes )
			{
				#ifdef DEBUG_MARKERS
					//wxLogDebug(" . . . . . . . (%s)  marker %i at position %i is in range %i+%i",
					//	wxDateTime::Now().FormatISOTime().c_str(),
					//	m_markerId, m_markerPos, pos, bytes);
				#endif

				return TRUE;
			}
		}
	}

	return FALSE;
}


/*
void SjRingbuffer::RemoveSilenceAtEnd(long maxBytes)
{
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().
	wxASSERT( maxBytes % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );

	long startPos, silentBytes, bytesOnEnd;

	// correct the max. number of bytes to remove
	if( maxBytes > m_validBytes )
	{
		maxBytes = m_validBytes;
		if( maxBytes <= 0 )
		{
			return; // done, nothing to remove
		}
	}

	// get the starting position of bytes to remove
	startPos = ((m_validPos + m_validBytes) - maxBytes) % m_totalBytes;

	// remove the silence
	if( startPos + maxBytes > m_totalBytes )
	{
		bytesOnEnd = m_totalBytes - startPos;

		silentBytes = ::SjGetSilentBytes(m_buffer, maxBytes - bytesOnEnd, TRUE, SILENCE_THRESHOLD_ON_END);
		if( silentBytes )
		{
			silentBytes += ::SjGetSilentBytes(m_buffer+startPos, bytesOnEnd, TRUE, SILENCE_THRESHOLD_ON_END);
		}
	}
	else
	{
		silentBytes = ::SjGetSilentBytes(m_buffer+startPos, maxBytes, TRUE, SILENCE_THRESHOLD_ON_END);
	}

	// move marker if it is in the removed area
	if( IsMarkerInRange(m_validPos+(m_validBytes-silentBytes), silentBytes) )
	{
		#if defined(__WXDEBUG__) && defined(DEBUG_MARKERS)
			long debug_oldMarkerPos = m_markerPos;
		#endif

		m_markerPos = m_validPos+(m_validBytes-silentBytes);

		#if defined(__WXDEBUG__) && defined(DEBUG_MARKERS)
			wxLogDebug(" . . . . . . . (%s)  marker %i moved by %i \"silent bytes on end\" from %i to %i",
				wxDateTime::Now().FormatISOTime().c_str(),
				m_markerId, silentBytes, debug_oldMarkerPos, m_markerPos);
		#endif
	}

	m_validBytes -= silentBytes;

	// are the valid bytes small enough to perform a possibly waiting buffer shrinking?
	ResizeCheck();
}
*/


/*
void SjRingbuffer::RemoveSilenceOnPush(long maxBytes)
{
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().

	wxASSERT( maxBytes % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );

	m_removeSilenceOnPushBytesLeft		= maxBytes;
}
*/



/*
void SjRingbuffer::CrossfadeOnPush(long rewindBytes)
{
	// If you use this function in different threads, don't forget
	// to call Lock()/Unlock().

	wxASSERT( rewindBytes % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );

	// Are there enough valid bytes for crossfading?
	if( rewindBytes > m_validBytes )
	{
		rewindBytes = m_validBytes;
		if( rewindBytes == 0 )
		{
			return; // there is nothing to crossfade :-(
		}
	}


	m_crossfade.Init(NULL, rewindBytes);

	m_validBytes -= rewindBytes;
}
*/


/*
long SjRingbufferState::GetSecondsEverDecoded() const
{
	long secondsEverDecoded =
		m_gbytesEverDecoded*(SJ_ONE_GB/SJ_WW_BYTESPERSECOND) +
		m_bytesEverDecoded/SJ_WW_BYTESPERSECOND;

	return secondsEverDecoded;
}
*/

