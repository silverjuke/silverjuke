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
 * File:    cdg_reader.cpp
 * Authors: Björn Petersen
 * Purpose: High-level CDG handling for MP3+CDG
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_cdg_reader.h>
#include <sjmodules/vis/vis_cdg_raw.h>


SjCdgReader::SjCdgReader(wxFSFile* fsFile)
{
	m_fsFile                = fsFile;

	m_eof                   = false;
	m_eokMs                 = 0; // unknown

	m_fsFileData            = (unsigned char*)malloc(SJ_CDG_REALLOC_EVERY);
	m_fsFileDataAllocated   = SJ_CDG_REALLOC_EVERY;
	m_fsFileDataLoaded      = 0;
	m_fsFileDataPos         = 0;
}


SjCdgReader::~SjCdgReader()
{
	free(m_fsFileData);
	delete m_fsFile;
}


long SjCdgReader::Ms2Bytes(long ms)
{
#define BYTES_PER_MS 7.2F // (75*96)/1000
	long bytes = (long) ((float)ms * BYTES_PER_MS);
	long rest = (bytes % 96);
	if( rest )
	{
		bytes -= rest;
	}
	return bytes;
}


bool SjCdgReader::SetPosition(long ms)
{
	if( m_eokMs != 0 && ms > m_eokMs )
		return false;

	long newFilePos = Ms2Bytes(ms);

	if( newFilePos > m_fsFileDataPos )
	{
		// read all data up to the new position
		if( newFilePos > m_fsFileDataLoaded && !m_eof )
		{
			wxInputStream* stream = m_fsFile->GetStream();
			size_t fileBytes = stream->GetSize(); // may be 0 for unknown!

			long tenSecondsBytes = Ms2Bytes(10000);
			long bytesToRead = (newFilePos - m_fsFileDataLoaded) + tenSecondsBytes;
			if( fileBytes > 0 )
			{
				// do not leave a little part alone at the end of the file
				// (otherwise we cannot determinate the end)
				long bytesStillLeft = (fileBytes-m_fsFileDataLoaded)-bytesToRead;
				if( bytesStillLeft < tenSecondsBytes )
				{
					bytesToRead += bytesStillLeft + tenSecondsBytes/*force EOF*/;
				}
			}


			long freeBytes = m_fsFileDataAllocated-m_fsFileDataLoaded;
			if( bytesToRead > freeBytes )
			{
				long bytesToAllocate = bytesToRead + SJ_CDG_REALLOC_EVERY;

				m_fsFileData = (unsigned char*)realloc(m_fsFileData, m_fsFileDataAllocated+bytesToAllocate);

				m_fsFileDataAllocated += bytesToAllocate;
			}

			stream->Read(m_fsFileData+m_fsFileDataLoaded, bytesToRead);

			long bytesReallyRead = stream->LastRead();

			m_fsFileDataLoaded += bytesReallyRead;

			if( bytesReallyRead < bytesToRead || stream->Eof() )
			{
				m_eof = true;

				// now calculate the read end of the data
				long testDataPos = m_fsFileDataPos;
				long endTestDataPos = m_fsFileDataLoaded;
				long lastDataPos = testDataPos;
				while( testDataPos < endTestDataPos )
				{
					if( SjCdgRaw::IsDataSubcode((SjCdgSubcode*) (m_fsFileData+testDataPos)) )
						lastDataPos = testDataPos;
					testDataPos += sizeof(SjCdgSubcode);
				}
#ifdef __WXDEBUG__
				SjCdgRaw::IsDataSubcode((SjCdgSubcode*) (m_fsFileData+lastDataPos));
#endif
				m_eokMs = (long) (((float)lastDataPos) / BYTES_PER_MS) + 800;/*a little time for the last data*/;
			}
		}

		if( newFilePos > m_fsFileDataLoaded )
		{
			newFilePos = m_fsFileDataLoaded;
		}
	}
	else if( newFilePos < m_fsFileDataPos )
	{
		// execute the file from the beginning
		m_screen.Rewind();
		m_fsFileDataPos = 0;
	}
	else
	{
		// same position, nothing to do
		return true;
	}

	wxASSERT( (m_fsFileDataPos % sizeof(SjCdgSubcode)) == 0 );
	wxASSERT( (newFilePos % sizeof(SjCdgSubcode)) == 0 );

	while( m_fsFileDataPos < newFilePos )
	{
		m_screen.AddSubcode((SjCdgSubcode*) (m_fsFileData+m_fsFileDataPos));

		m_fsFileDataPos += sizeof(SjCdgSubcode);
	}

	return true;
}


