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
#include "tg_ogg_file.h"



/***************************************************************************
 * OGG Page Header
 ***************************************************************************/



Ogg_PageHeader::Ogg_PageHeader(Ogg_File *f, long pageOffset)
{
	m_file = f;
	m_fileOffset = pageOffset;
	m_isValid = false;
	m_firstPacketContinued = false;
	m_lastPacketCompleted = false;
	m_firstPageOfStream = false;
	m_lastPageOfStream = false;
	m_absoluteGranularPosition = 0;
	m_streamSerialNumber = 0;
	m_pageSequenceNumber = -1;
	m_size = 0;
	m_dataSize = 0;

	if(f && pageOffset >= 0)
	{
		read();
	}
}



SjByteVector Ogg_PageHeader::render() const
{
	SjByteVector data;

	// capture patern

	data.append((unsigned char*)"OggS");

	// stream structure version

	data.append((unsigned char)0);

	// header type flag

	/*std::bitset<8> flags;
	flags[0] = d->firstPacketContinued;
	flags[1] = d->pageSequenceNumber == 0;
	flags[2] = d->lastPageOfStream;*/
	unsigned char flags = 0;
	if( m_firstPacketContinued )      { flags |= (1<<0); }
	if( m_pageSequenceNumber == 0 )   { flags |= (1<<1); }
	if( m_lastPageOfStream )          { flags |= (1<<2); }

	data.append(flags);

	// absolute granular position

	data.append(SjByteVector::fromLongLong(m_absoluteGranularPosition, false));

	// stream serial number

	data.append(SjByteVector::fromUint(m_streamSerialNumber, false));

	// page sequence number

	data.append(SjByteVector::fromUint(m_pageSequenceNumber, false));

	// checksum -- this is left empty and should be filled in by the Ogg::Page
	// class

	data.append(SjByteVector(4, 0));

	// page segment count and page segment table

	SjByteVector pageSegments = lacingValues();

	data.append(((unsigned char)(pageSegments.size())));
	data.append(pageSegments);

	return data;
}



void Ogg_PageHeader::read()
{
	m_file->Seek(m_fileOffset);

	// An Ogg page header is at least 27 bytes, so we'll go ahead and read that
	// much and then get the rest when we're ready for it.

	SjByteVector data = m_file->ReadBlock(27);

	// Sanity check -- make sure that we were in fact able to read as much data as
	// we asked for and that the page begins with "OggS".

	if(data.size() != 27 || !data.startsWith((unsigned char*)"OggS")) {
		wxLogDebug(wxT("Ogg::PageHeader::read() -- error reading page header"));
		return;
	}

	/*std::bitset<8> flags(data[5]);
	d->firstPacketContinued = flags.test(0);
	d->firstPageOfStream = flags.test(1);
	d->lastPageOfStream = flags.test(2);*/
	unsigned char flags = data[5];
	m_firstPacketContinued = (flags&(1<<0))!=0;
	m_firstPageOfStream = (flags&(1<<1))!=0;
	m_lastPageOfStream = (flags&(1<<2))!=0;

	m_absoluteGranularPosition = data.mid(6, 8).toLongLong(false);
	m_streamSerialNumber = data.mid(14, 4).toUInt(false);
	m_pageSequenceNumber = data.mid(18, 4).toUInt(false);

	// Byte number 27 is the number of page segments, which is the only variable
	// length portion of the page header.  After reading the number of page
	// segments we'll then read in the coresponding data for this count.

	int pageSegmentCount = (unsigned char)(data[26]);

	SjByteVector pageSegments = m_file->ReadBlock(pageSegmentCount);

	// Another sanity check.

	if(pageSegmentCount < 1 || int(pageSegments.size()) != pageSegmentCount)
		return;

	// The base size of an Ogg page 27 bytes plus the number of lacing values.

	m_size = 27 + pageSegmentCount;

	int packetSize = 0;

	for(int i = 0; i < pageSegmentCount; i++) {
		m_dataSize += (unsigned char)(pageSegments[i]);
		packetSize += (unsigned char)(pageSegments[i]);

		if((unsigned char)(pageSegments[i]) < 255) {
			m_packetSizes.Add(packetSize);
			packetSize = 0;
		}
	}

	if(packetSize > 0)
	{
		m_packetSizes.Add(packetSize);
		m_lastPacketCompleted = false;
	}
	else
	{
		m_lastPacketCompleted = true;
	}

	m_isValid = true;
}



