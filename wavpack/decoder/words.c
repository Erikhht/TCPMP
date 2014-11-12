////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2004 Conifer Software.               //
//                          All Rights Reserved.                          //
////////////////////////////////////////////////////////////////////////////

// words.c

// This module provides entropy word encoding and decoding functions using
// a variation on the Rice method.  This was introduced in version 3.93
// because it allows splitting the data into a "lossy" stream and a
// "correction" stream in a very efficient manner and is therefore ideal
// for the "hybrid" mode.  For 4.0, the efficiency of this method was
// significantly improved by moving away from the normal Rice restriction of
// using powers of two for the modulus divisions and now the method can be
// used for both hybrid and pure lossless encoding.

// Samples are divided by median probabilities at 5/7 (71.43%), 10/49 (20.41%),
// and 20/343 (5.83%). Each zone has 3.5 times fewer samples than the
// previous. Using standard Rice coding on this data would result in 1.4
// bits per sample average (not counting sign bit). However, there is a
// very simple encoding that is over 99% efficient with this data and
// results in about 1.22 bits per sample.

#include "wavpack.h"

#include <string.h>

//////////////////////////////// local macros /////////////////////////////////

#define LIMIT_ONES 16	// maximum consecutive 1s sent for "div" data

// these control the time constant "slow_level" which is used for hybrid mode
// that controls bitrate as a function of residual level (HYBRID_BITRATE).
#define SLS 8
#define SLO ((1 << (SLS - 1)))

// these control the time constant of the 3 median level breakpoints
#define DIV0 128	// 5/7 of samples
#define DIV1 64		// 10/49 of samples
#define DIV2 32		// 20/343 of samples

// this macro retrieves the specified median breakpoint (without frac; min = 1)
#define GET_MED(med) (((wps->w.median [med] [chan]) >> 4) + 1)

// These macros update the specified median breakpoints. Note that the median
// is incremented when the sample is higher than the median, else decremented.
// They are designed so that the median will never drop below 1 and the value
// is essentially stationary if there are 2 increments for every 5 decrements.

#define INC_MED0() (wps->w.median [0] [chan] += ((wps->w.median [0] [chan] + DIV0) / DIV0) * 5)
#define DEC_MED0() (wps->w.median [0] [chan] -= ((wps->w.median [0] [chan] + (DIV0-2)) / DIV0) * 2)
#define INC_MED1() (wps->w.median [1] [chan] += ((wps->w.median [1] [chan] + DIV1) / DIV1) * 5)
#define DEC_MED1() (wps->w.median [1] [chan] -= ((wps->w.median [1] [chan] + (DIV1-2)) / DIV1) * 2)
#define INC_MED2() (wps->w.median [2] [chan] += ((wps->w.median [2] [chan] + DIV2) / DIV2) * 5)
#define DEC_MED2() (wps->w.median [2] [chan] -= ((wps->w.median [2] [chan] + (DIV2-2)) / DIV2) * 2)

#define count_bits(av) ( \
 (av) < (1 << 8) ? nbits_table [av] : \
  ( \
   (av) < (1L << 16) ? nbits_table [(av) >> 8] + 8 : \
   ((av) < (1L << 24) ? nbits_table [(av) >> 16] + 16 : nbits_table [(av) >> 24] + 24) \
  ) \
)

///////////////////////////// local table storage ////////////////////////////

const char nbits_table [] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,	// 0 - 15
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,	// 16 - 31
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,	// 32 - 47
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,	// 48 - 63
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,	// 64 - 79
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,	// 80 - 95
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,	// 96 - 111
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,	// 112 - 127
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,	// 128 - 143
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,	// 144 - 159
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,	// 160 - 175
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,	// 176 - 191
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,	// 192 - 207
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,	// 208 - 223
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,	// 224 - 239
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8	// 240 - 255
};

static const uchar log2_table [] = {
    0x00, 0x01, 0x03, 0x04, 0x06, 0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x14, 0x15,
    0x16, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e, 0x20, 0x21, 0x22, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a,
    0x2c, 0x2d, 0x2e, 0x2f, 0x31, 0x32, 0x33, 0x34, 0x36, 0x37, 0x38, 0x39, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f, 0x41, 0x42, 0x43, 0x44, 0x45, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
    0x52, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63,
    0x64, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
    0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
    0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb2,
    0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc0,
    0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcb, 0xcc, 0xcd, 0xce,
    0xcf, 0xd0, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd8, 0xd9, 0xda, 0xdb,
    0xdc, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe4, 0xe5, 0xe6, 0xe7, 0xe7,
    0xe8, 0xe9, 0xea, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xee, 0xef, 0xf0, 0xf1, 0xf1, 0xf2, 0xf3, 0xf4,
    0xf4, 0xf5, 0xf6, 0xf7, 0xf7, 0xf8, 0xf9, 0xf9, 0xfa, 0xfb, 0xfc, 0xfc, 0xfd, 0xfe, 0xff, 0xff
};

