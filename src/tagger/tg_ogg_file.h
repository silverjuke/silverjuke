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




#ifndef TAGGER_OGGFILE_H
#define TAGGER_OGGFILE_H

#include "tg_tagger_base.h"




class Ogg_File;



//! An implementation of the page headers associated with each Ogg::Page

/*!
 * This class implements Ogg page headers which contain the information
 * about Ogg pages needed to break them into packets which can be passed on
 * to the codecs.
 */

class Ogg_PageHeader
{
public:
	/*!
	 * Reads a PageHeader from \a file starting at \a pageOffset.  The defaults
	 * create a page with no (and as such, invalid) data that must be set
	 * later.
	 */
	Ogg_PageHeader(Ogg_File *file = 0, long pageOffset = -1);

	/*!
	* Deletes this instance of the PageHeader.
	*/
	virtual ~Ogg_PageHeader() {}

	/*!
	 * Returns true if the header parsed properly and is valid.
	 */
	bool IsValid() const { return m_isValid; }

	/*!
	 * Ogg pages contain a list of packets (which are used by the contained
	 * codecs).  The sizes of these pages is encoded in the page header.  This
	 * returns a list of the packet sizes in bytes.
	 *
	 * \see setPacketSizes()
	 */
	wxArrayLong packetSizes() const { return m_packetSizes; }

	/*!
	 * Sets the sizes of the packets in this page to \a sizes.  Internally this
	 * updates the lacing values in the header.
	 *
	 * \see packetSizes()
	 */
	void setPacketSizes(const wxArrayLong &sizes) { m_packetSizes = sizes; }

	/*!
	 * Some packets can be <i>continued</i> across multiple pages.  If the
	 * first packet in the current page is a continuation this will return
	 * true.  If this is page starts with a new packet this will return false.
	 *
	 * \see lastPacketCompleted()
	 * \see setFirstPacketContinued()
	 */
	bool firstPacketContinued() const { return m_firstPacketContinued; }

	/*!
	 * Sets the internal flag indicating if the first packet in this page is
	 * continued to \a continued.
	 *
	 * \see firstPacketContinued()
	 */
	void setFirstPacketContinued(bool continued) { m_firstPacketContinued = continued; }

	/*!
	 * Returns true if the last packet of this page is completely contained in
	 * this page.
	 *
	 * \see firstPacketContinued()
	 * \see setLastPacketCompleted()
	 */
	bool lastPacketCompleted() const { return m_lastPacketCompleted; }

	/*!
	 * Sets the internal flag indicating if the last packet in this page is
	 * complete to \a completed.
	 *
	 * \see lastPacketCompleted()
	 */
	void setLastPacketCompleted(bool completed) { m_lastPacketCompleted = completed; }

	/*!
	 * This returns true if this is the first page of the Ogg (logical) stream.
	 *
	 * \see setFirstPageOfStream()
	 */
	bool firstPageOfStream() const { return m_firstPageOfStream; }

	/*!
	 * Marks this page as the first page of the Ogg stream.
	 *
	 * \see firstPageOfStream()
	 */
	void setFirstPageOfStream(bool first) { m_firstPageOfStream = first; }

	/*!
	 * This returns true if this is the last page of the Ogg (logical) stream.
	 *
	 * \see setLastPageOfStream()
	 */
	bool lastPageOfStream() const { return m_lastPageOfStream; }

	/*!
	 * Marks this page as the last page of the Ogg stream.
	 *
	 * \see lastPageOfStream()
	 */
	void setLastPageOfStream(bool last) { m_lastPageOfStream = last; }

	/*!
	 * A special value of containing the position of the packet to be
	 * interpreted by the codec.  In the case of Vorbis this contains the PCM
	 * value and is used to calculate the length of the stream.
	 *
	 * \see setAbsoluteGranularPosition()
	 */
	wxLongLong_t absoluteGranularPosition() const { return m_absoluteGranularPosition; }

	/*!
	 * A special value of containing the position of the packet to be
	 * interpreted by the codec.  It is only supported here so that it may be
	 * coppied from one page to another.
	 *
	 * \see absoluteGranularPosition()
	 */
	void setAbsoluteGranularPosition(wxLongLong_t agp) { m_absoluteGranularPosition = agp; }

	/*!
	 * Every Ogg logical stream is given a random serial number which is common
	 * to every page in that logical stream.  This returns the serial number of
	 * the stream associated with this packet.
	 *
	 * \see setStreamSerialNumber()
	 */
	SjUint streamSerialNumber() const { return m_streamSerialNumber; }

	/*!
	 * Every Ogg logical stream is given a random serial number which is common
	 * to every page in that logical stream.  This sets this pages serial
	 * number.  This method should be used when adding new pages to a logical
	 * stream.
	 *
	 * \see streamSerialNumber()
	 */
	void setStreamSerialNumber(SjUint n) { m_streamSerialNumber = n; }

	/*!
	 * Returns the index of the page within the Ogg stream.  This helps make it
	 * possible to determine if pages have been lost.
	 *
	 * \see setPageSequenceNumber()
	 */
	int pageSequenceNumber() const { return m_pageSequenceNumber; }

