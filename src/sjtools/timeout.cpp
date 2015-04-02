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
 * File:    timeout.cpp
 * Authors: Björn Petersen
 * Purpose: Smart timeout handling
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/timeout.h>


unsigned long SjTimestampDiff(unsigned long first, unsigned long afterFirst)
{
	if( afterFirst >= first )
	{
		return afterFirst - first;
	}
	else
	{
		// overflow - this will happen once a month
		return afterFirst + (0xFFFFFFFFL-first);
	}
}


/*******************************************************************************
 * SjTimeoutWatcher
 ******************************************************************************/


void SjTimeoutWatcher::SetTimeout(long timeoutMs)
{
	m_timeoutSet = TRUE;
	m_timeoutMs = timeoutMs;
	Reset();
}


void SjTimeoutWatcher::Reset()
{
	m_lastInputMs = SjTools::GetMsTicks();
}


bool SjTimeoutWatcher::IsExpired()
{
	// any timeout set?
	if( !m_timeoutSet )
	{
		return FALSE;
	}

	// get the number of ms since the last user input
	unsigned long thisInputMs = SjTools::GetMsTicks();
	unsigned long msSinceLastInput = ::SjTimestampDiff(m_lastInputMs, thisInputMs);

	// is expired?
	return (msSinceLastInput>=m_timeoutMs);
}


/*******************************************************************************
 * SjUptimeWatcher Class
 ******************************************************************************/


void SjUptimeWatcher::StartWatching()
{
	m_start = wxDateTime::Now();
	m_started = TRUE;
}


void SjUptimeWatcher::StopWatching()
{
	wxASSERT( !m_reg.IsEmpty() );

	// save new data by calling GetSeconds()...
	g_tools->m_config->Write(m_reg, GetSeconds());

	// ...BEFORE settings m_started to false
	// (GetSeconds() relies on m_started)
	m_started = FALSE;
}


long SjUptimeWatcher::GetSeconds()
{
	// get saved data
	wxASSERT( !m_reg.IsEmpty() );
	long uptimeSeconds = g_tools->m_config->Read(m_reg, 0L);

	// add the current running time
	if( m_started )
	{
		wxTimeSpan runningTime = wxDateTime::Now().Subtract(m_start);
		uptimeSeconds += runningTime.GetSeconds().GetLo();
	}

	// done
	return uptimeSeconds;
}
