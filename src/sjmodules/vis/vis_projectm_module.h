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
 * File:    vis_projectm_module.h
 * Authors: Björn Petersen
 * Purpose: ProjectM visualisation module header
 *
 ******************************************************************************/


#ifndef __SJ_VIS_PROJECTM_MODULE_H__
#define __SJ_VIS_PROJECTM_MODULE_H__
#if SJ_USE_PROJECTM


class SjProjectmGlCanvas;
class wxGLContext;
class projectM;


class SjProjectmModule : public SjVisRendererModule
{
public:
	                SjProjectmModule    (SjInterfaceBase* interf);
	                ~SjProjectmModule   ();

	bool            Start               (SjVisWindow*);
	void            Stop                ();

	void            ReceiveMsg          (int);
	void            AddMenuOptions      (SjMenu&);
	void            OnMenuOption        (int);
	void            PleaseUpdateSize    (SjVisWindow*);
	void            AddVisData          (const float* buffer, long bytes);

private:
	SjProjectmGlCanvas* m_glCanvas;
	wxGLContext*    m_glContext;
	projectM*       m_projectMobj;
	SjVisWindow*    m_impl;

	friend class    SjProjectmGlCanvas;
};


#endif // SJ_USE_PROJECTM
#endif // __SJ_VIS_PROJECTM_MODULE_H__
