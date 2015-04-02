/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/


#include "tg_tagger_base.h"
#include "tg_bytevector.h"

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayByteVector);


// This is a bit ugly to keep writing over and over again.
//
// A rather obscure feature of the C++ spec that I hadn't thought of that makes
// working with C libs much more effecient.  There's more here:
//
// http://www.informit.com/isapi/product_id~{9C84DAB4-FE6E-49C5-BB0A-FB50331233EA}/content/index.asp
#define DATA(x) (&(x->data[0]))


static const SjUint crcTable[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};


class SjByteVectorData
{
public:
	SjByteVectorData    ();
	~SjByteVectorData   () { delete m_data; }

	void            clear               () { m_size = 0; }
	void            appendArray         (const unsigned char* data, int size);
	void            appendChar          (unsigned char value, int repeat);

	void            ref                 () { m_refCount++; }
	bool            deref               () { return ! --m_refCount ; }

#define         DATA_INCR_BYTES 512
	unsigned char*  m_data;
	int             m_size;
	int             m_allocated;
	int             m_refCount;
};


SjByteVectorData::SjByteVectorData()
{
	m_data      = (unsigned char*)malloc(DATA_INCR_BYTES);
	m_size      = 0;
	m_allocated = DATA_INCR_BYTES;
	m_refCount  = 1;
}


void SjByteVectorData::appendArray(const unsigned char* data, int size)
{
	if( size > 0 )
	{
		if( size > (m_allocated-m_size) )
		{
			m_allocated += size+DATA_INCR_BYTES;
			m_data = (unsigned char*)realloc(m_data, m_allocated);
		}

		memcpy(m_data+m_size, data, size);
		m_size += size;
	}
}


void SjByteVectorData::appendChar(unsigned char value, int repeat)
{
	if( repeat > 0 )
	{
		if( repeat > (m_allocated-m_size) )
		{
			unsigned char* new_data = (unsigned char*)realloc(m_data, m_allocated+repeat+DATA_INCR_BYTES);
			if( new_data == NULL )
				return; // error, cannot allocate

			m_data = new_data;
			m_allocated += repeat+DATA_INCR_BYTES;
		}

		memset(m_data+m_size, value, repeat);
		m_size += repeat;
	}
}


void SjByteVector::appendChar(unsigned char value, int repeat)
{
	d->appendChar(value, repeat);
}


////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////


SjByteVector SjByteVector::null;


SjByteVector SjByteVector::fromCString(const char *s, SjUint length)
{
	SjByteVector v;

	if(length == 0xffffffff)
		v.setData((const unsigned char*)s);
	else
		v.setData((const unsigned char*)s, length);

	return v;
}


#define fromNumber \
    size_t/*using int here causes a compiler fault in MSW release?!*/ i, valueBytes = sizeof(value); \
    SjByteVector v(valueBytes, 0); \
    for(i = 0; i < valueBytes; i++) \
    { \
        v[i] = (unsigned char)(value >> ((mostSignificantByteFirst ? valueBytes - 1 - i : i) * 8) & 0xff); \
    } \
    return v;


SjByteVector SjByteVector::fromUint(SjUint value, bool mostSignificantByteFirst)
{
	fromNumber;
}


SjByteVector SjByteVector::fromShort(short value, bool mostSignificantByteFirst)
{
	fromNumber;
}


SjByteVector SjByteVector::fromLongLong(wxLongLong value__, bool mostSignificantByteFirst)
{
	wxLongLong_t value = value__.GetValue();
	fromNumber;
}


////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


SjByteVector::SjByteVector()
{
	d = new SjByteVectorData();
}


SjByteVector::SjByteVector(SjUint size, unsigned char value)
{
	d = new SjByteVectorData();
	d->appendChar(value, size);
}


SjByteVector::SjByteVector(const SjByteVector &v) : d(v.d)
{
	d->ref();
}


SjByteVector::SjByteVector(unsigned char c)
{
	d = new SjByteVectorData();
	d->appendChar(c, 1);
}


SjByteVector::SjByteVector(const unsigned char *data, SjUint length)
{
	d = new SjByteVectorData();
	setData(data, length);
}


SjByteVector::SjByteVector(const unsigned char *data)
{
	d = new SjByteVectorData();
	setData(data);
}


