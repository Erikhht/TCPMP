/* libFLAC++ - Free Lossless Audio Codec library
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

#include "FLAC++/encoder.h"
#include "FLAC/assert.h"

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

namespace FLAC {
	namespace Encoder {

		SeekableStream::SeekableStream():
		encoder_(::FLAC__seekable_stream_encoder_new())
		{ }

		SeekableStream::~SeekableStream()
		{
			if(0 != encoder_) {
				::FLAC__seekable_stream_encoder_finish(encoder_);
				::FLAC__seekable_stream_encoder_delete(encoder_);
			}
		}

		bool SeekableStream::is_valid() const
		{
			return 0 != encoder_;
		}

		bool SeekableStream::set_verify(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_verify(encoder_, value);
		}

		bool SeekableStream::set_streamable_subset(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_streamable_subset(encoder_, value);
		}

		bool SeekableStream::set_do_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_do_mid_side_stereo(encoder_, value);
		}

		bool SeekableStream::set_loose_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(encoder_, value);
		}

		bool SeekableStream::set_channels(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_channels(encoder_, value);
		}

		bool SeekableStream::set_bits_per_sample(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_bits_per_sample(encoder_, value);
		}

		bool SeekableStream::set_sample_rate(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_sample_rate(encoder_, value);
		}

		bool SeekableStream::set_blocksize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_blocksize(encoder_, value);
		}

		bool SeekableStream::set_max_lpc_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_max_lpc_order(encoder_, value);
		}

		bool SeekableStream::set_qlp_coeff_precision(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_qlp_coeff_precision(encoder_, value);
		}

		bool SeekableStream::set_do_qlp_coeff_prec_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(encoder_, value);
		}

		bool SeekableStream::set_do_escape_coding(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_do_escape_coding(encoder_, value);
		}

		bool SeekableStream::set_do_exhaustive_model_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(encoder_, value);
		}

		bool SeekableStream::set_min_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_min_residual_partition_order(encoder_, value);
		}

		bool SeekableStream::set_max_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_max_residual_partition_order(encoder_, value);
		}

		bool SeekableStream::set_rice_parameter_search_dist(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(encoder_, value);
		}

		bool SeekableStream::set_total_samples_estimate(FLAC__uint64 value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_total_samples_estimate(encoder_, value);
		}

		bool SeekableStream::set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_set_metadata(encoder_, metadata, num_blocks);
		}

		bool SeekableStream::set_metadata(FLAC::Metadata::Prototype **metadata, unsigned num_blocks)
		{
			FLAC__ASSERT(is_valid());
#if (defined _MSC_VER) || (defined __SUNPRO_CC)
			// MSVC++ can't handle:
			// ::FLAC__StreamMetadata *m[num_blocks];
			// so we do this ugly workaround
			::FLAC__StreamMetadata **m = new ::FLAC__StreamMetadata*[num_blocks];
#else
			::FLAC__StreamMetadata *m[num_blocks];
#endif
			for(unsigned i = 0; i < num_blocks; i++) {
				// we can get away with this since we know the encoder will only correct the is_last flags
				m[i] = const_cast< ::FLAC__StreamMetadata*>((::FLAC__StreamMetadata*)metadata[i]);
			}
#if (defined _MSC_VER) || (defined __SUNPRO_CC)
			// complete the hack
			const bool ok = (bool)::FLAC__seekable_stream_encoder_set_metadata(encoder_, m, num_blocks);
			delete [] m;
			return ok;
#else
			return (bool)::FLAC__seekable_stream_encoder_set_metadata(encoder_, m, num_blocks);
#endif
		}

		SeekableStream::State SeekableStream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::FLAC__seekable_stream_encoder_get_state(encoder_));
		}

		Stream::State SeekableStream::get_stream_encoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return Stream::State(::FLAC__seekable_stream_encoder_get_stream_encoder_state(encoder_));
		}

		Decoder::Stream::State SeekableStream::get_verify_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return Decoder::Stream::State(::FLAC__seekable_stream_encoder_get_verify_decoder_state(encoder_));
		}

		void SeekableStream::get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
		{
			FLAC__ASSERT(is_valid());
			::FLAC__seekable_stream_encoder_get_verify_decoder_error_stats(encoder_, absolute_sample, frame_number, channel, sample, expected, got);
		}

		bool SeekableStream::get_verify() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_get_verify(encoder_);
		}

		bool SeekableStream::get_streamable_subset() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_get_streamable_subset(encoder_);
		}

		bool SeekableStream::get_do_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_get_do_mid_side_stereo(encoder_);
		}

		bool SeekableStream::get_loose_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(encoder_);
		}

		unsigned SeekableStream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_channels(encoder_);
		}

		unsigned SeekableStream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_bits_per_sample(encoder_);
		}

		unsigned SeekableStream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_sample_rate(encoder_);
		}

		unsigned SeekableStream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_blocksize(encoder_);
		}

		unsigned SeekableStream::get_max_lpc_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_max_lpc_order(encoder_);
		}

		unsigned SeekableStream::get_qlp_coeff_precision() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_qlp_coeff_precision(encoder_);
		}

		bool SeekableStream::get_do_qlp_coeff_prec_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(encoder_);
		}

		bool SeekableStream::get_do_escape_coding() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_get_do_escape_coding(encoder_);
		}

		bool SeekableStream::get_do_exhaustive_model_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(encoder_);
		}

		unsigned SeekableStream::get_min_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_min_residual_partition_order(encoder_);
		}

		unsigned SeekableStream::get_max_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_max_residual_partition_order(encoder_);
		}

		unsigned SeekableStream::get_rice_parameter_search_dist() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(encoder_);
		}

		FLAC__uint64 SeekableStream::get_total_samples_estimate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__seekable_stream_encoder_get_total_samples_estimate(encoder_);
		}

		SeekableStream::State SeekableStream::init()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__seekable_stream_encoder_set_seek_callback(encoder_, seek_callback_);
			::FLAC__seekable_stream_encoder_set_tell_callback(encoder_, tell_callback_);
			::FLAC__seekable_stream_encoder_set_write_callback(encoder_, write_callback_);
			::FLAC__seekable_stream_encoder_set_client_data(encoder_, (void*)this);
			return State(::FLAC__seekable_stream_encoder_init(encoder_));
		}

		void SeekableStream::finish()
		{
			FLAC__ASSERT(is_valid());
			::FLAC__seekable_stream_encoder_finish(encoder_);
		}

		bool SeekableStream::process(const FLAC__int32 * const buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_process(encoder_, buffer, samples);
		}

		bool SeekableStream::process_interleaved(const FLAC__int32 buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__seekable_stream_encoder_process_interleaved(encoder_, buffer, samples);
		}

		::FLAC__SeekableStreamEncoderSeekStatus SeekableStream::seek_callback_(const ::FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->seek_callback(absolute_byte_offset);
		}

		::FLAC__SeekableStreamEncoderTellStatus SeekableStream::tell_callback_(const ::FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->tell_callback(absolute_byte_offset);
		}

		::FLAC__StreamEncoderWriteStatus SeekableStream::write_callback_(const ::FLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			SeekableStream *instance = reinterpret_cast<SeekableStream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(buffer, bytes, samples, current_frame);
		}

	}
}