SjByteVector Ogg_PageHeader::lacingValues() const
{
	SjByteVector data;

	//List<int> sizes = d->packetSizes;
	wxArrayLong sizes = m_packetSizes;

	//for(List<int>::ConstIterator it = sizes.begin(); it != sizes.end(); ++it)
	int it, it_i, it_count = sizes.GetCount();
	for( it_i = 0; it_i < it_count; it_i++ )
	{
		it = sizes.Item(it_i);

		// The size of a packet in an Ogg page is indicated by a series of "lacing
		// values" where the sum of the values is the packet size in bytes.  Each of
		// these values is a byte.  A value of less than 255 (0xff) indicates the end
		// of the packet.

		div_t n = div(it, 255);

		for(int i = 0; i < n.quot; i++)
			data.append(((unsigned char)(255)));

		//if(it != --sizes.end() || d->lastPacketCompleted)
		if(it_i != it_count-1 || m_lastPacketCompleted)
		{
			data.append(((unsigned char)(n.rem)));
		}
	}

	return data;
}



/***************************************************************************
 * OGG Page
 ***************************************************************************/



class Ogg_PagePrivate
{
public:
	Ogg_PagePrivate(Ogg_File *f = 0, long pageOffset = -1) :
		file(f),
		fileOffset(pageOffset),
		packetOffset(0),
		header(f, pageOffset),
		firstPacketIndex(-1)
	{
		if(file)
		{
			packetOffset = fileOffset + header.size();
			packetSizes = header.packetSizes();
			dataSize = header.dataSize();
		}
	}

	Ogg_File *file;
	long fileOffset;
	long packetOffset;
	int dataSize;

	wxArrayLong packetSizes; // was: List<int> packetSizes;

	Ogg_PageHeader header;
	int firstPacketIndex;
	SjArrayByteVector packets;
};



Ogg_Page::Ogg_Page(Ogg_File *file, long pageOffset)
{
	d = new Ogg_PagePrivate(file, pageOffset);
}



Ogg_Page::~Ogg_Page()
{
	delete d;
}



long Ogg_Page::fileOffset() const
{
	return d->fileOffset;
}



const Ogg_PageHeader *Ogg_Page::header() const
{
	return &d->header;
}



int Ogg_Page::firstPacketIndex() const
{
	return d->firstPacketIndex;
}



void Ogg_Page::setFirstPacketIndex(int index)
{
	d->firstPacketIndex = index;
}



Ogg_ContainsPacketFlags Ogg_Page::containsPacket(int index) const
{
	int lastPacketIndex = d->firstPacketIndex + packetCount() - 1;
	if(index < d->firstPacketIndex || index > lastPacketIndex)
		return Ogg_DoesNotContainPacket;

	Ogg_ContainsPacketFlags flags = Ogg_DoesNotContainPacket;

	if(index == d->firstPacketIndex)
		flags = Ogg_ContainsPacketFlags(flags | Ogg_BeginsWithPacket);

	if(index == lastPacketIndex)
		flags = Ogg_ContainsPacketFlags(flags | Ogg_EndsWithPacket);

	// If there's only one page and it's complete:

	if(packetCount() == 1 &&
	        !d->header.firstPacketContinued() &&
	        d->header.lastPacketCompleted())
	{
		flags = Ogg_ContainsPacketFlags(flags | Ogg_CompletePacket);
	}

	// Or if the page is (a) the first page and it's complete or (b) the last page
	// and it's complete or (c) a page in the middle.

	else if((flags & Ogg_BeginsWithPacket && !d->header.firstPacketContinued()) ||
	        (flags & Ogg_EndsWithPacket && d->header.lastPacketCompleted()) ||
	        (!(flags & Ogg_BeginsWithPacket) && !(flags & Ogg_EndsWithPacket)))
	{
		flags = Ogg_ContainsPacketFlags(flags | Ogg_CompletePacket);
	}

	return flags;
}



SjUint Ogg_Page::packetCount() const
{
	return d->header.packetSizes().GetCount();
}