SjByteVector::~SjByteVector()
{
	if( d->deref() )
	{
		delete d;
	}
}

void SjByteVector::setData(const unsigned char* data, SjUint length)
{
	detach();

	d->clear();
	d->appendArray(data, length);
}

void SjByteVector::setData(const unsigned char *data)
{
	setData(data, strlen((const char*)data));
}

unsigned char* SjByteVector::getWriteableData()
{
	detach();
	return d->m_data;
}

const unsigned char *SjByteVector::getReadableData() const
{
	return d->m_data;
}


SjByteVector SjByteVector::mid(SjUint index, SjUint length) const
{
	SjByteVector v;

	/*
	if( index > size() )
	    return v;

	ConstIterator endIt;

	if(length < 0xffffffff && length + index < size())
	    endIt = d->data.begin() + index + length;
	else
	    endIt = d->data.end();

	v.d->data.insert(v.d->data.begin(), ConstIterator(d->data.begin() + index), endIt);
	v.d->size = v.d->data.size();
	*/

	long bytesAvailAfterIndex = (long)d->m_size - (long)index;
	if( bytesAvailAfterIndex > 0 )
	{
		SjUint bytesToCopy = length;
		if( bytesToCopy > (SjUint)bytesAvailAfterIndex )
		{
			bytesToCopy = bytesAvailAfterIndex;
		}

		// append already checks that bytesToCopy is larget than 0!
		v.d->appendArray(&d->m_data[index], bytesToCopy);
	}

	return v;
}



unsigned char SjByteVector::at(SjUint index) const
{
	return index < size() ? d->m_data[index] : 0;
}


bool SjByteVector::containsAt(const SjByteVector& pattern, SjUint offset, SjUint patternOffset, SjUint patternLength) const
{
	if(pattern.size() < patternLength)
		patternLength = pattern.size();

	// do some sanity checking -- all of these things are needed for the search to be valid

	if(patternLength > size() || offset >= size() || patternOffset >= pattern.size() || patternLength == 0)
		return false;

	// loop through looking for a mismatch

	for(SjUint i = 0; i < patternLength - patternOffset; i++) {
		if(at(i + offset) != pattern.d->m_data[i + patternOffset])
			return false;
	}

	return true;
}



bool SjByteVector::startsWith(const SjByteVector &pattern) const
{
	return containsAt(pattern, 0);
}

bool SjByteVector::endsWith(const SjByteVector &pattern) const
{
	return containsAt(pattern, size() - pattern.size());
}



int SjByteVector::endsWithPartialMatch(const SjByteVector &pattern) const
{
	if(pattern.size() > size())
		return -1;

	const int startIndex = size() - pattern.size();

	// try to match the last n-1 bytes from the vector (where n is the pattern
	// size) -- continue trying to match n-2, n-3...1 bytes

	for(SjUint i = 1; i < pattern.size(); i++) {
		if(containsAt(pattern, startIndex + i, 0, pattern.size() - i))
			return startIndex + i;
	}

	return -1;
}


void SjByteVector::append(const SjByteVector &v)
{
	detach();
	d->appendArray(v.d->m_data, v.d->m_size);
}

void SjByteVector::clear()
{
	detach();
	d->clear();
}

SjUint SjByteVector::size() const
{
	return d->m_size;
}


SjByteVector& SjByteVector::resize(SjUint size, unsigned char padding)
{
	if((SjUint)d->m_size < size)
	{
		d->appendChar(padding, size - d->m_size);
	}

	d->m_size = size;

	return *this;
}


/*
SjByteVector::Iterator SjByteVector::begin()
{
  return d->data.begin();
}

SjByteVector::ConstIterator SjByteVector::begin() const
{
  return d->data.begin();
}

SjByteVector::Iterator SjByteVector::end()
{
  return d->data.end();
}

SjByteVector::ConstIterator SjByteVector::end() const
{
  return d->data.end();
}
*/

bool SjByteVector::isNull() const
{
	return d == null.d;
}

bool SjByteVector::isEmpty() const
{
	return d->m_size == 0;
}

SjUint SjByteVector::checksum() const
{
	SjUint sum = 0;
	/*for(SjByteVector::ConstIterator it = begin(); it != end(); ++it)
	  sum = (sum << 8) ^ crcTable[((sum >> 24) & 0xff) ^ uchar(*it)];*/
	int i, iCount = d->m_size;
	const unsigned char* data = d->m_data;
	for( i = 0; i < iCount; i++ )
	{
		sum = (sum << 8) ^ crcTable[((sum >> 24) & 0xff) ^ data[i]];
	}
	return sum;
}

