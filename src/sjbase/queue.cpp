/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2015 Björn Petersen Software Design and Development
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
 * File:    queue.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke player
 *
 ******************************************************************************/


#include <sjbase/base.h>


/*******************************************************************************
 * Constructor
 ******************************************************************************/


SjQueue::SjQueue()
	: m_dummyPlaylistInfo(&m_playlist, SJ_PROGRAM_NAME /*used. eg. by the vis.*/, TRUE, 0)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	m_pos                   = -1;

	m_shuffle               = SJ_DEF_SHUFFLE_STATE;
	m_shuffleIntensity      = SJ_DEF_SHUFFLE_INTENSITY;

	m_repeat                = SJ_REPEAT_OFF;
	m_repeatRound           = 1;

	m_playNextId            = 0;

	m_queueFlags            = SJ_QUEUEF_DEFAULT;
	m_boredomTrackMinutes   = SJ_DEF_BOREDOM_TRACK_MINUTES;
	m_boredomArtistMinutes  = SJ_DEF_BOREDOM_ARTIST_MINUTES;

	m_isInitialized         = false;

	CleanupNextShufflePos();
}


/*******************************************************************************
 * Previous / Next
 ******************************************************************************/


bool SjQueue::IsBoring(const wxString& artistName, const wxString& trackName, unsigned long currTimestamp) const
{
	if( m_queueFlags&SJ_QUEUEF_BOREDOM_TRACKS )
	{
		unsigned long itemTimestamp = m_historyTracks.Lookup(artistName+wxT("/")+trackName);
		if( itemTimestamp!=0 && SjTimestampDiff(itemTimestamp, currTimestamp) <= (unsigned long)(m_boredomTrackMinutes*60*1000) )
			return true; // this is boring: track found in "boredom track list", position not allowed
	}

	if( m_queueFlags&SJ_QUEUEF_BOREDOM_ARTISTS )
	{
		unsigned long itemTimestamp = m_historyArtists.Lookup(artistName);
		if( itemTimestamp!=0 && SjTimestampDiff(itemTimestamp, currTimestamp) <= (unsigned long)(m_boredomArtistMinutes*60*1000) )
			return true; // this is boring: track found in "boredom artist list", position not allowed
	}

	return false; // this is not boring :-) position allowed
}


long SjQueue::GetNextShufflePos_GetPossibleTrack(bool regardBoredom, long repeatRound, unsigned long currTimestamp)
{
	// get an array of possible tracks; this array still does not regard the boredom settings
	wxArrayLong possibleTracks;
	long i, cnt = GetCount();
	for( i = 0; i < cnt; i++ )
	{
		if( m_playlist[i].GetPlayCount() < repeatRound
		        && i != m_pos )
		{
			possibleTracks.Add(i);
		}
	}

	// select a track by random
	long nextPos = -1, maxRnd;
	while( 1 )
	{
		// any tracks left possible?
		cnt = possibleTracks.GetCount();
		if( cnt == 0 )
			break; // nothing found :-(

		// regard the shuffle intensity and calculate the max. index
		maxRnd = cnt;
		if( m_shuffleIntensity >= 1 && m_shuffleIntensity <= 100 )
		{
			maxRnd = (m_shuffleIntensity*cnt) / 100;
			if( maxRnd <= 3 ) maxRnd = 3;
			if( maxRnd > cnt ) maxRnd = cnt;
		}

		// calculate a random position
		i = SjTools::Rand(maxRnd);
		if( !regardBoredom || !IsBoring(possibleTracks[i], currTimestamp) )
		{
			nextPos = possibleTracks[i];
			break; // position found :-)
		}

		// position bad by boredom settings - remove this from the possible tracks
		possibleTracks.RemoveAt(i);
	}

	// done
	return nextPos;
}


