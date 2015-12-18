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
 * File:    imgthread.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke image thread'n'cache
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjtools/console.h>

#include <wx/listimpl.cpp> // sic!
WX_DEFINE_LIST(SjImgThreadObjList);


const wxEventType wxEVT_IMAGE_THERE = wxNewEventType();


/*******************************************************************************
 * SjImgThread - Thread Entry Point
 ******************************************************************************/


void* SjImgThread::Entry()
{
	int                         getFirst = -1;
	SjImgThreadObjList::Node*   node;
	SjImgThreadObj*             obj = NULL;
	bool                        objOk;
	long                        imagesThisRound;

	wxLog::SetThreadActiveTarget(SjLogGui::s_this);

	/* Start endless loop
	 *
	 * NB: the image thread _MUST NOT_ use GUI routines, eg. wxBitmap does not work on GTK.
	 * For performance reasons, in older versions of Silverjuke we had a switch to allow
	 * drawing from theads on OS that allow this.  Today, 2015, this is no longer needed and
	 * it is much better to have the same code running on all OS.
	 */
	while( 1 )
	{
		if( getFirst != -1 )        /* for the very first loop, do not wait for our event, */
		{	/* it may already be signaled */
			m_mutex.Lock();
			m_condition->Wait();
			m_mutex.Unlock();
		}

		/* loop through waiting images, exit by break
		 */
		imagesThisRound = 0;
		while( 1 )
		{
			/* get the first or last waiting image;
			 * we're not always using the first image to get a nicer redraw effect
			 * (not from left to right but from the sides to the middle)
			 */

			{
				wxCriticalSectionLocker locker(m_critsect);

				if( m_doExitThread )
				{
					/* the main thread signaled us to stop
					 */
					return NULL;
				}

				if( getFirst )
				{
					node = m_anchorWaiting.GetFirst();
					getFirst = 0;
				}
				else
				{
					node = m_anchorWaiting.GetLast();
					getFirst = 1;
				}

				if( node )
				{
					obj = node->GetData();
					wxASSERT(obj);
					wxASSERT(obj->m_processing==0);

					obj->m_processing = 1;
				}
				else
				{
					if( imagesThisRound == 0 && m_ramCacheUsedBytes > m_ramCacheMaxBytes )
					{
						CleanupRamCache(m_ramCacheMaxBytes);
					}
					break; /* keep on waiting */
				}
			}

			/* waiting image found: process this image
			 */
			{
				SjLogString logString;
				bool logError = false;

				objOk = FALSE;
				if( m_errorousUrls.Index(obj->m_url) == wxNOT_FOUND )
				{
					if( m_useDiskCache && !m_directDiskCache )
					{
						objOk = obj->LoadFromDiskCache();
					}

					if( !objOk )
					{
						objOk = obj->LoadFromFile();
					}

					if( !objOk )
					{
						m_errorousUrls.Add(obj->m_url);
						logError = true;
					}
				}
				else
				{
					logError = true;
				}

				if( logError )
				{
					wxString filename = obj->m_url;
					if( filename.StartsWith("file:") ) { filename = wxFileSystem::URLToFileName(filename).GetFullPath(); }
					wxLogError(_("Cannot open \"%s\"."), filename.c_str());
				}

				obj->m_errors = logString.GetAndClearErrors();
			}

			/* move the image from the waiting to the cached list
			 */

			{
				wxCriticalSectionLocker locker(m_critsect);

				m_anchorWaiting.DeleteNode(node);
				m_anchorCached.Append(obj);

				obj->m_processing = 0;

				m_imagesRendered++;
				if( objOk )
				{
					m_ramCacheUsedBytes += obj->GetBytes();
				}

				/* inform all callbacks waiting that the new image is there.
				 * this must NOT be done from inside the critical section as
				 * the receiver will call ReleaseImage()!
				 *
				 * we also send this message if the image could not be loaded,
				 * so the caller can draw sth. other.
				 */
				wxASSERT( obj->m_usage == 0 );
				if( obj->m_evtHandler )
				{
					obj->m_usage = 1;
					SjImageThereEvent evt(obj);
					obj->m_evtHandler->AddPendingEvent(evt);
				}
			}

			imagesThisRound++;
		}
	}

	/* this should be never reached
	 */
	return 0;
}


