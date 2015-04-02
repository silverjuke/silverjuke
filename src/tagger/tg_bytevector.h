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



#ifndef SJ_BYTEVECTOR_H
#define SJ_BYTEVECTOR_H



#include <sjtools/types.h>



enum SjStringType
{
    // !!! do NOT modify these values !!! they're read directly from the mp3 files

    /*!
     * IS08859-1, or <i>Latin1</i> encoding.  8 bit characters.
     */
    SJ_LATIN1 = 0,
    /*!
     * UTF16 with a <i>byte order mark</i>.  16 bit characters.
     */
    SJ_UTF16 = 1,
    /*!
     * UTF16 <i>big endian</i>.  16 bit characters.  This is the encoding used
     * internally
     */
    SJ_UTF16BE = 2,
    /*!
     * UTF8 encoding.  Characters are usually 8 bits but can be up to 32.
     */
    SJ_UTF8 = 3,
    /*!
     * UTF16 <i>little endian</i>.  16 bit characters.
     */
    SJ_UTF16LE = 4,
};



class SjByteVectorData;
class SjArrayByteVector;



class SjByteVector
{
public:
	// Constructs an empty byte vector.
	SjByteVector();

	// Construct a vector of size size with all values set to value by default.
	SjByteVector(SjUint size, unsigned char value = 0);

	// Contructs a byte vector that is a copy of v.
	SjByteVector(const SjByteVector &v);

	// Contructs a byte vector that contains c.
	SjByteVector(unsigned char c);

	// Constructs a byte vector that copies data for up to length bytes.
	SjByteVector(const unsigned char *data, SjUint length);

	// Constructs a byte vector that copies data up to the first null
	// byte.  The behavior is undefined if data is not null terminated.
	// This is particularly useful for constructing byte arrays from string
	// constants.
	SjByteVector(const unsigned char *data);

	// Destroys this SjByteVector instance.
	~SjByteVector();

	// Sets the data for the byte array using the first length bytes of data
	void setData(const unsigned char *data, SjUint length);

	// Sets the data for the byte array copies data up to the first null
	// byte.  The behavior is undefined if data is not null terminated.
	void setData(const unsigned char *data);

	// Returns a pointer to the internal data structure.
	// Warning: Care should be taken when modifying this data structure as it is
	// easy to corrupt the SjByteVector when doing so.  Specifically, while the
	// data may be changed, its length may not be.
	unsigned char *getWriteableData();

	// Returns a pointer to the internal data structure which may not be modified.
	const unsigned char *getReadableData() const;

	// Returns a byte vector made up of the bytes starting at index and
	// for length bytes.  If length is not specified it will return the bytes
	// from index to the end of the vector.
	SjByteVector mid(SjUint index, SjUint length = 0xffffffff) const;

	// This essentially performs the same as operator[](), but instead of causing
	// a runtime error if the index is out of bounds, it will return a null byte.
	unsigned char at(SjUint index) const;

	// Searches the SjByteVector for pattern starting at offset and returns
	// the offset.  Returns -1 if the pattern was not found.  If byteAlign is
	// specified the pattern will only be matched if it starts on a byteDivisible
	// by byteAlign.
	int find(const SjByteVector &pattern, SjUint offset = 0, int byteAlign = 1) const;

	// Searches the SjByteVector for pattern starting from either the end of the
	// vector or offset and returns the offset.  Returns -1 if the pattern was
	// not found.  If byteAlign is specified the pattern will only be matched
	// if it starts on a byteDivisible by byteAlign.
	int rfind(const SjByteVector &pattern, SjUint offset = 0, int byteAlign = 1) const;

	// Checks to see if the vector contains the pattern starting at position
	// offset.  Optionally, if you only want to search for part of the pattern
	// you can specify an offset within the pattern to start from.  Also, you can
	// specify to only check for the first patternLength bytes of pattern with
	// the patternLength argument.
	bool containsAt(const SjByteVector &pattern, SjUint offset, SjUint patternOffset = 0, SjUint patternLength = 0xffffffff) const;