long SjQueue::GetNextShufflePos(int flags, unsigned long currTimestamp)
{
	wxASSERT( wxThread::IsMain() );

	// if not yet done, calculate the next shuffle position
	// this does not alter the queue!
	if( m_nextShufflePosFor != m_pos )
	{
		// collect all possible tracks and calculate the next shuffled track.
		//
		// as the new boredom methods compare the artist and track names, and retrieving this
		// may be very time consuming (database- and/or ID3-tag-access) we check this *very* last -
		// so, possibleTracks may still contain positions not possible due to boredom settings!
		m_nextShufflePosFor = m_pos;
		m_nextShufflePos = GetNextShufflePos_GetPossibleTrack(true, m_repeatRound, currTimestamp);
		if( m_nextShufflePos == -1 )
		{
			if( m_repeat == SJ_REPEAT_ALL )
			{
				m_nextShufflePos = GetNextShufflePos_GetPossibleTrack(true, m_repeatRound+1, currTimestamp);
				if( m_nextShufflePos != -1 )
				{
					m_nextShuffleIncRepeatRound = true;
				}
				else
				{
					m_nextShufflePos = GetNextShufflePos_GetPossibleTrack(false, m_repeatRound, currTimestamp);
					if( m_nextShufflePos == -1 && GetCount() )
					{
						m_nextShufflePos = GetNextShufflePos_GetPossibleTrack(false, m_repeatRound+1, currTimestamp);
						if( m_nextShufflePos != -1 )
						{
							m_nextShuffleIncRepeatRound = true;
						}
						else
						{
							m_nextShufflePos = m_pos;
						}
					}
				}
			}
			else
			{
				m_nextShufflePos = GetNextShufflePos_GetPossibleTrack(false, m_repeatRound, currTimestamp);
			}
		}

		wxASSERT( m_nextShufflePosFor == m_pos );
	}

	// return the next shuffle position, this may increase the repeat rounf
	long newPos = m_nextShufflePos;
	if( !(flags&SJ_PREVNEXT_LOOKUP_ONLY) )
	{
		if( m_nextShuffleIncRepeatRound )
		{
			IncRepeatRound();
		}
		CleanupNextShufflePos();
	}

	return newPos;
}


long SjQueue::GetNextPos(int flags)
{
	wxASSERT( wxThread::IsMain() );
	long queueCount = GetCount();
	long newPos = -1;
	unsigned long currTimestamp = SjTools::GetMsTicks();


	// First, check if we have a "play next" ID and setup "newPos" for this
	///////////////////////////////////////////////////////////////////////

	if( m_playNextId )
	{
		// Notes: m_playNextId is set if the enqueing option "play track next" is
		//        used.  As this does not work with the normal shuffle/repeat single/etc. modes,
		//        we ignore these modes for this single ID and play this ID always next.  If the
		//        user selects "play track next" multiple times, only the last track ID is
		//        remembered and used - if the user wants to have a special order, he should
		//        really turn shuffle off.
		newPos = m_playlist.GetPosById(m_playNextId);
		if( newPos < 0 || m_playlist[newPos].GetPlayCount()>0 )
			m_playNextId = 0;
	}


	// Second, see what to do
	///////////////////////////////////////////////////////////////////////

	if( queueCount == 0 || (m_pos < 0 && !(flags&SJ_PREVNEXT_INIT)) )
	{
		// queue is empty, there is no next position
		newPos = -1;
	}
	else if( m_playNextId != 0 )
	{
		// play the track maked as "play next" - this overrules shuffle and repeat settings
		wxASSERT( newPos >= 0 && newPos < queueCount );
		if( !(flags&SJ_PREVNEXT_LOOKUP_ONLY) )
			m_playNextId = 0;
	}
	else if( m_repeat == SJ_REPEAT_SINGLE && m_pos >= 0 && (flags&SJ_PREVNEXT_REGARD_REPEAT) )
	{
		// repeat the current position
		newPos = m_pos;
	}
	else if( m_shuffle )
	{
		// shuffle to any position
		newPos = GetNextShufflePos(flags, currTimestamp);
	}
	else if( m_pos >= queueCount-1 )
	{
		// at end of queue, repeat all?
		// We do not regard the boredom settings in this case (normally, all files are just played,
		// we cannot optimize the order to regard the boredom settings).
		if( m_repeat == SJ_REPEAT_ALL && (flags&SJ_PREVNEXT_REGARD_REPEAT) )
		{
			if( !(flags&SJ_PREVNEXT_LOOKUP_ONLY) )
				IncRepeatRound();
			newPos = 0;
		}
		else
		{
			newPos = -1;
		}
	}
	else
	{
		// just the next position
		newPos = m_pos + 1;

		// regard the "avoid boredeom" settings, see http://www.silverjuke.net/forum/topic-2610.html
		if( !(flags&SJ_PREVNEXT_LOOKUP_ONLY) )
		{
			long betterPos = newPos;
			while( betterPos < queueCount && IsBoring(betterPos, currTimestamp) )
			{
				betterPos++;
			}

			if( betterPos < queueCount && betterPos != newPos )
			{
				// we will alter the playlist:
				//
				// log some information about this, we had some trouble ourselves to understand this
				// behaviour, see http://www.silverjuke.net/forum/topic-2866.html
				// (note that wxLogInfo() should be placed _before_ MovePos() - after MovePos(), the betterPos is unusable ...)
				//
				// however, do not pop up a message box, this may disturb alot as this may happen _any_time, not only just after enqueing
				long markPosAsMoved;
				for( markPosAsMoved = newPos; markPosAsMoved < betterPos; markPosAsMoved++ )
					m_playlist.Item(markPosAsMoved).SetFlag(SJ_PLAYLISTENTRY_MOVED_DOWN);

				wxLogInfo(wxT("%s moved up due to avoid bordedom settings"), m_playlist.Item(betterPos).GetUrl().c_str());

				// move "betterPos" to "newPos", the returned "newPos" will not be changed
				wxASSERT( betterPos > newPos  );
				m_playlist.MovePos(betterPos, newPos);
			}
		}
	}

	wxASSERT( newPos == -1 || (newPos >= 0 && newPos < GetCount()) );
	return newPos;
}


