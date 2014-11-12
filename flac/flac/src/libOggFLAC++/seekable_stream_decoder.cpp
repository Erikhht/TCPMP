/* libOggFLAC++ - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "OggFLAC++/decoder.h"
#include "FLAC/assert.h"

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

namespace OggFLAC {
	namespace Decoder {

		SeekableStream::SeekableStream():
		decoder_(::OggFLAC__seekable_stream_decoder_new())
		{ }

		SeekableStream::~SeekableStream()
		{
			if(0 != decoder_) {
				(void) ::OggFLAC__seekable_stream_decoder_finish(decoder_);
				::OggFLAC__seekable_stream_decoder_delete(decoder_);
			}
		}

		bool SeekableStream::is_valid() const
		{
			return 0 != decoder_;
		}

		bool SeekableStream::set_serial_number(long value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_serial_number(decoder_, value);
		}

		bool SeekableStream::set_md5_checking(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_md5_checking(decoder_, value);
		}

		bool SeekableStream::set_metadata_respond(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_metadata_respond(decoder_, type);
		}

		bool SeekableStream::set_metadata_respond_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_metadata_respond_application(decoder_, id);
		}

		bool SeekableStream::set_metadata_respond_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder_);
		}

		bool SeekableStream::set_metadata_ignore(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_metadata_ignore(decoder_, type);
		}

		bool SeekableStream::set_metadata_ignore_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder_, id);
		}

		bool SeekableStream::set_metadata_ignore_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder_);
		}

		SeekableStream::State SeekableStream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::OggFLAC__seekable_stream_decoder_get_state(decoder_));
		}

		OggFLAC::Decoder::Stream::State SeekableStream::get_stream_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return OggFLAC::Decoder::Stream::State(::OggFLAC__seekable_stream_decoder_get_stream_decoder_state(decoder_));
		}

		FLAC::Decoder::Stream::State SeekableStream::get_FLAC_stream_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return FLAC::Decoder::Stream::State(::OggFLAC__seekable_stream_decoder_get_FLAC_stream_decoder_state(decoder_));
		}

		bool SeekableStream::get_md5_checking() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_get_md5_checking(decoder_);
		}

		unsigned SeekableStream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__seekable_stream_decoder_get_channels(decoder_);
		}

		::FLAC__ChannelAssignment SeekableStream::get_channel_assignment() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__seekable_stream_decoder_get_channel_assignment(decoder_);
		}

		unsigned SeekableStream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__seekable_stream_decoder_get_bits_per_sample(decoder_);
		}

		unsigned SeekableStream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__seekable_stream_decoder_get_sample_rate(decoder_);
		}

		unsigned SeekableStream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::OggFLAC__seekable_stream_decoder_get_blocksize(decoder_);
		}

		SeekableStream::State SeekableStream::init()
		{
			FLAC__ASSERT(is_valid());
			::OggFLAC__seekable_stream_decoder_set_read_callback(decoder_, read_callback_);
			::OggFLAC__seekable_stream_decoder_set_seek_callback(decoder_, seek_callback_);
			::OggFLAC__seekable_stream_decoder_set_tell_callback(decoder_, tell_callback_);
			::OggFLAC__seekable_stream_decoder_set_length_callback(decoder_, length_callback_);
			::OggFLAC__seekable_stream_decoder_set_eof_callback(decoder_, eof_callback_);
			::OggFLAC__seekable_stream_decoder_set_write_callback(decoder_, write_callback_);
			::OggFLAC__seekable_stream_decoder_set_metadata_callback(decoder_, metadata_callback_);
			::OggFLAC__seekable_stream_decoder_set_error_callback(decoder_, error_callback_);
			::OggFLAC__seekable_stream_decoder_set_client_data(decoder_, (void*)this);
			return State(::OggFLAC__seekable_stream_decoder_init(decoder_));
		}

		bool SeekableStream::finish()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_finish(decoder_);
		}

		bool SeekableStream::flush()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_flush(decoder_);
		}

		bool SeekableStream::reset()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_reset(decoder_);
		}

		bool SeekableStream::process_single()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_process_single(decoder_);
		}

		bool SeekableStream::process_until_end_of_metadata()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_process_until_end_of_metadata(decoder_);
		}

		bool SeekableStream::process_until_end_of_stream()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_process_until_end_of_stream(decoder_);
		}

		bool SeekableStream::seek_absolute(FLAC__uint64 sample)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::OggFLAC__seekable_stream_decoder_seek_absolute(decoder_, sample);
		}

		::OggFLAC__SeekableStreamDecoderReadStatus SeekableStream::read_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->read_callback(buffer, bytes);
		}

		::OggFLAC__SeekableStreamDecoderSeekStatus SeekableStream::seek_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->seek_callback(absolute_byte_offset);
		}

		::OggFLAC__SeekableStreamDecoderTellStatus SeekableStream::tell_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->tell_callback(absolute_byte_offset);
		}

		::OggFLAC__SeekableStreamDecoderLengthStatus SeekableStream::length_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->length_callback(stream_length);
		}

		FLAC__bool SeekableStream::eof_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->eof_callback();
		}

		::FLAC__StreamDecoderWriteStatus SeekableStream::write_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(frame, buffer);
		}

		void SeekableStream::metadata_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, const ::FLAC__StreamMetadata *metadata, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->metadata_callback(metadata);
		}

		void SeekableStream::error_callback_(const ::OggFLAC__SeekableStreamDecoder *decoder, ::FLAC__StreamDecoderErrorStatus status, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->error_callback(status);
		}

	}
}