	// Returns true if the vector starts with pattern.
	bool startsWith(const SjByteVector &pattern) const;

	// Returns true if the vector ends with pattern.
	bool endsWith(const SjByteVector &pattern) const;

	// Checks for a partial match of pattern at the end of the vector.  It
	// returns the offset of the partial match within the vector, or -1 if the
	// pattern is not found.  This method is particularly useful when searching for
	// patterns that start in one vector and end in another.  When combined with
	// startsWith() it can be used to find a pattern that overlaps two buffers.
	//
	// Note:  This will not match the complete pattern at the end of the string; use
	// endsWith() for that.
	int endsWithPartialMatch(const SjByteVector &pattern) const;

	// Appends v to the end of the SjByteVector.
	void append(const SjByteVector &v);

	// appends the string data to the byte vector.
	// the string data are converted to the desired encoding before they're appended.
	// NO NULL-CHARACTERS ARE ADDED TO THE BYTE VECTOR!!!
	// (if you want to be them there, you have to add them yourself)
	void appendString(const wxString&, SjStringType appendedEncoding);
	void appendChar(unsigned char value, int repeat);

	// Clears the data.
	void clear();

	// Returns the size of the array.
	SjUint size() const;

	// Resize the vector to size.  If the vector is currently less than
	// size, pad the remaining spaces with padding.  Returns a reference
	// to the resized vector.
	SjByteVector &resize(SjUint size, unsigned char padding = 0);

	// Returns an Iterator that points to the front of the vector.
	/*
	Iterator begin();
	*/

	// Returns a ConstIterator that points to the front of the vector.
	/*
	ConstIterator begin() const;
	*/

	// Returns an Iterator that points to the back of the vector.
	/*
	Iterator end();
	*/

	// Returns a ConstIterator that points to the back of the vector.
	/*
	ConstIterator end() const;
	*/

	// Returns true if the vector is null.
	//
	// Note:  A vector may be empty without being null.
	// Also see:  isEmpty()
	bool isNull() const;

	// Returns true if the SjByteVector is empty.
	//
	// see size()
	// see isNull()
	bool isEmpty() const;

	// Returns a CRC checksum of the byte vector's data.
	SjUint checksum() const;

	// Converts the first 4 bytes of the vector to an unsigned integer.
	//
	// If mostSignificantByteFirst is true this will operate left to right
	// evaluating the integer.  For example if mostSignificantByteFirst is
	// true then $00 $00 $00 $01 == 0x00000001 == 1, if false, $01 00 00 00 ==
	// 0x01000000 == 1.
	//
	// see fromUint()
	SjUint toUInt(bool mostSignificantByteFirst = true) const;

	// Converts the first 2 bytes of the vector to a short.
	//
	// If mostSignificantByteFirst is true this will operate left to right
	// evaluating the integer.  For example if mostSignificantByteFirst is
	// true then $00 $01 == 0x0001 == 1, if false, $01 00 == 0x01000000 == 1.
	//
	// Also see:  fromShort()
	short toShort(bool mostSignificantByteFirst = true) const;

	// Converts the first 8 bytes of the vector to a (signed) long long.
	//
	// If mostSignificantByteFirst is true this will operate left to right
	// evaluating the integer.  For example if mostSignificantByteFirst is
	// true then $00 00 00 00 00 00 00 01 == 0x0000000000000001 == 1,
	// if false, $01 00 00 00 00 00 00 00 == 0x0100000000000000 == 1.
	//
	// see fromUint()
	wxLongLong_t toLongLong(bool mostSignificantByteFirst = true) const;


	// the string may or may not be null-terminated!
	// if it is not null-terminated, everything up to size() goes to the string
	wxString toString(SjStringType) const;
	static wxString toString(SjStringType, const void* data, int size);

