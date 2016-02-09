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
 * File:    vis_frame.cpp
 * Authors: Björn Petersen
 * Purpose: Frame for floating/ownscreen visualizations
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_frame.h>
#include <sjmodules/vis/vis_module.h>


SjVisFrame::SjVisFrame( wxWindow* parent, const wxPoint& pos, const wxSize& size, long style)
	: wxFrame(parent, wxID_ANY, _("Video screen"), pos, size,
				wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE | style)
{
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));
}


SjVisFrame::~SjVisFrame()
{
}


BEGIN_EVENT_TABLE(SjVisFrame, wxFrame)
	//EVT_MENU_RANGE    (IDO_VIS_FIRST__, IDO_VIS_LAST__, SjVisFrame::OnFwdToMainFrame )
	EVT_MENU_RANGE      (IDT_FIRST, IDT_LAST,   SjVisFrame::OnFwdToMainFrame    )
	EVT_CLOSE           (                       SjVisFrame::OnCloseWindow       )
END_EVENT_TABLE()


void SjVisFrame::OnFwdToMainFrame(wxCommandEvent& e)
{
	g_mainFrame->GetEventHandler()->ProcessEvent(e);
}


void SjVisFrame::OnCloseWindow(wxCloseEvent&)
{
	g_visModule->StopVis();
}