/*template <class T>
T toNumber(const std::vector<char> &data, bool mostSignificantByteFirst)
{
  T sum = 0;





  for(SjUint i = 0; i <= last; i++)
    sum |= (T) uchar(data[i]) << ((mostSignificantByteFirst ? last - i : i) * 8);

  return sum;
}*/

#define toNumber(T) \
    T sum = 0; \
    if(d->m_size <= 0) { \
        wxLogDebug(wxT("toNumber() -- data is empty, returning 0")); \
        return sum; \
    } \
    int size = sizeof(T), i; \
    int last = d->m_size > size ? size - 1 : d->m_size - 1; \
    for(i = 0; i <= last; i++) \
    { \
        sum |= (T) (unsigned char)(d->m_data[i]) << ((mostSignificantByteFirst ? last - i : i) * 8); \
    }



SjUint SjByteVector::toUInt(bool mostSignificantByteFirst) const
{
	toNumber(SjUint);
	return sum;
}

short SjByteVector::toShort(bool mostSignificantByteFirst) const
{
	toNumber(short);
	return sum;
}

wxLongLong_t SjByteVector::toLongLong(bool mostSignificantByteFirst) const
{
	toNumber(wxLongLong_t);
	return sum;
}

wxString SjByteVector::toString(SjStringType stringType) const
{
	return toString(stringType, d->m_data, d->m_size);
}

wxString SjByteVector::toString(SjStringType stringType, const void* m_data, int m_size)
{
	// the string may or may not be null-terminated!
	if( m_data == NULL )
		return wxT("");

	if( m_size < 0 )
	{
		if( stringType == SJ_UTF16 || stringType == SJ_UTF16LE || stringType == SJ_UTF16BE )
		{
			const unsigned short* s = (const unsigned short*)m_data;
			const unsigned short* e = s;
			while(*e)
				e++;
			m_size = e-s;
		}
		else
		{
			const unsigned char* s = (const unsigned char*)m_data;
			const unsigned char* e = s;
			while(*e)
				e++;
			m_size = e-s;
		}
	}

	// if it is not null-terminated, everything up to size() goes to the string
	wxString ret;
	const unsigned char*  achar = (const unsigned char*)m_data;
	const unsigned char*  acharEnd = &achar[m_size];
	unsigned short wchar;
	ret.Alloc(m_size+1); // just an asumption, in characters; too much for unicode, however ...
	switch( stringType )
	{
		default:
#ifdef __WXDEBUG__
		{
			static bool warningPrinted = FALSE;
			if( !warningPrinted )
			{
				wxLogError(wxT("Unsupported string decoding #%i."), (int)stringType);
				warningPrinted = TRUE;
			}
		}
#endif
		// fall through to latin-1

		case SJ_LATIN1:
			while( *achar && achar < acharEnd )
			{
				ret << (wxChar)(*achar);
				achar++;
			}
			break;

		case SJ_UTF16:
			if( m_size > 1 )
			{
				if( achar[0] == 0xFE && achar[1] == 0xFF )
				{
					achar += 2;
					goto SJ_UTF16BE_Label;
					// do big endian decoding
				}
				else if( achar[0] == 0xFF && achar[1] == 0xFE )
				{
					achar += 2;
				}
			}
			// fall through for little endian decoding

		case SJ_UTF16LE:
			while( achar < acharEnd )
			{
				wchar = (achar[1] << 8) | achar[0];
				if( wchar == 0 )
				{
					break;
				}
				else if( wchar != 0xfffe && wchar != 0xfeff )
				{
#if wxUSE_UNICODE
					ret << (wxChar)wchar;
#else
					ret << (wxChar)(wchar>0xFF? '?' : wchar);
#endif
				}
				achar += 2;
			}
			break;

		case SJ_UTF16BE:
SJ_UTF16BE_Label:
			while( achar < acharEnd )
			{
				wchar = (achar[0] << 8) | achar[1];
				if( wchar == 0 )
				{
					break;
				}
				else if( wchar != 0xfffe && wchar != 0xfeff )
				{
#if wxUSE_UNICODE
					ret << (wxChar)wchar;
#else
					ret << (wxChar)(wchar>0xFF? '?' : wchar);
#endif
				}
				achar += 2;
			}
			break;

		case SJ_UTF8:
		{
			// only convert up to the first null-byte - eg. the APE tagger relies on this at (***)
			const unsigned char* test = achar;
			while( *test && test < acharEnd )
				test ++;
			// convert to string
			ret = wxString((const char*)achar, wxConvUTF8, (long)(test-achar));
		}
#if 0
		{
			// wx seems to have errors if the string is not null-terminated
			wchar_t* wcharBuf = (wchar_t*)malloc((d->m_size+1)*sizeof(wchar_t)/*worst case*/);
			if( wcharBuf )
			{
				size_t wcharBufSize = wxConvUTF8.MB2WC(wcharBuf, (const char*)achar, d->m_size);
				if( wcharBufSize > 0 && wcharBufSize <= (size_t)d->m_size )
				{
					wcharBuf[wcharBufSize] = 0;
					ret = wxString(wcharBuf);
				}
				free(wcharBuf);
			}
		}
#endif
		break;
	}

	return ret;
}


