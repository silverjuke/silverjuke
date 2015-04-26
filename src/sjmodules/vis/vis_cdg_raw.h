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
 * File:    cdg_raw.h
 * Authors: Björn Petersen
 * Purpose: Low-level CDG handling
 *
 ******************************************************************************/


#ifndef __SJ_CDG_RAW_H__
#define __SJ_CDG_RAW_H__


struct SjCdgSubcode
{
	unsigned char   command;        // only use CDG_MASK bits!
	unsigned char   instruction;    // only use CDG_MASK bits!
	unsigned char   parityQ[2];
	unsigned char   data[16];       // only use CDG_MASK bits!
	unsigned char   parityP[4];
};


// CDG Command Code
#define CDG_COMMAND                 9

// CDG Instruction Codes
#define CDG_INST_MEMORY_PRESET      1
#define CDG_INST_BORDER_PRESET      2
#define CDG_INST_TILE_BLOCK         6
#define CDG_INST_SCROLL_PRESET      20
#define CDG_INST_SCROLL_COPY        24
#define CDG_INST_DEF_TRANSP_COL     28
#define CDG_INST_LOAD_COL_TBL_0_7   30
#define CDG_INST_LOAD_COL_TBL_8_15  31
#define CDG_INST_TILE_BLOCK_XOR     38

// Bitmask for all CDG fields
#define CDG_MASK                    0x3F


class SjCdgRaw
{
public:
					SjCdgRaw            ();
	void            Rewind              ();
	void            AddSubcode          (const SjCdgSubcode* subcode);
	static bool     IsDataSubcode       (const SjCdgSubcode* subcode);
	#define         CDG_IMAGE_W     294 // 294/6  = 49 tiles (+1 for the border)
	#define         CDG_IMAGE_H     204 // 204/12 = 17 tiles (+1 for the border)

	#define         CDG_SCREEN_W    300
	#define         CDG_SCREEN_H    216
	unsigned char   m_screen[CDG_SCREEN_W * CDG_SCREEN_H];

	#define         PARTS_PER_ROW   6
	#define         PARTS_PER_COL   4
	#define         PART_WIDTH      (CDG_IMAGE_W / PARTS_PER_ROW)
	#define         PART_HEIGHT     (CDG_IMAGE_H / PARTS_PER_COL)
	#define         ALL_PARTS       0xFFFFFFL
	unsigned long   m_updatedParts;

	unsigned char   m_colourTable[16 * 3];
	int             m_presetColourIndex;
	int             m_borderColourIndex;
	int             m_transparentColour;
	bool            m_hasData;

private:
	void            PresetScreen        ();
	void            LoadColourTable     (const SjCdgSubcode* subcode, int startIndex);
	void            TileBlock           (const SjCdgSubcode* subcode, bool doXor);
	void            Scroll              (const SjCdgSubcode* subcode, bool scrollRotate);
	unsigned char   m_scrollTempBuf     [CDG_SCREEN_W*12];
};


#endif /* __SJ_CDG_RAW_H__ */