/*******************************************************************************
 * SjImgThread - Requiring images
 ******************************************************************************/


void SjImgThread::RequireKill(wxEvtHandler* evtHandler)
{
	// this sequence removes any pending objects
	RequireStart(evtHandler);
	RequireEnd(evtHandler);

	// remove the event handler from all rest objects
	{
		wxCriticalSectionLocker locker(m_critsect);
		SjImgThreadObjList::Node    *node;
		SjImgThreadObj              *obj;
		bool                        cacheChecked = false;

		node = m_anchorWaiting.GetFirst();
		while( node )
		{
			obj = node->GetData();

			if( obj->m_evtHandler == evtHandler )
			{
				obj->m_evtHandler = NULL;
			}

			// next
			node = node->GetNext();
			if( node == NULL && !cacheChecked )
			{
				node = m_anchorCached.GetFirst();
				cacheChecked = true;
			}
		}
	}
}


void SjImgThread::RequireStart(wxEvtHandler* evtHandler)
{
	if( m_shutdownCalled )
		return;

	SjImgThreadObjList::Node    *node, *nodeNext;
	SjImgThreadObj              *obj;

	/* init the thread if not yet done
	 */
	if( m_condition == NULL )
	{
		if( m_triedCreation )
		{
			return; // failed on previous call, there's nothing we can do
		}
		else
		{
			m_triedCreation = TRUE;

			wxBusyCursor wait;

			if( (m_condition = new wxCondition(m_mutex)) != NULL
			        &&  m_condition->IsOk() != FALSE )
			{
				Create();
				Run();
			}
			else
			{
				if( m_condition )
				{
					delete m_condition;
					m_condition = NULL;
				}

				wxLogError(wxT("Cannot create SjImgThread."));
				return;
			}
		}
	}

	/* remove all references of waiting images for the given event handler
	 */
	{
		wxCriticalSectionLocker locker(m_critsect);

		node = m_anchorWaiting.GetFirst();
		while( node )
		{
			nodeNext = node->GetNext();

			obj = node->GetData();
			wxASSERT(obj);
			wxASSERT(obj->m_usage==0);

			if( !obj->m_processing && obj->m_evtHandler == evtHandler )
			{
				delete obj;
				m_anchorWaiting.DeleteNode(node);
			}

			node = nodeNext;
		}
	}
}


SjImgThreadObj* SjImgThread::RequireImage(wxEvtHandler*     evtHandler,
        const wxString&   url,
        const SjImgOp&    op  )
{
	if( m_shutdownCalled )
		return NULL;

	if( m_condition )
	{
		wxCriticalSectionLocker     locker(m_critsect);
		SjImgThreadObjList::Node    *nodeInCache, *nodeWaiting;
		SjImgThreadObj              *objInCache, *newObj;
		unsigned long               timestamp = 0;

		/* find out the timestamp of the URL
		 */
		if( m_regardTimestamp )
		{
			// find out the timestamp using ::wxFileModificationTime() or wxFileSystem::GetModificationTime(),
			// the first is _much_ faster but does not work with archives etc.
			wxDateTime dt;
			time_t tt = ::wxFileModificationTime(url);
			if( tt != 0 )
			{
				dt = tt;
			}
			else
			{
				wxFileSystem fileSystem;
				wxFSFile* fsFile = fileSystem.OpenFile(url);
				if( fsFile )
				{
					dt = fsFile->GetModificationTime();
					delete fsFile;
				}
			}

			if( dt.IsValid() )
				timestamp = dt.GetAsDOS();
		}

		/* image in RAM cache?
		 */
		nodeInCache = SearchImg(m_anchorCached, url, timestamp, op, FALSE/*no need to be exact on operation match*/);

		/* if we found an image, move it to the end of the list
		 * this is needed, as we clean up the list from the beginning
		 */
		if( nodeInCache )
		{
			objInCache = nodeInCache->GetData();
			wxASSERT( objInCache );

			objInCache->m_usage++;
			m_anchorCached.DeleteNode(nodeInCache);
			m_anchorCached.Append(objInCache);
		}
		else
		{
			objInCache = NULL;
		}

		/* do we have to signal the thread to create a new image?
		 */
		if( nodeInCache == NULL
		        || objInCache->m_op != op )
		{
			if( (nodeWaiting=SearchImg(m_anchorWaiting, url, timestamp, op, TRUE/*exact operation match*/))
			        && (nodeWaiting->GetData())->m_evtHandler==evtHandler )
			{
				/* there is already an image waiting with the same edit operations,
				 * and the same event handler. TODO: support multiple event handlers
				 * for waiting images by storing them in a list in SjImgThreadObj
				 * (however, currently this is not needed by Silverjuke as we have
				 * only one browser window)
				 */

				;
			}
			else
			{
				/* add image
				 */
				newObj = new SjImgThreadObj(evtHandler, url, timestamp, op);

				if( m_useDiskCache
				        && m_directDiskCache
				        && newObj->LoadFromDiskCache() )
				{
					/* could load the image from the disk cache -- add to cached objects
					 */
					newObj->m_usage = 1;
					m_ramCacheUsedBytes += newObj->GetBytes();
					m_anchorCached.Append(newObj);
					objInCache = newObj;
				}
				else if( newObj->m_url.StartsWith(wxT("cover:"))
				         && newObj->LoadFromFile() )
				{
					/* the image is a stub-cover and the thread cannot draw -- create the
					 * image now and add it to the cached objeccts
					 */
					newObj->m_usage = 1;
					m_ramCacheUsedBytes += newObj->GetBytes();
					m_anchorCached.Append(newObj);
					objInCache = newObj;
				}
				else
				{
					/* add the image to the list of waiting images
					 */
					m_anchorWaiting.Append(newObj);
				}
			}
		}

		return objInCache; /* may be null */
	}
	else
	{
		return NULL; /* thread not started successfully */
	}
}


