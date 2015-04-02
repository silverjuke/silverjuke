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




#ifndef TAGGER_MPEGHEADER_H
#define TAGGER_MPEGHEADER_H




/*!
 * The MPEG Version.
 */
enum MPEG_HeaderVersion
{
    //! MPEG Version 1
    MPEG_Version1 = 0,
    //! MPEG Version 2
    MPEG_Version2 = 1,
    //! MPEG Version 2.5
    MPEG_Version2_5 = 2
};


/*!
 * There are a few combinations or one or two channel audio that are
 * possible:
 */
enum MPEG_HeaderChannelMode
{
    //! Stereo
    MPEG_Stereo        = 0,
    //! Stereo
    MPEG_JointStereo   = 1,
    //! Dual Mono
    MPEG_DualChannel   = 2,
    //! Mono
    MPEG_SingleChannel = 3
};



//! An implementation of MP3 frame headers

/*!
 * This is an implementation of MPEG Layer III headers.  The API follows more
 * or less the binary format of these headers.  I've used
 * <a href="http://www.mp3-tech.org/programmer/frame_header.html">this</a>
 * document as a reference.
 */
class MPEG_Header
{
public:
	/*!
	 * Parses an MPEG header based on \a data.
	 */
	MPEG_Header(const SjByteVector &data) { parse(data); }
	void parse(const SjByteVector &data);

	/*!
	 * Returns true if the frame is at least an appropriate size and has
	 * legal values.
	 */
	bool IsValid() const { return m_isValid; }


	/*!
	 * Returns the MPEG Version of the header.
	 */
	MPEG_HeaderVersion version() const { return m_version; }

	/*!
	 * Returns the layer version.  This will be between the values 1-3.
	 */
	int layer() const { return m_layer; }

	/*!
	 * Returns true if the MPEG protection bit is enabled.
	 */
	bool protectionEnabled() const { return m_protectionEnabled; }

	/*!
	 * Returns the bitrate encoded in the header.
	 */
	int bitrate() const {  return m_bitrate; }

	/*!
	 * Returns the sample rate in Hz.
	 */
	int sampleRate() const { return m_sampleRate; }

	/*!
	 * Returns true if the frame is padded.
	 */
	bool isPadded() const { return m_isPadded; }

	/*!
	 * Returns the channel mode for this frame.
	 */
	MPEG_HeaderChannelMode channelMode() const { return m_channelMode; }

	/*!
	 * Returns true if the copyrighted bit is set.
	 */
	bool isCopyrighted() const { return m_isCopyrighted; }

	/*!
	 * Returns true if the "original" bit is set.
	 */
	bool isOriginal() const { return m_isOriginal; }

	/*!
	 * Returns the frame length.
	 */
	int frameLength() const { return m_frameLength; }

	/*!
	 * Returns the emphasis (seldomly used)
	 */
	unsigned char emphasis() const { return m_emphasis; }

#ifdef __WXDEBUG__
	MPEG_Header &operator=(const MPEG_Header &h) { wxASSERT_MSG(0, wxT("do not copy MPEG_Header objects this way!")); return *this; }
#endif

private:
	bool                    m_isValid;
	MPEG_HeaderVersion      m_version;
	int                     m_layer;
	bool                    m_protectionEnabled;
	int                     m_bitrate;
	int                     m_sampleRate;
	bool                    m_isPadded;
	MPEG_HeaderChannelMode  m_channelMode;
	bool                    m_isCopyrighted;
	bool                    m_isOriginal;
	unsigned char           m_emphasis;
	int                     m_frameLength;
};



//! An implementation of the Xing VBR headers
/*!
 * This is a minimalistic implementation of the Xing VBR headers.  Xing
 * headers are often added to VBR (variable bit rate) MP3 streams to make it
 * easy to compute the length and quality of a VBR stream.  Our implementation
 * is only concerned with the total size of the stream (so that we can
 * calculate the total playing time and the average bitrate).  It uses
 * <a href="http://home.pcisys.net/~melanson/codecs/mp3extensions.txt">this text</a>
 * and the XMMS sources as references.
 */
class MPEG_XingHeader
{
public:
	/*!
	 * Parses a Xing header based on \a data.  The data must be at least 16
	 * bytes long (anything longer than this is discarded).
	 */
	MPEG_XingHeader(const SjByteVector &data);

	/*!
	 * Destroy this XingHeader instance.
	 */
	virtual ~MPEG_XingHeader() {}

	/*!
	 * Returns true if the data was parsed properly and if there is a vaild
	 * Xing header present.
	 */
	bool IsValid() const { return m_valid; }

	/*!
	 * Returns the total number of frames.
	 */
	SjUint totalFrames() const { return m_frames; }

	/*!
	 * Returns the total size of stream in bytes.
	 */
	SjUint totalSize() const { return m_size; }

	/*!
	 * Returns the offset for the start of this Xing header, given the
	 * version and channels of the frame
	 */
	static int xingHeaderOffset(MPEG_HeaderVersion v,
	                            MPEG_HeaderChannelMode c);

private:
	void parse(const SjByteVector &data);

	SjUint m_frames;
	SjUint m_size;
	bool m_valid;
};



#endif
