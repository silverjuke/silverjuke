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
 * File:    cdg_raw.cpp
 * Authors: Björn Petersen
 * Purpose: Low-level CDG handling for MP3+CDG
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_cdg_raw.h>


SjCdgRaw::SjCdgRaw()
{
	Rewind();
}


void SjCdgRaw::Rewind()
{
	memset(m_colourTable, 0, 16 * 3); // all black
	memset(m_screen, 0, CDG_SCREEN_W * CDG_SCREEN_H); // all black

	m_presetColourIndex = -1;
	m_borderColourIndex = -1;
	m_transparentColour = -1;
	m_updatedParts      = ALL_PARTS;
	m_hasData           = false;
}


bool SjCdgRaw::IsDataSubcode(const SjCdgSubcode* subcode)
{
	wxASSERT( sizeof(SjCdgSubcode) == 24 );

	if( (subcode->command&CDG_MASK) == CDG_COMMAND )
	{
		switch( subcode->instruction&CDG_MASK )
		{
			case CDG_INST_TILE_BLOCK:
			case CDG_INST_TILE_BLOCK_XOR:
				return true; // the other commands do not affect the data directly
		}
	}

	return false;
}


void SjCdgRaw::AddSubcode(const SjCdgSubcode* subcode)
{
	wxASSERT( sizeof(SjCdgSubcode) == 24 );

	if( (subcode->command&CDG_MASK) == CDG_COMMAND )
	{
		switch( subcode->instruction&CDG_MASK )
		{
			case CDG_INST_MEMORY_PRESET:
				m_presetColourIndex = subcode->data[0] & 0x0F; // Ignore repeat because this is a reliable data stream
				if( m_borderColourIndex == -1)
					m_borderColourIndex = m_presetColourIndex;
				PresetScreen();
				break;

			case CDG_INST_BORDER_PRESET:
				m_borderColourIndex = subcode->data[0] & 0x0F;
				if( m_presetColourIndex == -1 )
					m_presetColourIndex = m_borderColourIndex;
				PresetScreen();
				break;

			case CDG_INST_TILE_BLOCK:
				TileBlock(subcode, false/*xor*/);
				break;

			case CDG_INST_SCROLL_PRESET:
				Scroll(subcode, false/*scrollRotate*/);
				break;

			case CDG_INST_SCROLL_COPY:
				Scroll(subcode, true/*scrollRotate*/);
				break;

			case CDG_INST_DEF_TRANSP_COL:
				m_transparentColour = subcode->data[0] & 0x0F;
				m_updatedParts = ALL_PARTS;
				break;

			case CDG_INST_LOAD_COL_TBL_0_7:
				LoadColourTable(subcode, 0/*start index*/);
				break;

			case CDG_INST_LOAD_COL_TBL_8_15:
				LoadColourTable(subcode, 8/*start index*/);
				break;

			case CDG_INST_TILE_BLOCK_XOR:
				TileBlock(subcode, true/*xor*/);
				break;

			default:
				wxLogDebug(wxT("CDG: Unknown instruction %i"), (int)(subcode->instruction&CDG_MASK));
				break;
		}
	}
}


void SjCdgRaw::PresetScreen()
{
	memset(m_screen, m_presetColourIndex, CDG_SCREEN_W*CDG_SCREEN_H);
	m_updatedParts = ALL_PARTS;
}


void SjCdgRaw::LoadColourTable(const SjCdgSubcode* subcode, int startIndex)
{
	const unsigned char* src = subcode->data;
	unsigned char*       dest = &m_colourTable[startIndex * 3];
	int i, r, g, b;

	for( i = 0; i < 8; i++ )
	{
#define HI_BYTE src[0]
#define LO_BYTE src[1]
		r =  (HI_BYTE & 0x3c) >> 2;
		g = ((HI_BYTE & 0x03) << 2) | ((LO_BYTE & 0x30) >> 4);
		b =  (LO_BYTE & 0x0f);
		src += 2;

		wxASSERT( r < 16 && g < 16 && b < 16 );

		*dest++ = r << 4;
		*dest++ = g << 4;
		*dest++ = b << 4;
	}

	m_updatedParts = ALL_PARTS;
}