void SjImgThread::RequireEnd(wxEvtHandler* evtHandler)
{
	if( m_shutdownCalled )
		return;

	/* Signal the image thread to wake up and to render the
	 * new images.
	 * If the thread is already waked up, nothing will happen.
	 */
	if( m_condition )
	{
		m_condition->Signal();
	}
}


void SjImgThread::ReleaseImage(wxEvtHandler* evtHandler, SjImgThreadObj* obj, bool removeFromRamCache)
{
	if( obj ) // it is okay to give us a NULL pointer (makes some things easier), but we do nothing in this case
	{
		wxCriticalSectionLocker locker(m_critsect);

		wxASSERT(obj);

		obj->m_usage--;

		if( removeFromRamCache && obj->m_usage == 0 )
		{
			SjImgThreadObjList::Node* node = m_anchorCached.Find(obj);
			if( node )
			{
				m_ramCacheUsedBytes -= obj->GetBytes();

				delete obj;
				m_anchorCached.DeleteNode(node);
			}
		}
	}
}


/*******************************************************************************
 * SjImgThread - Misc..
 ******************************************************************************/


bool SjImgThread::HasWaitingImages()
{
	if( m_condition )
	{
		wxCriticalSectionLocker locker(m_critsect);

		return m_anchorWaiting.IsEmpty()? FALSE : TRUE;
	}
	else
	{
		return FALSE;
	}
}


void SjImgThread::CleanupRamCache(long cacheLeaveBytes)
{
	/* this function must be called from within m_critsect allocated!
	 */
	SjImgThreadObjList::Node *node = m_anchorCached.GetFirst(), *nextNode;
	while( node )
	{
		nextNode = node->GetNext();

		SjImgThreadObj* obj = node->GetData();

		wxASSERT(obj);

		if( obj->m_usage == 0 )
		{
			// save object to disk cache?
			if( m_useDiskCache )
			{
				obj->SaveToDiskCache();
			}

			// delete object
			m_ramCacheUsedBytes -= obj->GetBytes();

			delete obj;
			m_anchorCached.DeleteNode(node);

			if( m_ramCacheUsedBytes <= cacheLeaveBytes )
			{
				return; /* done */
			}
		}

		node = nextNode;
	}
}


SjImgThreadObjList::Node* SjImgThread::SearchImg( SjImgThreadObjList&   anchor,
        const wxString&       url,
        unsigned long         timestamp,
        const SjImgOp&        op,
        bool                  matchOp )
{
	/* this function must be called from within m_critsect allocated!
	 */
	SjImgThreadObjList::Node *node, *otherNode = 0;
	SjImgThreadObj* img;

	node = anchor.GetFirst();
	while( node )
	{
		img = node->GetData();
		wxASSERT(img);

		if( url == img->m_url && timestamp == img->m_timestamp )
		{
			if( op == img->m_op )
			{
				return node;
			}
			else if( !matchOp )
			{
				otherNode = node;
			}
		}

		node = node->GetNext();
	}

	return otherNode; /* may be null */
}


