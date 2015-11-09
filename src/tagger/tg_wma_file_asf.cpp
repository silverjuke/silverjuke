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
#include "tg_wma_file_asf.h"
#include "tg_wma_file.h"
#include "tg_wma_tag.h"



#undef NDEBUG
#include <assert.h>

#define FRAME_HEADER_SIZE 17
// Fix Me! FRAME_HEADER_SIZE may be different.

static const TAGGER_GUID index_guid = {
	0x33000890, 0xe5b1, 0x11cf, { 0x89, 0xf4, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xcb },
};

/**********************************/
/* decoding */

//#define DEBUG

static uint64_t get_le64(WMA_File *f)
{
	SjByteVector bv = f->ReadBlock(8);
	return bv.toLongLong(false);
}

static unsigned int get_le32(WMA_File *f)
{
	SjByteVector bv = f->ReadBlock(4);
	return bv.toUInt(false);
}

static unsigned int get_le16(WMA_File *f)
{
	SjByteVector bv = f->ReadBlock(2);
	return bv.toShort(false);
}

static int get_byte(WMA_File *f)
{
	SjByteVector bv = f->ReadBlock(1);
	return bv[0];
}

static void get_guid(WMA_File *f, TAGGER_GUID *g)
{
	int i;

	g->v1 = get_le32(f);
	g->v2 = get_le16(f);
	g->v3 = get_le16(f);
	for(i=0; i<8; i++)
		g->v4[i] = get_byte(f);
}

#if 0
static void get_str16(WMA_File *f, String& s)
{
	int len; //, c;
	//  char *q;

	len = get_le16(f);

	ByteVector bv = f->ReadBlock(len * 2); // len is number of wide characters (see below)
	bv.append( ByteVector::fromShort(0) ); // NULL terminator
	s = String((wchar_t *) bv.data(), String::UTFLE16);

//   q = buf;
//   while (len > 0) {
//     c = get_le16(f);
//     if ((q - buf) < buf_size - 1)
//             *q++ = c;
//     len--;
//   }
//   *q = '\0';
}
#endif

static void get_str16_nolen(WMA_File *f, int len, wxString& s)
{
	SjByteVector bv = f->ReadBlock(len); // len here is apparently in bytes (see code below)
	bv.append( SjByteVector::fromShort(0) ); // NULL terminator
	s = bv.toString(SJ_UTF16LE);
	//s = String(bv, String::UTF16LE);

//   int c;
//   char *q;

//   q = buf;
//   while (len > 0) {
//     c = get_le16(f);
//     if ((q - buf) < buf_size - 1)
//             *q++ = c;
//     len-=2;
//   }
//   *q = '\0';
}



/* We could be given one of the three possible structures here:
 * WAVEFORMAT, PCMWAVEFORMAT or WAVEFORMATEX. Each structure
 * is an expansion of the previous one with the fields added
 * at the bottom. PCMWAVEFORMAT adds 'WORD wBitsPerSample' and
 * WAVEFORMATEX adds 'WORD  cbSize' and basically makes itself
 * an openended structure.
 */
void WMA_File::getWavHeader(int size)
{
	if ( wmaProperties ) {
		get_le16(this); // id
		wmaProperties->m_channels = get_le16(this);
		wmaProperties->m_sampleRate = get_le32(this);
		wmaProperties->m_bitrate = (get_le32(this) * 8) / 1000; // in kb/s
		/*codec->block_align = */get_le16(this);
	} else {
		Seek(2 + 2 + 4 + 4 + 2, SJ_SEEK_CUR);
	}

	if (size == 14) {  /* We're dealing with plain vanilla WAVEFORMAT */
		//      codec->bits_per_sample = 8;
	} else
		/* codec->bits_per_sample = */ get_le16(this);

	if (size > 16) {  /* We're obviously dealing with WAVEFORMATEX */
		unsigned int extradata_size = get_le16(this);
		if (extradata_size > 0) {
			if (extradata_size > (unsigned int)size - 18)
				extradata_size = size - 18;
			Seek(extradata_size, SJ_SEEK_CUR); // Just skip the extra data
		} else {
			extradata_size = 0;
		}

		/* It is possible for the chunk to contain garbage at the end */
		if (size - extradata_size - 18 > 0)
			Seek(size - extradata_size - 18, SJ_SEEK_CUR);
	}
}