void SjCdgReader::Render(wxDC& dc, SjVisBg& bg, bool pleaseUpdateAll)
{
	wxSize      dcSize = dc.GetSize(); if( dcSize.y <= 0 ) return;
	float       dcAspect = (float)dcSize.x / (float)dcSize.y;

	float       orgAspect = (float)CDG_IMAGE_W / (float)CDG_IMAGE_H;

	long scaledX, scaledY, scaledW, scaledH;
	if( dcAspect > orgAspect )
	{
		// scale by height
		scaledH = long( (float)dcSize.y * KARAOKE_RENDER_SHRINK );
		scaledW = long( (float)scaledH * orgAspect );
	}
	else
	{
		// scale by width
		scaledW = long( (float)dcSize.x * KARAOKE_RENDER_SHRINK );
		scaledH = long( (float)scaledW / orgAspect );
	}

	// make sure, the width and the height are a multiple of PARTS_PER_ROW/PARTS_PER_COL
	long scaledPartW = scaledW / PARTS_PER_ROW;
	long scaledPartH = scaledH / PARTS_PER_COL;
	if( scaledPartW <= 0 || scaledPartH <= 0 ) return;
	scaledW = scaledPartW * PARTS_PER_ROW;
	scaledH = scaledPartH * PARTS_PER_COL;

	// find center
	scaledX = (dcSize.x - scaledW) / 2;
	scaledY = (dcSize.y - scaledH) / 2;

	// create an image for the parts
	wxImage partImage((int)scaledPartW, (int)scaledPartH, false);
	unsigned char* destData = partImage.GetData();

	// go through all parts
	const unsigned char*        palette = m_screen.m_colourTable;
	const unsigned char*        colour;
	static const unsigned char  blackPixel[3] = { 0, 0, 0 };

	long transpColourIndex = m_screen.m_presetColourIndex;
	if( transpColourIndex >= 0 )
	{
		colour = &palette[transpColourIndex*3];
		if( colour[0]+colour[1]+colour[2] >= 255 )
			transpColourIndex = -1;
	}

	const unsigned char* currSrcLinePtr, *currBgLinePtr, *prevSrcLinePtr;
	unsigned char*       currDestLinePtr;
	long partX, partY, partIndex = 0, destX, destY, srcY, srcX, colourIndex;
	bool hasData = m_screen.m_hasData;
	for( partY = 0; partY < PARTS_PER_COL; partY++ )
	{
		for( partX = 0; partX < PARTS_PER_ROW; partX++ )
		{
			if( m_screen.m_updatedParts & (1<<partIndex)
			        || pleaseUpdateAll )
			{
				// create the part
				for( destY = 0; destY < scaledPartH; destY++ )
				{
					/* calculate current source y position (rounded integer position) */
					srcY = PART_HEIGHT * destY / scaledPartH;

					/* buffer current input scanline (saves us some multiplications) */
					currSrcLinePtr = &m_screen.m_screen[
					                     (partY*PART_HEIGHT+srcY+12/*12=border top*/)*CDG_SCREEN_W
					                     +   partX*PART_WIDTH+6/*6=border left*/ ];

					prevSrcLinePtr = (currSrcLinePtr > m_screen.m_screen)? currSrcLinePtr-CDG_SCREEN_W : NULL;

					/* buffer current output scanline (saves us some multiplications) */
					currDestLinePtr = &destData[ scaledPartW * destY * 3 ];

					currBgLinePtr = bg.GetBackgroundBits(
					                    scaledX+partX*scaledPartW, scaledY+partY*scaledPartH+destY, scaledPartW);

					for( destX = 0; destX < scaledPartW; destX++ )
					{
						/* calculate current source x position (rounded integer position) */
						srcX = PART_WIDTH * destX / scaledPartW;

						colourIndex = currSrcLinePtr [ srcX ];
						if( !hasData )
						{
							colour = &currBgLinePtr[ destX * 3 ];
						}
						else if( colourIndex == transpColourIndex )
						{
							colour = &currBgLinePtr[ destX * 3 ];
							if( prevSrcLinePtr && srcX > 0
							        && prevSrcLinePtr[srcX-1] != transpColourIndex )
							{
								colour = blackPixel;
							}
						}
						else
						{
							colour = &palette[ colourIndex * 3 ];
						}

						*currDestLinePtr++ = *colour++;
						*currDestLinePtr++ = *colour++;
						*currDestLinePtr++ = *colour;
					}
				}

				wxBitmap scaledBitmap(partImage);
				dc.DrawBitmap(scaledBitmap, scaledX+partX*scaledPartW, scaledY+partY*scaledPartH, true);
			}

			partIndex++;
		}
	}

	m_screen.m_updatedParts = 0;
}

