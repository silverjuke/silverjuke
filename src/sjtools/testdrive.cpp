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
	f.Write(content.c_str(), content.Len() * sizeof(wxChar));

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

	/* for newer versions, we have to check the changelog first (eg. the behaviour of c_str() has changed ...) */
#if wxCHECK_VERSION(3, 0, 0)
#error wxWidgets 3 will cause problems, use wxWidgets 2.8 instead
#endif


	/* Make sure, HTTP Auhorization is added to http.cpp in all used releases (see remarks like EDIT BY ME in wx/http.cpp and wx/http.h) */
#if !wxCHECK_VERSION(2, 8, 7)
	wxLogWarning(wxT("Testdrive: wxHTTP auhorization functionality missing."));
#endif


	/* wxString::Replace() seems to be very inefficient for replacements of only one character, see modified wx source */
#if !wxCHECK_VERSION(2, 8, 10) // since 2.8.10, this is optimized by the wxWidgets team
	wxLogWarning(wxT("Testdrive: wxString::Replace() may get some optimizations"));
#endif

	/* Two things to fix for wxDisplay:
	- on wxDisplay::GetCount(), the whole display array should  be reloaded (to reflect changes directly)
	  in wx 2.6, we've fixed this in "src/msw/display.cpp", for wx 2.8 the file is "src/common/dpycmn.cpp"
	  in wx 2.6, internally EnumDisplayMonitors() was used, not DirectX, same for 2.8
	- note the changes in src/common/wincmn.cpp ("wxRect clientrect = wxGetClientDisplayRect();"
	  must be replaced with the functionality from SjDialog::GetDisplayWorkspaceRect()) - in wx 2.8 this seems to be okay by default :-) */
#if !wxCHECK_VERSION(2, 8, 0)
	wxLogWarning(wxT("Testdrive: wxDisplay may contain a bug, see testdrive.cpp for details."));
#endif

	/* Debug-versions without Beta-code are suspicious */
#if !defined(SJ_BETA) && defined(__WXDEBUG__)
	wxLogWarning(wxT("Testdrive: Debug versions without beta code are suspicious."));
#endif


	/*******************************************************************************
	 * PHYSICAL TESTS
	 ******************************************************************************/