	// Creates a 4 byte SjByteVector based on value.  If
	// mostSignificantByteFirst is true, then this will operate left to right
	// in building the SjByteVector.  For example if mostSignificantByteFirst is
	// true then $00 00 00 01 == 0x00000001 == 1, if false, $01 00 00 00 ==
	// 0x01000000 == 1.
	//
	// Also see:  toUint()
	static SjByteVector fromUint(SjUint value, bool mostSignificantByteFirst = true);

	// Creates a 2 byte SjByteVector based on value.  If
	// mostSignificantByteFirst is true, then this will operate left to right
	// in building the SjByteVector.  For example if mostSignificantByteFirst is
	// true then $00 01 == 0x0001 == 1, if false, $01 00 == 0x0100 == 1.
	//
	// Also see:  toShort()
	static SjByteVector fromShort(short value, bool mostSignificantByteFirst = true);

	// Creates a 8 byte SjByteVector based on value.  If
	// mostSignificantByteFirst is true, then this will operate left to right
	// in building the SjByteVector.  For example if mostSignificantByteFirst is
	// true then $00 00 00 01 == 0x0000000000000001 == 1, if false,
	// $01 00 00 00 00 00 00 00 == 0x0100000000000000 == 1.
	//
	// Also see:  toLongLong()
	static SjByteVector fromLongLong(wxLongLong value, bool mostSignificantByteFirst = true);

	// Returns a SjByteVector based on the CString s.
	static SjByteVector fromCString(const char *s, SjUint length = 0xffffffff);

	// Returns a const refernence to the byte at index.
	const unsigned char& operator[](int index) const;

	// Returns a reference to the byte at index.
	unsigned char &operator[](int index);

	// Returns true if this SjByteVector and v are equal.
	bool operator==(const SjByteVector &v) const;

	// Returns true if this SjByteVector and v are not equal.
	bool operator!=(const SjByteVector &v) const;

	// Returns true if this SjByteVector and the null terminated C string s
	// contain the same data.
	bool operator==(const char *s) const;

	// Returns true if this SjByteVector and the null terminated C string s
	// do not contain the same data.
	bool operator!=(const char *s) const;

	// Returns true if this SjByteVector is less than v.  The value of the
	// vectors is determined by evaluating the character from left to right, and
	// in the event one vector is a superset of the other, the size is used.
	bool operator<(const SjByteVector &v) const;

	// Returns true if this SjByteVector is greater than v.
	bool operator>(const SjByteVector &v) const;

	// Returns a vector that is v appended to this vector.
	SjByteVector operator+(const SjByteVector &v) const;

	// Copies SjByteVector v.
	SjByteVector &operator=(const SjByteVector &v);

	// Copies SjByteVector v.
	SjByteVector &operator=(unsigned char c);

	// Copies SjByteVector v.
	SjByteVector &operator=(const unsigned char *data);

	// A static, empty SjByteVector which is convenient and fast (since returning
	// an empty or "null" value does not require instantiating a new SjByteVector).
	static SjByteVector null;

	/*!
	 * Splits the ByteVector \a v into several strings at \a pattern.  This will
	 * not include the pattern in the returned ByteVectors.  \a max is the
	 * maximum number of entries that will be separated.  If \a max for instance
	 * is 2 then a maximum of 1 match will be found and the vector will be split
	 * on that match.
	 */
	SjArrayByteVector splitToArray(const SjByteVector& pattern, int byteAlign) const;
	SjArrayByteVector splitToArray(const SjByteVector& pattern, int byteAlign, int max) const;
	wxArrayString     splitToStrings(const SjByteVector& pattern, SjStringType) const;

protected:
	// If this SjByteVector is being shared via implicit sharing, do a deep copy
	// of the data and separate from the shared members.  This should be called
	// by all non-const subclass members.
	void detach();

private:
	SjByteVectorData* d;
};



WX_DECLARE_OBJARRAY(SjByteVector, SjArrayByteVector);



#endif // SJ_BYTEVECTOR_H