SjArrayByteVector Ogg_Page::packets() const
{
	if(!d->packets.IsEmpty())
		return d->packets;

	SjArrayByteVector l;

	if(d->file && d->header.IsValid()) {

		d->file->Seek(d->packetOffset);

		//List<int> packetSizes = d->header.packetSizes();
		wxArrayLong packetSizes = d->header.packetSizes();

		//List<int>::ConstIterator it = packetSizes.begin();
		//for(; it != packetSizes.end(); ++it)
		int i, iCount = packetSizes.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			l.Add(d->file->ReadBlock(packetSizes[i]));
		}
	}
	else
	{
		wxLogDebug(wxT("Ogg::Page::packets() -- attempting to read packets from an invalid page."));
	}

	return l;
}



int Ogg_Page::size() const
{
	return d->header.size() + d->header.dataSize();
}



SjByteVector Ogg_Page::render() const
{
	SjByteVector data;

	data.append(d->header.render());

	if(d->packets.IsEmpty()) {
		if(d->file) {
			d->file->Seek(d->packetOffset);
			data.append(d->file->ReadBlock(d->dataSize));
		}
		else
			wxLogDebug(wxT("Ogg::Page::render() -- this page is empty!"));
	}
	else {
		//ByteVectorList::ConstIterator it = d->packets.begin();
		//for(; it != d->packets.end(); ++it)
		int i, iCount = d->packets.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			data.append(d->packets[i]);
		}
	}

	// Compute and set the checksum for the Ogg page.  The checksum is taken over
	// the entire page with the 4 bytes reserved for the checksum zeroed and then
	// inserted in bytes 22-25 of the page header.

	SjByteVector checksum = SjByteVector::fromUint(data.checksum(), false);
	for(int i = 0; i < 4; i++)
		data[i + 22] = checksum[i];

	return data;
}



//List<Ogg::Page *>
wxArrayPtrVoid Ogg_Page::paginate(    const SjArrayByteVector &packets,
                                      Ogg_PaginationStrategy strategy,
                                      SjUint streamSerialNumber,
                                      int firstPage,
                                      bool firstPacketContinued,
                                      bool lastPacketCompleted,
                                      bool containsLastPacket)
{
	//List<Page *> l;
	wxArrayPtrVoid l;

	int totalSize = 0;

	//for(ByteVectorList::ConstIterator it = packets.begin(); it != packets.end(); ++it)
	int i, iCount = packets.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		totalSize += packets[i].size();
	}

	if(strategy == Ogg_Repaginate || totalSize + packets.GetCount() > 255 * 256) {
		wxLogDebug(wxT("Ogg::Page::paginate() -- Sorry!  Repagination is not yet implemented."));
		return l;
	}

	// TODO: Handle creation of multiple pages here with appropriate pagination.

	Ogg_Page *p = new Ogg_Page(packets, streamSerialNumber, firstPage, firstPacketContinued,
	                           lastPacketCompleted, containsLastPacket);
	l.Add(p);

	return l;
}



Ogg_Page::Ogg_Page(const SjArrayByteVector &packets,
                   SjUint streamSerialNumber,
                   int pageNumber,
                   bool firstPacketContinued,
                   bool lastPacketCompleted,
                   bool containsLastPacket)
{
	d = new Ogg_PagePrivate;

	SjByteVector data;

	//List<int> packetSizes;
	wxArrayLong packetSizes;

	d->header.setFirstPageOfStream(pageNumber == 0 && !firstPacketContinued);
	d->header.setLastPageOfStream(containsLastPacket);
	d->header.setFirstPacketContinued(firstPacketContinued);
	d->header.setLastPacketCompleted(lastPacketCompleted);
	d->header.setStreamSerialNumber(streamSerialNumber);
	d->header.setPageSequenceNumber(pageNumber);

	// Build a page from the list of packets.

	//for(ByteVectorList::ConstIterator it = packets.begin(); it != packets.end(); ++it)
	int i, iCount = packets.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		packetSizes.Add(packets[i].size());
		data.append(packets[i]);
	}
	d->packets = packets;
	d->header.setPacketSizes(packetSizes);
}



/***************************************************************************
 * OGG File
 ***************************************************************************/



Ogg_File::Ogg_File(const wxFSFile* fsFile, wxInputStream* inputStream)
	: Tagger_File(fsFile, inputStream)
{
	m_streamSerialNumber    = 0;
	m_firstPageHeader       = NULL;
	m_lastPageHeader        = NULL;
	m_currentPage           = NULL;
	m_currentPacketPage     = NULL;
}