static const uchar exp2_table [] = {
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a, 0x0b,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0e, 0x0f, 0x10, 0x10, 0x11, 0x12, 0x13, 0x13, 0x14, 0x15, 0x16, 0x16,
    0x17, 0x18, 0x19, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1d, 0x1e, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3a, 0x3b, 0x3c, 0x3d,
    0x3e, 0x3f, 0x40, 0x41, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x48, 0x49, 0x4a, 0x4b,
    0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a,
    0x5b, 0x5c, 0x5d, 0x5e, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x87, 0x88, 0x89, 0x8a,
    0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad,
    0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0,
    0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc8, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf, 0xd0, 0xd2, 0xd3, 0xd4,
    0xd6, 0xd7, 0xd8, 0xd9, 0xdb, 0xdc, 0xdd, 0xde, 0xe0, 0xe1, 0xe2, 0xe4, 0xe5, 0xe6, 0xe8, 0xe9,
    0xea, 0xec, 0xed, 0xee, 0xf0, 0xf1, 0xf2, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfc, 0xfd, 0xff
};

///////////////////////////// executable code ////////////////////////////////

static int log2 (unsigned long avalue);

// Read the median log2 values from the specifed metadata structure, convert
// them back to 32-bit unsigned values and store them. If length is not
// exactly correct then we flag and return an error.

int read_entropy_vars (WavpackStream *wps, WavpackMetadata *wpmd)
{
    uchar *byteptr = wpmd->data;

    if (wpmd->byte_length != ((wps->wphdr.flags & MONO_FLAG) ? 6 : 12))
	return FALSE;

    wps->w.median [0] [0] = exp2s (byteptr [0] + (byteptr [1] << 8));
    wps->w.median [1] [0] = exp2s (byteptr [2] + (byteptr [3] << 8));
    wps->w.median [2] [0] = exp2s (byteptr [4] + (byteptr [5] << 8));

    if (!(wps->wphdr.flags & MONO_FLAG)) {
	wps->w.median [0] [1] = exp2s (byteptr [6] + (byteptr [7] << 8));
	wps->w.median [1] [1] = exp2s (byteptr [8] + (byteptr [9] << 8));
	wps->w.median [2] [1] = exp2s (byteptr [10] + (byteptr [11] << 8));
    }

    return TRUE;
}

// Read the hybrid related values from the specifed metadata structure, convert
// them back to their internal formats and store them. The extended profile
// stuff is not implemented yet, so return an error if we get more data than
// we know what to do with.

