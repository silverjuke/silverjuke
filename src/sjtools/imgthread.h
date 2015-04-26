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
 * File:    imgthread.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke image thread'n'cache
 *
 ******************************************************************************/


#ifndef __SJ_IMGTHREAD_H__
#define __SJ_IMGTHREAD_H__


#include <sjtools/imgop.h>


class SjImgThreadObj
{
public:
	SjImgThreadObj      (wxEvtHandler* evtHandler, const wxString& url, unsigned long timestamp, const SjImgOp& op);
	wxString        m_url;
	wxString        m_errors;


	bool            Ok() const          { return m_image.Ok(); }
	int             GetWidth() const    { return m_image.GetWidth(); }
	int             GetHeight() const   { return m_image.GetHeight(); }

	wxBitmap*       CreateBitmap() const; // the returned bitmap is owned by the caller, may be NULL

private:

	wxEvtHandler*   m_evtHandler;
	SjImgOp         m_op;
	unsigned long   m_timestamp;
	long            m_usage,
	                m_processing;
	bool            m_loadedFromDiskCache;

	wxImage         m_image;

	long            GetBytes            () const;

	wxString        GetDiskCacheName    () const;
	bool            LoadFromFile        ();
	bool            LoadFromDiskCache   ();
	void            SaveToDiskCache     ();

	friend class    SjImgThread;
};


WX_DECLARE_LIST(SjImgThreadObj, SjImgThreadObjList);


extern const wxEventType wxEVT_IMAGE_THERE;

#define EVT_IMAGE_THERE(fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_IMAGE_THERE, -1, -1, \
        (wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)&fn, \
        (wxObject *) NULL \
    ),


class SjImageThereEvent : public wxEvent
{
public:
	SjImageThereEvent   (SjImgThreadObj* obj) : wxEvent(0, wxEVT_IMAGE_THERE) { m_obj = obj; }
	SjImgThreadObj* GetObj              () const { return m_obj; }

	wxEvent*        Clone               () const { return new SjImageThereEvent(m_obj); }

private:
	SjImgThreadObj* m_obj;
};


class SjImgThread : public wxThread
{
public:
	/* Construct an SjImgThread object. The thread is started implicit at the
	 * first call of RequireStart().
	 */
	SjImgThread         ();
	~SjImgThread        ();

	/* Start requiring images.  The event handler is used to identify waiting
	 * images from the last "require round" that are still waiting but may
	 * no longer be needed.  These images are remove from the waiting list.
	 */
	void            RequireStart        (wxEvtHandler*);

	/* Require the given image and optionally scale it or use other filters.
	 * The function returns an image object directly if the object is
	 * already in cache.  However, this returned object may have another
	 * size or other filters.  If the required image is not in cache, NULL
	 * is returned.
	 * If NULL is returned or if the returned image has the wrong size or the
	 * wrong filter, the caller will be informed by a wxEVT_IMAGE_THERE event
	 * when the image is "ready".
	 * Requiring the same image with different event handlers is okay and results in
	 * multiple events!
	 */
	SjImgThreadObj* RequireImage        (wxEvtHandler*, const wxString& url, const SjImgOp&);

	/* Stop requiring images.  After this function is called, SjImgThread
	 * will start rendering the required images and inform the caller by
	 * the given callbacks.
	 */
	void            RequireEnd          (wxEvtHandler*);

	/* This function MUST be called if an event handler is about to be deleted
	 */
	void            RequireKill         (wxEvtHandler*);

	/* Release the usage of a given cache image. This function must _always_ be
	 * called if SjWinImgRequire returns an object or if you receive an object
	 * by a callback / an event.
	 */
	void            ReleaseImage        (wxEvtHandler*, SjImgThreadObj*, bool removeFromRamCache=false);

	/* Are there any waiting images?
	 */
	bool            HasWaitingImages    ();

	/* Cache settings
	 */
	long            GetMaxRamCacheBytes () const { return m_ramCacheMaxBytes; }
	long            GetRamCacheUsage    (int what /*'i'mages or '%'*/);
	int             GetDiskCache        () const { return m_directDiskCache? 2 : (m_useDiskCache? 1 : 0); }
	bool            GetRegardTimestamp  () const { return m_regardTimestamp; }

	void            SetCacheSettings    (long bytes, int useDiskCache, bool regardTimestamp);

	/* Shutdown() waits for the rendering thread to terminate - this should be called _before_ the object is destroyed;
	 * we provide an extra function for this purpose as this allows you to keep the pointer alive longer.
	 */
	void            Shutdown            ();

private:
	/* thread execution starts here,
	 * this function should NEVER be called directly!
	 */
	void*           Entry               ();

	/* needed members
	 */
	wxCriticalSection
	m_critsect;
	wxMutex         m_mutex;
	wxCondition*    m_condition; // use this to check if all other objets are okay
	SjImgThreadObjList
	m_anchorWaiting;
	SjImgThreadObjList
	m_anchorCached;

	long            m_ramCacheMaxBytes;
	long            m_ramCacheUsedBytes;
	unsigned long   m_imagesRendered;
	bool            m_useDiskCache;
	bool            m_directDiskCache;
	bool            m_regardTimestamp;
	bool            m_shutdownCalled;

	void            CleanupRamCache     (long cacheLeaveBytes);
	SjImgThreadObjList::Node*
	SearchImg           (SjImgThreadObjList& list, const wxString& url, unsigned long timestamp, const SjImgOp&, bool matchOp);

	bool            m_triedCreation;
	bool            m_doExitThread;

	wxArrayString   m_errorousUrls;

	void            SaveSettings        ();

	#ifdef SG_DEBUG_IMGTHREAD
	void           LogDebug            ();
	#endif
};


#endif /* __SJ_IMGTHREAD_H__ */
