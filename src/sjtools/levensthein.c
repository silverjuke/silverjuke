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
 * File:    levensthein.c
 * Authors: Björn Petersen
 * Purpose: calculate the levensthein difference
 *
 ******************************************************************************/


#include <sjtools/levensthein.h>
#include <assert.h>
#ifdef __WXMAC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <memory.h>
#include <string.h>


/********************************************************************************
 * LEVENSTHEIN implementation
 ********************************************************************************/


/* standardizing ANSI strings to characters A-Z */
static const unsigned char MAnsiStrStandardizeTable[512] =

    /* from:                        */  /* to:                          */
    /* ---------------------------- */  /* ---------------------------- */

    /* 0,  - Steuerzeichen -        */  "  "
    /* 1,  - Steuerzeichen -        */  "  "
    /* 2,  - Steuerzeichen -        */  "  "
    /* 3,  - Steuerzeichen -        */  "  "
    /* 4,  - Steuerzeichen -        */  "  "
    /* 5,  - Steuerzeichen -        */  "  "
    /* 6,  - Steuerzeichen -        */  "  "
    /* 7,  - Steuerzeichen -        */  "  "
    /* 8,  - Steuerzeichen -        */  "  "
    /* 9,  - Steuerzeichen -        */  "  "
    /* 10, - Steuerzeichen -        */  "  "
    /* 11, - Steuerzeichen -        */  "  "
    /* 12, - Steuerzeichen -        */  "  "
    /* 13, - Steuerzeichen -        */  "  "
    /* 14, - Steuerzeichen -        */  "  "
    /* 15, - Steuerzeichen -        */  "  "
    /* 16, - Steuerzeichen -        */  "  "
    /* 17, - Steuerzeichen -        */  "  "
    /* 18, - Steuerzeichen -        */  "  "
    /* 19, - Steuerzeichen -        */  "  "
    /* 20, - Steuerzeichen -        */  "  "
    /* 21, - Steuerzeichen -        */  "  "
    /* 22, - Steuerzeichen -        */  "  "
    /* 23, - Steuerzeichen -        */  "  "
    /* 24, - Steuerzeichen -        */  "  "
    /* 25, - Steuerzeichen -        */  "  "
    /* 26, - Steuerzeichen -        */  "  "
    /* 27, - Steuerzeichen -        */  "  "
    /* 28, - Steuerzeichen -        */  "  "
    /* 29, - Steuerzeichen -        */  "  "
    /* 30, - Steuerzeichen -        */  "  "
    /* 31, - Steuerzeichen -        */  "  "
    /* 32, Leerzeichen/Space        */  "  "
    /* 33, Ausrufungszeichen ("!")  */  "  "
    /* 34, Anf. Z. oben ("''")      */  "  "
    /* 35, Nummernzeichen ("#")     */  "  "
    /* 36, Dollarzeichen ("$")      */  "  "
    /* 37, Prozentzeichen ("%")     */  "  "
    /* 38, Geschaeftl. und ("&")    */  "  "
    /* 39, Hochkomma/Quote ("'")    */  "  "
    /* 40, einf. Klammer auf ("(")  */  "  "
    /* 41, einf. Klammer zu (")")   */  "  "
    /* 42, Sternchen/Asterix ("*")  */  "  "
    /* 43, Plus ("+")               */  "  "
    /* 44, Komma (",")              */  "  "
    /* 45, Minus ("-")              */  "  "
    /* 46, Punkt f. Satzende (".")  */  "  "
    /* 47, Schraegstr./Slash ("/")  */  "  "
    /* 48, Ziffer Null ("0")        */  "  "
    /* 49, Ziffer Eins ("1")        */  "  "
    /* 50, Ziffer Zwei ("2")        */  "  "
    /* 51, Ziffer Drei ("3")        */  "  "
    /* 52, Ziffer Vier ("4")        */  "  "
    /* 53, Ziffer Fuenf ("5")       */  "  "
    /* 54, Ziffer Sechs ("6")       */  "  "
    /* 55, Ziffer Sieben ("7")      */  "  "
    /* 56, Ziffer Acht ("8")        */  "  "
    /* 57, Ziffer Neun ("9")        */  "  "
    /* 58, Doppelpunkt (":")        */  "  "
    /* 59, Semikolon (";")          */  "  "
    /* 60, Z. "kleiner als" ("<")   */  "  "
    /* 61, Z. "gleich" ("=")        */  "  "
    /* 62, Z. "groesser als" (">")  */  "  "
    /* 63, Fragezeichen ("?")       */  "  "
    /* 64, Klammeraffe ("@")        */  "  "
    /* 65, gr. "A"                  */  "A "
    /* 66, gr. "B"                  */  "B "
    /* 67, gr. "C"                  */  "C "
    /* 68, gr. "D"                  */  "D "
    /* 69, gr. "E"                  */  "E "
    /* 70, gr. "F"                  */  "F "
    /* 71, gr. "G"                  */  "G "
    /* 72, gr. "H"                  */  "H "
    /* 73, gr. "I"                  */  "I "
    /* 74, gr. "J"                  */  "J "
    /* 75, gr. "K"                  */  "K "
    /* 76, gr. "L"                  */  "L "
    /* 77, gr. "M"                  */  "M "
    /* 78, gr. "N"                  */  "N "
    /* 79, gr. "O"                  */  "O "
    /* 80, gr. "P"                  */  "P "
    /* 81, gr. "Q"                  */  "Q "
    /* 82, gr. "R"                  */  "R "
    /* 83, gr. "S"                  */  "S "
    /* 84, gr. "T"                  */  "T "
    /* 85, gr. "U"                  */  "U "
    /* 86, gr. "V"                  */  "V "
    /* 87, gr. "W"                  */  "W "
    /* 88, gr. "X"                  */  "X "
    /* 89, gr. "Y"                  */  "Y "
    /* 90, gr. "Z"                  */  "Z "
    /* 91, eckige Klammer auf ("[") */  "  "
    /* 92, Backslash ("\")          */  "  "
    /* 93, eckige Klammer zu ("]")  */  "  "
    /* 94, Z. "potenziert mit" ("^")*/  "  "
    /* 95, Underscore ("_")         */  "  "
    /* 96, Backquote ("`")          */  "  "
    /* 97,  kl. "a"                 */  "A "
    /* 98,  kl. "b"                 */  "B "
    /* 99,  kl. "c"                 */  "C "
    /* 100, kl. "d"                 */  "D "
    /* 101, kl. "e"                 */  "E "
    /* 102, kl. "f"                 */  "F "
    /* 103, kl. "g"                 */  "G "
    /* 104, kl. "h"                 */  "H "
    /* 105, kl. "i"                 */  "I "
    /* 106, kl. "j"                 */  "J "
    /* 107, kl. "k"                 */  "K "
    /* 108, kl. "l"                 */  "L "
    /* 109, kl. "m"                 */  "M "
    /* 110, kl. "n"                 */  "N "
    /* 111, kl. "o"                 */  "O "
    /* 112, kl. "p"                 */  "P "
    /* 113, kl. "q"                 */  "Q "
    /* 114, kl. "r"                 */  "R "
    /* 115, kl. "s"                 */  "S "
    /* 116, kl. "t"                 */  "T "
    /* 117, kl. "u"                 */  "U "
    /* 118, kl. "v"                 */  "V "
    /* 119, kl. "w"                 */  "W "
    /* 120, kl. "x"                 */  "X "
    /* 121, kl. "y"                 */  "Y "
    /* 122, kl. "z"                 */  "Z "
    /* 123, "{"                     */  "  "
    /* 124, Pipe ("|")              */  "  "
    /* 125, "}"                     */  "  "
    /* 126, Tilde ("~")             */  "  "
    /* 127, - unbelegt -            */  "  "
    /* 128, - unbelegt -            */  "  "
    /* 129, - unbelegt -            */  "  "
    /* 130, wie Komma (",")         */  "  "
    /* 131, Forte ("f")             */  "  "
    /* 132, Anf. Z. unten (",,")    */  "  "
    /* 133, drei Punkte ("...")     */  "  "
    /* 134, Zeichen "gestorben"     */  "  "
    /* 135, Zeichen "2 x gestorben" */  "  "
    /* 136, Accent circonflex ("^") */  "  "
    /* 137, Promille ("0/00")       */  "  "
    /* 138, gr. "S" m. Acc. ("\/")  */  "S "
    /* 139, Anf. einf. Anfz. ("<")  */  "  "
    /* 140, gr. "OE"                */  "OE"
    /* 141, - unbelegt -            */  "  "
    /* 142, - unbelegt -            */  "  "
    /* 143, - unbelegt -            */  "  "
    /* 144, - unbelegt -            */  "  "
    /* 145, Anf. einf. Anfz. ("'")  */  "  "
    /* 146, Ende einf. Anfz. ("'")  */  "  "
    /* 147, Anf. dopp. Anfz. ("''") */  "  "
    /* 148, Ende dopp. Anfz. ("''") */  "  "
    /* 149, Malpunkt                */  "  "
    /* 150, Gedankenstr. kurz       */  "  "
    /* 151, Gedankenstr. lang       */  "  "
    /* 152, Nasal Accent ("~")      */  "  "
    /* 153, Trademark, ("TM")       */  "  "
    /* 154, kl. "s" m. Acc. ("\/")  */  "S "
    /* 155, Ende einf. Anfz. (">")  */  "  "
    /* 156, kl. "oe"                */  "OE"
    /* 157, - unbelegt -            */  "  "
    /* 158, - unbelegt -            */  "  "
    /* 159, gr. "Y" m. ".."         */  "Y "
    /* 160, - unbelegt -            */  "  "
    /* 161, "!" ueberkopf           */  "  "
    /* 162, kl. "c" durchgestr.     */  "C "
    /* 163, Zeichen "engl. Pfund"   */  "  "
    /* 164, Zeichen                 */  "  "
    /* 165, gr. "Y" 2 x durchgestr. */  "Y "
    /* 166, Pipe                    */  "  "
    /* 167, Zeichen "Paragraph"     */  "  "
    /* 168, Acc. "2 Punkte" ("..")  */  "  "
    /* 169, Z. "Copyright" ("(c)")  */  "  "
    /* 170, kl. "a" hochgestellt    */  "  "
    /* 171, Anf. dopp. Anfz. ("<<") */  "  "
    /* 172, Zeichen                 */  "  "
    /* 173, aehnl. Gedankenstrich   */  "  "
    /* 174, Z. "Registered" ("(R)") */  "  "
    /* 175, Zeichen                 */  "  "
    /* 176, Zeichen                 */  "  "
    /* 177, Zeichen "+/-"           */  "  "
    /* 178, Zeichen "hoch zwei"     */  "  "
    /* 179, Zeichen "hoch drei"     */  "  "
    /* 180, Acc-Aigue o. Backquote  */  "  "
    /* 181, kl. gr. mue             */  "  "
    /* 182, Zeichen "carriage ret." */  "  "
    /* 183, Malpunkt                */  "  "
    /* 184, Cedille (",")           */  "  "
    /* 185, Zeichen "hoch eins"     */  "  "
    /* 186, Zeichen "hoch null"     */  "  "
    /* 187, Ende dopp. Anfz. (">>") */  "  "
    /* 188, "1/4"                   */  "  "
    /* 189, "1/2"                   */  "  "
    /* 190, "3/4"                   */  "  "
    /* 191, "?" ueberkopf           */  "  "
    /* 192, gr. "A" m. "\"          */  "A "
    /* 193, gr. "A" m. "/"          */  "A "
    /* 194, gr. "A" m. "^"          */  "A "
    /* 195, gr. "A" m. "~"          */  "A "
    /* 196, gr. "A" m. ".."         */  "AE"
    /* 197, gr. "A" m. Kringel      */  "A "
    /* 198, gr. verschl. "AE"       */  "AE"
    /* 199, gr. "C" m. ","          */  "C "
    /* 200, gr. "E" m. "\"          */  "E "
    /* 201, gr. "E" m. "/"          */  "E "
    /* 202, gr. "E" m. "^"          */  "E "
    /* 203, gr. "E" m. ".."         */  "E "
    /* 204, gr. "I" m. "\"          */  "I "
    /* 205, gr. "I" m. "/"          */  "I "
    /* 206, gr. "I" m. "^"          */  "I "
    /* 207, gr. "I" m. ".."         */  "I "
    /* 208, gr. "D" halb durchgestr.*/  "D "
    /* 209, gr. "N" m. "~"          */  "N "
    /* 210, gr. "O" m. "\"          */  "O "
    /* 211, gr. "O" m. "/"          */  "O "
    /* 212, gr. "O" m. "^"          */  "O "
    /* 213, gr. "O" m. "~"          */  "O "
    /* 214, gr. "O" m. ".."         */  "OE"
    /* 215, "x" f. "mult. mit"      */  "  "
    /* 216, gr. "O" durchgestr.     */  "O "
    /* 217, gr. "U" m. "\"          */  "U "
    /* 218, gr. "U" m. "/"          */  "U "
    /* 219, gr. "U" m. "^"          */  "U "
    /* 220, gr. "U" m. ".."         */  "UE"
    /* 221, gr. "Y" m. "/"          */  "Y "
    /* 222, halbes gr. "B"          */  "B "
    /* 223, dt. "s-z"               */  "SS"
    /* 224, kl. "a" m. "\"          */  "A "
    /* 225, kl. "a" m. "/"          */  "A "
    /* 226, kl. "a" m. "^"          */  "A "
    /* 227, kl. "a" m. "~"          */  "A "
    /* 228, kl. "a" m. ".."         */  "AE"
    /* 229, kl. "a" m. Kringel      */  "A "
    /* 230, kl. verschl. "ae"       */  "AE"
    /* 231, kl. "c" m. ","          */  "C "
    /* 232, kl. "e" m. "\"          */  "E "
    /* 233, kl. "e" m. "/"          */  "E "
    /* 234, kl. "e" m. "^"          */  "E "
    /* 235, kl. "e" m. ".."         */  "E "
    /* 236, kl. "i" m. "\"          */  "I "
    /* 237, kl. "i" m. "/"          */  "I "
    /* 238, kl. "i" m. "^"          */  "I "
    /* 239, kl. "i" m. ".."         */  "I "
    /* 240, kl. "d" halb durchgestr.*/  "D "
    /* 241, kl. "n" m. "~"          */  "N "
    /* 242, kl. "o" m. "\"          */  "O "
    /* 243, kl. "o" m. "/"          */  "O "
    /* 244, kl. "o" m. "^"          */  "O "
    /* 245, kl. "o" m. "~"          */  "O "
    /* 246, kl. "o" m. ".."         */  "OE"
    /* 247, dt. "geteilt durch"     */  "  "
    /* 248, kl. "o" durchstr.       */  "O "
    /* 249, kl. "u" m. "\"          */  "U "
    /* 250, kl. "u" m. "/"          */  "U "
    /* 251, kl. "u" m. "^"          */  "U "
    /* 252, kl. "u" m. ".."         */  "UE"
    /* 253, kl. "y" m. "/"          */  "Y "
    /* 254, halbes kl. "b"          */  "B "
    /* 255, kl. "y" m. ".."         */  "Y "
    ;