void Ogg_File::m_dirtyPackets_clear()
{
	SjHashIterator iterator;
	SjByteVector* it;
	long itKey;
	while( (it=(SjByteVector*)m_dirtyPackets.Iterate(iterator, &itKey)) )
	{
		delete it;
	}
	m_dirtyPackets.Clear();
}



Ogg_File::~Ogg_File()
{
	if( m_firstPageHeader )
	{
		delete m_firstPageHeader;
	}

	if( m_lastPageHeader )
	{
		delete m_lastPageHeader;
	}

	int i, iCount = m_pages.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		Ogg_Page* page = m_pages_lookup(i);
		delete page;
	}

	iCount = m_packetToPageMap.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		wxArrayLong* pages = m_packetToPageMap_lookup(i);
		delete pages;
	}

	m_dirtyPackets_clear();
}



SjByteVector Ogg_File::packet(SjUint i)
{
	// Check to see if we're called setPacket() for this packet since the last
	// save:

	SjByteVector* test = m_dirtyPackets_lookup(i);
	if( test )
		return *test;

	// If we haven't indexed the page where the packet we're interested in starts,
	// begin reading pages until we have.

	while( m_packetToPageMap.GetCount() <= i)
	{
		if( !nextPage() )
		{
			wxLogDebug(wxT("Ogg::File::packet() -- Could not find the requested packet."));
			return SjByteVector::null;
		}
	}

	// Start reading at the first page that contains part (or all) of this packet.
	// If the last read stopped at the packet that we're interested in, don't
	// reread its packet list.  (This should make sequential packet reads fast.)

	SjUint pageIndex = m_packetToPageMap_lookup(i)->Item(0);
	if( m_currentPacketPage != m_pages_lookup(pageIndex) )
	{
		m_currentPacketPage = m_pages_lookup(pageIndex);
		m_currentPackets    = m_currentPacketPage->packets();
	}

	// If the packet is completely contained in the first page that it's in, then
	// just return it now.

	if( m_currentPacketPage->containsPacket(i) & Ogg_CompletePacket)
	{
		int index = i - m_currentPacketPage->firstPacketIndex();
		return m_currentPackets[index];
	}

	// If the packet is *not* completely contained in the first page that it's a
	// part of then that packet trails off the end of the page.  Continue appending
	// the pages' packet data until we hit a page that either does not end with the
	// packet that we're fetching or where the last packet is complete.

	SjByteVector packet = m_currentPackets.Last();
	while( m_currentPacketPage->containsPacket(i) & Ogg_EndsWithPacket
	        && !m_currentPacketPage->header()->lastPacketCompleted() )
	{
		pageIndex++;
		if( pageIndex == m_pages.GetCount() )
		{
			if( !nextPage() )
			{
				wxLogDebug(wxT("Ogg::File::packet() -- Could not find the requested packet."));
				return SjByteVector::null;
			}
		}
		m_currentPacketPage = m_pages_lookup(pageIndex);
		m_currentPackets = m_currentPacketPage->packets();

		packet.append(m_currentPackets.Item(0));
	}

	return packet;
}



static int Ogg_File_LSort(long* i1__, long* i2__)
{
	return *i1__ - *i2__;
}
void Ogg_File::setPacket(SjUint i, const SjByteVector &p)
{
	while( m_packetToPageMap.GetCount() <= i )
	{
		if(!nextPage())
		{
			wxLogDebug(wxT("Ogg::File::setPacket() -- Could not set the requested packet."));
			return;
		}
	}

	/*  List<int>::ConstIterator it = d->packetToPageMap[i].begin();
	  for(; it != d->packetToPageMap[i].end(); ++it)
	    d->dirtyPages.sortedInsert(*it, true);

	  d->dirtyPackets.insert(i, p); */

	wxArrayLong* list = (wxArrayLong*)m_packetToPageMap_lookup(i);
	int j, jCount = list->GetCount();
	long thePage;
	for( j = 0; j < jCount; j++ )
	{
		thePage = list->Item(j);
		m_dirtyPages.Add(thePage, true);
	}
	m_dirtyPages.Sort(Ogg_File_LSort);

	m_dirtyPackets.Insert(i, new SjByteVector(p));
}



