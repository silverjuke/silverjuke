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
 * File:    gcalloc.cpp
 * Authors: Björn Petersen
 * Purpose: Garbage Collection - see gcalloc.h for further information
 *
 *******************************************************************************
 *
 * Testing scripts:
 * var i = 0; program.setTimeout(function(){print('block'+i++)}, 10, true)
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/gcalloc.h>
#if SJ_USE_SCRIPTS


#define PRINT_CLEANUP_INFO 0


#define GcADR uintptr_t


struct GcBlock // total size of the structure is 8*4 = 32 - this is a fine size!
{
	GcBlock*        next;
	uint32_t        size;
	int32_t         flags;

	int32_t         references; // 0   : known to have no references, please free!
	                            // 1   : may have references through other blocks, please check!
	                            // >=2 : known to have static references, do not free, use as anchor

	int32_t         oneRefValidated; // used only during cleanup
	SJ_GC_PROC      finalizeFn;
	void*           finalizeUserData1;
	void*           finalizeUserData2;
};

static GcBlock*     s_gc_firstBlock     = NULL;
SjGcSystem          g_gc_system = { 0, 0, 0, 0, 0, 0, 0 };

#define CHECK_BLOCK(b)  \
    wxASSERT( (b->flags&SJ_GC_FLAGS_MAGIC_MASK) == SJ_GC_FLAGS_MAGIC ); \
    wxASSERT(  b->references >= 0 ); \
    wxASSERT(  b->oneRefValidated == 0 || b->oneRefValidated == 1 );


/*******************************************************************************
 * Alloc and optional Free
 ******************************************************************************/


void* SjGcAlloc(unsigned long size, long flags,
                SJ_GC_PROC finalizeFn, void* userData1, void* userData2)
{
	wxASSERT( g_gc_system.locked >= 1 );
	wxASSERT( wxThread::IsMain() );

	// runtime check of our assumptions
	wxASSERT( sizeof(GcADR) == sizeof(void*) );
	wxASSERT( (sizeof(void*)==4 && sizeof(GcBlock)==32) || (sizeof(void*)==8 && sizeof(GcBlock)==48) ); // not really important, but interesting
	wxASSERT( sizeof(char) == 1 );

	// allocate the memory
	if( size <= 0 )
		return NULL;

	GcBlock* ptr = (GcBlock*)malloc(sizeof(GcBlock) + size);
	if( ptr == NULL )
		return NULL;

	// set up block and add the block to the list of blocks
	ptr->size               = size;
	ptr->flags              = flags | SJ_GC_FLAGS_MAGIC;
	ptr->references         = flags&SJ_GC_ALLOC_STATIC? 2 : 1;
	ptr->oneRefValidated    = 1;
	ptr->finalizeFn         = finalizeFn;
	ptr->finalizeUserData1  = userData1;
	ptr->finalizeUserData2  = userData2;

	CHECK_BLOCK( ptr );

	// add to list of blocks
	ptr->next               = s_gc_firstBlock;
	s_gc_firstBlock         = ptr;

	// some statistics
	g_gc_system.curSize += size;
	if( g_gc_system.curSize > g_gc_system.peakSize )
		g_gc_system.peakSize = g_gc_system.curSize;

	g_gc_system.curBlockCount ++;

	g_gc_system.sizeChangeSinceLastCleanup += size;

	// zero memory?
	if( flags&SJ_GC_ZERO )
	{
		memset(((char*)ptr) + sizeof(GcBlock), 0, size);
	}

	// done
	return ((char*)ptr) + sizeof(GcBlock);
}


#if 0 // currently not needed
void SjGcRef(void* ptr)
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( g_gc_system.locked >= 1 );

	// get the block pointer
	GcBlock* block = (GcBlock*) (((GcADR)ptr) - sizeof(GcBlock));

	CHECK_BLOCK( block );

	wxASSERT( block->references >= 1 );

	block->references ++;
}
#endif