/*******************************************************************************
 * SjImgThread - Constructor
 ******************************************************************************/


SjImgThread::SjImgThread()
	: wxThread(wxTHREAD_JOINABLE)
{
	#define SJ_IMGTHREAD_USE_DISK_CACHE     0x00010000L
	#define SJ_IMGTHREAD_DIRECT_DISK_CACHE  0x00020000L
	#define SJ_IMGTHREAD_REGARD_TIMESTAMP   0x00040000L
	long settings = g_tools->m_config->Read(wxT("main/imgThread"), 2L);

	m_ramCacheMaxBytes      = (settings&0xFFFF) * SJ_ONE_MB;
	m_useDiskCache          = (settings&SJ_IMGTHREAD_USE_DISK_CACHE)!=0;
	m_directDiskCache       = (settings&SJ_IMGTHREAD_DIRECT_DISK_CACHE)!=0;
	m_regardTimestamp       = (settings&SJ_IMGTHREAD_REGARD_TIMESTAMP)!=0;
	m_ramCacheUsedBytes     = 0;
	m_imagesRendered        = 0;
	m_condition             = NULL;
	m_triedCreation         = false;
	m_doExitThread          = false;
	m_shutdownCalled        = false;

}


void SjImgThread::SaveSettings()
{
	long settings = m_ramCacheMaxBytes/SJ_ONE_MB;
	if( m_useDiskCache )    settings |= SJ_IMGTHREAD_USE_DISK_CACHE;
	if( m_directDiskCache ) settings |= SJ_IMGTHREAD_DIRECT_DISK_CACHE;
	if( m_regardTimestamp ) settings |= SJ_IMGTHREAD_REGARD_TIMESTAMP;
	g_tools->m_config->Write(wxT("main/imgThread"), settings);
}


void SjImgThread::SetCacheSettings(long bytes, int useDiskCache, bool regardTimestamp)
{
	if( m_ramCacheMaxBytes != bytes )
	{
		m_ramCacheMaxBytes = bytes;
		if( m_condition )
		{
			wxCriticalSectionLocker locker(m_critsect);
			CleanupRamCache(m_ramCacheMaxBytes);
		}
	}

	m_useDiskCache      =   (useDiskCache!=0);
	m_directDiskCache   =   (useDiskCache==2);
	m_regardTimestamp   =   regardTimestamp;

	SaveSettings();
}


long SjImgThread::GetRamCacheUsage(int what /*'i'mages or '%'*/)
{
	long ret = 0;
	if( m_condition )
	{
		wxCriticalSectionLocker locker(m_critsect);
		if( what == 'i' )
		{
			ret = m_anchorCached.GetCount();
		}
		else
		{
			long cacheMaxKb = m_ramCacheMaxBytes / 1024;
			long cacheUsedKb = m_ramCacheUsedBytes / 1024;

			ret = cacheMaxKb>0? ((cacheUsedKb*100)/cacheMaxKb) : 100;

			if( ret>100 ) ret = 100;
		}
	}
	return ret;
}


void SjImgThread::Shutdown()
{
	wxASSERT( !m_shutdownCalled );

	m_shutdownCalled = true;

	if( IsRunning() && m_condition )
	{
		m_critsect.Enter();
		m_doExitThread = TRUE;
		m_critsect.Leave();
		m_condition->Signal();

		#if 0
			Wait();
		#else
			unsigned long startWaiting = SjTools::GetMsTicks();
			while( 1 )
			{
				if( !IsRunning() )
				{
					break;
				}

				if( SjTools::GetMsTicks()-startWaiting > 4000 )
				{
					if( g_debug )
					{
						::wxMessageBox(wxT("I'm waiting since 4 seconds for the image thread to terminate ... what's on? I will exit now."),
								   SJ_PROGRAM_NAME);
					}
					break;
				}

				wxThread::Sleep(50);
			}
		#endif
	}
}


