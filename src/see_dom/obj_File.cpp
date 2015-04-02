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
 * File:    obj_File.cpp
 * Authors: Björn Petersen
 * Purpose: The File class for SEE
 *
 ******************************************************************************/



#include "../basesj.h"
#include "../tools/bytefile.h"
#include "sj_see.h"
#include "sj_see_helpers.h"

// data used by our object
struct file_object
{
	SEE_native      native; // MUST be first!!!
	SEE_value       name_;
	bool            binary_;
	long            backupPos;
	wxFSFile*       fsFile;
	SjByteFile*     byteFile;
};
static void file_finalize(SEE_interpreter* interpr, void* ptr, void* closure)
{
	file_object* fo = (file_object*)ptr;
	delete fo->byteFile;
	delete fo->fsFile;
}
static file_object* alloc_file_object(SEE_interpreter* interpr, SEE_value* name_, bool binary_)
{
	file_object* fo = (file_object*)SEE_malloc_finalize(interpr, sizeof(file_object), file_finalize, NULL);
	memset(fo, 0, sizeof(file_object));
	if( name_ )
		SEE_VALUE_COPY(&fo->name_, name_);
	fo->binary_ = binary_;
	return fo;
}
static file_object* toFile(SEE_interpreter* interpr, SEE_object* o, int init);



/*******************************************************************************
 *  File Object Functions
 ******************************************************************************/



IMPLEMENT_FUNCTION(file, construct)
{
	RETURN_OBJECT( HOST_DATA->File_new(
	                   argc_>=1? argv_[0] : NULL,  /*name*/
	                   ARG_LONG(1)!=0              /*binary?*/
	               ) );
}

IMPLEMENT_FUNCTION(file, read)
{
	file_object* fo = toFile(interpr_, this_, 1/*init*/);
	if( fo->byteFile == NULL )
	{
		RETURN_STRING(wxT(""));
		return;
	}

	// find out the number of bytes to read
	long bytesToRead;
	if( argc_ >= 1 )
	{
		bytesToRead = ARG_LONG(0);
	}
	else
	{
		long startPos = fo->byteFile->Tell();
		long endPos = fo->byteFile->Find((unsigned char)'\n', startPos);
		if( endPos == -1 )
			endPos = fo->byteFile->Length();
		else
			endPos++;
		bytesToRead = endPos-startPos;
		fo->byteFile->Seek(startPos);
	}

	if( bytesToRead <= 0 )
	{
		RETURN_STRING(wxT(""));
		return;
	}

	// read the data
	SjByteVector                src = fo->byteFile->ReadBlock(bytesToRead);

	// convert the data to a string for SEE
	if( fo->binary_ )
	{
		long                    srcBytes = src.size(), i;
		const unsigned char*    srcPtr = src.getReadableData();
		SEE_string*             dest = SEE_string_new(interpr_, srcBytes); // the object is freed in the garbage collection
		SEE_char_t*             destPtr = dest->data;

		for( i = 0; i < srcBytes; i++ )
			destPtr[i] = srcPtr[i];
		dest->length = srcBytes;

		SEE_SET_STRING(res_, dest);
	}
	else
	{
		wxString dest = src.toString(SJ_UTF8);

		RETURN_STRING( dest );
	}
}

IMPLEMENT_FUNCTION(file, write)
{
	file_object* fo = toFile(interpr_, this_, 2/*init+create*/);
	if( fo->byteFile == NULL )
		SEE_error_throw(interpr_, interpr_->Error, "cannot write");

	if( argc_>=1 && argv_[0] )
	{
		if( SEE_VALUE_GET_TYPE(argv_[0]) == SEE_STRING )
		{
			SEE_string*         src = argv_[0]->u.string;
			if( fo->binary_ )
			{
				SEE_string*     src = argv_[0]->u.string;
				long            srcLen = src->length, i;
				SEE_char_t*     srcPtr = src->data;
				SjByteVector    dest((SjUint)srcLen);
				unsigned char*  destPtr = dest.getWriteableData();

				for( i = 0; i < srcLen; i++ )
					destPtr[i] = (unsigned char)srcPtr[i];

				fo->byteFile->WriteBlock(dest);
			}
			else
			{
				SEE_size_t      srcLen = SEE_string_utf8_size(interpr_, src) + 1/*SEE_string_utf8_size() does not add a nullbytes, but SEE_string_toutf8() does*/;
				SjByteVector    dest((SjUint)srcLen);
				unsigned char*  destPtr = dest.getWriteableData();

				SEE_string_toutf8(interpr_, (char*)destPtr, srcLen, src);
				dest.resize(srcLen - 1/*do not write the implicit null-byte*/);

				fo->byteFile->WriteBlock(dest);
			}
		}
		else
		{
			SjByteVector dest;
			dest.appendChar((unsigned char)ARG_LONG(0), 1);

			fo->byteFile->WriteBlock(dest);
		}
	}
	RETURN_UNDEFINED;
}

