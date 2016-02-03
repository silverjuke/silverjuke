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
	long           GetTotalBytes       () const { return m_totalBytes; }
	long           GetFreeBytes        () const { return m_waitingForResizeBytes?
															   0
															: ((m_totalBytes - m_validBytes));
												 }
	long            GetValidBytes       () const { return m_validBytes; }

	// Writing and reading to/from the buffer.
	void            PushToEnd           (const unsigned char* src, long bytes);
	void            PeekFromBeg         (unsigned char* dest, long offset, long bytes) const;
	void            RemoveFromBeg       (long bytes);

	// Empty the buffer content, this does NOT free the buffer.
	void            Empty               ();

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

	long            m_validPos;
	long            m_validBytes;

	bool            ResizeDo            (long totalBytes);
	void            ResizeCheck         () { if( m_waitingForResizeBytes && m_validBytes <= m_waitingForResizeBytes ) { ResizeDo(m_waitingForResizeBytes); } }
	long            m_waitingForResizeBytes;
};


#endif // __SJ_RINGBUFFER_H__

