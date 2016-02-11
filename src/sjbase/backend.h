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
 * File:    backend.h
 * Authors: Björn Petersen
 * Purpose: Backend base class, derived classes implement engines as Xine,
 *          GStreamer, BASS, DirectShow etc.
 *
 ******************************************************************************/


#ifndef __SJ_BACKEND_H__
#define __SJ_BACKEND_H__


class SjBackendStream;
class SjBackendUserdata;


enum SjBackendId
{
	SJBE_ID_STDOUTPUT  = 0,
	SJBE_ID_PRELISTEN  = 1,
	SJBE_ID_FAKEOUTPUT = 2 // do not output anything but call the DSP callback; used eg. for silence detection
};


enum SjBackendState
{
	SJBE_STATE_CLOSED = 0,
	SJBE_STATE_PLAYING,
	SJBE_STATE_PAUSED
};


enum SjBackendMsg
{
	SJBE_MSG_CREATE = 1,      // send by the base on stream creation (in fact, CreateStream() may still fail) before the first SJBE_MSG_DSP,
	                          // may be used to init some data.  Do not expect samplerate/channels/etc. to contain correct data at this moment.

	SJBE_MSG_VIDEO_DETECTED,  // must be send by the implementation if the stream contains video data

	SJBE_MSG_DSP,             // must be send by the implementation

	SJBE_MSG_END_OF_STREAM,   // must be send by the implementation

	SJBE_MSG_DESTROY_USERDATA // send by the base
};


struct SjBackendCallbackParam
{
    SjBackendMsg     msg;
    float*           buffer;
    long             bytes;
    int              samplerate;
    int              channels;
	uint32_t         startingTime;
	SjBackend*       backend;
	SjBackendStream* stream;
};


typedef long (SjBackendCallback)(SjBackendCallbackParam*);


class SjBackend
{
public:
	// The following function must be added by the backend implementation.
	// - after construction the device should be CLOSED
	// - to switch from CLOSED to PLAYING, use CreateStream(). CreateStream() is not called if the device is PAUSED.
	// - then, the state can be changed using SetDeviceState() - the user makes sure, all streams are deleted before CLOSING it again. Pleasant, isn't it?
	                         SjBackend        (SjBackendId id); // The device is in the CLOSED state after construction
	virtual                  ~SjBackend       ();
	virtual void             GetLittleOptions (SjArrayLittleOption&) = 0;
	virtual SjBackendStream* CreateStream     (const wxString& url, long seekMs, SjBackendCallback*, SjBackendUserdata*) = 0;
	virtual SjBackendState   GetDeviceState   () const = 0;
	virtual void             SetDeviceState   (SjBackendState state) = 0;
	virtual void             SetDeviceVol     (double gain) = 0; // 0.0 - 1.0, only called on opened devices

	// higher-level functions
	bool                     IsDeviceOpened   () const { return (GetDeviceState()!=SJBE_STATE_CLOSED); }
	SjBackendId              GetId            () const { return m_id; };
	wxString                 GetName          () const;
	const wxArrayPtrVoid&    GetAllStreams    () const { return m_allStreams; }
	bool                     WantsVideo       () const { return (m_id==SJBE_ID_STDOUTPUT); }

private:
	SjBackendId              m_id;
	wxArrayPtrVoid           m_allStreams;
	friend class             SjBackendStream;
};


class SjBackendStream
{
	// The following function must be added by the backend implementation.
	// The user creates stream using SjBackend::CreateStream().
protected:                   SjBackendStream  (const wxString& url, SjBackend* backend, SjBackendCallback* cb, SjBackendUserdata*);
public: virtual              ~SjBackendStream ();
	virtual void             GetTime          (long& totalMs, long& elapsedMs) = 0; // -1=unknown
	virtual void             SeekAbs          (long ms) = 0;

	// higher-level functions
	wxString                 GetUrl           () const { return m_url; }
	uint32_t                 GetStartingTime  () const { return m_cbp.startingTime; }

	// these fields may be used by user for any purposes; must _not_ be used by derived classes!
	SjBackendUserdata*       m_userdata;

	// the following fiels should be treated as "private" to SjBackendStream and derived classes,
	// howver, they're declared as public to be usable from callbacks (for speed reasons, this avoids one level of iteration)
	wxString                 m_url;
	SjBackendCallback*       m_cb;
	SjBackendCallbackParam   m_cbp;
};


#endif // __SJ_BACKEND_H__