long SjQueue::GetPrevPos(int flags)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	long queueCount = GetCount();
	long newPos;

	if( queueCount == 0 || m_pos < 0 )
	{
		// queue is empty, there is no previous position
		newPos = -1;
	}
	else if( m_repeat == SJ_REPEAT_SINGLE && m_pos >= 0 && (flags&SJ_PREVNEXT_REGARD_REPEAT) )
	{
		// repeat the current position
		newPos = m_pos;
	}
	else if( m_shuffle )
	{
		// shuffle to any position
		newPos = PopFromHistory(flags);
	}
	else if( m_pos == 0 )
	{
		// at start of queue
		if( m_repeat == SJ_REPEAT_ALL && (flags&SJ_PREVNEXT_REGARD_REPEAT) )
		{
			newPos = queueCount-1;
		}
		else
		{
			newPos = -1;
		}
	}
	else
	{
		// just the previous position
		newPos = m_pos - 1;
	}

	return newPos;
}


/*******************************************************************************
 * Get / Set Information
 ******************************************************************************/


long SjQueue::GetClosestPosByUrl(const wxString& url) const
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	if( m_playlist.IsInPlaylist(url) )
	{
		long queueCount = GetCount(), i;

		// first, search forward
		for( i = m_pos; i < queueCount; i++ )
		{
			if( m_playlist.Item(i).GetUrl()==url )
			{
				return i;
			}
		}

		// then, search backward
		for( i = queueCount-1; i >= 0; i-- )
		{
			if( m_playlist.Item(i).GetUrl()==url )
			{
				return i;
			}
		}
	}

	// url not found
	return wxNOT_FOUND;
}


long SjQueue::GetAllPosByUrl(const wxString& url, wxArrayLong& ret, bool unplayedOnly) const
{
	// This function may only be called from the main thread.
	//
	// The returned array is not cleared here - by design - if this is wanted, the
	// caller is responsible for this.
	wxASSERT( wxThread::IsMain() );

	long allUrlCount = m_playlist.GetCountInPlaylist(url);
	if( allUrlCount > 0 )
	{
		long queueCount = GetCount(), i;

		// search all urls
		for( i = 0; i < queueCount; i++ )
		{
			SjPlaylistEntry& item = m_playlist.Item(i);
			if( item.GetUrl()==url )
			{
				if( !unplayedOnly
				        || (item.GetPlayCount()==0 || i==m_pos) )
				{
					ret.Add(i);
					if( (long)ret.GetCount() == allUrlCount )
					{
						break;
					}
				}
			}
		}
	}

	wxASSERT( (long)ret.GetCount() == allUrlCount || unplayedOnly );
	return (long)ret.GetCount();
}


