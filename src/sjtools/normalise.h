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
 * File:    normalise.h
 * Authors: Björn Petersen
 * Purpose: Normalizing Strings for easy sorting and searching
 *
 ******************************************************************************/


#ifndef __SJ_NORMALISE_H__
#define __SJ_NORMALISE_H__


// String stuff
#define     SJ_NUM_SORTABLE  1 // do not change this value as it is also used by the SQL function sortable()
#define     SJ_NUM_TO_END    2 //           - " -
#define     SJ_EMPTY_TO_END  4 //           - " -
#define     SJ_OMIT_ARTIST   8 //           - " -                   // VC2008: why was this missing?
#define     SJ_OMIT_ALBUM   16 //           - " -                   // VC2008: why was this missing?
#define     SJ_NUM_TO_END_ZZZZZZZZZZ wxT("zzzzzzzzzz")
wxString    SjNormaliseString   (const wxString&, long flags);


#endif // __SJ_NORMALISE_H__