SjImgThread::~SjImgThread()
{
	wxASSERT( m_shutdownCalled );

	SjImgThreadObjList::Node* objnode = m_anchorWaiting.GetFirst();
	while( objnode )
	{
		delete objnode->GetData();
		delete objnode;
		objnode = m_anchorWaiting.GetFirst();
	}

	objnode = m_anchorCached.GetFirst();
	while( objnode )
	{
		if( m_useDiskCache )
		{
			objnode->GetData()->SaveToDiskCache();
		}

		delete objnode->GetData();
		delete objnode;
		objnode = m_anchorCached.GetFirst();
	}

	if( m_condition )
	{
		delete m_condition;
	}
}


/*******************************************************************************
 * SjImgThreadObj - Constructor
 ******************************************************************************/


SjImgThreadObj::SjImgThreadObj(wxEvtHandler* evtHandler, const wxString& url, unsigned long timestamp, const SjImgOp& op)
{
	m_evtHandler            = evtHandler;
	m_url                   = url;
	m_timestamp             = timestamp;
	m_op                    = op;
	m_usage                 = 0;
	m_processing            = 0;
	m_loadedFromDiskCache   = FALSE;
}


wxBitmap* SjImgThreadObj::CreateBitmap() const
{
	// the returned bitmap is owned by the caller
	// (otherwise we have to delete it our destructor which is usually called from a non-main thread -
	// and non-main threads are not allowed to use GUI operations (deleting bitmaps is also a GUI operation as eg. some IDs and handles may be locked)

	wxASSERT( wxThread::IsMain() ); // non-main threads may not draw nor use GUI operations!
	if( m_image.IsOk() )
	{
		wxBitmap* bitmap = new wxBitmap(m_image);
		if( bitmap )
		{
			if( bitmap->IsOk() )
			{
				return bitmap; // success
			}

			delete bitmap;
		}
	}

	return NULL; // error
}


wxString SjImgThreadObj::GetDiskCacheName() const
{
	return wxString::Format(wxT("%s/%lu/%s.bmp"), m_url.c_str(), m_timestamp, m_op.GetAsString().c_str());
}


long SjImgThreadObj::GetBytes() const
{
	if( m_image.IsOk() )
	{
		return m_image.GetWidth() * m_image.GetHeight() * 3;
	}

	return 0;
}


bool SjImgThreadObj::LoadFromFile()
{
	bool objOk = FALSE;

	if( m_url.StartsWith(wxT("cover:")) )
	{
		int w = m_op.m_resizeW;
		if( w < 10 ) w = 200;
		if( m_op.m_flags&SJ_IMGOP_SMOOTH ) w *= 2;

		if( wxThread::IsMain() )
		{
			m_image = SjImgOp::CreateDummyCover(m_url, w);
		}
		else
		{
			; // normally, the image is drawn directly in the main thread in SjImgThread::RequireImage();
			// however, if this failes, we may get here - and cannot do anything more than informing
			// the main thread the image cannot be loaded
		}
	}
	else
	{
		wxFileSystem fileSystem;
		wxFSFile* fsFile = fileSystem.OpenFile(m_url, wxFS_READ|wxFS_SEEKABLE); // i think, seeking is needed by at least one format ...
		if( fsFile )
		{
			m_image.LoadFile(*(fsFile->GetStream()), wxBITMAP_TYPE_ANY);
			delete fsFile;
		}
	}

	if( m_image.IsOk() )
	{
		objOk = TRUE;
		m_op.Do(m_image);
	}

	return objOk;
}


bool SjImgThreadObj::LoadFromDiskCache()
{
	wxString diskCacheName = g_tools->m_cache.LookupCache(GetDiskCacheName());

	if( !diskCacheName.IsEmpty() )
	{
		m_loadedFromDiskCache = m_image.LoadFile(diskCacheName, wxBITMAP_TYPE_BMP);
	}

	return m_loadedFromDiskCache;
}


void SjImgThreadObj::SaveToDiskCache()
{
	if( !m_loadedFromDiskCache )
	{
		wxString tempName = g_tools->m_cache.AddToCache(GetDiskCacheName());
		if( !tempName.IsEmpty() )
		{
			if( m_image.IsOk() )
			{
				if( !m_image.SaveFile(tempName, wxBITMAP_TYPE_BMP) )
				{
					g_tools->m_cache.RemoveFromCache(GetDiskCacheName());
				}
			}
		}
	}
}