wxString SjQueue::GetUrlByPos(long pos) const
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	// use current queue position?
	if( pos < 0 )
	{
		pos = m_pos;
	}

	// position valid?
	if( pos < 0 || pos >= GetCount() )
	{
		return wxEmptyString;
	}

	// return URL
	return m_playlist.Item(pos).GetUrl();
}


wxArrayString SjQueue::GetUrls() const
{
	wxArrayString ret;

	long queueCount = GetCount(), i;

	for( i = 0; i < queueCount; i++ )
	{
		ret.Add(m_playlist.Item(i).GetUrl());
	}

	return ret;
}


wxArrayString SjQueue::GetUrlsByIds(const SjLLHash& ids) const
{
	wxArrayString ret;

	long idsCount   = ids.GetCount();
	long queueCount = GetCount(), i;

	// I think, iterating the playlist and looking up all ids
	// is better than iterating the IDs and looking up the URLs.
	// Moreover, this preserves the order of the URLs.

	for( i = 0; i < queueCount && idsCount > 0; i++ )
	{
		if( ids.Lookup(m_playlist.Item(i).GetId()) )
		{
			ret.Add(m_playlist.Item(i).GetUrl());
			idsCount--;
		}
	}

	return ret;
}


static int SjQueue_GetPosByIds_Sort(long* i1, long* i2)
{
	return (*i1)-(*i2);
}
wxArrayLong SjQueue::GetPosByIds(const SjLLHash& ids) const
{
	wxArrayLong ret;

	long idsCount   = ids.GetCount();
	long queueCount = GetCount(), i;

	for( i = 0; i < queueCount && idsCount > 0; i++ )
	{
		if( ids.Lookup(m_playlist.Item(i).GetId()) )
		{
			ret.Add(i);
			idsCount--;
		}
	}

	ret.Sort(SjQueue_GetPosByIds_Sort);

	return ret;
}


wxString SjQueue::GetUrlById(long id) const
{
	long queueCount = GetCount(), i;

	for( i = 0; i < queueCount; i++ )
	{
		if( m_playlist.Item(i).GetId() == id )
		{
			return m_playlist.Item(i).GetUrl();
		}
	}

	return wxEmptyString;
}


SjPlaylistEntry& SjQueue::GetInfo(long pos/*-1 for current*/)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	// use current queue position?
	if( pos < 0 )
	{
		pos = m_pos;
	}

	// position valid?
	if( pos < 0 || pos >= GetCount() )
	{
		return m_dummyPlaylistInfo;
	}

	return m_playlist[pos];
}


