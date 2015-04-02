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
 * File:    gcalloc.h
 * Authors: Björn Petersen
 * Purpose: Garbage Collection
 *
 *******************************************************************************
 *
 * We use "mark and sweep", however, our simple garbage collector only traces
 * blocks allocated by SjGcAlloc().  So make sure, no global, static or stack
 * pointers reference blocks alloced by SjGcAlloc directly or indirectly --
 * or mark these blocks with SJ_GC_ALLOC_STATIC:
 *
 * Blocks flaged with SJ_GC_ALLOC_STATIC are used as entry points - all memory
 * _not_ referenced by one of these blocks is freed during a SjGcDoCleanup().
 *
 * Finally, you can flag blocks that are known to contain no pointers
 * using the flag SJ_GC_ALLOC_STRING - this will speed up garbage collection
 * as there is no need to trace these blocks.
 *
 * As this is a garbage collection framework, there is no need to free the
 * memory allocated by SjGcAlloc() explicitly.  Just call SjCleanup() from time
 * to time; SjNeedsCleanup() may give you some hints.  If you like to free
 * a block explicitly - this is useful esp. for blocks marked with
 * SJ_GC_ALLOC_STATIC - call SjGcUnref().  Do _not_ use free() for delete for
 * this purpose!
 *
 * Note that this framework is not thread safe! If you need this, you have to
 * enclose the functions yourself eg. using critical sections.
 *
 ******************************************************************************/


#ifndef __SJ_GCALLOC_H__
#define __SJ_GCALLOC_H__

#if SJ_USE_SCRIPTS


// finalize function prototype
typedef void (*SJ_GC_PROC)(void* userData1, void* ptr, void* userData2);



// SjGcAlloc() allocates a block handled by the garbage collector. For the
// flags, see the comments atop of this file.  You can also define a
// "finalize" function that is called just before a block is freed by
// SjCleanup().
#define SJ_GC_ALLOC_STATIC      0x00000001
#define SJ_GC_ALLOC_STRING      0x00000002
#define SJ_GC_ZERO              0x00000004
#define SJ_GC_FLAGS_MAGIC_MASK  0x7FFFFFF0
#define SJ_GC_FLAGS_MAGIC       0x75CD89A0
void*   SjGcAlloc           (unsigned long size, long flags=0,
                             SJ_GC_PROC finalizeFn=0, void* userData1=0, void* userData2=0);


// With SjGcRef() you can bring any block into the "static" state after allocation.
// This may be needed if you have to save pointers that may not be referenced by
// the garbage collection (for SEE, we need this for callback objects).
void    SjGcRef             (void* ptr);


// SjGcUnref() frees a block allocated by SjGcAlloc(), this is esp. needed
// for blocks allocated with the SJ_GC_ALLOC_STATIC flags set (otherwise, these
// blocks cannot be freed ever!).  However, you may call this also for
// other blocks known have no more references.
void    SjGcUnref           (void* ptr);



// Do the garbage collection and free all unused blocks
void    SjGcDoCleanup       ();


// Make some assumtion if a call to SjGcDoCleanup() is useful
bool    SjGcNeedsCleanup    ();



// Conditions for SjGcNeedsCleanup(); the function returns true
// if the last cleanup is older than the given number of seconds AND
// more than the given number of bytes were allocated since the last cleanup.
#define SJ_GC_CLEANUP_MS        30000L  // 30 seconds
#define SJ_GC_CLEANUP_BYTES     262144L // 0.25 MB


// SjGcShutdown() just frees all blocks independingly of their state.
// This function is useful esp. the the debugging version to avoid
// errors of unfreed memory. Finalize functions are not called from SjGcShutdown();
// if this is requred, call SjGcDoCleanup() first.
void    SjGcShutdown        ();



// the following structure contains some useful information
struct SjGcSystem
{
	unsigned long   lastCleanupTimestamp;
	unsigned long   sizeChangeSinceLastCleanup;
	unsigned long   curSize;
	unsigned long   peakSize;
	unsigned long   curBlockCount;
	long            locked;
	long            forceGc;
};
extern SjGcSystem g_gc_system;


// some stuff for debugging checks
#ifdef __WXDEBUG__
bool    SjGcIsValidPtr      (void* ptr);
bool    SjGcHasStaticRef    (void* ptr);
#endif


// helper class to lock the garbage collection.
// if the garbage collection is locked, SjGcDoCleanup() won't do anything; this is most useful
// if some pointers to gcblocks may be stored on the stack.
class SjGcLocker
{
public:
	SjGcLocker()
	{
		wxASSERT( wxThread::IsMain() );
		wxASSERT( g_gc_system.locked >= 0 );
		if( g_gc_system.locked == 0 )
		{
			if( ::SjGcNeedsCleanup() )
				::SjGcDoCleanup();
		}
		g_gc_system.locked++;
	}

	~SjGcLocker()
	{
		wxASSERT( wxThread::IsMain() );
		wxASSERT( g_gc_system.locked >= 1 );
		g_gc_system.locked--;
		if( g_gc_system.locked == 0 && g_gc_system.forceGc )
		{
			::SjGcDoCleanup();
		}
	}
};


#endif // SJ_USE_SCRIPTS

#endif // __SJ_GCALLOC_H__
