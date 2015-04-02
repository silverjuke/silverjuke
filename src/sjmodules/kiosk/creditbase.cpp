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
 * File:    creditbase.cpp
 * Authors: Björn Petersen
 * Purpose: Kiosk credit handling
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/kiosk/creditbase.h>
#include <sjmodules/kiosk/kiosk.h>
#include <wx/cmdline.h>


/*******************************************************************************
 * SjCreditBase - constructor / destructor
 ******************************************************************************/


SjCreditBase::SjCreditBase()
{
	// m_flags should be set first to avoid recursion
	wxASSERT( SJ_CREDITBASEF_DEFAULT != -1 );
	m_flags = g_tools->m_config->Read(wxT("kiosk/creditbasef"), SJ_CREDITBASEF_DEFAULT);
	if( m_flags == -1 )
	{
		m_flags = SJ_CREDITBASEF_DEFAULT;
	}

	// init credit count by the registry/ini
	m_creditCount = 0;
	if( m_flags & SJ_CREDITBASEF_SAVE_CREDITS )
	{
		m_creditCount = g_tools->m_config->Read(wxT("kiosk/creditsaved"), 0L);
		if( m_creditCount < 0 )
		{
			m_creditCount = 0;
		}
	}

	// init the credit by the command line
	if( m_flags & SJ_CREDITBASEF_LISTEN_TO_DDE )
	{
		long addCredits;
		if( SjMainApp::s_cmdLine->Found(wxT("setcredit"), &addCredits) )
		{
			m_creditCount = addCredits;
			SaveCreditCount();
		}

		if( SjMainApp::s_cmdLine->Found(wxT("addcredit"), &addCredits) )
		{
			m_creditCount += addCredits;
			SaveCreditCount();
		}
	}

	UpdateSkinTarget();
}


SjCreditBase::~SjCreditBase()
{
}


/*******************************************************************************
 * Get credit information
 ******************************************************************************/


bool SjCreditBase::CheckAndDecreaseCredit(long requestedTitleCount)
{
	// credit system enabled?
	if( !IsCreditSystemEnabled() )
	{
		return TRUE; // yes, there are enough credits as the credit system is disabled
	}

	// enough credit?
	if( m_creditCount >= requestedTitleCount )
	{
		m_creditCount -= requestedTitleCount;

		SaveCreditCount();

		UpdateSkinTarget();

		return TRUE; // yes, there are enough credits
	}

	// not enough credits :-/
	g_mainFrame->SetDisplayMsg(_("No credit."));
	return FALSE;
}


void SjCreditBase::AddCredit(long titleCount, long addFlags)
{
	// adding credit allowed?
	if(  (addFlags & SJ_ADDCREDIT_FROM_DDE)
	        && !(m_flags & SJ_CREDITBASEF_LISTEN_TO_DDE) )
	{
		wxLogWarning(wxT("DDE, command line and shortcut credit adding disabled.")/*n/t*/);
		wxLogWarning(wxT("Cannot add credit.")/*n/t*/);
		return;
	}

	// change credit count
	long oldCreditCount = m_creditCount;

	if( addFlags & SJ_ADDCREDIT_SET_TO_NULL_BEFORE_ADD )
	{
		m_creditCount = 0;
	}

	if( titleCount > 0 )
	{
		m_creditCount += titleCount;
	}

	// credit count really changed?
	if( oldCreditCount != m_creditCount )
	{
		if( ShowCreditInDisplay() )
			g_mainFrame->SetDisplayMsg( wxString::Format(_("Credit: %i"), m_creditCount));

		SaveCreditCount();

		g_kioskModule->UpdateCreditSpinCtrl();

		UpdateSkinTarget();
	}
}


void SjCreditBase::UpdateSkinTarget() const
{
	if( g_mainFrame )
	{
		SjSkinValue v;
		v.value  = SJ_VFLAG_CENTER;
		v.string = wxT("oo");
		if( IsCreditSystemEnabled() )
		{
			v.string.Printf(wxT("%i"), (int)m_creditCount);
		}

		g_mainFrame->SetSkinTargetValue(IDT_CURR_CREDIT, v);
	}
}


/*******************************************************************************
 * Settings
 ******************************************************************************/


void SjCreditBase::SetFlag(long flag, bool set)
{
	if( SjTools::SetFlagRetChanged(m_flags, flag, set) )
	{
		g_tools->m_config->Write(wxT("kiosk/creditbasef"), m_flags);

		if( flag & SJ_CREDITBASEF_ENABLED )
		{
			UpdateSkinTarget();
		}
	}
}


void SjCreditBase::SaveCreditCount()
{
	g_tools->m_config->Write(wxT("kiosk/creditsaved"), m_creditCount);
}


bool SjCreditBase::ShowCreditInDisplay() const
{
	if( g_mainFrame )
		return (g_mainFrame->GetTargetProp(IDT_CURR_CREDIT)&SJ_SKIN_PROP_HIDE_CREDIT_IN_DISPLAY)==0;
	return true;
}


bool SjCreditBase::IsCreditSystemEnabled()
{
	/* This function has an add. static implementation as the state may be important,
	 * before g_kioskModule is set up, see http://www.silverjuke.net/forum/topic-2943.html
	 */

	static long s_flags = -1;

	if( g_kioskModule )
	{
		s_flags = g_kioskModule->m_creditBase.GetFlags();
	}
	else if( s_flags == -1 )
	{
		s_flags = g_tools->m_config->Read(wxT("kiosk/creditbasef"), SJ_CREDITBASEF_DEFAULT);
	}

	return (s_flags & SJ_CREDITBASEF_ENABLED) != 0;
}