void SjByteVector::appendString(const wxString& s, SjStringType appendedEncoding)
{
	int iCount, i;
	unsigned short wchar;
	switch( appendedEncoding )
	{
		case SJ_LATIN1:
#if wxUSE_UNICODE
			iCount = s.Len();
			for( i = 0; i < iCount; i++ )
			{
				wchar = s.GetChar(i);
				d->appendChar(wchar>0xFF? '?' : wchar, 1);
			}
			// d->append((unsigned char)0, 1); -- no implicit null-characters!
#else
			append((unsigned char*)s.c_str());
#endif
			break;

		case SJ_UTF16:
			// assume little endian and add the Byte-Order-Mark
			d->appendChar(0xff, 1);
			d->appendChar(0xfe, 1);
			// fall through
		case SJ_UTF16LE:
			iCount = s.Len();
			for( i = 0; i < iCount; i++ )
			{
				wchar = (unsigned short)s.GetChar(i);
				d->appendChar(wchar & 0xff, 1);
				d->appendChar(wchar >> 8, 1);
			}
			// d->append((unsigned char)0, 2); -- no implicit null-characters!
			break;

		case SJ_UTF16BE:
			iCount = s.Len();
			for( i = 0; i < iCount; i++ )
			{
				wchar = (unsigned short)s.GetChar(i);
				d->appendChar(wchar >> 8, 1);
				d->appendChar(wchar & 0xff, 1);
			}
			// d->append((unsigned char)0, 2); -- no implicit null-characters!
			break;

		case SJ_UTF8:
		{
#if wxUSE_UNICODE
			wxCharBuffer tempBuffer = s.mb_str(wxConvUTF8);
			const char* tempPtr = tempBuffer.data();
			d->appendArray((const unsigned char*)tempPtr, strlen(tempPtr));
#else
			iCount = s.Len();
			wxASSERT( sizeof(wchar_t) >= sizeof(unsigned short) );
			wchar_t* wbuf = (wchar_t*)malloc((iCount+1)*sizeof(wchar_t)/*worst case*/);
			char* utf8Buf = (char*)malloc((iCount+1)*10/*worst case*/);
			if( wbuf && utf8Buf )
			{
				for( i = 0; i < iCount; i++ )
				{
					wbuf[i] = (wchar_t)s.GetChar(i);
				}
				wbuf[i] = 0;

				size_t charBufSize = wxConvUTF8.WC2MB(utf8Buf, wbuf, iCount);

				d->appendArray((unsigned char*)utf8Buf, charBufSize);

				free(utf8Buf);
				free(wbuf);
			}
#endif
		}
		break;

		default:
#ifdef __WXDEBUG__
		{
			static bool warningPrinted = FALSE;
			if( !warningPrinted )
			{
				wxLogError(wxT("Unsupported string encoding %i."), (int)appendedEncoding);
				warningPrinted = TRUE;
			}
		}
#endif
		break;
	}
}






const unsigned char& SjByteVector::operator[](int index) const
{
	return d->m_data[index];
}


unsigned char& SjByteVector::operator[](int index)
{
	detach();
	return d->m_data[index];
}


