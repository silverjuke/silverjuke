<?php

/*****************************************************************************
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
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/


/*****************************************************************************
 * Common Functions
 *****************************************************************************/


function openDest($filename)
{
	global $destHandle;
	global $destEpilogue;
	global $srcCnt;
	global $totalBytes;
	
	$srcCnt = 0;
	$destHandle = fopen($filename, 'w');
	if( !$destHandle )
	{
		echo "Cannot open \"$filename\" for writing.";
		exit();
	}
	
	fwrite($destHandle, "// created on ".strftime("%Y-%m-%d %H:%M:%S")." automatically by bin2c.sh, do not edit by hand\n#include <sjbase/base.h>\n#include <wx/fs_mem.h>\n");
	
	$destEpilogue = "void SjModuleSystem::InitMemoryFS()\n{";
	$totalBytes = 0;
}


function write_data_as_hex(&$data, $filebytes, &$destHandle)
{
	$to_write = '';
	
	for( $i=0; $i<$filebytes; $i++ )
	{
		if( $i > 0 )
		{
			$to_write .= ",";
		}

		$to_write .= ord($data[$i]);
		
		if( $i%16==15 )
		{
			$to_write .= "\n";
		}
	}
	
	fwrite($destHandle, $to_write);
}


function addFile($filename, $addAs = "")
{
	global $destHandle;
	global $destEpilogue;
	global $srcCnt;
	global $totalBytes;

	if( $addAs == "" )
	{
		$addAs = $filename;
	}

	// get the size of the file
	$filebytes = filesize($filename);
	$totalBytes += $filebytes;
	
	// open file
	$srcHandle = fopen($filename, 'rb');
	if( !$srcHandle )
	{
		echo "Cannot open \"$filename\" for reading.";
	}
	
	// write data
	fwrite($destHandle, "static const unsigned char s_bin2c_file{$srcCnt}[{$filebytes}] = {\n");
	
		$data = fread($srcHandle, $filebytes);
		write_data_as_hex($data, $filebytes, $destHandle);
	
	fwrite($destHandle, "};\n");
	
	$destEpilogue .= "\n    wxMemoryFSHandler::AddFile(wxT(\"$addAs\"), s_bin2c_file{$srcCnt}, {$filebytes});";
	
	// close file
	fclose($srcHandle);
	$srcCnt++;
}


function closeDest()
{
	global $destHandle;
	global $destEpilogue;
	global $srcCnt;
	global $totalBytes;

	fwrite($destHandle, $destEpilogue);
	fwrite($destHandle, "\n    /"."/ Totally, {$totalBytes} Bytes in {$srcCnt} Files added.\n}\n");
	fclose($destHandle);
}


function addDir($dirname, $mask)
{
	$dirhandle = @opendir($dirname);
	if( !$dirhandle )
	{
		echo "Cannot open directory \"$dirname\".";
		exit();
	}
	
	while( $folderentry = readdir($dirhandle) ) 
	{
		if( $folderentry{0} != '.' 
		 && !is_dir("$dirname/$folderentry") )
		{
			if( preg_match($mask, $folderentry) )
			{
				addFile("$dirname/$folderentry", $folderentry);
			}
		}
	}			
	
	closedir($dirhandle);
}


/*****************************************************************************
 * Needed Files
 *****************************************************************************/


openDest('data.cpp');
	addFile  ('skins/silveriness.sjs/main.xml', 'defaultskin.xml');
	addDir   ('skins/silveriness.sjs', '/^z_.*\.xml$/');
	addDir   ('skins/silveriness.sjs', '/^z_.*\.png$/');
	addDir   ('karaoke_bg/', '/^.*\.jpg$/');
	addFile  ('icons/icons16.png', 'icons16.png');
	addFile  ('icons/icons32.png', 'icons32.png');
	addFile	 ('icons/aboutlogo.gif', 'aboutlogo.gif');
	addFile	 ('keyboards/en.sjk', 'en.sjk');
closeDest();



