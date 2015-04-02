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
 * File:    levensthein.h
 * Authors: Björn Petersen
 * Purpose: calculate the levensthein difference
 *
 ******************************************************************************/


#ifndef _LEVENSTHEIN_H_
#define _LEVENSTHEIN_H_


#ifdef __cplusplus
extern "C"
{
#endif


int levensthein(const unsigned char *s1,
                const unsigned char *s2,
                int limit
                /*int cost_ins, int cost_rep, int cost_del*/ );


#ifdef __cplusplus
};
#endif


#endif /* _LEVENSTHEIN_H_ */