	/*!
	 * Sets the page's position in the stream to \a sequenceNumber.
	 *
	 * \see pageSequenceNumber()
	 */
	void setPageSequenceNumber(int sequenceNumber) { m_pageSequenceNumber = sequenceNumber; }

	/*!
	 * Returns the complete header size.
	 */
	int size() const { return m_size; }

	/*!
	 * Returns the size of the data portion of the page -- i.e. the size of the
	 * page less the header size.
	 */
	int dataSize() const { return m_dataSize; }

	/*!
	 * Render the page header to binary data.
	 *
	 * \note The checksum -- bytes 22 - 25 -- will be left empty and must be
	 * filled in when rendering the entire page.
	 */
	SjByteVector render() const;

private:
#ifdef __WXDEBUG__
	Ogg_PageHeader &operator=(const Ogg_PageHeader &) { wxASSERT_MSG(0, wxT("do not copy MPEG_Header objects this way!")); return *this; }
#endif

	void read();
	SjByteVector lacingValues() const;


	Ogg_File*       m_file;
	long            m_fileOffset;
	bool            m_isValid;

	wxArrayLong     m_packetSizes; // was: List<int>

	bool            m_firstPacketContinued;
	bool            m_lastPacketCompleted;
	bool            m_firstPageOfStream;
	bool            m_lastPageOfStream;
	wxLongLong_t    m_absoluteGranularPosition;
	SjUint          m_streamSerialNumber;
	int             m_pageSequenceNumber;
	int             m_size;
	int             m_dataSize;
};

/*!
 * When checking to see if a page contains a given packet this set of flags
 * represents the possible values for that packets status in the page.
 *
 * \see containsPacket()
 */
enum Ogg_ContainsPacketFlags
{
    //! No part of the packet is contained in the page
    Ogg_DoesNotContainPacket = 0x0000,
    //! The packet is wholly contained in the page
    Ogg_CompletePacket       = 0x0001,
    //! The page starts with the given packet
    Ogg_BeginsWithPacket     = 0x0002,
    //! The page ends with the given packet
    Ogg_EndsWithPacket       = 0x0004
};


/*!
* Defines a strategy for pagination, or grouping pages into Ogg packets,
* for use with pagination methods.
*
* \note Yes, I'm aware that this is not a canonical "Strategy Pattern",
* the term was simply convenient.
*/
enum Ogg_PaginationStrategy
{
    /*!
     * Attempt to put the specified set of packets into a single Ogg packet.
     * If the sum of the packet data is greater than will fit into a single
     * Ogg page -- 65280 bytes -- this will fall back to repagination using
     * the recommended page sizes.
     */
    Ogg_SinglePagePerGroup,
    /*!
     * Split the packet or group of packets into pages that conform to the
     * sizes recommended in the Ogg standard.
     */
    Ogg_Repaginate
};


//! An implementation of Ogg pages

/*!
 * This is an implementation of the pages that make up an Ogg stream.
 * This handles parsing pages and breaking them down into packets and handles
 * the details of packets spanning multiple pages and pages that contiain
 * multiple packets.
 *
 * In most Xiph.org formats the comments are found in the first few packets,
 * this however is a reasonably complete implementation of Ogg pages that
 * could potentially be useful for non-meta data purposes.
 */

class Ogg_PagePrivate;
class Ogg_Page
{
public:
	/*!
	 * Read an Ogg page from the \a file at the position \a pageOffset.
	 */
	Ogg_Page(Ogg_File *file, long pageOffset);

	virtual ~Ogg_Page();

	/*!
	 * Returns the page's position within the file (in bytes).
	 */
	long fileOffset() const;

	/*!
	 * Returns a pointer to the header for this page.  This pointer will become
	 * invalid when the page is deleted.
	 */
	const Ogg_PageHeader *header() const;

	/*!
	 * Returns the index of the first packet wholly or partially contained in
	 * this page.
	 *
	 * \see setFirstPacketIndex()
	 */
	int firstPacketIndex() const;

	/*!
	 * Sets the index of the first packet in the page.
	 *
	 * \see firstPacketIndex()
	 */
	void setFirstPacketIndex(int index);


	/*!
	 * Checks to see if the specified \a packet is contained in the current
	 * page.
	 *
	 * \see ContainsPacketFlags
	 */
	Ogg_ContainsPacketFlags containsPacket(int index) const;

	/*!
	 * Returns the number of packets (whole or partial) in this page.
	 */
	SjUint packetCount() const;

	/*!
	 * Returns a list of the packets in this page.
	 *
	 * \note Either or both the first and last packets may be only partial.
	 * \see PageHeader::firstPacketContinued()
	 */
	SjArrayByteVector packets() const;

	/*!
	 * Returns the size of the page in bytes.
	 */
	int size() const;

	SjByteVector render() const;


