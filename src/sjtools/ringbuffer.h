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
 * File:    ringbuffer.h
 * Authors: Björn Petersen
 * Purpose: A simple ringbuffer
 *
 ******************************************************************************/


#ifndef __SJ_RINGBUFFER_H__
#define __SJ_RINGBUFFER_H__


class SjRingbuffer
{
public:
	// Constructor / Destructor
	               SjRingbuffer        ();
	               ~SjRingbuffer       ();

	// (Re-)Allocating the buffer
	bool            Alloc               (long totalBytes);
	bool            Resize              (long totalBytes);
	void            Free                ();
	bool            IsAllocated         () const { return m_buffer!=NULL; }

	// Common Buffer Information
	// Note: free+valid is not always equal to the number of total bytes eg. when
	// a buffer resizing is waiting!!
	//long           GetTotalMs          () const { return m_totalMs; }
	long           GetTotalBytes       () const { return m_totalBytes; }
	long           GetFreeBytes        () const { return m_waitingForResizeBytes?
															   0
															: ((m_totalBytes - m_validBytes) - (SJ_WW_CH*SJ_WW_BYTERES))/*make sure, markers are unique*/;
												 }
	long            GetValidBytes       () const { return m_validBytes; }

	// Writing and reading to/from the buffer. On reading, the function returns
	// the marker ID (if the given position was marked) or 0.  You can also use a NULL-pointer
	// instead of a buffer for peeking just to check the presence of markers.
	void            PushToEnd           (const unsigned char* src, long bytes);
	long            PeekFromBeg         (unsigned char* dest, long offset, long bytes) const;
	void            RemoveFromBeg       (long bytes, long offsetForMarkerRemovment);

	// Removing silence. RemoveSilenceAtEnd() rewinds the internal end of the
	// valid bytes (m_validPos + m_validBytes) the max. number of silent bytes.
	// RemoveSilenceOnPush() sets an internal value, the next calls to PushToEnd()
	// won't add silence data up to maxBytes.
	//void            RemoveSilenceAtEnd   (long maxBytes);
	//void            RemoveSilenceOnPush   (long maxBytes);

	// CrossfadeAtEnd() rewinds the internal end of the valid bytes
	// (m_validPos + m_validBytes) to the given number of bytes.  The next calls to
	// PushToEnd() will mix the given data with the already stored data until the
	// given number of bytes are written; after this PushToEnd() works as usual.
	//void            CrossfadeOnPush       (long bytes);
	//bool            IsCrossfadingOnPush   () const { return m_crossfade.GetRemainingBytes()!=0; }

	// Empty the buffer content, this does NOT free the buffer.
	void            Empty               ();

	// Mark the current buffer position (more exact, the next data added
	// by PushToEnd() are marked)
	void            SetMarker           (long markerId, long offset);
	void            RemoveMarker        () { SetMarker(0, 0); }
	bool            HasMarker           () const { return m_markerId!=0; }
	long            GetMarker           () const { return m_markerId; }

	// Locking the buffer, if you use the buffer from withing
	// threads you should call Lock()/Unlock() before/after
	// every function that will modify the buffer.  SjRingbuffer does
	// not use Lock()/Unlock() itself for performance reasons.
	void            Lock                () { m_criticalSection.Enter(); }
	void		    Unlock              () { m_criticalSection.Leave(); }

private:
	// private stuff
	wxCriticalSection m_criticalSection;

	unsigned char*  m_buffer;
	long            m_totalBytes;
	//long            m_totalMs;

	long            m_validPos;
	long            m_validBytes;

	//SjCrossfade   m_crossfade;

	//long          m_removeSilenceOnPushBytesLeft;

	long            m_markerPos;
	long            m_markerId;

	bool            ResizeDo            (long totalBytes);
	void            ResizeCheck         () { if( m_waitingForResizeBytes && m_validBytes <= m_waitingForResizeBytes ) { ResizeDo(m_waitingForResizeBytes); } }
	long            m_waitingForResizeBytes;

	bool            IsMarkerInRange     (long pos, long bytes) const;
};


#endif // __SJ_RINGBUFFER_H__

