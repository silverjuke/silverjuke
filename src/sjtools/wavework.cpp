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
 * File:    wavework.cpp
 * Authors: Björn Petersen
 * Purpose: Working with waves
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/backend.h>


double SjDecibel2Gain(double db)
{
	// for some common parameters, we're using predefined
	// return values to avoid rounding errors
	if( db == 14.00 )
	{
		return 5.0;
	}
	else if( db == 12.00 )
	{
		return 4.0;
	}
	else
	{
		return pow(10.0, db / 20.0);
	}
}


double SjGain2Decibel(double gain)
{
	// for some common parameters, we're using predefined
	// return values to avoid rounding errors
	if( gain == 5.0 )
	{
		return 14.0;
	}
	else if( gain > 0.0 )
	{
		return (20.0 * log10(gain));
	}
	else
	{
		return 0.0;
	}
}


long SjGain2Long(double gain)
{
	if( gain > 0 )
	{
		return (long)(gain*1000.0F);
	}
	else
	{
		return 0;
	}
}


double SjLong2Gain(long lng)
{
	return (double) (((double)lng) / 1000.0F);
}


void SjApplyVolume(float* buffer, long bytes, float gain)
{
	const float* bufferEnd = buffer + (bytes/sizeof(float));
	while( buffer < bufferEnd ) {
		*buffer++ *= gain;
	}
}


void SjMixdownChannels(float* buffer, long bytes, int channels, int destCh)
{
	// in a buffer defined by buffer-bytes-channels, mix all channels to destCh and mute the other ones
	if( channels <= 1 || channels > 256 || destCh < 0 || destCh >= channels ) return; // error

	float subsamsSum;
	long sampleStart, subsam, subsams = bytes / sizeof(float);
	for( sampleStart = 0; sampleStart < subsams; sampleStart += channels )
	{
		subsamsSum = 0;
		for( subsam = 0; subsam < channels; subsam++ )
		{
			subsamsSum += buffer[sampleStart+subsam];
			buffer[sampleStart+subsam] = 0;
		}
		buffer[sampleStart+destCh] = subsamsSum / channels;
	}
}


/*******************************************************************************
 * Detect Silence
 ******************************************************************************/


#define float_abs(x) ((x)<0? -(x) : (x))
#define threshold 0.0153 // = 500 when processing 16 bit samples


class SjDetectSilenceUserdata
{
public:
	SjDetectSilenceUserdata()
	{
		m_doScan = 1;
		m_subsamsProcessedOnStart = 0;
		m_isVideo = false;
		m_samplerate = 44100;
		m_channels = 2;
		m_silenceBegDetected = false;
	}
    int               m_doScan; // 0=wait, 1=start, 2=end
    bool              m_isVideo, m_silenceBegDetected;
    long              m_subsamsProcessedOnStart;
    int               m_samplerate, m_channels;
};


void SjDetectSilence_Callback(SjBackendCallbackParam* cbp)
{
	SjDetectSilenceUserdata* userdata = (SjDetectSilenceUserdata*)cbp->stream->m_userdata;
	if( cbp->channels <= 0 ) return;

	if( cbp->msg == SJBE_MSG_DSP )
	{
		const float* buf = cbp->buffer;
		long a, b = cbp->bytes;
		userdata->m_samplerate = cbp->samplerate;
		userdata->m_channels = cbp->channels;
		if( userdata->m_doScan == 1 )
		{
			// scan beginning of file
			b /= sizeof(float); // bytes -> samples
			for( a=0; a<b && float_abs(buf[a])<=threshold; a++ ) ; // count silent samples
			if( a<b ) { // sound has begun!
				for ( ; a && float_abs(buf[a])>threshold/4; a-- ) ; // move back to a quieter sample (to avoid "click")
				userdata->m_doScan = 0;
			}
			userdata->m_subsamsProcessedOnStart += a;
			userdata->m_silenceBegDetected = true;
		}
		else if( userdata->m_doScan == 2 )
		{
			// scan end of file
		}
	}
	else if( cbp->msg == SJBE_MSG_END_OF_STREAM )
	{
		userdata->m_doScan = 0;
	}
	else if( cbp->msg == SJBE_MSG_VIDEO_DETECTED )
	{
		userdata->m_doScan = 0;
		userdata->m_isVideo = 0;
	}
}


void SjDetectSilence(SjBackend* backend, const wxString& url, long& silenceBegMs, long& silenceEndMs)
{
	unsigned long debugStartTimeMs = SjTools::GetMsTicks();

	silenceBegMs = -1;
	silenceEndMs = -1;

	SjDetectSilenceUserdata userdata;

	// check, if the URL is locally available
	wxString file = wxFileSystem::URLToFileName(url).GetFullPath();
	if( !::wxFileExists(file) ) { return; } // URL is not local, checking for silence would take longer than time is saved by initial seek

	// check, for common video extensions, TODO: We should use our extension repository here
	wxString ext = SjTools::GetExt(file);
	if( ext == "avi"  || ext == "mpg" || ext == "flv" || ext == "vob" ) { return; } // video silence detection takes too long, normally

	// create the stream - this and scanning takes about 30 ms for both, GStreamer and BASS
	SjBackendStream* stream = backend->CreateStream(url, 0, SjDetectSilence_Callback, (SjBackendUserdata*)&userdata);
	if( stream == NULL ) { return; } // error - cannot create stream

	// scan the beginning of the file
	#define TIMEOUT_MS 500
	unsigned long timeoutMs = SjTools::GetMsTicks() + TIMEOUT_MS;
	bool cont = true;
	while( cont ) {
		wxMilliSleep(20);
		if( userdata.m_doScan == 0 ) {
			if( userdata.m_silenceBegDetected ) {
				silenceBegMs = ( ((float)(userdata.m_subsamsProcessedOnStart/userdata.m_channels) / (float)userdata.m_samplerate) * (float)1000 );
			}
			cont = false;
		}
		if( SjTools::GetMsTicks() > timeoutMs || userdata.m_isVideo ) { delete stream; return; } // timeout or video
	}

	#if 0 // for GStreamer, seeking to the end took about 500 ms - this is far too long to save eg. 200 ms on startup
	// scan the end of the file
	#define SEARCH_FROM_END_MS 1000
	long totalMs, elapsedMs;
	stream->GetTime(totalMs, elapsedMs);
	if( totalMs > 0 ) {
		backend->SetDeviceState(SJBE_STATE_PAUSED);
			stream->SeekAbs(totalMs-SEARCH_FROM_END_MS);
			userdata.m_doScan = 2;
		backend->SetDeviceState(SJBE_STATE_PLAYING);

		timeoutMs = SjTools::GetMsTicks() + TIMEOUT_MS;
		cont = true;
		while( cont ) {
			wxMilliSleep(20);
			if( userdata.m_doScan == 0 ) {
				cont = false;
			}
			if( SjTools::GetMsTicks() > timeoutMs ) { break; } // timeout
		}
	}
	#endif

	// done
	if( g_debug ) { wxLogInfo("%i ms needed to detect silence of: %i ms / %i ms", (int)(SjTools::GetMsTicks()-debugStartTimeMs), (int)silenceBegMs, (int)silenceEndMs); }
	delete stream;
}
