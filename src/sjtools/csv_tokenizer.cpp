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
 * File:    cvs_tokenizer.cpp
 * Authors: Björn Petersen
 * Purpose: Handling (large) CVS-files
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/csv_tokenizer.h>


SjCsvTokenizer::SjCsvTokenizer(const wxString& fieldsTerminatedBy,
                               const wxString& fieldsEnclosedBy,
                               const wxString& escapeBy)
{
	m_data = (unsigned char*)malloc(DATA_INCR_BYTES);
	m_dataTotalBytes = DATA_INCR_BYTES;
	m_dataUsedBytes = 0;

	m_sep   = fieldsTerminatedBy.IsEmpty()  ? ','   : fieldsTerminatedBy[0];
	m_quote = fieldsEnclosedBy.IsEmpty()    ? '\"'  : fieldsEnclosedBy[0];
	m_esc   = escapeBy.IsEmpty()            ? '\"'  : escapeBy[0];

	m_x = 0;
}


SjCsvTokenizer::~SjCsvTokenizer()
{
	if( m_data )
	{
		free(m_data);
	}
}


void SjCsvTokenizer::AddData(const unsigned char* dataToAdd, long bytesToAdd)
{
	if( bytesToAdd > 0 )
	{
		if( bytesToAdd > (m_dataTotalBytes-m_dataUsedBytes) )
		{
			// not enough bytes available ...

			if( m_x > 0 )
			{
				// ... skip already processed data at the beginning of the buffer

				wxASSERT( m_dataUsedBytes >= m_x );

				if( m_dataUsedBytes > m_x )
				{
					memmove(m_data, m_data+m_x, m_dataUsedBytes-m_x);
				}

				m_dataUsedBytes -= m_x;
				m_x = 0;
			}

			if( bytesToAdd > (m_dataTotalBytes-m_dataUsedBytes) )
			{
				// if there are still to few bytes available, we have to reallocate the buffer

				m_dataTotalBytes += DATA_INCR_BYTES+bytesToAdd;
				m_data = (unsigned char*)realloc(m_data, m_dataTotalBytes);
			}
		}

		memcpy(m_data+m_dataUsedBytes, dataToAdd, bytesToAdd);
		m_dataUsedBytes += bytesToAdd;
	}
}


wxArrayString* SjCsvTokenizer::GetRecord()
{
	unsigned char*  data = m_data;
	long            x = m_x;
	wxString        token;

	m_record.Clear();

	while( x < m_dataUsedBytes )
	{
		if( data[x] == m_sep )
		{
			// token end found, add token (even if empty)
			m_record.Add(token);
			token.Empty();
			x++;
		}
		else if( data[x] == m_quote && token.IsEmpty() )
		{
			// read quoted token
			x++;
			while( x < m_dataUsedBytes )
			{
				if( data[x] == m_esc )
				{
					x++;
					if( x >= m_dataUsedBytes )
					{
						return NULL; // record not yet finished
					}

					if( m_esc != m_quote
					        || data[x] == m_quote )
					{
						token.Append(data[x]);
						x++;
					}
					else
					{
						break; // end of string
					}
				}
				else if( data[x] == m_quote )
				{
					x++;
					break; // end of string
				}
				else
				{
					token.Append(data[x]);
					x++;
				}
			}
		}
		else if( data[x] == '\n' || data[x] == '\r' )
		{
			// end of line found, add record (if not empty)
			if( !token.IsEmpty() )
			{
				m_record.Add(token);
				token.Empty();
			}
			x++;

			// any record?
			if( m_record.GetCount() )
			{
				m_x = x;
				return &m_record; // done so far
			}
		}
		else
		{
			// normal character, add to token
			token.Append(data[x]);
			x++;
		}
	}

	return NULL;
}
