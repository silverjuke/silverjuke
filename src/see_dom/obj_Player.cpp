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
 * File:    obj_Player.cpp
 * Authors: Björn Petersen
 * Purpose: The Player class for SEE
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/browser.h>
#include <see_dom/sj_see.h>
#include <see_dom/sj_see_helpers.h>


// data used by our object
struct player_object
{
	SEE_native  m_native;       // MUST be first!

	SEE_object* m_onTrackChange;
	SEE_object* m_onPlaybackDone;
};


static player_object* alloc_player_object(SEE_interpreter* interpr)
{
	player_object* po = (player_object*)SEE_malloc(interpr, sizeof(player_object));
	memset(po, 0, sizeof(player_object));
	return po;
}


static player_object* get_player_object(SEE_interpreter* interpr, SEE_object* o);


/*******************************************************************************
 * play, pause(), next() etc.
 ******************************************************************************/


IMPLEMENT_FUNCTION(player, construct)
{
	RETURN_OBJECT( HOST_DATA->Player_new() );
}


IMPLEMENT_FUNCTION(player, play)
{
	g_mainFrame->Play( ARG_LONG(0)/*offset*/ );
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(player, pause)
{
	g_mainFrame->Pause();
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(player, stop)
{
	g_mainFrame->Stop();
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(player, prev)
{
	g_mainFrame->GotoPrev(false);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(player, next)
{
	if( g_mainFrame->HasNextIgnoreAP() || !ARG_LONG(0) )
		g_mainFrame->GotoNextRegardAP(false);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(player, isPlaying)
{
	RETURN_BOOL( g_mainFrame->IsPlaying() );
}


IMPLEMENT_FUNCTION(player, isPaused)
{
	RETURN_BOOL( g_mainFrame->IsPaused() );
}


IMPLEMENT_FUNCTION(player, isStopped)
{
	RETURN_BOOL( g_mainFrame->IsStopped() );
}


IMPLEMENT_FUNCTION(player, hasPrev)
{
	RETURN_BOOL( g_mainFrame->HasPrev() );
}


IMPLEMENT_FUNCTION(player, hasNext)
{
	if( !ARG_LONG(0) )
	{
		RETURN_BOOL( g_mainFrame->HasNextRegardAP() );
	}
	else
	{
		RETURN_BOOL( g_mainFrame->HasNextIgnoreAP() );
	}
}


/*******************************************************************************
 * queuePos, addAtPos()  etc.
 ******************************************************************************/


#define ARG_POS ARG_LONG_OR_DEF(0,-1)


IMPLEMENT_FUNCTION(player, getUrlAtPos)
{
	RETURN_STRING( g_mainFrame->GetQueueUrl(ARG_POS));
}


IMPLEMENT_FUNCTION(player, getAutoplayAtPos)
{
	RETURN_BOOL( (g_mainFrame->GetQueueInfo(ARG_POS).GetFlags()&SJ_PLAYLISTENTRY_AUTOPLAY)!=0 );
}


IMPLEMENT_FUNCTION(player, getPlayCountAtPos)
{
	RETURN_LONG( g_mainFrame->GetQueueInfo(ARG_POS).GetPlayCount() );
}


IMPLEMENT_FUNCTION(player, getArtistAtPos)
{
	RETURN_STRING( g_mainFrame->GetQueueInfo(ARG_POS).GetLeadArtistName() );
}


IMPLEMENT_FUNCTION(player, getAlbumAtPos)
{
	RETURN_STRING( g_mainFrame->GetQueueInfo(ARG_POS).GetAlbumName() );
}


IMPLEMENT_FUNCTION(player, getTitleAtPos)
{
	RETURN_STRING( g_mainFrame->GetQueueInfo(ARG_POS).GetTrackName() );
}


IMPLEMENT_FUNCTION(player, getDurationAtPos)
{
	RETURN_LONG( g_mainFrame->GetQueueInfo(ARG_POS).GetPlaytimeMs() );
}


IMPLEMENT_FUNCTION(player, addAtPos)
{
	g_mainFrame->Enqueue(ARG_STRING(1), ARG_POS,
	                     false/*verified?*/, ARG_BOOL(2)/*autoplay?*/, false/*uiChecks?*/);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(player, removeAtPos)
{
	g_mainFrame->UnqueueByPos(ARG_POS);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(player, removeAll)
{
	g_mainFrame->UnqueueAll();
	g_mainFrame->m_browser->RefreshSelection();
	RETURN_UNDEFINED;
}


/*******************************************************************************
 * Properties
 ******************************************************************************/


IMPLEMENT_HASPROPERTY(player)
{
	if(
	    VAL_PROPERTY( time )
	 || VAL_PROPERTY( duration )
	 || VAL_PROPERTY( queueLength )
	 || VAL_PROPERTY( queuePos )
	 || VAL_PROPERTY( volume )
	 || VAL_PROPERTY( onTrackChange )
	 || VAL_PROPERTY( onPlaybackDone )
	 || VAL_PROPERTY( repeat )
	 || VAL_PROPERTY( shuffle )
	 || VAL_PROPERTY( stopAfterThisTrack )
	 || VAL_PROPERTY( stopAfterEachTrack )
	 || VAL_PROPERTY( removePlayed )
	 || VAL_PROPERTY( avoidBoredom )
	)
	{
		RETURN_HAS;
	}
	else
	{
		RETURN_HASNOT;
	}
}


IMPLEMENT_GET(player)
{
	player_object* po = get_player_object(interpr_, this_);

	if( g_mainFrame == NULL )
	{
		RETURN_GET_DEFAULTS; // may happen, noticed first in Silverjuke 3.x
	}
	else if( VAL_PROPERTY( time ) )
	{
		RETURN_LONG( g_mainFrame->GetElapsedTime() );
	}
	else if( VAL_PROPERTY( duration ) )
	{
		RETURN_LONG( g_mainFrame->GetTotalTime() );
	}
	else if( VAL_PROPERTY( queueLength ) )
	{
		RETURN_LONG( g_mainFrame->GetQueueCount() );
	}
	else if( VAL_PROPERTY( queuePos ) )
	{
		RETURN_LONG( g_mainFrame->GetQueuePos() );
	}
	else if( VAL_PROPERTY( volume ) )
	{
		RETURN_LONG( g_mainFrame->GetMainVol() );
	}
	else if( VAL_PROPERTY( onTrackChange ) )
	{
		if( po->m_onTrackChange ) RETURN_OBJECT(po->m_onTrackChange); else RETURN_UNDEFINED;
	}
	else if( VAL_PROPERTY( onPlaybackDone ) )
	{
		if( po->m_onPlaybackDone ) RETURN_OBJECT(po->m_onPlaybackDone); else RETURN_UNDEFINED;
	}
	else if( VAL_PROPERTY( repeat ) )
	{
		RETURN_LONG( (long)g_mainFrame->GetRepeat() );
	}
	else if( VAL_PROPERTY( shuffle ) )
	{
		RETURN_BOOL( g_mainFrame->GetShuffle() );
	}
	else if( VAL_PROPERTY( stopAfterThisTrack ) )
	{
		RETURN_BOOL( g_mainFrame->m_player.StopAfterThisTrack() );
	}
	else if( VAL_PROPERTY( stopAfterEachTrack ) )
	{
		RETURN_BOOL( g_mainFrame->m_player.StopAfterEachTrack() );
	}
	else if( VAL_PROPERTY( removePlayed ) )
	{
		long queueFlags = g_mainFrame->m_player.m_queue.GetQueueFlags();
		RETURN_BOOL( (queueFlags&SJ_QUEUEF_REMOVE_PLAYED)!=0 );
	}
	else if( VAL_PROPERTY( avoidBoredom ) )
	{
		long queueFlags = g_mainFrame->m_player.m_queue.GetQueueFlags();
		queueFlags &= SJ_QUEUEF_BOREDOM_TRACKS|SJ_QUEUEF_BOREDOM_ARTISTS;
		RETURN_LONG( queueFlags );
	}
	else
	{
		RETURN_GET_DEFAULTS;
	}
}


IMPLEMENT_PUT(player)
{
	player_object* po = get_player_object(interpr_, this_);

	if( VAL_PROPERTY( time ) )
	{
		g_mainFrame->m_player.SeekAbs( VAL_LONG );
		g_mainFrame->UpdateDisplay();
	}
	else if( VAL_PROPERTY( queuePos ) )
	{
		g_mainFrame->GotoAbsPos( VAL_LONG );
		g_mainFrame->UpdateDisplay();
	}
	else if( VAL_PROPERTY( volume ) )
	{
		g_mainFrame->SetAbsMainVol( VAL_LONG );
	}
	else if( VAL_PROPERTY( onTrackChange ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onTrackChange);
		HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER); // you can also assign "undefined" as the new callback, AddPersistenObject handles this
		po->m_onTrackChange = newCallback;
	}
	else if( VAL_PROPERTY( onPlaybackDone ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onPlaybackDone);
		HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER); // you can also assign "undefined" as the new callback, AddPersistenObject handles this
		po->m_onPlaybackDone = newCallback;
	}
	else if( VAL_PROPERTY( repeat ) )
	{
		g_mainFrame->SetRepeat( (SjRepeat)VAL_LONG );
	}
	else if( VAL_PROPERTY( shuffle ) )
	{
		g_mainFrame->SetShuffle( VAL_BOOL );
	}
	else if( VAL_PROPERTY( stopAfterThisTrack ) )
	{
		g_mainFrame->StopAfterThisTrack( VAL_BOOL ); // do not use g_mainFrame->m_player->StopAfterThisTrack(), see http://www.silverjuke.net/forum/post.php?p=10381#10381
	}
	else if( VAL_PROPERTY( stopAfterEachTrack ) )
	{
		g_mainFrame->StopAfterEachTrack( VAL_BOOL ); //             - " -
	}
	else if( VAL_PROPERTY( removePlayed ) )
	{
		long queueFlags, b1, b2;
		g_mainFrame->m_player.m_queue.GetQueueFlags(queueFlags, b1, b2);
		SjTools::SetFlag(queueFlags, SJ_QUEUEF_REMOVE_PLAYED, VAL_BOOL);
		g_mainFrame->m_player.m_queue.SetQueueFlags(queueFlags, b1, b2);
		g_mainFrame->SetSkinTargetValue(IDT_TOGGLE_REMOVE_PLAYED, (queueFlags&SJ_QUEUEF_REMOVE_PLAYED)? 1 : 0);
	}
	else if( VAL_PROPERTY( avoidBoredom ) )
	{
		long queueFlags, b1, b2;
		g_mainFrame->m_player.m_queue.GetQueueFlags(queueFlags, b1, b2);
		queueFlags &= ~(SJ_QUEUEF_BOREDOM_TRACKS|SJ_QUEUEF_BOREDOM_ARTISTS);
		queueFlags |= VAL_LONG;
		g_mainFrame->m_player.m_queue.SetQueueFlags(queueFlags, b1, b2);
	}
	else
	{
		DO_PUT_DEFAULTS;
	}
}


/*******************************************************************************
 * Let SEE know about this class (this part is a little more complicated)
 ******************************************************************************/


/* object class for Player.prototype and player instances */
static SEE_objectclass player_inst_class = {
	"Player",                   /* Class */
	player_get,                 /* Get */
	player_put,                 /* Put */
	SEE_native_canput,          /* CanPut */
	player_hasproperty,         /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	NULL,                       /* Construct */
	NULL,                       /* Call */
	NULL                        /* HasInstance */
};


/* object class for Player constructor */
static SEE_objectclass player_constructor_class = {
	"PlayerConstructor",        /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	player_construct,           /* Construct */
	NULL                        /* Call */
};


void SjSee::Player_init()
{
	SEE_value temp;

	// Create the "Player.prototype" object, this is used as a template for the instances
	m_Player_prototype = (SEE_object *)alloc_player_object(m_interpr);

	SEE_native_init((SEE_native *)m_Player_prototype, m_interpr,
	                &player_inst_class, m_interpr->Object_prototype);

	PUT_FUNC(m_Player_prototype, player, play,              0);
	PUT_FUNC(m_Player_prototype, player, pause,             0);
	PUT_FUNC(m_Player_prototype, player, stop,              0);
	PUT_FUNC(m_Player_prototype, player, prev,              0);
	PUT_FUNC(m_Player_prototype, player, next,              0);
	PUT_FUNC(m_Player_prototype, player, isPlaying,         0);
	PUT_FUNC(m_Player_prototype, player, isPaused,          0);
	PUT_FUNC(m_Player_prototype, player, isStopped,         0);
	PUT_FUNC(m_Player_prototype, player, hasPrev,           0);
	PUT_FUNC(m_Player_prototype, player, hasNext,           0);

	PUT_FUNC(m_Player_prototype, player, getUrlAtPos,       0);
	PUT_FUNC(m_Player_prototype, player, getAutoplayAtPos,  0);
	PUT_FUNC(m_Player_prototype, player, getPlayCountAtPos, 0);
	PUT_FUNC(m_Player_prototype, player, getArtistAtPos,    0);
	PUT_FUNC(m_Player_prototype, player, getAlbumAtPos,     0);
	PUT_FUNC(m_Player_prototype, player, getTitleAtPos,     0);
	PUT_FUNC(m_Player_prototype, player, getDurationAtPos,  0);
	PUT_FUNC(m_Player_prototype, player, addAtPos,          0);
	PUT_FUNC(m_Player_prototype, player, removeAtPos,       0);
	PUT_FUNC(m_Player_prototype, player, removeAll,         0);

	// create the "Player" object
	SEE_object* Player = (SEE_object *)SEE_malloc(m_interpr, sizeof(SEE_native));

	SEE_native_init((SEE_native *)Player, m_interpr,
	                &player_constructor_class, m_interpr->Object_prototype);

	PUT_OBJECT(Player, str_prototype, m_Player_prototype)

	// now we can add the globals
	PUT_OBJECT(m_interpr->Global, str_Player,    Player);

	m_player = Player_new();
	PUT_OBJECT(m_interpr->Global, str_player,    m_player);
}


SEE_object* SjSee::Player_new()
{
	player_object* obj = alloc_player_object(m_interpr);

	SEE_native_init(&obj->m_native, m_interpr,
	                &player_inst_class, m_Player_prototype);

	return (SEE_object *)obj;
}


static player_object* get_player_object(SEE_interpreter* interpr, SEE_object* o)
{
	if( o->objectclass != &player_inst_class )
		SEE_error_throw(interpr, interpr->TypeError, NULL);

	return (player_object *)o;
}


void SjSee::Player_receiveMsg(int msg)
{
	if( m_interprInitialized )
	{
		if( msg == IDMODMSG_TRACK_ON_AIR_CHANGED )
		{
			SeeCallCallback(m_interpr, ((player_object*)m_player)->m_onTrackChange, m_player);
		}
	}
}


void SjSee::Player_onPlaybackDone(const wxString& url__)
{
	SjGcLocker gclocker;

	SEE_value*      arg0 = NULL;
	SjSee*          curSee = s_first;
	player_object*  curPlayer;
	while( curSee )
	{
		if( curSee->m_interprInitialized )
		{
			curPlayer = (player_object*)curSee->m_player;
			if( curPlayer )
			{
				if( curPlayer->m_onPlaybackDone )
				{
					if( arg0 == NULL )
					{
						arg0 = (SEE_value*)SEE_malloc(curSee->m_interpr, sizeof(SEE_value));
						SEE_string* s = WxStringToSeeString(curSee->m_interpr, url__);
						SEE_SET_STRING(arg0, s);
					}

					SeeCallCallback(curSee->m_interpr, curPlayer->m_onPlaybackDone, (SEE_object*)curPlayer, arg0);
				}
			}
		}

		curSee = curSee->m_next;
	}
}