#define CHARBUF_SIZE 256

long  MmvStrStandardize (   const unsigned char*    src,
                            unsigned char*          dest    )
{
	int                     src_pos,dest_pos;
	const unsigned char*    standardize;

	src_pos = 0;
	dest_pos = 0;
	while(src[src_pos])
	{
		standardize = &MAnsiStrStandardizeTable[src[src_pos]*2];

		if(standardize[0]!=' ')
		{
			if( dest_pos > CHARBUF_SIZE-20 /*leave a little bit of space*/ )
			{
				break;
			}

			dest[dest_pos] = standardize[0];
			dest_pos++;

			if(standardize[1]!=' ')
			{
				dest[dest_pos] = standardize[1];
				dest_pos++;
			}
		}

		src_pos++;
	}
	dest[dest_pos] = 0;

	return (long)dest_pos;
}


static int MmvStrCalcWldLimit   (   long currLen    )
{
	if(currLen<=4)  return 1;
	if(currLen<=7)  return 2;
	if(currLen<=20) return 3;
	return 4;
}


int levensthein (const unsigned char *wort__, const unsigned char *muster__, int limit__)
{
#define CHARBUF_SIZE 256
	register int    spmin,
	         p,q,r,
	         lm,lw,
	         d1,d2,
	         i,k,
	         x1,x2,x3;
	char            c;
	int             d[CHARBUF_SIZE];
	int             limit = limit__;

	/* standardize the given strings*/
	unsigned char   wort[CHARBUF_SIZE];
	unsigned char   muster[CHARBUF_SIZE];

	lw = MmvStrStandardize(wort__, wort);
	lm = MmvStrStandardize(muster__, muster);


	/* this is done in MmvStrStandardize
	lw = strlen(wort);
	lm = strlen(muster);
	if (lw >= CHARBUF_SIZE) lw = (CHARBUF_SIZE-1);
	if (lm >= CHARBUF_SIZE) lm = (CHARBUF_SIZE-1);
	*/

	if( limit <= 0 )
	{
		limit = MmvStrCalcWldLimit(lm);
	}

	/****  Anfangswerte berechnen ****/
	if (*muster == '*')
	{
		for (k=0; k<=lw; k++)
		{
			d[k] = 0;
		}
	}
	else
	{
		d[0] = (*muster == 0) ? 0 : 1;
		i = (*muster == '?') ? 0 : 1;
		for (k=1; k<=lw; k++)
		{
			if (*muster == *(wort+k-1))
			{
				i = 0;
			}
			d[k] = k-1 + i;
		}
	}

	spmin = (d[0] == 0  ||  lw == 0) ?  d[0] : d[1];
	if (spmin > limit)
	{
		return limit__? CHARBUF_SIZE : 0; // no match
	}

	/****  Distanzmatrix durchrechnen  ****/
	for (i=2; i<=lm; i++)
	{
		c = *(muster+i-1);
		p = (c == '*'  ||  c == '?') ?  0 : 1;
		q = (c == '*') ?  0 : 1;
		r = (c == '*') ?  0 : 1;
		d2 = d[0];
		d[0] = d2 + q;
		spmin = d[0];

		for (k=1; k<=lw; k++)
		{
			/****  d[k] = Minimum dreier Zahlen  ****/
			d1 = d2;
			d2 = d[k];
			x1 = d1 + ((c == *(wort+k-1)) ?  0 : p);
			x2 = d2 + q;
			x3 = d[k-1] + r;

			if (x1 < x2)
			{
				x2 = x1;
			}
			d[k] = (x2 < x3) ?  x2 : x3;

			if (d[k] < spmin)
			{
				spmin = d[k];
			}
		}

		if (spmin > limit)
		{
			return limit__? CHARBUF_SIZE : 0; // no match
		}
	}

	if (d[lw] <= limit)
	{
		return limit__? d[lw] : 1; // match
	}
	else
	{
		return limit__? CHARBUF_SIZE : 0; // no match
	}
}



