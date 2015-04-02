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
 * File:    cvs_tokenizer.h
 * Authors: Björn Petersen
 * Purpose: Handling (large) CVS-files
 *
 ******************************************************************************/


#ifndef __SJ_CSV_TOKENIZER__
#define __SJ_CSV_TOKENIZER__


class SjCsvTokenizer
{
public:
	SjCsvTokenizer          (const wxString& fieldsTerminatedBy,
	                         const wxString& fieldsEnclosedBy,
	                         const wxString& escapeBy);
	~SjCsvTokenizer         ();
	void            AddData                 (const unsigned char*, long bytes);

	wxArrayString*  GetRecord               ();

private:
	unsigned char*  m_data;
	long            m_dataTotalBytes;
	long            m_dataUsedBytes;
#define         DATA_INCR_BYTES 0x20000L // 128K

	unsigned char   m_sep;
	unsigned char   m_quote;
	unsigned char   m_esc;
	unsigned char   m_trim1;
	unsigned char   m_trim2;

	long            m_x;

	wxArrayString   m_record;

};


#endif // __SJ_CSV_TOKENIZER__
