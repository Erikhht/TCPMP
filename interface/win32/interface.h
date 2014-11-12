/*****************************************************************************
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: interface.h 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __INTERFACE_H
#define __INTERFACE_H

void Interface_Init();
void Interface_Done();

#define MAXCMD							1024

#define MSG_INIT						WM_APP + 0x201
#define MSG_PLAYER						WM_APP + 0x202
#define MSG_ERROR						WM_APP + 0x203

#define IF_ALL_TYPES					0x0CA
#define IF_OPTIONS						0x0DD
#define IF_OPTIONS_SPEED				0x0CD
#define IF_OPTIONS_REPEAT			    0x132
#define IF_OPTIONS_SHUFFLE			    0x133
#define	IF_OPTIONS_ZOOM					0x0CF
#define IF_OPTIONS_ZOOM_FIT_SCREEN		0x138
#define IF_OPTIONS_ZOOM_FIT_110			0x234
#define IF_OPTIONS_ZOOM_FIT_120			0x235
#define IF_OPTIONS_ZOOM_FIT_130			0x236
#define IF_OPTIONS_ZOOM_FIT_140			0x237
#define IF_OPTIONS_ZOOM_STRETCH_SCREEN	0x24A
#define IF_OPTIONS_ZOOM_FILL_SCREEN		0x24B
#define IF_OPTIONS_ZOOM_50				0x139
#define IF_OPTIONS_ZOOM_100				0x13A
#define IF_OPTIONS_ZOOM_150				0x18E
#define IF_OPTIONS_ZOOM_200				0x13B
#define IF_OPTIONS_VIDEO				0x0D2
#define IF_OPTIONS_VIDEO_DITHER         0x134
#define IF_OPTIONS_VIDEO_ZOOM_SMOOTH50	0x13C
#define IF_OPTIONS_VIDEO_ZOOM_SMOOTHALWAYS 0x22A
#define IF_OPTIONS_AUDIO				0x0D3
#define IF_OPTIONS_AUDIO_QUALITY_LOW    0x144
#define IF_OPTIONS_AUDIO_QUALITY_MEDIUM 0x145
#define IF_OPTIONS_AUDIO_QUALITY_HIGH   0x146
#define IF_OPTIONS_SPEED_10	            0x0DE
#define IF_OPTIONS_SPEED_25             0x14D
#define IF_OPTIONS_SPEED_50             0x14E
#define IF_OPTIONS_SPEED_80             0x230
#define IF_OPTIONS_SPEED_90             0x231
#define IF_OPTIONS_SPEED_100            0x14F
#define IF_OPTIONS_SPEED_110            0x232
#define IF_OPTIONS_SPEED_120            0x233
#define IF_OPTIONS_SPEED_150            0x0CE
#define IF_OPTIONS_SPEED_200            0x154
#define IF_OPTIONS_SPEED_300            0x155
#define IF_OPTIONS_SPEED_BENCHMARK      0x156
#define IF_OPTIONS_VIDEO_TURNOFF        0x157
#define IF_OPTIONS_AUDIO_TURNOFF        0x158
#define IF_OPTIONS_FULLSCREEN           0x17F
#define IF_OPTIONS_MUTE                 0x180
#define IF_OPTIONS_SETTINGS             0x177
#define IF_OPTIONS_ORIENTATION			0x0D0
#define IF_OPTIONS_ORIENTATION_FULLSCREEN 0x0D1
#define IF_OPTIONS_ROTATE_GUI           0x247
#define IF_OPTIONS_ROTATE_0             0x188
#define IF_OPTIONS_ROTATE_90            0x185
#define IF_OPTIONS_ROTATE_180           0x186
#define IF_OPTIONS_ROTATE_270           0x189
#define IF_OPTIONS_ROTATE				0x0F0
#define IF_OPTIONS_VIDEO_DEVICE         0x18B
#define IF_OPTIONS_VIDEO_ACCEL          0x0E7
#define IF_OPTIONS_AUDIO_DEVICE         0x18C
#define IF_OPTIONS_AUDIO_QUALITY        0x18D
#define IF_OPTIONS_VOLUME_UP			0x0F3
#define IF_OPTIONS_VOLUME_DOWN			0x0F4
#define IF_OPTIONS_VOLUME_UP_FINE		0x22B
#define IF_OPTIONS_VOLUME_DOWN_FINE		0x22C
#define IF_OPTIONS_BRIGHTNESS_UP		0x0F5
#define IF_OPTIONS_BRIGHTNESS_DOWN		0x0F6
#define IF_OPTIONS_CONTRAST_UP			0x0F7
#define IF_OPTIONS_CONTRAST_DOWN		0x0F8
#define IF_OPTIONS_SATURATION_UP		0x102
#define IF_OPTIONS_SATURATION_DOWN		0x101
#define IF_OPTIONS_SCREEN				0x0F9
#define IF_OPTIONS_EQUALIZER			0x122
#define IF_OPTIONS_VIDEO_STREAM			0x201
#define IF_OPTIONS_VIDEO_STREAM_NONE	0x221
#define IF_OPTIONS_AUDIO_STREAM			0x202
#define IF_OPTIONS_AUDIO_STREAM_NONE	0x222
#define IF_OPTIONS_AUDIO_CHANNELS		0x216
#define IF_OPTIONS_AUDIO_STEREO			0x203
#define IF_OPTIONS_AUDIO_STEREO_SWAPPED 0x140
#define IF_OPTIONS_AUDIO_STEREO_JOINED  0x248
#define IF_OPTIONS_AUDIO_STEREO_LEFT	0x204
#define IF_OPTIONS_AUDIO_STEREO_RIGHT	0x205
#define IF_OPTIONS_VIEW					0x20E
#define IF_OPTIONS_VIEW_TITLEBAR		0x240
#define IF_OPTIONS_VIEW_TRACKBAR		0x241
#define IF_OPTIONS_VIEW_TASKBAR			0x242
#define IF_OPTIONS_VIEW					0x20E
#define IF_OPTIONS_ASPECT				0x20F
#define IF_OPTIONS_ASPECT_SQUARE		0x210
#define IF_OPTIONS_ASPECT_4_3_PAL		0x211
#define IF_OPTIONS_ASPECT_4_3_NTSC		0x212
#define IF_OPTIONS_ASPECT_16_9_PAL		0x213
#define IF_OPTIONS_ASPECT_16_9_NTSC		0x214
#define IF_OPTIONS_ASPECT_4_3_SCREEN	0x238
#define IF_OPTIONS_ASPECT_16_9_SCREEN	0x239
#define IF_OPTIONS_ASPECT_AUTO			0x215
#define IF_OPTIONS_ZOOM_FIT				0x224
#define IF_OPTIONS_ZOOM_IN				0x225
#define IF_OPTIONS_ZOOM_OUT				0x226
#define IF_OPTIONS_TOGGLE_VIDEO			0x227
#define IF_OPTIONS_TOGGLE_AUDIO			0x228
#define IF_OPTIONS_TOGGLE_SUBTITLE		0x229
#define IF_OPTIONS_SUBTITLE_STREAM		0x243
#define IF_OPTIONS_SUBTITLE_STREAM_NONE	0x244
#define IF_OPTIONS_SUBTITLE_DEFAULT		0x245
#define IF_OPTIONS_VIDEO_QUALITY        0x250
#define IF_OPTIONS_VIDEO_QUALITY_LOWEST 0x251
#define IF_OPTIONS_VIDEO_QUALITY_LOW    0x252
#define IF_OPTIONS_VIDEO_QUALITY_NORMAL 0x253
#define IF_OPTIONS_AUDIO_PREAMP_DEC	    0x254
#define IF_OPTIONS_AUDIO_PREAMP			0x255
#define IF_OPTIONS_AUDIO_PREAMP_INC		0x256
#define IF_OPTIONS_SKIN					0x260
#define IF_OPTIONS_AUDIO_STEREO_TOGGLE	0x261

#define IF_FILE							0x0DC
#define IF_FILE_OPENFILE                0x181
#define IF_FILE_PLAYLIST                0x17E
#define IF_FILE_CORETHEQUE              0x280
#define IF_FILE_MEDIAINFO               0x14A
#define IF_FILE_ABOUT                   0x14B
#define IF_FILE_BENCHMARK               0x0E6
#define IF_FILE_EXIT                    0x14C
#define IF_FILE_CHAPTERS				0x200
#define IF_FILE_CHAPTERS_NONE			0x223

#define IF_PLAY                         0x172
#define IF_PLAYPAUSE                    0x246
#define IF_PLAY_FULLSCREEN				0x0F1
#define IF_STOP                         0x174
#define IF_MOVE_FFWD					0x0FA
#define IF_MOVE_BACK					0x0FB
#define IF_NEXT                         0x17A
#define IF_NEXT2                        0x190
#define IF_PREV                         0x17B
#define IF_PREV_SMART                   0x183
#define IF_PREV_SMART2                  0x191
#define IF_FASTFORWARD                  0x182

#define IF_TITLEBAR						0x300
#define IF_TRACKBAR						0x301
#define IF_TASKBAR						0x302

#define IF_SKINPATH						0x503
#define IF_SKINNO						0x500
#define IF_SKIN0						0x501
#define IF_SKIN1						0x502
#define IF_ALLKEYS						0x505
#define IF_ALLKEYS_WARNING				0x506
#define IF_ALLKEYS_WARNING2				0x507

#define SKIN_CLASS FOURCC('R','S','K','N')


#endif