#ifdef SJ_BETA // (***)

	SjBusyInfo::Set(wxT("Some testdrives (beta only) ..."), TRUE);
	wxLogInfo(wxT("Testdrive: Some physical tests (beta only) ..."));

	/* Test SjTools::AreFilesSame() */
	{
		wxLogInfo(wxT("Testdrive: SjTools tests ..."));

#define FILES_ZIP_ID3   wxT("d:\\mp3\\test.zip#zip:file1.mp3#id3:cover1.gif")
#define FILES_ZIP_ID3_S wxT("d:/mp3/test.zip#zip:file1.mp3#id3:cover1.gif")
		if( !SjTools::AreFilesSame(FILES_ZIP_ID3, FILES_ZIP_ID3)
		        || !SjTools::AreFilesSame(FILES_ZIP_ID3, FILES_ZIP_ID3) )
			wxLogWarning(wxT("Testdrive: AreFilesSame() failed!"));

#if wxUSE_UNICODE
#define ORG_SCRAMBLE_STR wxT("Kalim\x00E9ra - \x039a\x03b1\x03bb\x03b7\x03bc\x03b5\x03c1\x03b1")
#else
#define ORG_SCRAMBLE_STR wxT("Kalim\xE9ra")
#endif
		wxString scrStr = SjTools::ScrambleString(ORG_SCRAMBLE_STR);
		wxString unscrStr = SjTools::UnscrambleString(scrStr);
		if( unscrStr != ORG_SCRAMBLE_STR )
			wxLogWarning(wxT("Testdrive: ScrambleString() failed"));
	}


	/* wxURI::Unescape() does not handle a single percent sign, which is correct, however,
	if one is a little bit lazy on paths, this may cause problems.
	*/
	{
		wxLogInfo(wxT("Testdrive: String tests ..."));
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
	}


	/* test SjCsvTokenizer
	*/
	{
		#define TEST_STR_AS_W_CHAR wxT("-abcÄÖÜ\x03B1\x1F82-")
		#define TEST_STR_AS_UTF8   "\x2D\x61\x62\x63\xC3\x84\xC3\x96\xC3\x9C\xCE\xB1\xE1\xBE\x82\x2D"
		wxString test1(TEST_STR_AS_W_CHAR);
		wxASSERT( strcmp(test1.mb_str(wxConvUTF8), TEST_STR_AS_UTF8)==0 );
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
			ser.AddFloat(123.456);
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
			test_long = ser.GetLong(); wxASSERT( test_long == 0 );
			wxASSERT( ser.HasErrors() );
		}
	}


	/* Stress wxFileSystem

	In wxFileSystem, the character "#" is used to start a new protocol inside a path,
	eg. you can read an embedded image in an MP3 file which is inside a
	ZIP file with the following path

	    file:/path/file.zip#zip:media.mp3#id3:cover.jpg

	This is very fine, however, the character "#" must be masked which is done
	eg. by wxFileSystem::FileNameToURL()  */
	{
		wxLogInfo(wxT("Testdrive: File tests ..."));

		// create a file with a "#" in its name using wxFile
		wxString name = wxT("test #1 100% hard.test");
		wxString content = wxT("test content");
		wxString path = CreateTestFileUsingWxFile(name, content);

		// try to open this file using wxFileSystem
		wxString pathAsUrl = wxFileSystem::FileNameToURL(path); // pathAsUrl should be file:/tmp/test %231 100%25 hard.test
		wxFileSystem fs;
		wxFSFile* fsFile = fs.OpenFile(pathAsUrl);
		if( fsFile )
		{
			wxInputStream* fsInputStream = fsFile->GetStream();
			if( fsInputStream )
			{
				wxString buffer;
				wxChar* buffer_ = buffer.GetWriteBuf(256);
				if( buffer_ )
				{
					long bytesRead = fsInputStream->Read(buffer_, 100).LastRead();
					buffer_[bytesRead / sizeof(wxChar)] = 0;
					buffer.UngetWriteBuf();
					if( buffer != content )
					{
						wxLogWarning(wxT("Testdrive: wxFileSystem failed: Bad content in %s."), path.c_str());
					}
				}
				else
				{
					wxLogWarning(wxT("Testdrive: wxFileSystem failed: Cannot get writebuf for %s."), path.c_str());
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

		// done.
		::wxRemoveFile(path);

		// another test checking the "dirExists" stuff:
		// since wx2.8, fileName.DirExists() checks if the base dir exists, not if the file is a directory!
		// (internally, wxFileName::DirExists(GetPath()) is called, for  wx2.6 internally, wxFileName::DirExists(GetFullPath()) was called)
#define FILE_ASSERT(c) if( !(c) ) { wxLogWarning(wxT("Testdrive: ") wxT(#c) wxT(" failed for %s"), path.c_str()); }
#if wxCHECK_VERSION(2,8,0)
#define FILE_ASSERT_NEW FILE_ASSERT
#else
#define FILE_ASSERT_NEW(a)
#endif
		{
			wxString path = SjTools::GetSilverjukeProgramDir();                             // existant dir
			wxFileName filename(path);
			FILE_ASSERT     (  filename.IsDir() );
			FILE_ASSERT_NEW (  filename.DirExists() );
			FILE_ASSERT     (  wxFileName::DirExists(path) );
			FILE_ASSERT     (  ::wxDirExists(path) );
			FILE_ASSERT     ( !filename.FileExists() );
			FILE_ASSERT     ( !::wxFileName::FileExists(path) );
			FILE_ASSERT     ( !::wxFileExists(path) );
		}
		{
			wxString path = SjTools::GetSilverjukeProgramDir() + wxT("Silverjuke.exe");     // existant file in existant dir - do not run this test if the program has not the name "silverjuke.exe"
			if( ::wxFileExists(path) ) {
				wxFileName filename(path);
				FILE_ASSERT     ( !filename.IsDir() );
				FILE_ASSERT_NEW (  filename.DirExists() );
				FILE_ASSERT     ( !wxFileName::DirExists(path) );
				FILE_ASSERT     ( !::wxDirExists(path) );
				FILE_ASSERT     (  filename.FileExists() );
				FILE_ASSERT     (  ::wxFileName::FileExists(path) );
				FILE_ASSERT     (  ::wxFileExists(path) );
			}
		}
		{
			wxString path = SjTools::GetSilverjukeProgramDir() + wxT("unexistant.file");    // unexistant file in existant dir
			wxFileName filename(path);
			FILE_ASSERT     ( !filename.IsDir() );
			FILE_ASSERT_NEW (  filename.DirExists() );
			FILE_ASSERT     ( !wxFileName::DirExists(path) );
			FILE_ASSERT     ( !::wxDirExists(path) );
			FILE_ASSERT     ( !filename.FileExists() );
			FILE_ASSERT     ( !::wxFileName::FileExists(path) );
			FILE_ASSERT     ( !::wxFileExists(path) );
		}
		{
			wxString path = wxT("/unexistant/dir/");                                        // unexistant dir
			wxFileName filename(path);
			FILE_ASSERT     (  filename.IsDir() );
			FILE_ASSERT_NEW ( !filename.DirExists() );
			FILE_ASSERT     ( !wxFileName::DirExists(path) );
			FILE_ASSERT     ( !::wxDirExists(path) );
			FILE_ASSERT     ( !filename.FileExists() );
			FILE_ASSERT     ( !::wxFileName::FileExists(path) );
			FILE_ASSERT     ( !::wxFileExists(path) );
		}
		{
			wxString path = wxT("/unexistant/unexistant.file");                             // unexistant file in unexistant dir
			wxFileName filename(path);
			FILE_ASSERT( !filename.IsDir() );
			FILE_ASSERT( !filename.DirExists() );
			FILE_ASSERT( !wxFileName::DirExists(path) );
			FILE_ASSERT( !::wxDirExists(path) );
			FILE_ASSERT( !filename.FileExists() );
			FILE_ASSERT( !::wxFileName::FileExists(path) );
			FILE_ASSERT( !::wxFileExists(path) );
		}
#undef FILE_ASSERT
	}


	/* Check the sqlite "file open bug" by a functionality test (force testing unicode in sqlite) */
	{
		wxLogInfo(wxT("Testdrive: sqlite tests ..."));

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
				if( !sql.Next() )
					wxLogWarning(wxT("Testdrive: sqlite3 LIKE-operator is not case insenssitive for german umlauts!"));

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
			if( db.Ok() )
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
	{
		wxLogInfo(wxT("Testdrive: Scripting tests ..."));

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

		if( see.GetResultString() != wxT("blablub") )
			wxLogWarning(wxT("Testdrive: SjSee::Execute(\"function test ...\") failed"));


		wxString testStr;
		testStr.Append(0xF6); // small german "o" with two points
		testStr.Append(0x1F82); // greek extended alpha
		see.Execute(wxT("test1='") + testStr + wxT("'; test2=test1; test2=encodeURI(test2); test2=decodeURI(test2); test1==test2;"));
		if( !see.GetResultLong() )
		{
			wxLogWarning(wxT("Testdrive: encodeURI()/decodeURI() failed, see http://www.silverjuke.net/forum/topic-3234.html"));
		}
	}
#else
	wxLogInfo(wxT("Testdrive: Script tests skipped - scripts are not supported in this build."));
#endif

	// Check if the floating point library is loaded and working well.
	// Under some circumstances, the library is not loaded when it is nedded; this results in
	// the error "R6002 - floating point support not loaded", see and http://www.silverjuke.net/forum/topic-3615.html
	{
		double dummyfloat;
		dummyfloat = 17.5f;
		wxString dummystring = wxString::Format(wxT("%f"), dummyfloat);
		dummystring.Clear();
		wxLogInfo(wxT("Testdrive: Floating point tests ...") + dummystring /*make sure, it is referenced*/);
	}

	/* Done */
	wxLogInfo(wxT("Testdrive: Done."));

#endif // of (***)
}