const Ogg_PageHeader* Ogg_File::firstPageHeader()
{
	if( m_firstPageHeader)
	{
		return m_firstPageHeader->IsValid() ? m_firstPageHeader : NULL;
	}

	long firstPageHeaderOffset = Find((unsigned char*)"OggS");

	if( firstPageHeaderOffset < 0 )
	{
		return NULL;
	}

	m_firstPageHeader = new Ogg_PageHeader(this, firstPageHeaderOffset);
	return m_firstPageHeader->IsValid() ? m_firstPageHeader : NULL;
}



const Ogg_PageHeader* Ogg_File::lastPageHeader()
{
	if( m_lastPageHeader)
	{
		return m_lastPageHeader->IsValid() ? m_lastPageHeader : 0;
	}

	long lastPageHeaderOffset = RFind((unsigned char*)"OggS");

	if( lastPageHeaderOffset < 0 )
	{
		return 0;
	}

	m_lastPageHeader = new Ogg_PageHeader(this, lastPageHeaderOffset);
	return m_lastPageHeader->IsValid() ? m_lastPageHeader : 0;
}



bool Ogg_File::save()
{
	if(ReadOnly())
	{
		wxLogDebug(wxT("Ogg::File::save() - Cannot save to a read only file."));
		return false;
	}

	/*  List<int> pageGroup;

	  for(List<int>::ConstIterator it = d->dirtyPages.begin(); it != d->dirtyPages.end(); ++it) {
	    if(!pageGroup.isEmpty() && pageGroup.back() + 1 != *it) {
	      writePageGroup(pageGroup);
	      pageGroup.clear();
	    }
	    else
	      pageGroup.append(*it);
	  }
	  writePageGroup(pageGroup);
	  d->dirtyPages.clear();
	  d->dirtyPackets.clear();

	  return true;*/

	wxArrayLong pageGroup;

	int i, iCount = m_dirtyPages.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		int it = m_dirtyPages[i];

		if(!pageGroup.IsEmpty() && pageGroup.Last() + 1 != it)
		{
			writePageGroup(pageGroup);
			pageGroup.Clear();
		}
		else
		{
			pageGroup.Add(it);
		}
	}

	writePageGroup(pageGroup);

	m_dirtyPages.Clear();
	m_dirtyPackets_clear();

	return true;
}


bool Ogg_File::nextPage()
{
	long nextPageOffset;
	int currentPacket;

	if( m_pages.IsEmpty() )
	{
		currentPacket = 0;
		nextPageOffset = Find((unsigned char*)"OggS");
		if(nextPageOffset < 0)
		{
			return false;
		}
	}
	else
	{
		if( m_currentPage->header()->lastPageOfStream() )
		{
			return false;
		}

		if( m_currentPage->header()->lastPacketCompleted() )
		{
			currentPacket = m_currentPage->firstPacketIndex() + m_currentPage->packetCount();
		}
		else
		{
			currentPacket = m_currentPage->firstPacketIndex() + m_currentPage->packetCount() - 1;
		}

		nextPageOffset = m_currentPage->fileOffset() + m_currentPage->size();
	}

	// Read the next page and add it to the page list.

	m_currentPage = new Ogg_Page(this, nextPageOffset);

	if( !m_currentPage->header()->IsValid() )
	{
		delete m_currentPage;
		m_currentPage = 0;
		return false;
	}

	m_currentPage->setFirstPacketIndex(currentPacket);

	if( m_pages.IsEmpty())
	{
		m_streamSerialNumber = m_currentPage->header()->streamSerialNumber();
	}

	m_pages_append(m_currentPage);

	// Loop through the packets in the page that we just read appending the
	// current page number to the packet to page map for each packet.

	for( SjUint i = 0; i < m_currentPage->packetCount(); i++ )
	{
		SjUint currentPacket = m_currentPage->firstPacketIndex() + i;
		if( m_packetToPageMap.GetCount() <= currentPacket )
		{
			m_packetToPageMap_append(new wxArrayLong());
		}

		m_packetToPageMap_lookup(currentPacket)->Add(m_pages.GetCount() - 1);
	}

	return true;
}