bool SjByteVector::operator==(const SjByteVector &v) const
{
	if(d->m_size != v.d->m_size)
		return false;

	return (memcmp(d->m_data, v.d->m_data, d->m_size) == 0);
}

bool SjByteVector::operator!=(const SjByteVector &v) const
{
	return !operator==(v);
}

bool SjByteVector::operator==(const char *s) const
{
	if(d->m_size != (int)::strlen(s))
		return false;

	return (memcmp(d->m_data, s, d->m_size) == 0);
}

bool SjByteVector::operator!=(const char *s) const
{
	return !operator==(s);
}

bool SjByteVector::operator<(const SjByteVector &v) const
{
	int result = ::memcmp(d->m_data, v.d->m_data, d->m_size<v.d->m_size? d->m_size : v.d->m_size);

	if(result != 0)
		return result < 0;
	else
		return d->m_size < v.d->m_size;
}

bool SjByteVector::operator>(const SjByteVector &v) const
{
	return v < *this;
}

SjByteVector SjByteVector::operator+(const SjByteVector &v) const
{
	SjByteVector sum(*this);
	sum.append(v);
	return sum;
}

SjByteVector &SjByteVector::operator=(const SjByteVector &v)
{
	if(&v == this)
		return *this;

	if(d->deref())
		delete d;

	d = v.d;
	d->ref();
	return *this;
}

SjByteVector &SjByteVector::operator=(unsigned char c)
{
	if(d->deref())
		delete d;
	*this = SjByteVector(c);
	return *this;
}