int read_hybrid_profile (WavpackStream *wps, WavpackMetadata *wpmd)
{
    uchar *byteptr = wpmd->data;
    uchar *endptr = byteptr + wpmd->byte_length;

    if (wps->wphdr.flags & HYBRID_BITRATE) {
	wps->w.slow_level [0] = exp2s (byteptr [0] + (byteptr [1] << 8));
	byteptr += 2;

	if (!(wps->wphdr.flags & MONO_FLAG)) {
	    wps->w.slow_level [1] = exp2s (byteptr [0] + (byteptr [1] << 8));
	    byteptr += 2;
	}
    }

    wps->w.bitrate_acc [0] = (long)(byteptr [0] + (byteptr [1] << 8)) << 16;
    byteptr += 2;

    if (!(wps->wphdr.flags & MONO_FLAG)) {
	wps->w.bitrate_acc [1] = (long)(byteptr [0] + (byteptr [1] << 8)) << 16;
	byteptr += 2;
    }

    if (byteptr < endptr) {
	wps->w.bitrate_delta [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	byteptr += 2;

	if (!(wps->wphdr.flags & MONO_FLAG)) {
	    wps->w.bitrate_delta [1] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	    byteptr += 2;
	}

	if (byteptr < endptr)
	    return FALSE;
    }
    else
	wps->w.bitrate_delta [0] = wps->w.bitrate_delta [1] = 0;

    return TRUE;
}

// This function is called during both encoding and decoding of hybrid data to
// update the "error_limit" variable which determines the maximum sample error
// allowed in the main bitstream. In the HYBRID_BITRATE mode (which is the only
// currently implemented) this is calculated from the slow_level values and the
// bitrate accumulators. Note that the bitrate accumulators can be changing.

static void update_error_limit (WavpackStream *wps)
{
    int bitrate_0 = (wps->w.bitrate_acc [0] += wps->w.bitrate_delta [0]) >> 16;

    if (wps->wphdr.flags & MONO_FLAG) {
	if (wps->wphdr.flags & HYBRID_BITRATE) {
	    int slow_log_0 = (wps->w.slow_level [0] + SLO) >> SLS;

	    if (slow_log_0 - bitrate_0 > -0x100)
		wps->w.error_limit [0] = exp2s (slow_log_0 - bitrate_0 + 0x100);
	    else
		wps->w.error_limit [0] = 0;
	}
	else
	    wps->w.error_limit [0] = exp2s (bitrate_0);
    }
    else {
	int bitrate_1 = (wps->w.bitrate_acc [1] += wps->w.bitrate_delta [1]) >> 16;

	if (wps->wphdr.flags & HYBRID_BITRATE) {
	    int slow_log_0 = (wps->w.slow_level [0] + SLO) >> SLS;
	    int slow_log_1 = (wps->w.slow_level [1] + SLO) >> SLS;

	    if (wps->wphdr.flags & HYBRID_BALANCE) {
		int balance = (slow_log_1 - slow_log_0 + bitrate_1 + 1) >> 1;

		if (balance > bitrate_0) {
		    bitrate_1 = bitrate_0 * 2;
		    bitrate_0 = 0;
		}
		else if (-balance > bitrate_0) {
		    bitrate_0 = bitrate_0 * 2;
		    bitrate_1 = 0;
		}
		else {
		    bitrate_1 = bitrate_0 + balance;
		    bitrate_0 = bitrate_0 - balance;
		}
	    }

	    if (slow_log_0 - bitrate_0 > -0x100)
		wps->w.error_limit [0] = exp2s (slow_log_0 - bitrate_0 + 0x100);
	    else
		wps->w.error_limit [0] = 0;

	    if (slow_log_1 - bitrate_1 > -0x100)
		wps->w.error_limit [1] = exp2s (slow_log_1 - bitrate_1 + 0x100);
	    else
		wps->w.error_limit [1] = 0;
	}
	else {
	    wps->w.error_limit [0] = exp2s (bitrate_0);
	    wps->w.error_limit [1] = exp2s (bitrate_1);
	}
    }
}

static ulong read_code (Bitstream *bs, ulong maxcode);

// Read the next word from the bitstream "wvbits" and return the value. This
// function can be used for hybrid or lossless streams, but since an
// optimized version is available for lossless this function would normally
// be used for hybrid only. If a hybrid lossless stream is being read then
// the "correction" offset is written at the specified pointer. A return value
// of WORD_EOF indicates that the end of the bitstream was reached (all 1s) or
// some other error occurred.

long get_word (WavpackStream *wps, int chan)
{
    ulong ones_count, low, mid, high;
    int sign;

    if (wps->w.zeros_acc) {
	if (--wps->w.zeros_acc) {
	    wps->w.slow_level [chan] -= (wps->w.slow_level [chan] + SLO) >> SLS;
	    return 0;
	}
    }
    else if (!wps->w.holding_zero && !wps->w.holding_one && !(wps->w.median [0] [0] & ~1) && !(wps->w.median [0] [1] & ~1)) {
	ulong mask;
	int cbits;

	for (cbits = 0; cbits < 33 && getbit (&wps->wvbits); ++cbits);

	if (cbits == 33)
	    return WORD_EOF;

	if (cbits < 2)
	    wps->w.zeros_acc = cbits;
	else {
	    for (mask = 1, wps->w.zeros_acc = 0; --cbits; mask <<= 1)
		if (getbit (&wps->wvbits))
		    wps->w.zeros_acc |= mask;

	    wps->w.zeros_acc |= mask;
	}

	if (wps->w.zeros_acc) {
	    wps->w.slow_level [chan] -= (wps->w.slow_level [chan] + SLO) >> SLS;
	    CLEAR (wps->w.median);
	    return 0;
	}
    }

    if (wps->w.holding_zero)
	ones_count = wps->w.holding_zero = 0;
    else {
#ifdef LIMIT_ONES
	for (ones_count = 0; ones_count < (LIMIT_ONES + 1) && getbit (&wps->wvbits); ++ones_count);

	if (ones_count == (LIMIT_ONES + 1))
	    return WORD_EOF;

	if (ones_count == LIMIT_ONES) {
	    ulong mask;
	    int cbits;

	    for (cbits = 0; cbits < 33 && getbit (&wps->wvbits); ++cbits);

	    if (cbits == 33)
		return WORD_EOF;

	    if (cbits < 2)
		ones_count = cbits;
	    else {
		for (mask = 1, ones_count = 0; --cbits; mask <<= 1)
		    if (getbit (&wps->wvbits))
			ones_count |= mask;

		ones_count |= mask;
	    }

	    ones_count += LIMIT_ONES;
	}
#else
	for (ones_count = 0; getbit (&wps->wvbits); ++ones_count);
#endif

	if (wps->w.holding_one) {
	    wps->w.holding_one = ones_count & 1;
	    ones_count = (ones_count >> 1) + 1;
	}
	else {
	    wps->w.holding_one = ones_count & 1;
	    ones_count >>= 1;
	}

	wps->w.holding_zero = ~wps->w.holding_one & 1;
    }

    if ((wps->wphdr.flags & HYBRID_FLAG) && !chan)
	update_error_limit (wps);

    if (ones_count == 0) {
	low = 0;
	high = GET_MED (0) - 1;
	DEC_MED0 ();
    }
    else {
	low = GET_MED (0);
	INC_MED0 ();

	if (ones_count == 1) {
	    high = low + GET_MED (1) - 1;
	    DEC_MED1 ();
	}
	else {
	    low += GET_MED (1);
	    INC_MED1 ();

	    if (ones_count == 2) {
		high = low + GET_MED (2) - 1;
		DEC_MED2 ();
	    }
	    else {
		low += (ones_count - 2) * GET_MED (2);
		high = low + GET_MED (2) - 1;
		INC_MED2 ();
	    }
	}
    }

    mid = (high + low + 1) >> 1;

    if (!wps->w.error_limit [chan])
	mid = read_code (&wps->wvbits, high - low) + low;
    else while (high - low > wps->w.error_limit [chan]) {
	if (getbit (&wps->wvbits))
	    mid = (high + (low = mid) + 1) >> 1;
	else
	    mid = ((high = mid - 1) + low + 1) >> 1;
    }

    sign = getbit (&wps->wvbits);

    if (wps->wphdr.flags & HYBRID_BITRATE) {
	wps->w.slow_level [chan] -= (wps->w.slow_level [chan] + SLO) >> SLS;
	wps->w.slow_level [chan] += log2 (mid);
    }

    return sign ? ~mid : mid;
}

// Read a single unsigned value from the specified bitstream with a value
// from 0 to maxcode. If there are exactly a power of two number of possible
// codes then this will read a fixed number of bits; otherwise it reads the
// minimum number of bits and then determines whether another bit is needed
// to define the code.

static ulong read_code (Bitstream *bs, ulong maxcode)
{
    int bitcount = count_bits (maxcode);
    ulong extras = (1L << bitcount) - maxcode - 1, code;

    if (!bitcount)
	return 0;

    getbits (&code, bitcount - 1, bs);
    code &= (1L << (bitcount - 1)) - 1;

    if (code >= extras) {
	code = (code << 1) - extras;

	if (getbit (bs))
	    ++code;
    }

    return code;
}

// The concept of a base 2 logarithm is used in many parts of WavPack. It is
// a way of sufficiently accurately representing 32-bit signed and unsigned
// values storing only 16 bits (actually fewer). It is also used in the hybrid
// mode for quickly comparing the relative magnitude of large values (i.e.
// division) and providing smooth exponentials using only addition.

// These are not strict logarithms in that they become linear around zero and
// can therefore represent both zero and negative values. They have 8 bits
// of precision and in "roundtrip" conversions the total error never exceeds 1
// part in 225 except for the cases of +/-115 and +/-195 (which error by 1).


// This function returns the log2 for the specified 32-bit unsigned value.
// The maximum value allowed is about 0xff800000 and returns 8447.

static int log2 (unsigned long avalue)
{
    int dbits;

    if ((avalue += avalue >> 9) < (1 << 8)) {
	dbits = nbits_table [avalue];
	return (dbits << 8) + log2_table [(avalue << (9 - dbits)) & 0xff];
    }
    else {
	if (avalue < (1L << 16))
	    dbits = nbits_table [avalue >> 8] + 8;
	else if (avalue < (1L << 24))
	    dbits = nbits_table [avalue >> 16] + 16;
	else
	    dbits = nbits_table [avalue >> 24] + 24;

	return (dbits << 8) + log2_table [(avalue >> (dbits - 9)) & 0xff];
    }
}

// This function returns the original integer represented by the supplied
// logarithm (at least within the provided accuracy). The log is signed,
// but since a full 32-bit value is returned this can be used for unsigned
// conversions as well (i.e. the input range is -8192 to +8447).

long exp2s (int log)
{
    ulong value;

    if (log < 0)
	return -exp2s (-log);

    value = exp2_table [log & 0xff] | 0x100;

    if ((log >>= 8) <= 9)
	return value >> (9 - log);
    else
	return value << (log - 9);
}

// These two functions convert internal weights (which are normally +/-1024)
// to and from an 8-bit signed character version for storage in metadata. The
// weights are clipped here in the case that they are outside that range.

int restore_weight (char weight)
{
    int result;

    if ((result = (int) weight << 3) > 0)
	result += (result + 64) >> 7;

    return result;
}