void Ogg_File::writePageGroup(const wxArrayLong& pageGroup)
{
	if( pageGroup.IsEmpty() )
	{
		return;
	}

	SjArrayByteVector packets;


	// If the first page of the group isn't dirty, append its partial content here.

	/*  if(!d->dirtyPages.contains(d->pages[pageGroup.front()]->firstPacketIndex()))
	    packets.append(d->pages[pageGroup.front()]->packets().front());*/

	Ogg_Page* thePage = m_pages_lookup(pageGroup[0]); if( thePage == NULL ) return;//should not happen
	if( m_dirtyPages.Index( thePage->firstPacketIndex() ) == wxNOT_FOUND )
	{
		packets.Add(thePage->packets().Item(0));
	}

	int previousPacket = -1;
	int originalSize = 0;

	/*for(List<int>::ConstIterator it = pageGroup.begin(); it != pageGroup.end(); ++it) {
	  uint firstPacket = d->pages[*it]->firstPacketIndex();
	  uint lastPacket = firstPacket + d->pages[*it]->packetCount() - 1;

	  List<int>::ConstIterator last = --pageGroup.end();

	  for(uint i = firstPacket; i <= lastPacket; i++) {

	    if(it == last && i == lastPacket && !d->dirtyPages.contains(i))
	      packets.append(d->pages[*it]->packets().back());
	    else if(int(i) != previousPacket) {
	      previousPacket = i;
	      packets.append(packet(i));
	    }
	  }
	  originalSize += d->pages[*it]->size();
	}*/
	int it, it_i, it_count = pageGroup.GetCount();
	for( it_i = 0; it_i < it_count; it_i++ )
	{
		it = pageGroup.Item(it_i);
		thePage = m_pages_lookup(it);

		SjUint firstPacket = thePage->firstPacketIndex();
		SjUint lastPacket = firstPacket + thePage->packetCount() - 1;

		for(SjUint i = firstPacket; i <= lastPacket; i++)
		{
			if( it_i == (it_count-1) && i == lastPacket && m_dirtyPages.Index(i)==wxNOT_FOUND)
			{
				packets.Add(thePage->packets().Last());
			}
			else if(int(i) != previousPacket)
			{
				previousPacket = i;
				packets.Add(packet(i));
			}
		}
		originalSize += thePage->size();
	}


	/*const bool continued = d->pages[pageGroup.front()]->header()->firstPacketContinued();
	const bool completed = d->pages[pageGroup.back()]->header()->lastPacketCompleted();*/
	thePage = m_pages_lookup(pageGroup.Item(0));
	const bool continued = thePage->header()->firstPacketContinued();

	thePage = m_pages_lookup(pageGroup.Last());
	const bool completed = thePage->header()->lastPacketCompleted();


	// TODO: This pagination method isn't accurate for what's being done here.
	// This should account for real possibilities like non-aligned packets and such.

	/*List<Page *> pages = Page::paginate(packets, Page::SinglePagePerGroup,
	                                    d->streamSerialNumber, pageGroup.front(),
	                                    continued, completed);*/
	wxArrayPtrVoid pages = Ogg_Page::paginate(packets, Ogg_SinglePagePerGroup,
	                       m_streamSerialNumber, pageGroup.Item(0),
	                       continued, completed);



	/*ByteVector data;
	for(List<Page *>::ConstIterator it = pages.begin(); it != pages.end(); ++it)
	  data.append((*it)->render());*/
	SjByteVector data;
	it_count = pages.GetCount();
	for( it_i = 0; it_i < it_count; it_i++ )
	{
		thePage = (Ogg_Page*)(pages.Item(it_i));
		data.append(thePage->render());
	}




	// The insertion algorithms could also be improve to queue and prioritize data
	// on the way out.  Currently it requires rewriting the file for every page
	// group rather than just once; however, for tagging applications there will
	// generally only be one page group, so it's not worth the time for the
	// optimization at the moment.

	/*insert(data, d->pages[pageGroup.front()]->fileOffset(), originalSize);*/
	thePage = m_pages_lookup(pageGroup.Item(0));
	Insert(data, thePage->fileOffset(), originalSize);


	// Update the page index to include the pages we just created and to delete the
	// old pages.

	/*for(List<Page *>::ConstIterator it = pages.begin(); it != pages.end(); ++it) {
	  const int index = (*it)->header()->pageSequenceNumber();
	  delete d->pages[index];
	  d->pages[index] = *it;
	}*/
	it_count = pages.GetCount();
	for( it_i = 0; it_i < it_count; it_i++ )
	{
		thePage = (Ogg_Page*)(pages.Item(it_i));
		const int index = thePage->header()->pageSequenceNumber();

		Ogg_Page* pageToDelete = (Ogg_Page*)(m_pages.Item(index));
		delete pageToDelete;

		m_pages[index] = thePage;
	}
}