void SjQueue::AddToHistory(long pos)
{
	// position vaild?
	if( pos < 0 || pos >= GetCount() )
		return;

	SjPlaylistEntry& item = m_playlist.Item(pos);

	// add the current position (if any) to the history if it
	// differs from the last
	long historyCount = m_historyIds.GetCount();
	long lastPlayedId = (historyCount>0)? m_historyIds.Last() : -1;

	long newPlayedId = item.GetId();
	if( lastPlayedId != newPlayedId )
	{
		m_historyIds.Add(newPlayedId);
	}

	// Cleanup every ~ 100 tracks inserted ...
	#define MAX_HISTORY_SIZE 100
	if( historyCount > MAX_HISTORY_SIZE )   // decrease the history size if it is "too large"
	{	// (i think goin' back 100 tracks is enough - remember this is only used in shuffle mode)
		m_historyIds.RemoveAt(0, MAX_HISTORY_SIZE/10);
	}

	// moreover, add the current track artist and title -
	// this is needed for the boredom functions
	unsigned long currTimestamp = SjTools::GetMsTicks();
	wxString trackName  = item.GetTrackName();
	wxString artistName = item.GetLeadArtistName();

	m_historyTracks .Insert(artistName+wxT("/")+trackName, currTimestamp);
	m_historyArtists.Insert(artistName,                    currTimestamp);

	// Cleanup every ~ 100 tracks inserted ...
	if( (m_historyTracks.GetCount() % 100) == 0 )
	{
		for( int cleanupRound = 0; cleanupRound <= 1; cleanupRound ++ )
		{
			unsigned long   stayMs  = (cleanupRound == 0?  m_boredomTrackMinutes :  m_boredomArtistMinutes) * 60 * 1000;
			SjSLHash*       hash    =  cleanupRound == 0? &m_historyTracks       : &m_historyArtists;
			wxString        itemString;
			unsigned long   itemTimestamp;
			SjHashIterator  iterator;
			while( (itemTimestamp=hash->Iterate(iterator, itemString)) != 0 )
			{
				if( SjTimestampDiff(itemTimestamp, currTimestamp) > stayMs )
				{
					// the iteration functionality allows us to remove the
					// current element
					hash->Remove(itemString);
				}
			}
		}
	}
}


long SjQueue::PopFromHistory(int flags)
{
	long i; // must be signed!
	long id, index;
	for( i = (long)m_historyIds.GetCount()-2/*the last is the current*/; i>=0; i-- )
	{
		id = m_historyIds[i];
		index = m_playlist.GetPosById(id);
		if( index == -1 )
		{
			// remove dead history items
			// (for speed reasons, we don't do this in unqueue)
			m_historyIds.RemoveAt(i);
		}
		else
		{
			if( !(flags&SJ_PREVNEXT_LOOKUP_ONLY) )
			{
				m_historyIds.RemoveAt(i);
				m_historyIds.RemoveAt(i); // same index as the count is one less after RemoveAt() before
			}
			return index;
		}
	}

	return -1;
}


void SjQueue::SetCurrErroneous()
{
	if( m_pos >= 0 && m_pos < m_playlist.GetCount() )
	{
		m_playlist.Item(m_pos).SetFlag(SJ_PLAYLISTENTRY_ERRONEOUS);
	}
}


void SjQueue::SetCurrPos(long pos)
{
	// set new position
	CleanupNextShufflePos();
	m_pos = pos;

	// mark as played
	m_playlist.Item(pos).SetPlayCount(m_repeatRound);

	// add to history (needed for the "previous" button and for "avoid boredom")
	AddToHistory(pos);
}


void SjQueue::ToggleRepeat()
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	switch( GetRepeat() )
	{
		case SJ_REPEAT_OFF:     SetRepeat(SJ_REPEAT_ALL);    break;
		case SJ_REPEAT_ALL:     SetRepeat(SJ_REPEAT_SINGLE); break;
		case SJ_REPEAT_SINGLE:  SetRepeat(SJ_REPEAT_OFF);    break;
	}
}


long SjQueue::MoveByIds(const SjLLHash& idsToMove, long motionAmount)
{
	// anything to move?
	if( motionAmount==0 || idsToMove.GetCount()==0 )
	{
		return 0;
	}

	// get the postions of all IDs to move
	wxArrayLong posToMove = GetPosByIds(idsToMove);

	// correct the max. motion amount
	if( motionAmount < 0 )
	{
		// move up
		long itemsBeforeFirstPosToMove = posToMove[0];
		wxASSERT( itemsBeforeFirstPosToMove >= 0 );
		if( motionAmount*-1 > itemsBeforeFirstPosToMove )
		{
			motionAmount = itemsBeforeFirstPosToMove*-1;
		}
	}
	else
	{
		// move down
		long itemsAfterLastPosToMove = (GetCount()-posToMove.Last())-1;
		wxASSERT( itemsAfterLastPosToMove >= 0 );
		if( motionAmount > itemsAfterLastPosToMove )
		{
			motionAmount = itemsAfterLastPosToMove;
		}
	}

	// any amount left to move?
	if( motionAmount == 0 )
	{
		return 0;
	}

	// remember the ID of the current postion to set it after the movement
	long currPosId = 0;
	if( m_pos >= 0 )
	{
		currPosId = GetIdByPos(m_pos);
	}

	// move all requested items
	int i, iCount = posToMove.GetCount();
	if( motionAmount < 0 )
	{
		for( i = 0; i < iCount; i++ )
		{
			m_playlist.MovePos(posToMove[i], posToMove[i]+motionAmount);
		}
	}
	else
	{
		for( i = iCount-1; i >= 0; i-- )
		{
			m_playlist.MovePos(posToMove[i], posToMove[i]+motionAmount);
		}
	}

	// correct the current position as it may have changed by the movement
	if( m_pos >= 0 )
	{
		m_pos = GetPosById(currPosId);
	}

	return motionAmount;
}


