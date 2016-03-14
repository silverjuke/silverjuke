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
 * File:    vis_overlay.h
 * Authors: Björn Petersen
 * Purpose: Visualisation overlay with cover and title
 *
 ******************************************************************************/


#ifndef __SJ_VIS_OVERLAY_H__
#define __SJ_VIS_OVERLAY_H__


// The purpose of SjVisOverlay is to provide some text laying over any video screen
// the video screen may be wxDC, OpenGL or even DirectX; this makes drawing in the
// same window complicated or impossible.
// The alternative is to use different windows that _look_ as if they belong to the
// main window. There are two problems:
// - the overlay can only consist out of rectangle areas - this is solved by us by using a layout that fits this needs
// - _how_ can we let window lay over the video screen - there are three approached that are unfortunately all
//   needed by the different OS: 0=child=MSW, 1=sibling=GTK, 2=popup=MAC
// - the popup approach is the most generic and works on all OS, however, it has the backdraft that the popup
//   would lie _above_ any focused foreground window - our settings dialog, but even above other programs.
//   so we close the overlay if our window loses the focus.
#ifndef SJ_OVERLAY_BASE
	#if defined(__WXMAC__)
		#define SJ_OVERLAY_BASE 2 // 0=child, 1=sibling, 2=popup
	#elif defined(__WXMSW__)
		#define SJ_OVERLAY_BASE 0
	#else
		#define SJ_OVERLAY_BASE 1 // fine eg. for GTK
	#endif
#endif


class SjVoWindow;
class SjVoActionTimer;
#if SJ_OVERLAY_BASE==2
class SjVoPopupTimer;
#endif


class SjVisOverlay
{
public:
	                 SjVisOverlay         ();
	                 ~SjVisOverlay        ();

	void             TrackOnAirChanged    (wxWindow* parentForOverlay, long delayMs=-1);
	void			 UpdatePositions      ();
	void             CloseOverlay         ();

	bool             m_freshOpened;

private:
	void			 CalcPositions        (wxRect& coverRect, wxRect& artistNameRect, wxRect& trackNameRect);
	void             RequireCover         ();
	void             AllocateOverlay      (wxWindow* parentForOverlay);
	void             ShowOverlay          ();
	void             SetTimer             (long ms, int type);

	SjVoWindow*      GetCoverWindow       () { return (SjVoWindow*)wxWindow::FindWindowById(IDW_OVERLAY_COVER); }
	SjVoWindow*      GetArtistNameWindow  () { return (SjVoWindow*)wxWindow::FindWindowById(IDW_OVERLAY_ARTIST_NAME); }
	SjVoWindow*      GetTrackNameWindow   () { return (SjVoWindow*)wxWindow::FindWindowById(IDW_OVERLAY_TRACK_NAME); }

	wxString         m_leadArtistName;
	wxString         m_trackName;
	wxString         m_coverUrl;

	SjVoActionTimer* m_actionTimer;
	long             m_delayMs;

	#if SJ_OVERLAY_BASE==2
	SjVoPopupTimer*  m_popupTimer;
	friend class     SjVoPopupTimer;
	#endif

	friend class     SjVoActionTimer;

};


#endif // __SJ_VIS_OVERLAY_H__
