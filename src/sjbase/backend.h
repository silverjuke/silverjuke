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


// possible backends that can be created using new SjBackend()
enum SjBackendId
{
	SJBE_ID_AUDIOOUT = 0, // do not change the IDs as they may be saved to disk or used otherwise
	SJBE_ID_PRELISTEN = 1
};


// the device state
enum SjBackendState
{
	SJBE_STATE_CLOSED = 0,
	SJBE_STATE_PLAYING,
	SJBE_STATE_PAUSED
};


// DSP and informational callback, if not stated otherwise, the function should return 0.
// TAKE CARE: this function ist called while processing the audio data,
// just before the output.  So please, do not do weird things here!
enum SjBackendMsg
{
	SJBE_MSG_NONE = 0,
	SJBE_MSG_DSP,
	SJBE_MSG_END_OF_STREAM
};
struct SjBackendCallbackParam
{
    SjBackendMsg     msg;
    float*           buffer;
    long             bytes;
	void*            userdata;
	SjBackend*       backend;
	int              lane;
	SjBackendStream* stream;
};
typedef long (SjBackendCallback)(SjBackendCallbackParam*);


// the backend base class
class SjBackend
{
public:
	// constructor. The device is in the CLOSED state after construction
	SjBackend(SjBackendId id, int lanes)
	{
		m_id     = id; // Default, Prelisten, etc. The real output selection can be implemented in GetLittleOptions()
		m_lanes  = lanes;  // Max. number of lanes. On every lane one stream at the same time can be played. May or may not be used by the implementation.
	}

	virtual void             GetLittleOptions (SjArrayLittleOption&) = 0;

	// Open the device (if not yet done), create a stream and set the device to PLAYING.
	// CreateStream() is not called if the the device is PAUSED.
	virtual SjBackendStream* CreateStream     (int lane, const wxString& url, long seekMs, SjBackendCallback*, void* userdata) = 0;

	// Get/set the device state.
	// To switch from CLOSED to PLAYING, use CreateStream(), for all other combinations, use SetDeviceState().
	// Before we set the device to the CLOSED state, we will destroy all steams on it.  Pleasant, isn't it?
	// (closed devices should release eg. pen audio outputs here - and open them again on the next call to CreateStream())
	virtual SjBackendState   GetDeviceState   () = 0;
	virtual void             SetDeviceState   (SjBackendState state) = 0;
	bool                     IsDeviceOpened   () { return (GetDeviceState()!=SJBE_STATE_CLOSED); }

	// Set the main volume. If the IsDeviceOpen() returns false, this function is not called.
	virtual void             SetDeviceVol     (double gain) = 0; // 0.0 - 1.0

	// DestroyBackend() is normally called on program shutdown when the backend is not needed any longer.
	// Normally not followed by a recreation.
	virtual void             DestroyBackend   () = 0;

	// Get the name of the Device (this is not the output/sink that is used by the backend)
	wxString GetName() const
	{
		     if(m_id==SJBE_ID_AUDIOOUT)  { return "audioout";  }
		else if(m_id==SJBE_ID_PRELISTEN) { return "prelisten"; }
		else                             { return "unknown";   }
	}


/*private:
declared as public to be usable from callbacks (for speed reasons, this avoids one level of iteration)*/
	virtual                  ~SjBackend       () {}
	SjBackendId              m_id;
	int                      m_lanes;
};


// streams created by classes derived from SjBackend
class SjBackendStream
{
public:
    virtual void             GetTime          (long& totalMs, long& elapsedMs) = 0; // -1=unknown
    virtual void             SeekAbs          (long ms) = 0;
    int                      GetLane          () const { return m_cbp.lane; }
	wxString                 GetUrl           () const { return m_url; }
	uint32_t                 GetStartingTime  () const { return m_startingTime; }
	virtual void             DestroyStream    () = 0;

/*protected:
declared as public to be usable from callbacks (for speed reasons, this avoids one level of iteration)*/
	SjBackendStream(int lane, const wxString& url, SjBackend* backend, SjBackendCallback* cb, void* userdata)
	{
		m_url          = url;
		m_startingTime = wxDateTime::Now().GetAsDOS();
		m_cb           = cb;
		m_cbp.lane     = lane;
		m_cbp.msg      = SJBE_MSG_NONE;
		m_cbp.buffer   = NULL;
		m_cbp.bytes    = 0;
		m_cbp.userdata = userdata;
		m_cbp.backend  = backend;
		m_cbp.stream   = this;
	}
	virtual                  ~SjBackendStream () {}
	wxString                 m_url;
	uint32_t                 m_startingTime;
	SjBackendCallback*       m_cb;
    SjBackendCallbackParam   m_cbp;
};


#endif // __SJ_BACKEND_H__