IMPLEMENT_FUNCTION(file, flush)
{
	file_object* fo = toFile(interpr_, this_, 0/*no init*/);
	if( fo->byteFile )
	{
		fo->backupPos = fo->byteFile->Tell();

		delete fo->byteFile;
		fo->byteFile = NULL;

		delete fo->fsFile;
		fo->fsFile = NULL;
	}
	RETURN_UNDEFINED;
}



/*******************************************************************************
 *  File Object Properties
 ******************************************************************************/


IMPLEMENT_HASPROPERTY(file)
{
	if(
	    VAL_PROPERTY( length )
	    || VAL_PROPERTY( pos )
	)
	{
		RETURN_HAS;
	}
	else
	{
		RETURN_HASNOT;
	}
}



IMPLEMENT_GET(file)
{
	if( VAL_PROPERTY( length ) )
	{
		file_object* fo = toFile(interpr_, this_, 1/*init*/);
		if( fo->byteFile )
			RETURN_LONG( fo->byteFile->Length() );
		else
			RETURN_LONG( 0 );
	}
	else if( VAL_PROPERTY( pos ) )
	{
		file_object* fo = toFile(interpr_, this_, 1/*init*/);
		if( fo->byteFile )
			RETURN_LONG( fo->byteFile->Tell() );
		else
			RETURN_LONG( 0 );
	}
	else
	{
		RETURN_GET_DEFAULTS;
	}
}



IMPLEMENT_PUT(file)
{
	if( VAL_PROPERTY( length ) )
	{
		long l = VAL_LONG;
		file_object* fo = toFile(interpr_, this_, 2/*init+create*/);
		if( fo->byteFile == NULL || l < 0 )
			SEE_error_throw(interpr_, interpr_->Error, "cannot set length");

		fo->byteFile->Truncate(l);
		fo->byteFile->Seek(l, SJ_SEEK_BEG);
	}
	else if( VAL_PROPERTY( pos ) )
	{
		long l = VAL_LONG;
		file_object* fo = toFile(interpr_, this_, 2/*init+create*/);
		if( fo->byteFile == NULL )
			SEE_error_throw(interpr_, interpr_->Error, "cannot seek");

		if( l >= 0 )
			fo->byteFile->Seek(l, SJ_SEEK_BEG);
		else
			fo->byteFile->Seek(l, SJ_SEEK_END); // not: "Seek(l*-1, SJ_SEEK_END)", to move in-file, SJ_SEEK_END expects negative values, see http://www.silverjuke.net/forum/topic-2630.html
	}
	else
	{
		DO_PUT_DEFAULTS;
	}
}


/*******************************************************************************
 *  Statics
 ******************************************************************************/


IMPLEMENT_FUNCTION(file, exists)
{
	wxString f(ARG_STRING(0));
	RETURN_BOOL( ::wxFileExists(f) || ::wxDirExists(f) );
}



IMPLEMENT_FUNCTION(file, isdir)
{
	RETURN_BOOL( ::wxDirExists(ARG_STRING(0)) );
}



IMPLEMENT_FUNCTION(file, dir)
{
	wxArrayString ret;
	wxString path = ARG_STRING(0);
	if( !path.IsEmpty() )
	{
		if( path.Last()!=wxT('/') && path.Last()!=wxT('\\') )
			path += wxT('/');
		wxFileSystem fs;
		fs.ChangePathTo(path);

		int what = wxFILE;
		switch( ARG_LONG(1) )
		{
			case 1: what = wxDIR; break;
			case 2: what = 0;/*both*/ break;
		}
		path = fs.FindFirst(wxT("*"), what);
		while( !path.IsEmpty() )
		{
			ret.Add(path);
			path = fs.FindNext();
		}
	}

	RETURN_ARRAY_STRING( ret );
}



IMPLEMENT_FUNCTION(file, mkdir)
{
	RETURN_BOOL( ::wxMkdir(ARG_STRING(0)) );
}