void SjQueue::EqualizeRepeatRound()
{
	long maxPlayCount = 1, currPlayCount;
	long i, iCount = GetCount();
	for( i = 0; i < iCount; i++ )
	{
		currPlayCount = m_playlist.Item(i).GetPlayCount();
		if( currPlayCount > maxPlayCount )
		{
			maxPlayCount = currPlayCount;
		}
	}

	m_repeatRound = maxPlayCount;
}


/*******************************************************************************
 * Enqueue
 ******************************************************************************/


long SjQueue::EnqueueDo__(const wxString& url, long addBeforeThisPos, bool verified, SjLLHash* addedIds,
                          long playlistEntryFlags)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );
	wxLogDebug(wxT("SjQueue::EnqueueDo__(): url=%s, verified=%i"), url.c_str(), (int)verified);

	long newPos;

	// add to array, check playing position
	if( addBeforeThisPos < 0 || addBeforeThisPos > GetCount() )
	{
		newPos = m_playlist.GetCount();
		m_playlist.Add(url, verified,
		               playlistEntryFlags);
		if( addedIds )
		{
			addedIds->Insert(m_playlist[m_playlist.GetCount()-1].GetId(), 1);
		}
	}
	else
	{
		newPos = addBeforeThisPos;
		m_playlist.Insert(url, addBeforeThisPos, verified/*bug fixed: before Silverjuke 1.00 we set this always to TRUE*/,
		                  playlistEntryFlags);
		if( addedIds )
		{
			addedIds->Insert(m_playlist[newPos].GetId(), 1);
		}

		if( m_pos >= addBeforeThisPos )
		{
			m_pos++;    // does not change the physical position, so a call to
			// SetCurrPos() is not needed (and not good...)
		}
	}

	return newPos;
}


void SjQueue::EnqueueFinish__(long oldPos)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	CleanupNextShufflePos();

	if( oldPos == -1 )
	{
		long newPos = GetNextPos(SJ_PREVNEXT_INIT);
		if( newPos != -1 )
			SetCurrPos(newPos); // in Silverjuke <= 2.52beta15, we set m_pos directly instead of calling SetCurrPos() --
		// this results in a missing playing mark for the first track played -- s. http://www.silverjuke.net/forum/topic-2593.html
	}
}


long SjQueue::Enqueue(const wxArrayString& urls, long addBeforeThisPos, bool verified, SjLLHash* addedIds, long playlistEntryFlags)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	long    newPos, oldPos = m_pos, currPos;
	long    urlsCount = urls.GetCount(), i;
	bool    markForPlayNext = false;

	if( playlistEntryFlags & SJ_PLAYLISTENTRY_PLAYNEXT )
	{
		playlistEntryFlags &= ~SJ_PLAYLISTENTRY_PLAYNEXT;
		markForPlayNext = true;
	}

	if( addBeforeThisPos < 0 || addBeforeThisPos > GetCount() )
	{
		newPos = urlsCount;
		for( i = 0; i < urlsCount; i++ )
		{
			currPos = EnqueueDo__(urls.Item(i), -1, verified, addedIds, playlistEntryFlags);
			if( markForPlayNext )
			{
				// m_playNextId is set if the enqueing option "play track next" is
				// used.  As this does not work with the normal shuffle mode, we ignore
				// the shuffle mode for this single ID and play this ID always next.  If the
				// user selects "play track next" multiple times, only the last track ID is
				// remembered and used - if the user wants to have a special order, he should
				// really turn shuffle of.
				m_playNextId = m_playlist[currPos].GetId();
				markForPlayNext = false;
			}
		}
	}
	else
	{
		newPos = addBeforeThisPos;
		for( i = 0; i < urlsCount; i++ )
		{
			currPos = EnqueueDo__(urls.Item(i), addBeforeThisPos+i, verified, addedIds, playlistEntryFlags);
			if( markForPlayNext )
			{
				// See remark above.
				m_playNextId = m_playlist[currPos].GetId();
				markForPlayNext = false;
			}
		}
	}

	EnqueueFinish__(oldPos);

	return newPos;
}