void SjGcUnref(void* ptr)
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( g_gc_system.locked >= 1 );

	// get the block pointer
	GcBlock* block = (GcBlock*) (((GcADR)ptr) - sizeof(GcBlock));

	CHECK_BLOCK( block );

	// just decrease the protection counter; if the block is really no
	// longer referenced, it is freed on the next garbage collection run.
	wxASSERT( block->references > 0 );
	if( block->references > 0 )
	{
		block->references--;

		if( block->flags&SJ_GC_ALLOC_STATIC )
		{
			wxASSERT( block->references >= 1 );
			if( block->references == 1 )
				block->references--; // speed up cleanup
		}

		if( block->references == 0 )
		{
			g_gc_system.sizeChangeSinceLastCleanup += block->size;
		}
	}
}


bool SjGcIsValidPtr(void* ptr)
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( g_gc_system.locked >= 1 );
	wxASSERT( ptr );

	GcBlock* block = (GcBlock*) (((GcADR)ptr) - sizeof(GcBlock));
	CHECK_BLOCK( block );
	return true;
}


#if 0
bool SjGcHasStaticRef(void* ptr)
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( g_gc_system.locked >= 1 );
	wxASSERT( ptr );

	GcBlock* block = (GcBlock*) (((GcADR)ptr) - sizeof(GcBlock));
	CHECK_BLOCK( block );
	return (block->references>=2);
}
#endif


/*******************************************************************************
 * Shutdown - free all
 ******************************************************************************/

void SjGcShutdown()
{
	GcBlock *cur = s_gc_firstBlock, *next;
	while( cur )
	{
		CHECK_BLOCK( cur );

		if( cur->finalizeFn )
			cur->finalizeFn(cur->finalizeUserData1, (char*)cur+sizeof(GcBlock), cur->finalizeUserData2);

		next = cur->next;

		#ifdef __WXDEBUG__

			// if we're not in debug mode and we're in shutdown,
			// just let the OS free the memory, there is no advantage
			// to do it here (but some disadvantages,  eg. speed)

			free(cur);

		#endif

		cur = next;
	}

	s_gc_firstBlock = NULL;
	g_gc_system.curSize = 0;
}


/*******************************************************************************
 * Cleanup
 ******************************************************************************/


static GcADR s_gc_minAdr, s_gc_maxAdr, *s_gc_allAdr = NULL, s_gc_allAdrCount;
#if PRINT_CLEANUP_INFO
static long s_infoAssumedPointers, s_infoPointersFollowed;
#endif


static void SjGcSweep(GcBlock* block)
{
	// mark block as checked
	wxASSERT( block->oneRefValidated == 0 );
	CHECK_BLOCK( block );
	block->oneRefValidated = 1;

	// if the block is allocated to contain strings only,
	// there is no need to check the content for pointers
	if( (block->flags&SJ_GC_ALLOC_STRING) )
		return;

	GcADR   *dataPtr, *dataEnd, adr;
	GcBlock *cur2;
	int     left, right, mid;


	// go through all possible addresses of the block
	dataPtr = (GcADR*) ( ((char*)block)     + sizeof(GcBlock)   );
	dataEnd = (GcADR*) ( ((char*)dataPtr)   + block->size       );
	while( dataPtr < dataEnd )
	{
		adr = *dataPtr;

		if(  adr >= s_gc_minAdr
		 &&  adr <= s_gc_maxAdr )
		{
			#if PRINT_CLEANUP_INFO
				s_infoAssumedPointers ++;
			#endif

			// binary search for adr in s_gc_allAdr
			left = 0; right = s_gc_allAdrCount - 1;
			while( left <= right )
			{
				mid = left + ((right - left) / 2);
				if( s_gc_allAdr[mid] > adr )
				{
					right = mid - 1;
				}
				else if( s_gc_allAdr[mid] < adr )
				{
					left = mid + 1;
				}
				else
				{
					cur2 = (GcBlock*)( s_gc_allAdr[mid]-sizeof(GcBlock) );
					CHECK_BLOCK( cur2 );
					if( !cur2->oneRefValidated
					        /*&& cur2->references -- no needed, only blocks with referenced are added to s_gc_allAdr[]*/ )
					{
						// pointer found!
						#if PRINT_CLEANUP_INFO
							s_infoPointersFollowed ++;
						#endif
						SjGcSweep(cur2);
					}
					break;
				}
			}

			// sequential search
			#if 0
				cur2 = s_gc_firstBlock;
				while( cur2 )
				{
					CHECK_BLOCK( cur2 );

					if( adr == ((GcADR)cur2) + sizeof(GcBlock) )
					{
						if( !cur2->oneRefValidated
								&&  cur2->references/*in contras to the binary search above, this comparison is needed here!*/ )
						{
							// pointer found!
							#if PRINT_CLEANUP_INFO
								s_infoPointersFollowed ++;
							#endif
							SjGcSweep(cur2);
						}
						break;
					}
					cur2 = cur2->next;
				}
			#endif
		}

		// check the next possible pointer ("++" goes to the next pointer (normally +4 bytes as GcADR is just "unsigned long")
		dataPtr ++;
	}
}