IMPLEMENT_FUNCTION(file, copy)
{
	RETURN_BOOL( SjTools::CopyFile(ARG_STRING(0), ARG_STRING(1)) );
}



IMPLEMENT_FUNCTION(file, rename)
{
	RETURN_BOOL( ::wxRenameFile(ARG_STRING(0), ARG_STRING(1)) );
}



IMPLEMENT_FUNCTION(file, remove)
{
	wxString f(ARG_STRING(0));
	if( ::wxDirExists(f) )
		RETURN_BOOL( ::wxRmdir(f) );
	else
		RETURN_BOOL( ::wxRemoveFile(f) );
}



IMPLEMENT_FUNCTION(file, time)
{
	RETURN_LONG( ::wxFileModificationTime(ARG_STRING(0)) );
}



/*******************************************************************************
 *  Let SEE know about this class (this part is a little more complicated)
 ******************************************************************************/



/* object class for File.prototype and file instances */
static SEE_objectclass file_inst_class = {
	"File",                     /* Class */
	file_get,                   /* Get */
	file_put,                   /* Put */
	SEE_native_canput,          /* CanPut */
	file_hasproperty,           /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	NULL,                       /* Construct */
	NULL,                       /* Call */
	NULL                        /* HasInstance */
};



/* object class for File constructor */
static SEE_objectclass file_constructor_class = {
	"FileConstructor",          /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	file_construct,             /* Construct */
	NULL                        /* Call */
};



void SjSee::File_init()
{
	SEE_value temp;

	// Create the "File.prototype" object, this is used as a template for the instances
	m_File_prototype = (SEE_object*)alloc_file_object(m_interpr, NULL, false);

	SEE_native_init((SEE_native *)m_File_prototype, m_interpr,
	                &file_inst_class, m_interpr->Object_prototype);

	PUT_FUNC(m_File_prototype, file, read, 0);
	PUT_FUNC(m_File_prototype, file, write, 0);
	PUT_FUNC(m_File_prototype, file, flush, 0);

	// create the "File" object
	SEE_object* File = (SEE_object *)SEE_malloc(m_interpr, sizeof(SEE_native));

	SEE_native_init((SEE_native *)File, m_interpr,
	                &file_constructor_class, m_interpr->Object_prototype);

	PUT_OBJECT(File, str_prototype, m_File_prototype)
	PUT_FUNC(File, file, exists, 0); // static function
	PUT_FUNC(File, file, isdir, 0); // static function
	PUT_FUNC(File, file, dir, 0); // static function
	PUT_FUNC(File, file, mkdir, 0); // static function
	PUT_FUNC(File, file, copy, 0); // static function
	PUT_FUNC(File, file, rename, 0); // static function
	PUT_FUNC(File, file, remove, 0); // static function
	PUT_FUNC(File, file, time, 0); // static function

	// now we can add the globals
	PUT_OBJECT(m_interpr->Global, str_File,  File);
}



SEE_object* SjSee::File_new(SEE_value* name, bool binary)
{
	file_object* obj = alloc_file_object(m_interpr, name, binary);

	SEE_native_init(&obj->native, m_interpr,
	                &file_inst_class, m_File_prototype);

	return (SEE_object *)obj;
}



static file_object* toFile(SEE_interpreter* interpr, SEE_object* o, int init)
{
	if( o->objectclass != &file_inst_class )
		SEE_error_throw(interpr, interpr->TypeError, NULL);

	file_object* fo = (file_object*)o;

	if( init && fo->byteFile == NULL )
	{
		// alloc fsFile, this may fail
		wxASSERT( fo->fsFile == NULL );

		wxFileSystem fs;
		wxString name = SeeValueToWxString(interpr, &fo->name_);
		fo->fsFile = fs.OpenFile(name, wxFS_READ|wxFS_SEEKABLE);
		if( fo->fsFile == NULL && init == 2 )
		{
			// create the file physically and try again
			{ wxFile file; file.Create(name); }
			fo->fsFile = fs.OpenFile(name, wxFS_READ|wxFS_SEEKABLE);
		}

		if( fo->fsFile )
		{
			// create byteFile, seek to old position if flush() was called before
			fo->byteFile = new SjByteFile(fo->fsFile, NULL);
			if( fo->backupPos )
				fo->byteFile->Seek(fo->backupPos, SJ_SEEK_BEG);
		}
	}

	return (file_object *)o;
}



