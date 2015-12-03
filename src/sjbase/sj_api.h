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
 * File:    sj_api.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke C-Interface for Plugins
 *
 ******************************************************************************/


#ifndef SJ_API_H
#define SJ_API_H
#ifdef __cplusplus
extern "C" {
#endif


/* Includes, Basic types */
#ifdef WIN32
#include <windows.h>
#else
#define CALLBACK
#define LPARAM long
#define UINT unsigned int
#endif


/* Plugin initialization */
typedef LPARAM (CALLBACK SJPROC)(struct SjInterface*, UINT msg,
                                 LPARAM param1, LPARAM param2, LPARAM param3);

typedef struct SjInterface
{
	SJPROC*  CallPlugin;
	SJPROC*  CallMaster;
	LPARAM   rsvd;
	LPARAM   user;
} SjInterface;

SjInterface* SjGetInterface(void);


/* Messages Silverjuke -> Plugin */
#define SJ_PLUGIN_INIT              10000
#define SJ_PLUGIN_EXIT              10001
#define SJ_PLUGIN_CALL              10010


/* Messages Plugin -> Silverjuke */
#define SJ_GET_VERSION              20000
#define SJ_EXECUTE                  20040


#ifdef __cplusplus
}
#endif
#endif /* #ifndef SJ_API_H */