/*******************************************************************************
 * Unqueue
 ******************************************************************************/


void SjQueue::UnqueueReplay__(SjPlayer* player, int replayRet)
{
	// This function is called if an unqueue is finished AND the currently
	// playing track is removed; SjQueue is already in the correct state,
	// however, we should check the shuffle and the repeat state of the player.
	//
	// If replayRet == 2, the LAST track was removed from the queue.

	if( m_pos == -1 )
		return; // nothing to replay, may happed from a call of UnqueueByIds()

	wxASSERT( replayRet );
	wxASSERT( player );
	wxASSERT( m_pos >= 0 && m_pos < GetCount() );

	// this is the new implementaton
	long oldPos = m_pos;
	if( m_shuffle )
	{
		// Shuffle is ON -> select a new random track
		m_pos = -1;
		CleanupNextShufflePos();
		long newPos = GetNextPos(SJ_PREVNEXT_INIT|SJ_PREVNEXT_REGARD_REPEAT|SJ_PREVNEXT_LOOKUP_ONLY);
		if( newPos != -1 )
		{
			m_pos = newPos;
			CleanupNextShufflePos();
			player->GotoAbsPos(GetCurrPos());
		}
		else
		{
			m_pos = oldPos;
			CleanupNextShufflePos();
			player->Stop();
			return; // player stopped as ther's nothing more to play
		}
	}
	else
	{
		// Shuffle is OFF -> check if the removed track was the LAST one
		if( replayRet == 2 )
		{
			// last track removed, set the position to the first track
			m_pos = 0;
			CleanupNextShufflePos();
			if( m_repeat == SJ_REPEAT_ALL )
			{
				player->GotoAbsPos(GetCurrPos());
			}
			else
			{
				if( !MoveToTopOnEoq() )
				{
					m_pos = oldPos;
				}

				player->GotoAbsPos(GetCurrPos());
				player->Stop();
			}
		}
		else
		{
			CleanupNextShufflePos();
			player->GotoAbsPos(GetCurrPos());
		}
	}

	// this was the old implementation
	/*
	CleanupNextShufflePos();
	player->GotoAbsPos(GetCurrPos());
	*/
}


long SjQueue::UnqueueByPos(long pos, SjPlayer* player, int* replayRet)
{
	// This function may only be called from the main thread.
	// The function returns the number of items with the same URL
	// that are still in the queue.

	wxASSERT( wxThread::IsMain() );

	long restUrls = m_playlist.RemoveAt(pos);
	int  replayHere = 0;

	// correct the queue position
	if( pos < m_pos )
	{
		m_pos--;   // if the currently playing item is removed,
		// m_queue.pos points to the next item which
		// will have the same index.
		// a call to SetCurrPos() is not needed (and not good...)
	}
	else if( pos == m_pos )
	{
		replayHere = 1;
	}

	if( m_pos >= (long)m_playlist.GetCount() )
	{
		replayHere = 2; // last track removed!
		m_pos = m_playlist.GetCount()-1;
		// may result as -1
		if( m_pos == -1 )
		{
			if( player )
			{
				player->Stop();
				replayHere = 0;
			}
		}
	}

	// change playing position
	if( player && replayHere )
	{
		if( replayRet )
		{
			// UnqueueReplay__() called by the caller
			*replayRet = replayHere;
		}
		else
		{
			UnqueueReplay__(player, replayHere);
		}
	}

	return restUrls;
}