SjByteVector &SjByteVector::operator=(const unsigned char *data)
{
	if(d->deref())
		delete d;
	*this = SjByteVector(data);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void SjByteVector::detach()
{
	if( d->m_refCount > 1 )
	{
		unsigned char*  data = d->m_data;
		int             size = d->m_size;

		d->deref();

		d = new SjByteVectorData();
		d->appendArray(data, size);
	}
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

/*
std::ostream &operator<<(std::ostream &s, const ByteVector &v)
{
  for(unsinged int i = 0; i < v.size(); i++)
    s << v[i];
  return s;
}
*/




/*******************************************************************************
 *  Searching forward
 ******************************************************************************/



int SjByteVector::find(const SjByteVector &pattern, SjUint offset, int byteAlign) const
{
	wxASSERT( pattern.size() > 0 );

	if(pattern.size() > size() || pattern.size() <=0 || offset >= size() - 1)
		return -1;

	// if an offset was specified, just do a recursive call on the substring

	if(offset > 0) {

		// start at the next byte aligned block

		SjByteVector section = mid(offset + byteAlign - 1 - offset % byteAlign);
		int match = section.find(pattern, 0, byteAlign);
		return match >= 0 ? int(match + offset) : -1;
	}

	// this is a simplified Boyer-Moore string searching algorithm

	unsigned char lastOccurrence[256];

	SjUint i;
	for(i = 0; i < 256; ++i)
		lastOccurrence[i] = (unsigned char)(pattern.size());

	for(i = 0; i < pattern.size() - 1; ++i)
		lastOccurrence[unsigned(pattern[i])] = (unsigned char)(pattern.size() - i - 1);

	for(i = pattern.size() - 1; i < size(); i += lastOccurrence[(unsigned char)(at(i))]) {
		int iBuffer = i;
		int iPattern = pattern.size() - 1;

		while(iPattern >= 0 && at(iBuffer) == pattern[iPattern]) {
			--iBuffer;
			--iPattern;
		}

		if(-1 == iPattern && (iBuffer + 1) % byteAlign == 0)
			return iBuffer + 1;
	}

	return -1;
}



/*******************************************************************************
 *  Searching backward
 ******************************************************************************/



/*!
 * Wraps the accessors to a ByteVector to make the search algorithm access the
 * elements in reverse.
 *
 * \see vectorFind()
 * \see SjByteVector::rfind()
 */
class ByteVectorMirror
{
public:
	ByteVectorMirror(const SjByteVector &source) : v(source) {}
	const char operator[](int index) const
	{
		return v[v.size() - index - 1];
	}

	const char at(int index) const
	{
		return v.at(v.size() - index - 1);
	}

	ByteVectorMirror mid(SjUint index, SjUint length = 0xffffffff) const
	{
		return length == 0xffffffff ? v.mid(0, index) : v.mid(index - length, length);
	}

	SjUint size() const
	{
		return v.size();
	}

	int find(const ByteVectorMirror &pattern, SjUint offset = 0, int byteAlign = 1) const;

private:
	const SjByteVector v;
};


static int mirrorVectorFind(const ByteVectorMirror &v, const ByteVectorMirror &pattern, SjUint offset, int byteAlign)
{
	wxASSERT( pattern.size() > 0 );

	if(pattern.size() > v.size() || pattern.size() <= 0 || offset >= v.size() - 1)
		return -1;

	// if an offset was specified, just do a recursive call on the substring

	if(offset > 0) {

		// start at the next byte aligned block

		ByteVectorMirror section = v.mid(offset + byteAlign - 1 - offset % byteAlign);
		int match = section.find(pattern, 0, byteAlign);
		return match >= 0 ? int(match + offset) : -1;
	}

	// this is a simplified Boyer-Moore string searching algorithm

	unsigned char lastOccurrence[256];

	SjUint i;
	for(i = 0; i < 256; ++i)
		lastOccurrence[i] = (unsigned char)(pattern.size());

	for(i = 0; i < pattern.size() - 1; ++i)
		lastOccurrence[unsigned(pattern[i])] = (unsigned char)(pattern.size() - i - 1);

	for(i = pattern.size() - 1; i < v.size(); i += lastOccurrence[(unsigned char)(v.at(i))]) {
		int iBuffer = i;
		int iPattern = pattern.size() - 1;

		while(iPattern >= 0 && v.at(iBuffer) == pattern[iPattern]) {
			--iBuffer;
			--iPattern;
		}

		if(-1 == iPattern && (iBuffer + 1) % byteAlign == 0)
			return iBuffer + 1;
	}

	return -1;
}


int ByteVectorMirror::find(const ByteVectorMirror &pattern, SjUint offset, int byteAlign) const
{
	ByteVectorMirror v(*this);

	const int pos = mirrorVectorFind(v, pattern, offset, byteAlign);

	// If the offset is zero then we need to adjust the location in the search
	// to be appropriately reversed.  If not we need to account for the fact
	// that the recursive call (called from the above line) has already ajusted
	// for this but that the normal templatized find above will add the offset
	// to the returned value.
	//
	// This is a little confusing at first if you don't first stop to think
	// through the logic involved in the forward search.

	if(pos == -1)
		return -1;

	if(offset == 0)
		return size() - pos - pattern.size();
	else
		return pos - offset;
}



int SjByteVector::rfind(const SjByteVector &pattern, SjUint offset, int byteAlign) const
{
	// Ok, this is a little goofy, but pretty cool after it sinks in.  Instead of
	// reversing the find method's Boyer-Moore search algorithm I created a "mirror"
	// for a ByteVector to reverse the behavior of the accessors.

	ByteVectorMirror v(*this);
	ByteVectorMirror p(pattern);

	return v.find(p, offset, byteAlign);
}



/*******************************************************************************
 *  Searching backward
 ******************************************************************************/



SjArrayByteVector SjByteVector::splitToArray(const SjByteVector& pattern, int byteAlign) const
{
	return splitToArray(pattern, byteAlign, 0);
}



SjArrayByteVector SjByteVector::splitToArray(const SjByteVector& pattern, int byteAlign, int max) const
{
	SjArrayByteVector l;

	SjUint previousOffset = 0;
	for(int offset = find(pattern, 0, byteAlign);
	        offset != -1 && (max == 0 || max > int(l.GetCount()) + 1);
	        offset = find(pattern, offset + pattern.size(), byteAlign))
	{
		l.Add(mid(previousOffset, offset - previousOffset));
		previousOffset = offset + pattern.size();
	}

	if(previousOffset < size())
		l.Add(mid(previousOffset, size() - previousOffset));

	return l;
}



wxArrayString SjByteVector::splitToStrings(const SjByteVector& pattern, SjStringType enc) const
{
	SjArrayByteVector a = splitToArray(pattern, (enc==SJ_UTF8||enc==SJ_LATIN1)? 1 : 2);
	int i, iCount = a.GetCount();
	wxArrayString ret;
	for( i = 0; i < iCount; i++ )
	{
		ret.Add(a.Item(i).toString(enc));
	}
	return ret;
}

