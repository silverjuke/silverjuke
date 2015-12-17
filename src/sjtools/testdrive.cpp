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
 * File:    testdrive.cpp
 * Authors: Björn Petersen
 * Purpose: Test some functions on startup or on call;
 *          mainly used in the debug version of Silverjuke
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/url.h>
#include <sjtools/testdrive.h>
#include <sjtools/csv_tokenizer.h>
#include <see_dom/sj_see.h>
#include <tagger/tg_wma_file.h>
#include <tagger/tg_mpeg_file.h>
#include <tagger/tg_oggvorbis_file.h>
#include <tagger/tg_mpc_file.h>


static wxString GetTestFilePath(const wxString& fileName)
{
	wxString test = wxFileName::CreateTempFileName(wxT("tmp"));
	::wxRemoveFile(test);

	wxFileName fn(test);
	fn.SetFullName(fileName); // important is the character "#"

	return fn.GetLongPath();
}


static wxString CreateTestFileUsingWxFile(const wxString& fileName, const wxString& content)
{
	wxString path = GetTestFilePath(fileName);

	wxFile f;
	f.Create(path, TRUE);
	f.Write(static_cast<const wxChar*>(content.c_str()), content.Len() * sizeof(wxChar));

	return path;
}


void SjTestdrive1()
{

	/*******************************************************************************
	 * LOGICAL TESTS
	 ******************************************************************************/

	/* older versions of wxWidgets are incompatible (in fact, the wxWidgets interface has changed) */
	#if !wxCHECK_VERSION(2, 8, 0)
		#error at least wxWidgets 2.8 required
	#endif

	/* Make sure, HTTP Auhorization is added to http.cpp in all used releases (see remarks like EDIT BY ME in wx/http.cpp and wx/http.h) */
	#if !wxCHECK_VERSION(2, 8, 7)
		wxLogWarning(wxT("Testdrive: wxHTTP auhorization functionality missing."));
	#endif

	/* wxString::Replace() seems to be very inefficient for replacements of only one character, see modified wx source */
	#if !wxCHECK_VERSION(2, 8, 10) // since 2.8.10, this is optimized by the wxWidgets team
		wxLogWarning(wxT("Testdrive: wxString::Replace() may get some optimizations"));
	#endif


	/*******************************************************************************
	 * PHYSICAL TESTS
	 ******************************************************************************/

	wxLogInfo(wxT("Testdrive: Some physical tests ..."));

	/* Test SjTools::AreFilesSame() */
	{
		#define FILES_ZIP_ID3   wxT("d:\\mp3\\test.zip#zip:file1.mp3#id3:cover1.gif")
		#define FILES_ZIP_ID3_S wxT("d:/mp3/test.zip#zip:file1.mp3#id3:cover1.gif")
		if( !SjTools::AreFilesSame(FILES_ZIP_ID3, FILES_ZIP_ID3)
		 || !SjTools::AreFilesSame(FILES_ZIP_ID3, FILES_ZIP_ID3) ) {
			wxLogWarning(wxT("Testdrive: AreFilesSame() failed!"));
		}

		#if wxUSE_UNICODE
		#define ORG_SCRAMBLE_STR wxT("Kalim\x00E9ra - \x039a\x03b1\x03bb\x03b7\x03bc\x03b5\x03c1\x03b1")
		#else
		#define ORG_SCRAMBLE_STR wxT("Kalim\xE9ra")
		#endif
		wxString scrStr = SjTools::ScrambleString(ORG_SCRAMBLE_STR);
		wxString unscrStr = SjTools::UnscrambleString(scrStr);
		if( unscrStr != ORG_SCRAMBLE_STR ) {
			wxLogWarning(wxT("Testdrive: ScrambleString() failed"));
		}
	}


	/* wxURI::Unescape() does not handle a single percent sign, which is correct, however,
	if one is a little bit lazy on paths, this may cause problems.
	*/
	{
		if( wxURI::Unescape(wxT("%20")) != wxT(" ")
		 || wxURI::Unescape(wxT("%25")) != wxT("%") )
		{
			wxLogWarning(wxT("Testdrive: wxURI Tests failed"));
		}

		if( tolower(0xc4) != 0xe4 )
		{
			//wxLogInfo(wxT("Testdrive: tolower() does not work eg. for german umlauts"));
			// well, I do not think, we really use this. wx use the ones below
		}

		wxString umlauteLower(wxT("\xE4\xF6\xFC\xDF (ae oe ue ss)"));
		wxString umlauteUpper(wxT("\xC4\xD6\xDC\xDF (AE OE UE SS)"));
		if( umlauteLower.Upper() != umlauteUpper || umlauteLower != umlauteUpper.Lower() )
		{
			wxLogWarning(wxT("Testdrive: wxString::Lower()/wxString::Upper() does not work eg. for german umlauts."));
		}

		#define URLENCODE_TEST_W_CHAR wxT("+ urlencode test _-+ \x00C4\x00D6\x00DC\x03B1\x1F82") // umlaute and some greek characters
		wxASSERT( SjTools::Urldecode(SjTools::Urlencode(URLENCODE_TEST_W_CHAR        /*encode as UTF-8*/)) == URLENCODE_TEST_W_CHAR );
		wxASSERT( SjTools::Urldecode(SjTools::Urlencode(URLENCODE_TEST_W_CHAR, true  /*encode as UTF-8*/)) == URLENCODE_TEST_W_CHAR );
		wxASSERT( SjTools::Urldecode(SjTools::Urlencode(URLENCODE_TEST_W_CHAR, false /*encode as ANSI */)) != URLENCODE_TEST_W_CHAR );

		wxASSERT( SjNormaliseString(wxT("abc-def \x00C4,\x00D6.12345"), 0) == wxT("abcdefaeoe12345") );
		wxASSERT( SjNormaliseString(wxT("abc-def \x00C4,\x00D6.12345"), SJ_NUM_SORTABLE) == wxT("abcdefaeoe0000012345") );

		wxString testindex(wxT("ab"));
		wxASSERT( testindex[0] == (wxChar)'a' && (int)testindex[1] == 98 ); // check if we can get a character from the string using the [] operator
	}


	/* test SjCsvTokenizer
	*/
	{
		#define TEST_STR_AS_W_CHAR wxT("-abc\x00C4\x00D6\x00DC\x03B1\x1F82-") // umlaute and some greek characters
		#define TEST_STR_AS_UTF8   "-abc\xC3\x84\xC3\x96\xC3\x9C\xCE\xB1\xE1\xBE\x82-"
		wxString test1(TEST_STR_AS_W_CHAR);
		wxASSERT( strcmp(static_cast<const char*>(test1.mb_str(wxConvUTF8)), TEST_STR_AS_UTF8)==0 );
		wxString test2(TEST_STR_AS_UTF8, wxConvUTF8);
		wxASSERT( test2 == TEST_STR_AS_W_CHAR );

        SjCsvTokenizer tknzr(wxT(","), wxT("\""), wxT("\\"));
        const unsigned char* data = (const unsigned char*)"r0f0,\"r0f1\",\"r0\\\"\\\\\nf2\",r0f3\nr1f0" TEST_STR_AS_UTF8 ",r1f1;\t, r1f2 ,r1f3\nEOF";
        long data_bytes = strlen((const char*)data)-3;  /*make sure, there is no NULL-byte at the end  (see EOF above)*/
        tknzr.AddData(data /*UTF-8*/, data_bytes);
        wxArrayString* r = tknzr.GetRecord();
        wxASSERT(r);
        if( r ) {
			wxASSERT( r->GetCount()==4 );
			wxASSERT( r->Item(0)==wxT("r0f0") ); // test normal field
			wxASSERT( r->Item(1)==wxT("r0f1") ); // test quoted field
			wxASSERT( r->Item(2)==wxT("r0\"\\\nf2") ); // test "quote", "backslash" and "new line" in quoted field
			wxASSERT( r->Item(3)==wxT("r0f3") );
        }
        r = tknzr.GetRecord();
        wxASSERT(r);
        if( r ) {
			wxASSERT( r->GetCount()==4 );
			wxASSERT( r->Item(0) == wxT("r1f0") TEST_STR_AS_W_CHAR ); // test UTF8
			wxASSERT( r->Item(1) == wxT("r1f1;\t") ); // make sure, semicolons and tabs to not break fields
			wxASSERT( r->Item(2) == wxT(" r1f2 ") ); // spaces are not ignored
			wxASSERT( r->Item(3) == wxT("r1f3") );
        }
        r = tknzr.GetRecord();
        wxASSERT(r==NULL);
	}


	/* test SjStringSerializer
	*/
	{
		wxString str;
		{
			SjStringSerializer ser;
			ser.AddString(wxT("simple"));
			ser.AddString(TEST_STR_AS_W_CHAR);
			ser.AddLong(666);
			ser.AddLong(-1);
			ser.AddFloat((float)123.456);
			str = ser.GetResult();
		}
		{
			SjStringSerializer ser(str);
			wxString test_str = ser.GetString(); wxASSERT( test_str == wxT("simple") );
			test_str = ser.GetString(); wxASSERT( test_str == TEST_STR_AS_W_CHAR );
			long test_long = ser.GetLong(); wxASSERT( test_long == 666 );
			test_long = ser.GetLong(); wxASSERT( test_long == -1 );
			float test_float = ser.GetFloat(); wxASSERT( test_float >= 123.455 && test_float <= 123.457 );
			wxASSERT( !ser.HasErrors() );
			// some tests behind the data ...
			test_long = ser.GetLong();
			wxASSERT( test_long == 0 );
			wxASSERT( ser.HasErrors() );
		}
	}


	/* test various string functions
	*/
	{
		wxASSERT( SjTools::EnsureTrailingSlash("/test")  == "/test/" );
		wxASSERT( SjTools::EnsureTrailingSlash("/test/") == "/test/" );
		#ifdef __WXMSW__
			wxASSERT( SjTools::EnsureTrailingSlash("\\test")  == "\\test\\" );
			wxASSERT( SjTools::EnsureTrailingSlash("\\/test") == "\\/test/" );
			wxASSERT( SjTools::EnsureTrailingSlash("/\\test") == "/\\test/" );
		#endif

		wxASSERT( SjTools::VersionString2Long("1.2.3")  == 0x01020300 );
		wxASSERT( SjTools::VersionString2Long("10.11")  == 0x0A0B0000 );
		wxASSERT( SjTools::VersionString2Long("20")     == 0x14000000 );
		wxASSERT( SjTools::VersionString2Long("15.10.3")== 0x0F0A0300 );
	}


	/* make sure, SjSLHash, SjSSHash are case-sensitibe while SjExtList is not (important eg. to allow exentensions as "MP3" on linux)
	*/
	{
		SjSLHash slHash;
		slHash.Insert(wxT("TEST"), 666);
		wxASSERT( slHash.Lookup(wxT("TEST"))==666 );
		wxASSERT( slHash.Lookup(wxT("test"))==0 );
		wxASSERT( slHash.Lookup(wxT("notinhash"))==0 );

		SjSSHash ssHash;
		ssHash.Insert(wxT("foo"), wxT("bar"));
		wxASSERT( ssHash.Lookup(wxT("foo"))!=NULL );
		wxASSERT( ssHash.Lookup(wxT("FOO"))==NULL );
		wxASSERT( ssHash.Lookup(wxT("notinhash"))==NULL );

		SjExtList extList(wxT("MP3 jpg,png;Js"));
		wxASSERT( extList.LookupExt(wxT("mp3")));
		wxASSERT( extList.LookupExt(wxT("MP3")));
		wxASSERT( extList.LookupExt(wxT("jPg")));
		wxASSERT( extList.LookupExt(wxT("pNG")));
		wxASSERT( extList.LookupExt(wxT("js")));
		wxASSERT( extList.LookupExt(wxT("jS")));
		wxASSERT( extList.LookupExt(wxT("JS")));

		wxASSERT( SjTools::GetExt(wxT("someWhat.MP3"))==wxT("mp3") );
	}

	/* Stress wxFileSystem

	In wxFileSystem, the character "#" is used to start a new protocol inside a path,
	eg. you can read an embedded image in an MP3 file which is inside a
	ZIP file with the following path

	    file:/path/file.zip#zip:media.mp3#id3:cover.jpg

	This is very fine, however, the character "#" must be masked which is done
	eg. by wxFileSystem::FileNameToURL()  */
	{
		// create a file with a "#" in its name using wxFile
		wxString name = wxT("test #1 100% hard '\xE4\xF6\xFC\xC4\xD6\xDC\xDF'.test");
		wxString content = wxT("test content");
		wxString path = CreateTestFileUsingWxFile(name, content);

		// try to open this file using wxFileSystem
		{
		wxString pathAsUrl = wxFileSystem::FileNameToURL(path); // pathAsUrl should be file:/tmp/test %231 100%25 hard.test
		wxFileSystem fs;
		wxFSFile* fsFile = fs.OpenFile(pathAsUrl);
		if( fsFile )
		{
			wxInputStream* fsInputStream = fsFile->GetStream();
			if( fsInputStream )
			{
				wxChar buffer_[512];
				long bytesRead = fsInputStream->Read(buffer_, 100).LastRead(); // the test content was written using ASCII, UCS-2 or UCS-4, _not_ UTF-8!
				buffer_[bytesRead / sizeof(wxChar)] = 0;
				if( wxString(buffer_) != content )
				{
					wxLogWarning(wxT("Testdrive: wxFileSystem failed: Bad content in %s."), path.c_str());
				}
			}
			else
			{
				wxLogWarning(wxT("Testdrive: wxFileSystem failed: Cannot get stream for %s."), path.c_str());
			}
			delete fsFile;
		}
		else
		{
			wxLogWarning(wxT("Testdrive: wxFileSystem failed: Cannot open %s."), path.c_str());
		}
		}


		// check if we can read the file using wxFileSystem::FindFirst()
		{
			wxFileName fn(path); // check wxFileName to work with FILEs
			if(  path.Left(5)=="file:" ) { wxLogWarning("Testdrive: fn should not start with 'file:'"); }
			if( !fn.IsOk()             ) { wxLogWarning("Testdrive: fn.IsOk() failed"); }
			if(  fn.IsDir()            ) { wxLogWarning("Testdrive: fn.IsDir() failed"); }
			if( !fn.FileExists()       ) { wxLogWarning("Testdrive: fn.FileExists() failed"); }
			if( !fn.DirExists()        ) { wxLogWarning("Testdrive: fn.DirExists() failed"); } // Notice that this function tests the directory part of this object, i.e. the string returned by GetPath(), and not the full path returned by GetFullPath().

			wxString fn2DirStr = fn.GetPath(wxPATH_GET_SEPARATOR);
			if(  fn2DirStr.Left(5)=="file:" ) { wxLogWarning("Testdrive: fn2DirStr should not start with 'file:'"); }
			if(  fn2DirStr.Last()!='/' && fn2DirStr.Last()!='\\' ) { wxLogWarning("Testdrive: fn2DirStr: Where is the trailing slash?"); }
			wxFileName fn2(fn2DirStr);  // the wxFileName constructor creates DIRs without FILEs if there is a trailing slash!s
			if( !fn2.IsOk() )       { wxLogWarning("Testdrive: fn2.IsOk() failed"); }
			if( !fn2.IsDir() )      { wxLogWarning("Testdrive: fn2.IsDir() failed"); }
			if(  fn2.FileExists() ) { wxLogWarning("Testdrive: fn2.FileExists() failed"); }
			if( !fn2.DirExists() )  { wxLogWarning("Testdrive: fn2.DirExists() failed"); }

			// create filesystem object and let if point to the given directory
			wxFileSystem fs;
			wxString locationUrl = wxFileSystem::FileNameToURL(fn2DirStr);
			if( fn2DirStr.Left(5) == "file:" ) { wxLogWarning("Testdrive: fn2DirStr should not start with 'file:'"); }
			if( locationUrl.Left(5) != "file:" ) { wxLogWarning("Testdrive: locationUrl should start with 'file:'"); }
			fs.ChangePathTo(locationUrl, true);
			wxString pathChangedTo = fs.GetPath();
			if( pathChangedTo.Left(5) != "file:" ) { wxLogWarning("Testdrive: pathChangedTo should start with 'file:'"); }

			// search for all files in the given directory
			bool testFileFound = false;
			wxString dirEntryLocationUrl = fs.FindFirst("*", wxFILE);
			while( !dirEntryLocationUrl.IsEmpty() )
			{
				if( dirEntryLocationUrl.Left(5) != "file:" ) { wxLogWarning("Testdrive: dirEntryLocationUrl should start with 'file:'"); }

				wxFileName dirEntryFileName = wxFileSystem::URLToFileName(dirEntryLocationUrl);
				if( !dirEntryFileName.IsOk()       ) { wxLogWarning("Testdrive: dirEntryFileName.IsOk() failed"); }
				if(  dirEntryFileName.IsDir()      ) { wxLogWarning("Testdrive: dirEntryFileName.IsDir() failed"); } // we use wxFILE in FindFirst(), there should be no directories!
				if( !dirEntryFileName.FileExists() ) { wxLogWarning("Testdrive: dirEntryFileName.FileExists() failed"); }
				if( !dirEntryFileName.DirExists()  ) { wxLogWarning("Testdrive: dirEntryFileName.DirExists() failed"); }

				wxString dirEntryFileNameFullPathStr = dirEntryFileName.GetFullPath();
				if( dirEntryFileNameFullPathStr.Left(5) == "file:" ) { wxLogWarning("Testdrive: dirEntryFileNameFullPathStr should start with 'file:'"); }

				wxString dirEntryFileNameFullNameStr = dirEntryFileName.GetFullName();
				if( dirEntryFileNameFullNameStr.Left(5) == "file:" ) { wxLogWarning("Testdrive: dirEntryFileNameFullNameStr should start with 'file:'"); }
				if( dirEntryFileNameFullNameStr == name )
				{
					testFileFound = true;
				}

				dirEntryLocationUrl = fs.FindNext();
			}

			if( !testFileFound ) { wxLogWarning("Testdrive: Testfile not found using wxFileSystem::Find*()"); }

			// finally check, if the second parameter (dir/file indicator works as expected)
			wxString locationUrl2 = wxFileSystem::FileNameToURL(path);
			fs.ChangePathTo(locationUrl2, false);
			wxString pathChangedTo2 = fs.GetPath();
			wxASSERT( pathChangedTo.Lower() == pathChangedTo2.Lower() ); // under Windows, there may differences regarding the case of the volume, so test case-insensitive ...
		}

		// done.
		::wxRemoveFile(path);

		// another test checking the "dirExists" stuff:
		// since wx2.8, fileName.DirExists() checks if the base dir exists, not if the file is a directory!
		// (internally, wxFileName::DirExists(GetPath()) is called, for  wx2.6 internally, wxFileName::DirExists(GetFullPath()) was called)
		{
			wxString path = SjTools::GetSilverjukeProgramDir();                             // existant dir
			wxFileName filename(path);
			wxASSERT(  filename.IsDir() );
			wxASSERT(  filename.DirExists() );
			wxASSERT(  wxFileName::DirExists(path) );
			wxASSERT(  ::wxDirExists(path) );
			wxASSERT( !filename.FileExists() );
			wxASSERT( !::wxFileName::FileExists(path) );
			wxASSERT( !::wxFileExists(path) );
		}
		{
			wxString path = SjTools::GetSilverjukeProgramDir() + wxT("Silverjuke.exe");     // existant file in existant dir - do not run this test if the program has not the name "silverjuke.exe"
			if( ::wxFileExists(path) ) {
				wxFileName filename(path);
				wxASSERT( !filename.IsDir() );
				wxASSERT(  filename.DirExists() );
				wxASSERT( !wxFileName::DirExists(path) );
				wxASSERT( !::wxDirExists(path) );
				wxASSERT(  filename.FileExists() );
				wxASSERT(  ::wxFileName::FileExists(path) );
				wxASSERT(  ::wxFileExists(path) );
			}
		}
		{
			wxString path = SjTools::GetSilverjukeProgramDir() + wxT("unexistant.file");    // unexistant file in existant dir
			wxFileName filename(path);
			wxASSERT( !filename.IsDir() );
			wxASSERT(  filename.DirExists() );
			wxASSERT( !wxFileName::DirExists(path) );
			wxASSERT( !::wxDirExists(path) );
			wxASSERT( !filename.FileExists() );
			wxASSERT( !::wxFileName::FileExists(path) );
			wxASSERT( !::wxFileExists(path) );
		}
		{
			wxString path = wxT("/unexistant/dir/");                                        // unexistant dir
			wxFileName filename(path);
			wxASSERT(  filename.IsDir() );
			wxASSERT( !filename.DirExists() );
			wxASSERT( !wxFileName::DirExists(path) );
			wxASSERT( !::wxDirExists(path) );
			wxASSERT( !filename.FileExists() );
			wxASSERT( !::wxFileName::FileExists(path) );
			wxASSERT( !::wxFileExists(path) );
		}
		{
			wxString path = wxT("/unexistant/unexistant.file");                             // unexistant file in unexistant dir
			wxFileName filename(path);
			wxASSERT( !filename.IsDir() );
			wxASSERT( !filename.DirExists() );
			wxASSERT( !wxFileName::DirExists(path) );
			wxASSERT( !::wxDirExists(path) );
			wxASSERT( !filename.FileExists() );
			wxASSERT( !::wxFileName::FileExists(path) );
			wxASSERT( !::wxFileExists(path) );
			wxASSERT( !::wxFileExists("") ); // we rely on this when eg. wxFileSystem::URLToFileName(..).GetFullPath(); fails and returns the empty string
		}
	}


	/* Check the sqlite "file open bug" by a functionality test (force testing unicode in sqlite) */
	{
		wxString path1 = GetTestFilePath(wxT("test1.db")); // get the path for the test file
		wxString path2 = GetTestFilePath(wxT("test2-\xE4\xF6\xFC\xC4\xD6\xDC\xDF...db"));
		{	// create the test file (note the "{" here, which ensures, the db-object gets destroyed before the file is deleted)
			wxSqltDb db(path1);
			{
				wxSqlt sql(&db);
				if( !db.ExistsBeforeOpening() ) {
					sql.Query(wxT("CREATE TABLE test (col1 TEXT);"));
					sql.Query(wxT("INSERT INTO test (col1) VALUES ('") + path2 + wxT("');"));
				}

				// some string conversion tests
				#define ORG_SQLITE_STR wxT("this is a test\xE4\xF6\xFC\xC4\xD6\xDC\xDF") // this only uses ISO 8859-1 characters, so it should bypass all conversions
				wxString test = ORG_SQLITE_STR;

				WXSTRING_TO_SQLITE3(test);
				SQLITE3_TO_WXSTRING(testSqlite3Str);

				if( testSqlite3StrWxStr != ORG_SQLITE_STR )
				{
					wxLogWarning(wxT("Testdrive: sqlite3 string test testdrive failed!"));
				}

				// does sqlite work well with upper/lower case and with german umlauts?
				// see http://www.silverjuke.net/forum/topic-1196.html
				sql.Query(wxT("INSERT INTO test (col1) VALUES ('Die ") +wxString(wxChar(0xc4))+ wxT("rzte');"));
				sql.Query(wxT("SELECT * FROM test WHERE col1 LIKE 'die ") +wxString(wxChar(0xe4))+  wxT("%'"));
				if( !sql.Next() ) {
					wxLogInfo(wxT("Testdrive: sqlite3 LIKE-operator is not case insenssitive for german umlauts!"));
				}

				sql.Query(wxT("INSERT INTO test (col1) VALUES ('ABC');"));
				sql.Query(wxT("SELECT * FROM test WHERE col1 LIKE 'abc'"));
				if( !sql.Next() )
					wxLogWarning(wxT("Testdrive: sqlite3 LIKE-operator is not case insenssitive at all!"));
			}
		}

		if( !::wxRenameFile(path1, path2) ) { // rename the test file
			wxLogWarning(wxT("Testdrive: sqlite file open testdrive failed: Cannot rename %s to %s."), path1.c_str(), path2.c_str());
		}

		{	// re-open the test file
			wxSqltDb db(path2);
			if( db.IsOk() )
			{
				// check a query
				wxSqlt sql(&db);
				sql.Query(wxT("SELECT col1 FROM test WHERE col1='") + path2 + wxT("';"));
				if( sql.Next() ) {
					if( sql.GetString(0) != path2 ) {
						wxLogWarning(wxT("Testdrive: sqlite file open testdrive failed: Bad data in %s."), path2.c_str());
					}
				}
				else {
					wxLogWarning(wxT("Testdrive: sqlite file open testdrive failed: Cannot read test record for %s."), path2.c_str());
				}
			}
			else {
				wxLogWarning(wxT("Testdrive: sqlite file open testdrive failed: Cannot create test database %s."), path2.c_str());
			}
		}
		::wxRemoveFile(path2); // done
	}



	/* Scripting tests */
	#if SJ_USE_SCRIPTS
	if( g_debug&0x04 )
	{
		SjSee see;
		see.SetExecutionScope(wxT("Testdrive"));

		see.Execute(wxT("2+2"));

		if( see.GetResultString()!=wxT("4")
		 || see.GetResultLong()!=4
		 || see.GetResultDouble()!=4.0 )
		{
			wxLogWarning(wxT("Testdrive: SjSee::Execute(\"2+2\") failed"));
		}

		see.Execute(wxT("function test() {\n")
		            wxT(" return 'bla'+\"blub\";\n")
		            wxT("}\n"));

		see.Execute(wxT("test();"));

		if( see.GetResultString() != wxT("blablub") ) {
			wxLogWarning(wxT("Testdrive: SjSee::Execute(\"function test ...\") failed"));
		}

		see.Execute("program.version");
		wxASSERT( see.GetResultLong() == ((SJ_VERSION_MAJOR<<24)|(SJ_VERSION_MINOR<<16)|(SJ_VERSION_REVISION<<8)) );

		wxString testStr;
		testStr.Append((wxChar)0xF6); // small german "o" with two points
		testStr.Append((wxChar)0x1F82); // greek extended alpha
		see.Execute(wxT("test1='") + testStr + wxT("'; test2=test1; test2=encodeURI(test2); test2=decodeURI(test2); test1==test2;"));
		if( !see.GetResultLong() )
		{
			wxLogWarning(wxT("Testdrive: encodeURI()/decodeURI() failed, see http://www.silverjuke.net/forum/topic-3234.html"));
		}
	}
	#endif

	// Check if the floating point library is loaded and working well.
	// Under some circumstances, the library is not loaded when it is nedded; this results in
	// the error "R6002 - floating point support not loaded", see and http://www.silverjuke.net/forum/topic-3615.html
	{
		double dummyfloat;
		dummyfloat = 17.5f;
		wxString floatingpointstring = wxString::Format(wxT("%f"), dummyfloat);
		wxASSERT(!floatingpointstring.IsEmpty());
	}

	// other math tests
	wxASSERT( SjTools::Rand(0) == 0 );
	wxASSERT( SjTools::Rand(1) == 0 );

	/* Done */
	wxLogInfo(wxT("Testdrive: Done."));
}