static int compareAdr(const void *arg1, const void *arg2)
{
	// CAVE: do not use arg1-arg2 as the pointer may be 64bit and the return value may be 32bit only...
	if( *((GcADR*)arg1) < *((GcADR*)arg2) ) return -1;
	if( *((GcADR*)arg1) > *((GcADR*)arg2) ) return  1;
	return 0;
}


void SjGcDoCleanup()
{
	// check if garbage collection is possible at the moment - if not, it is delayed
	// until g_gc_system.locked is 0 again.
	wxASSERT( wxThread::IsMain() );
	if( g_gc_system.locked > 0 )
	{
		g_gc_system.forceGc = 1;
		return;
	}
	g_gc_system.forceGc = 0;


	// any blocks?
	wxASSERT( g_gc_system.curBlockCount >= 0 );
	if( g_gc_system.curBlockCount == 0 )
		return;


	// collect information of all blocks, mark all blocks as unused
	GcBlock* curBlock;
	#if PRINT_CLEANUP_INFO
		long infoCleanupStartTimestamp = SjTools::GetMsTicks();
	#endif
	{
		GcADR adr;

		s_gc_minAdr         = 0; // initialize, we may get a s_gc_allAdrCount==0 after this part and the initialization with s_gc_allAdr[0] below will not be done
		s_gc_maxAdr         = 0; //     - " -

		wxASSERT( s_gc_allAdr == NULL );
		s_gc_allAdr         = (GcADR*)malloc(g_gc_system.curBlockCount*sizeof(GcADR));
		s_gc_allAdrCount    = 0;

		curBlock = s_gc_firstBlock;
		while( curBlock )
		{
			CHECK_BLOCK( curBlock );

			curBlock->oneRefValidated = 0;

			if( curBlock->references )
			{
				adr = ((GcADR)curBlock) + sizeof(GcBlock);

				if( g_debug )
				{
					// only set for an additional check at (X), always set after qsort()
					if( s_gc_minAdr == 0 || adr < s_gc_minAdr ) { s_gc_minAdr = adr; }
					if( s_gc_maxAdr == 0 || adr > s_gc_maxAdr ) { s_gc_maxAdr = adr; }
				}

				s_gc_allAdr[s_gc_allAdrCount++] = adr;
			}

			// next block
			curBlock = curBlock->next;
		}
	}

	if( s_gc_allAdrCount )
	{
		qsort(s_gc_allAdr, s_gc_allAdrCount, sizeof(GcADR), compareAdr);

		if( g_debug )
		{
			wxASSERT( s_gc_minAdr == s_gc_allAdr[0] ); // (X) see comment above
			wxASSERT( s_gc_maxAdr == s_gc_allAdr[s_gc_allAdrCount-1] );
		}

		s_gc_minAdr = s_gc_allAdr[0];
		s_gc_maxAdr = s_gc_allAdr[s_gc_allAdrCount-1];
	}

	// start scanning with the only blocks used directly
	// (there may be zero used blocks, however, continue anyway as some blocks may be freed)
	#if PRINT_CLEANUP_INFO
		s_infoAssumedPointers = 0;
		s_infoPointersFollowed = 0;
	#endif
	curBlock = s_gc_firstBlock;
	while( curBlock )
	{
		CHECK_BLOCK( curBlock );

		if(  curBlock->references >= 2 /* static? */
		 && !curBlock->oneRefValidated /* the flag may change for any block in SjGcSweep() */)
		{
			SjGcSweep(curBlock);
		}

		curBlock = curBlock->next;
	}

	// free the memory that is not used
	wxASSERT( s_gc_allAdr );
	free(s_gc_allAdr);
	s_gc_allAdr = NULL;

	unsigned long   infoBlocksFreed = 0;
	unsigned long   infoBytesFreed = 0;
	unsigned long   infoOldSize = g_gc_system.curSize;
	#if PRINT_CLEANUP_INFO
		unsigned long infoOldBlockCount = g_gc_system.curBlockCount;
	#endif
	{
		GcBlock *toDel, *prevBlock = NULL;

		curBlock = s_gc_firstBlock;
		while( curBlock )
		{
			CHECK_BLOCK( curBlock );

			if( curBlock->oneRefValidated )
			{
				prevBlock = curBlock;
				curBlock = curBlock->next;
			}
			else
			{
				if( prevBlock )
					prevBlock->next = curBlock->next;
				else
					s_gc_firstBlock = curBlock->next;

				// remember the block to free
				toDel = curBlock;

				// point to next
				curBlock = curBlock->next;

				// free the block
				infoBlocksFreed++;
				infoBytesFreed += toDel->size;

				wxASSERT( g_gc_system.curSize >= toDel->size );
				wxASSERT( g_gc_system.curBlockCount > 0 );
				wxASSERT( toDel->references <= 1 );

				g_gc_system.curSize -= toDel->size;
				g_gc_system.curBlockCount--;

				if( toDel->finalizeFn )
					toDel->finalizeFn(toDel->finalizeUserData1, (char*)toDel+sizeof(GcBlock), toDel->finalizeUserData2);

				free(toDel);
			}
		}
	}

	// integry check
	if( g_debug )
	{
			unsigned long cnt = 0, cntBytes = 0;
			GcBlock* iter = s_gc_firstBlock;
			while( iter )
			{
				cnt++;
				cntBytes += iter->size;
				iter = iter->next;
			}
			wxASSERT( cnt == g_gc_system.curBlockCount );
			wxASSERT( cntBytes == g_gc_system.curSize );
	}

	// done
	wxASSERT( infoOldSize-g_gc_system.curSize == infoBytesFreed );

	g_gc_system.sizeChangeSinceLastCleanup = 0;
	g_gc_system.lastCleanupTimestamp = SjTools::GetMsTicks();

	#if PRINT_CLEANUP_INFO
		wxLogDebug( "%i ms needed to free %iK of %iK (%i of %i blocks, %i/%i/%i possible/assumed/followed pointers) [gc]",
					(int)(g_gc_system.lastCleanupTimestamp-infoCleanupStartTimestamp),
					(int)(infoBytesFreed/1024),
					(int)(infoOldSize/1024),
					(int)infoBlocksFreed,
					(int)infoOldBlockCount,
					(int)(infoOldSize/sizeof(GcADR)), (int)s_infoAssumedPointers, (int)s_infoPointersFollowed
				  );
	#endif
}


bool SjGcNeedsCleanup()
{
	if( g_gc_system.forceGc )
		return true;

	if( g_gc_system.sizeChangeSinceLastCleanup > SJ_GC_CLEANUP_BYTES
	        && SjTools::GetMsTicks() > g_gc_system.lastCleanupTimestamp+SJ_GC_CLEANUP_MS )
	{
		return true;
	}

	return false;
}


#endif // SJ_USE_SCRIPTS