	/*!
	 * Pack \a packets into Ogg pages using the \a strategy for pagination.
	 * The page number indicater inside of the rendered packets will start
	 * with \a firstPage and be incremented for each page rendered.
	 * \a containsLastPacket should be set to true if \a packets contains the
	 * last page in the stream and will set the appropriate flag in the last
	 * rendered Ogg page's header.  \a streamSerialNumber should be set to
	 * the serial number for this stream.
	 *
	 * \note The "absolute granule position" is currently always zeroed using
	 * this method as this suffices for the comment headers.
	 *
	 * \warning The pages returned by this method must be deleted by the user.
	 * You can use List<T>::setAutoDelete(true) to set these pages to be
	 * automatically deleted when this list passes out of scope.
	 *
	 * \see PaginationStrategy
	 * \see List::setAutoDelete()
	 */
	static wxArrayPtrVoid paginate(const SjArrayByteVector &packets,
	                               Ogg_PaginationStrategy strategy,
	                               SjUint streamSerialNumber,
	                               int firstPage,
	                               bool firstPacketContinued = false,
	                               bool lastPacketCompleted = true,
	                               bool containsLastPacket = false);

protected:
	/*!
	 * Creates an Ogg packet based on the data in \a packets.  The page number
	 * for each page will be set to \a pageNumber.
	 */
	Ogg_Page(const SjArrayByteVector &packets,
	         SjUint streamSerialNumber,
	         int pageNumber,
	         bool firstPacketContinued = false,
	         bool lastPacketCompleted = true,
	         bool containsLastPacket = false);

private:
	Ogg_Page(const Ogg_Page &);
	Ogg_Page &operator=(const Ogg_Page &);

	Ogg_PagePrivate *d;
};



//! An implementation of Tagger_File with some helpers for Ogg based formats

/*!
 * This is an implementation of Ogg file page and packet rendering and is of
 * use to Ogg based formats.  While the API is small this handles the
 * non-trivial details of breaking up an Ogg stream into packets and makes
 * these available (via subclassing) to the codec meta data implementations.
 */
class Ogg_File : public Tagger_File
{
public:
	virtual ~Ogg_File();

	/*!
	 * Returns the packet contents for the i-th packet (starting from zero)
	 * in the Ogg bitstream.
	 *
	 * \warning The requires reading at least the packet header for every page
	 * up to the requested page.
	 */
	SjByteVector packet(SjUint i);

	/*!
	 * Sets the packet with index \a i to the value \a p.
	 */
	void setPacket(SjUint i, const SjByteVector &p);

	/*!
	 * Returns a pointer to the PageHeader for the first page in the stream or
	 * null if the page could not be found.
	 */
	const Ogg_PageHeader *firstPageHeader();

	/*!
	 * Returns a pointer to the PageHeader for the last page in the stream or
	 * null if the page could not be found.
	 */
	const Ogg_PageHeader *lastPageHeader();

	virtual bool save();

protected:
	/*!
	 * Contructs an Ogg file from \a file.  If \a readProperties is true the
	 * file's audio properties will also be read using \a propertiesStyle.  If
	 * false, \a propertiesStyle is ignored.
	 *
	 * \note This constructor is protected since Ogg::File shouldn't be
	 * instantiated directly but rather should be used through the codec
	 * specific subclasses.
	 */
	Ogg_File            (const wxString& url, wxInputStream*);

private:
#ifdef __WXDEBUG__
	Ogg_File        &operator=          (const Ogg_File &) { wxLogWarning(wxT("do not copy Ogg_File objects this way!")); return *this; }
#endif

	/*!
	 * Reads the next page and updates the internal "current page" pointer.
	 */
	bool            nextPage            ();
	void            writePageGroup      (const wxArrayLong& group);

	SjUint                  m_streamSerialNumber;

	wxArrayPtrVoid          m_pages;            // contains Ogg_Page elements, was: List<Page *>
	Ogg_Page*               m_pages_lookup(int i) { return (Ogg_Page*)m_pages.Item(i); }
	void                    m_pages_append(Ogg_Page* p) { m_pages.Add(p); }

	Ogg_PageHeader*         m_firstPageHeader;
	Ogg_PageHeader*         m_lastPageHeader;


	wxArrayPtrVoid          m_packetToPageMap;  // contains wxArrayLong* elements, was std::vector< List<int> >
	wxArrayLong*            m_packetToPageMap_lookup(int i) { return (wxArrayLong*)m_packetToPageMap.Item(i); }
	void                    m_packetToPageMap_append(wxArrayLong* i) { m_packetToPageMap.Add(i); }


	SjLPHash                m_dirtyPackets;     // contains SjByteVector elements, was: Map<int, ByteVector>
	void                    m_dirtyPackets_clear();
	SjByteVector*           m_dirtyPackets_lookup(SjUint i) { return (SjByteVector*)m_dirtyPackets.Lookup(i); }


	wxArrayLong             m_dirtyPages;       // was: List<int>

	Ogg_Page*               m_currentPage;      // The current page for the reader -- used by nextPage() -- must not be deleted as in m_pages!!
	Ogg_Page*               m_currentPacketPage;// The current page for the packet parser -- used by packet()

	SjArrayByteVector       m_currentPackets;   // The packets for the currentPacketPage -- used by packet()
};



#endif