int WMA_File::asfReadHeader()
{
	int noEndlessLoop = 0;

//  ASFContext asf;
	TAGGER_GUID g;
//  int size, i;
	int64_t gsize;

	get_guid(this, &g);
	if (memcmp(&g, &asf_header, sizeof(TAGGER_GUID)))
		goto fail;
	get_le64(this);
	get_le32(this);
	get_byte(this);
	get_byte(this);
	//  memset(&asf->asfid2avid, -1, sizeof(asf->asfid2avid));
	for(;;) {


		noEndlessLoop++;
		if( noEndlessLoop > 256 )
		{
			goto fail;
		}

		get_guid(this, &g);
		gsize = get_le64(this);
// #ifdef DEBUG
//     printf("%08Lx: ", url_ftell(f) - 24);
//     print_guid(&g);
//     printf("  size=0x%Lx\n", gsize);
// #endif
		if (gsize < 24)
			goto fail;
		if (!memcmp(&g, &file_header, sizeof(TAGGER_GUID))) {
			get_guid(this, &g);
			// asf.hdr.file_size      =
			get_le64(this);
			// asf.hdr.create_time    =
			get_le64(this);
			// asf.hdr.packets_count  =
			get_le64(this);
			// asf.hdr.play_time
			int64_t play_time = get_le64(this);
			// play_time is in 100ns = 10^-7s units
			if (wmaProperties)
				wmaProperties->m_length = (play_time / 10000000L);
			// asf.hdr.send_time      =
			get_le64(this);
			// asf.hdr.preroll        =
			get_le32(this);
			// asf.hdr.ignore     =
			get_le32(this);
			// asf.hdr.flags      =
			get_le32(this);
			// asf.hdr.min_pktsize    =
			get_le32(this);
			// asf.hdr.max_pktsize    =
			get_le32(this);
			// asf.hdr.max_bitrate    =
			get_le32(this);
			// asf.packet_size = asf.hdr.max_pktsize;
			// asf.nb_packets = asf.hdr.packets_count;
		} else if (!memcmp(&g, &stream_header, sizeof(TAGGER_GUID))) {
			//int type;
			int type_specific_size;
			//unsigned int tag1;
			int64_t pos1, pos2;

			pos1 = Tell();

			get_guid(this, &g);
			if (!memcmp(&g, &audio_stream, sizeof(TAGGER_GUID))) {
				//        type = CODEC_TYPE_AUDIO;
			} else {
				wxLogDebug(wxT("Tagger: WMA file contains non-audio streams."));
				return false;
			}

			get_guid(this, &g);
			get_le64(this); // total size
			type_specific_size = get_le32(this);
			get_le32(this);
			get_le16(this) /*& 0x7f*/; /* stream id */

			get_le32(this);
//       st->codec.codec_type = type;
//       /* 1 fps default (XXX: put 0 fps instead) */
//       st->codec.frame_rate = 1000;
//       st->codec.frame_rate_base = 1;

			//      if (type == CODEC_TYPE_AUDIO) {
			getWavHeader(type_specific_size);
			//        st->need_parsing = 1;
			/* We have to init the frame size at some point .... */
			pos2 = Tell();
			if (gsize > (pos2 + 8 - pos1 + 24)) {
				/* asf_st.ds_span = */ get_byte(this);
				/* asf_st.ds_packet_size = */ get_le16(this);
				// asf_st.ds_chunk_size =
				get_le16(this);
				//  asf_st.ds_data_size =
				get_le16(this);
				// asf_st.ds_silence_data =
				get_byte(this);
			}

			pos2 = Tell();
			Seek(gsize - (pos2 - pos1 + 24), SJ_SEEK_CUR);
		} else if (!memcmp(&g, &data_header, sizeof(TAGGER_GUID))) {
			break;
		} else if (!memcmp(&g, &comment_header, sizeof(TAGGER_GUID))) {
			int len1, len2, len3, len4, len5;

			len1 = get_le16(this);
			len2 = get_le16(this);
			len3 = get_le16(this);
			len4 = get_le16(this);
			len5 = get_le16(this);

			if (wmaTag) {
				wxString s;
				//char buf[1];

				get_str16_nolen(this, len1, s);
				wmaTag->setTitle(s);

				get_str16_nolen(this, len2, s);
				wmaTag->setLeadArtist(s);

				get_str16_nolen(this, len3, s); // Copyright notice

				get_str16_nolen(this, len4, s);
				wmaTag->setComment(s);

				Seek(len5, SJ_SEEK_CUR);
			} else {
				Seek(len1+len2+len3+len4+len5, SJ_SEEK_CUR);
			}
		} else if (!memcmp(&g, &extended_content_header, sizeof(TAGGER_GUID))) {
			int desc_count, i;

			desc_count = get_le16(this);
			for(i=0; i<desc_count; i++)
			{
				int name_len,value_type,value_len = 0;
				int64_t value_num = 0;
				wxString name, value;

				name_len = get_le16(this);
				get_str16_nolen(this, name_len, name);
				value_type = get_le16(this);
				value_len = get_le16(this);

				//debug (name);
				//                printf ("value_type is %d; name_len is %d\n", value_type, name_len);

				if ((value_type == 0) || (value_type == 1)) // unicode or byte
				{
					get_str16_nolen(this, value_len, value);
					//debug ("string value:");
					//debug(value);
					if ( wmaTag ) {
						if (name == wxT("WM/AlbumTitle"))
						{ wmaTag->setAlbum(value); }

						if (name == wxT("WM/Genre"))
						{ wmaTag->setGenre(value); }

						if (name == wxT("WM/Year"))
						{ long l; if(!value.ToLong(&l)){l=0;} wmaTag->setYear(l); }

						if (name==wxT("WM/TrackNumber"))
						{ long l; if(!value.ToLong(&l)){l=0;} wmaTag->setTrack(l, 0); }
					}
				}
				if ((value_type >= 2) && (value_type <= 5)) // boolean or DWORD or QWORD or WORD
				{
					if (value_type==2) value_num = get_le32(this);
					if (value_type==3) value_num = get_le32(this);
					if (value_type==4) value_num = get_le64(this);
					if (value_type==5) value_num = get_le16(this);

					//printf("numerical value: %d\n", value_num);
					if (wmaTag) {
						if (name==wxT("WM/Track"))
						{ wmaTag->setTrack(value_num + 1, 0); }

						if (name==wxT("WM/TrackNumber"))
						{ wmaTag->setTrack(value_num, 0); }
					}
				}
			}
#if 0
		} else if (!memcmp(&g, &head1_guid, sizeof(TAGGER_GUID))) {
			int v1, v2;
			get_guid(f, &g);
			v1 = get_le32(this);
			v2 = get_le16(this);
		} else if (!memcmp(&g, &codec_comment_header, sizeof(TAGGER_GUID))) {
			int len, v1, n, num;
			char str[256], *q;
			char tag[16];

			get_guid(this, &g);
			print_guid(&g);

			n = get_le32(this);
			for(i=0; i<n; i++) {
				num = get_le16(this); /* stream number */
				get_str16(this, str, sizeof(str));
				get_str16(this, str, sizeof(str));
				len = get_le16(this);
				q = tag;
				while (len > 0) {
					v1 = get_byte(this);
					if ((q - tag) < sizeof(tag) - 1)
						*q++ = v1;
					len--;
				}
				*q = '\0';
			}
#endif
			// FIXME: Can we eliminate all further reads?

			// FIXME: implement EOF check
//     } else if (url_feof(f)) {
//       goto fail;
		} else {
			Seek(gsize - 24, SJ_SEEK_CUR);
		}
	}
	get_guid(this, &g);
	get_le64(this);
	get_byte(this);
	get_byte(this);
	// FIXME: implement EOF check
//   if (url_feof(f))
//           goto fail;
//   asf->data_offset = url_ftell(f);
//   asf->packet_size_left = 0;

	return true;

fail:
	return false;
//   for(i=0;i<s->nb_streams;i++) {
//     AVStream *st = s->streams[i];
//     if (st) {
//       av_free(st->priv_data);
//       av_free(st->codec.extradata);
//     }
//     av_free(st);
//   }
//   return -1;
}
