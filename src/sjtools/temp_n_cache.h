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
 * File:    temp_n_cache.h
 * Authors: Björn Petersen
 * Purpose: Handling temporary and cached files
 *
 ******************************************************************************/


#ifndef __SJ_TEMP_N_CACHE_H__
#define __SJ_TEMP_N_CACHE_H__


class SjArrayLittleOption;


class SjDirIterator
{
public:
	                SjDirIterator       (const wxString& dir);
	                ~SjDirIterator      ();

	// Iterate() gets the next file in the given director;
	// returns TRUE if there is a next file, otherwise FALSE
	bool            Iterate             ();

	// the following values are set if Iterater() returns TRUE.
	wxString        m_fullPath;
	unsigned long   m_bytes;
	wxDateTime      m_modTime;

private:
	// private stuff
	wxString        m_dir;
	void*           m_handle;
	void*           m_handle2;
};


class SjTempNCache
{
public:
	// Constructor etc.
	//
	// After constructio, Init() must be called by the user which required the
	// configuration object to be set up.
	//
	// Finally, call LastCallOnExit() as late as possible to allow delayed deletion
	// of files that could not be deleted in time.
	                SjTempNCache            ();
	                ~SjTempNCache           ();
	void            Init                    ();
	static void     LastCallOnExit          ();

	// Simply a tool to get a filename that is not used at the moment of
	// calling. The returned filenames do not intersect with the "normal"
	// filenames in the temporary directory, so you can delete the file
	// safely eg. using RemoveFromUnmanagedTemp() or RemoveFromManagedTemp().
	static wxString GetUnusedFileName       (const wxString& dir, const wxString& ext=wxT(""));

	// Unmanaged Temporary Files.
	//
	// Here you'll give the full name of a file and return a path to the
	// file in the temporary directory.
	//
	// The function does not follow any deletion rules, so the caller is responsible for this.
	// Doing so, you should use RemoveFromUnmanagedTemp() as this allowes delayed deletion
	// of files.
	//
	// You may use RemoveFromUnmanagedTemp() also for files not allocated by
	// AddToUnmanagedTemp() - in this case the file is just deleted (delayed).
	wxString        AddToUnmanagedTemp      (const wxString& fileName);
	wxString        LookupUnmanagedTemp     (const wxString& fileName);
	void            RemoveFromUnmanagedTemp (const wxString& tempPath);

	// Managed Temporary Files.
	//
	// Just give an extension and a deletion method and you get a new name
	// of a file in the temporary direcory.
	//
	// The file is automatically removed following the given deletion method,
	// however, you can also delete if manually using RemoveFromManagedTemp()
	//
	// You may use AddToManagedTemp() also for files not allocated by
	// AddToUnmanagedTemp() - in this case the file is just deleted (delayed).
	#define         SJ_TEMP_NO_PROTECT          FALSE
	#define         SJ_TEMP_PROTECT_TIL_EXIT    TRUE
	wxString        AddToManagedTemp        (const wxString& ext, bool  protectTilExit);
	void            RemoveFromManagedTemp   (const wxString& tempPath);



	// The Cache.
	//
	// Files in the cache are identified by their original names (or any other unique string
	// id) and are deleted as needed (eg. when the cache gets full). AddToCache() returns
	// the filename and the full path in the cache; the file is not created, but the returned filename
	// will be unique for the original file.  Moreover, the original extension
	// will be preserved.
	//
	// The returned cache file may already exist, in this case the caller
	// will normally overwrite it as he has a newer version.
	wxString        AddToCache          (const wxString& orgFileName) { return Add(orgFileName, SJ_TEMP_NO_PROTECT, TRUE); }
	wxString        AddToCache          (const wxString& orgFileName,
	                                     const void*     orgData,
	                                     long            orgBytes);
	wxString        LookupCache         (const wxString& orgFileName,
	                                     bool            lock=TRUE/*internal use only!*/);
	void            RemoveFromCache     (const wxString& orgFileName, bool neverCacheAgain=FALSE);



	// Configuration etc.
	wxString        GetTempDir          ();
	static wxString GetDefaultTempDir   ();
	void            SetTempDir          (const wxString&);

    #define         SJ_TEMP_MIN_MB      64L
    #define         SJ_TEMP_DEF_MB      512L
	long            GetMaxMB            ();
	void            SetMaxMB            (long);

	void            GetLittleOptions    (SjArrayLittleOption&);

	void            CleanupOldFiles     (bool force=FALSE);

private:
	// Private stuff
    #define             SJ_TEMP_PREFIX  wxT("sj-")

	wxString            m_tempDir;

	static SjSLHash*    s_removeOnLastCall;
	wxCriticalSection   m_removeOnLastCallCritical;

	wxSqltDb*           m_sqlDb;
	SjSLHash            m_sqlDbNeverCache;
	SjLLHash            m_sqlDbProtectedIds;
	wxCriticalSection   m_sqlDbCritical;

	bool                InitDb          (bool lock=TRUE);
	static wxSqltDb*    CreateOrOpenDb  (const wxString& tempDir);

	wxString            Add             (const wxString& orgFileName,
	                                     bool            protectTilExit,
	                                     bool            lock);

	long                m_filesAdded;
	long                m_filesAddedMax;
    #define             SJ_MIN_FILESADDEDMAX 16
    #define             SJ_MAX_FILESADDEDMAX 256
};


#endif //  __SJ_TEMP_N_CACHE_H__