void SjCdgRaw::TileBlock(const SjCdgSubcode* subcode, bool doXor)
{
	int colour0 = subcode->data[0] & 0x0F;
	int colour1 = subcode->data[1] & 0x0F;
	int colour, y, x;

	int tile_y = ((subcode->data[2] & 0x1F) * 12);
	int tile_x = ((subcode->data[3] & 0x3F) * 6);
	if( tile_y >= 18*12 || tile_x >= 50*6 )
	{
		return; // error - bad data!
	}

	unsigned char  src;
	unsigned char* dest;
	for( y = 0; y < 12; y++ )
	{
		src = subcode->data[4+y];

		dest = &m_screen[(tile_y+y)*CDG_SCREEN_W + tile_x];

		for( x = 5; x >= 0; x-- )
		{
			colour = (src & (1<<x))? colour1 : colour0;
			if( doXor )
			{
				*dest ^= colour;
			}
			else
			{
				*dest = colour;
			}

			dest++;
		}
	}

	// mark the updated parts ...
	int partIndex = 0;

	// ... remove the left and top border for easier calculations
	tile_y -= 12;
	tile_x -= 6;

	// ... go through all 24 parts and check the intersections with the tiles
	for( y = 0; y < CDG_IMAGE_H; y += PART_HEIGHT )
	{
		for( x = 0; x < CDG_IMAGE_W; x += PART_WIDTH )
		{
			if( tile_x+6  >= x
			        && tile_x    <= x+PART_WIDTH
			        && tile_y+12 >= y
			        && tile_y    <= y+PART_HEIGHT )
			{
				m_updatedParts |= 1<<partIndex;
			}

			partIndex++;
		}
	}

	m_hasData = true;
}


void SjCdgRaw::Scroll(const SjCdgSubcode* subcode, bool scrollRotate)
{
	int     color   = subcode->data[0] & 0x0F; // only used if scrollRotate is false

	char    hScroll = subcode->data[1] & 0x3F; // contains hSCmd and hOffset, see below
	int     hSCmd   = (hScroll & 0x30) >> 4;
	int     hOffset = (hScroll & 0x07);

	char    vScroll = subcode->data[2] & 0x3F; // contains vSCmd and vOffset, see below
	int     vSCmd   = (vScroll & 0x30) >> 4;
	int     vOffset = (vScroll & 0x0F);

	wxASSERT( CDG_SCREEN_H-12 == CDG_IMAGE_H );


	// scroll vertically ...
	// (example: "NENA - LEUCHTTURM.CDG")
	if( vOffset == 0 )
	{
		switch( vSCmd )
		{
			case 1: // ... 12 pixels down
				memcpy (m_scrollTempBuf, m_screen+CDG_SCREEN_W*CDG_IMAGE_H, CDG_SCREEN_W*12); // backup the part that will be overwritten by memmove()
				memmove(m_screen+CDG_SCREEN_W*12, m_screen, CDG_SCREEN_W*CDG_IMAGE_H);

				if( scrollRotate )
					memcpy(m_screen, m_scrollTempBuf, CDG_SCREEN_W*12);
				else
					memset(m_screen, color, CDG_SCREEN_W*12);

				m_updatedParts = ALL_PARTS;
				break;

			case 2: // ... 12 pixels up
				memcpy (m_scrollTempBuf, m_screen, CDG_SCREEN_W*12); // backup the part that will be overwritten by memmove()
				memmove(m_screen, m_screen+CDG_SCREEN_W*12, CDG_SCREEN_W*CDG_IMAGE_H);

				if( scrollRotate )
					memcpy(m_screen+CDG_SCREEN_W*CDG_IMAGE_H, m_scrollTempBuf, CDG_SCREEN_W*12);
				else
					memset(m_screen+CDG_SCREEN_W*CDG_IMAGE_H, color, CDG_SCREEN_W*12);

				m_updatedParts = ALL_PARTS;
				break;
		}
	}

	// scroll horizontally ...
	// (not yet implemented - I need an example for testing)
	if( hOffset == 0 )
	{
		switch( hSCmd )
		{
			case 1: // ... 6 pixels to the right
				break;

			case 2: // ... 6 pixels to the left
				break;
		}
	}


	// There is also a "fine scrolling" possible by the lower 4 bits in vScroll and hScroll
	// (vOffset and hOffset != 0).
	//
	// However, this is not very useful for us as we update the screen normally only 100 ms;
	// the "rough" scrolling above seems to be sufficient.
	// Moreover, scrolling at all is not a feature used very often in CDG, so the stuff above
	// is just "good enough".
}