void SjQueue::UnqueueByUrl(const wxString& url, SjPlayer* player, int* replayRet)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	int  replayHere = 0;
	long queueArrayCount = m_playlist.GetCount(), i;
	for( i = 0; i < queueArrayCount; i++ )
	{
		if( m_playlist[i].GetUrl() == url )
		{
			if( UnqueueByPos(i, player, &replayHere) == 0 )
			{
				break; // done, all URLs removed
			}
			i--; // as we removed one item, continue with the same position
		}
	}

	// change playing position
	if( player && replayHere )
	{
		if( replayRet )
		{
			// UnqueueReplay__() called by the caller
			*replayRet = replayHere;
		}
		else
		{
			UnqueueReplay__(player, replayHere);
		}
	}
}


void SjQueue::UnqueueByUrls(const wxArrayString& urls, SjPlayer* player)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	int  replayHere = 0;
	long urlsCount = urls.GetCount(), i;
	wxString url;
	for( i = 0; i < urlsCount; i++ )
	{
		url = urls[i];
		if( m_playlist.IsInPlaylist(url) )
		{
			UnqueueByUrl(url, player, &replayHere);
		}
	}

	// change playing position
	if( player && replayHere )
	{
		UnqueueReplay__(player, replayHere);
	}
}


void SjQueue::UnqueueByIds(const SjLLHash& ids, long removeVal, SjPlayer* player)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	int  replayHere = 0;
	long i, iCount = GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( ids.Lookup(m_playlist.Item(i).GetId()) == removeVal )
		{
			UnqueueByPos(i, player, &replayHere);
			i--;
			iCount--;
		}
	}

	// change playing position
	if( player && replayHere )
	{
		UnqueueReplay__(player, replayHere);
	}
}


void SjQueue::UnqueueAll(SjPlayer* player, bool stopVisIfPlaying)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	if( player )
	{
		player->Stop(stopVisIfPlaying);
	}
	m_pos = -1;
	m_playlist.Clear();
	m_historyIds.Clear();

	CleanupNextShufflePos();
}


void SjQueue::UnqueuePlayed()
{
	SjLLHash ids;
	long i, iLast = m_shuffle? (m_playlist.GetCount()-1) : m_pos, itemPlayCount;
	for( i = 0; i <= iLast; i++ )
	{
		if( i != m_pos )
		{
			itemPlayCount = m_playlist.Item(i).GetPlayCount();
			if( itemPlayCount >= m_repeatRound )
				ids.Insert(m_playlist.Item(i).GetId(), 1);
		}
	}

	if( ids.GetCount() )
		UnqueueByIds(ids, 1, NULL);
}


bool SjQueue::MoveToTopOnEoq() const
{
	// This function is normally called on the end of queue
	// and internally by SjQueue if the last available track in the
	// queue is unqueued.
	//
	// The function checks some states and returns TRUE if it seems to be best
	// to select the first queue item (this is useful eg. if the user hits "play" again)

	if( !m_isInitialized
	        || !g_mainFrame->IsOpAvailable(SJ_OP_PLAYPAUSE)
	        ||  GetCount()<=0 )
	{
		return FALSE;   // we recomment the caller to leave the queue position as is
		// (as "play" is not avalable the user cannot play the song again,
		// but he can enqueue a new song; this is true eg. for the kiosk mode)
	}

	if( (g_mainFrame->m_autoCtrl.m_flags&SJ_AUTOCTRL_AUTOPLAY_ENABLED)
	        &&  g_mainFrame->m_autoCtrl.m_autoPlayWaitMinutes==0 )
	{
		return FALSE;   // we recomment the caller to leave the queue position as is
		// (autoplay will enqueue the next song at once)
	}

	return TRUE;        // we recomment the caller to move the queue position to position #0
	// (the user may hit "play" and does not want to hear the last
	// song again)
}
