APP=tcpmp

CC_M68K = m68k-palmos-gcc
CC_ARM = arm-palmos-gcc
CC_HOST = gcc
CPP_HOST = g++

OUTDIR_M68K=obj.m68k/
OUTDIR_ARM=obj.arm/

PEAL = $(OUTDIR_ARM)peal-postlink
PEALDIR = common/palmos/peal/
PEALFLAGS = -s 1000 
#PEALFLAGS += -v
PEALSRC = $(PEALDIR)postlink/image.cc $(PEALDIR)postlink/postlinker.cc $(PEALDIR)postlink/relocation.cc $(PEALDIR)postlink/section.cc 
PEALSRC += $(PEALDIR)postlink/symbol.cc $(PEALDIR)postlink/symboltable.cc $(PEALDIR)postlink/complain.cc

CFLAGS_M68K = -pipe -Wall -O3 -mnoshort

CFLAGS_ARM = -pipe -Wall -Wno-multichar -march=armv4 -fshort-enums -O3 -mno-apcs-frame -mtune=xscale -fomit-frame-pointer -fsigned-char 
CFLAGS_ARM += -fPIC -msingle-pic-base 
CFLAGS_ARM += -D ARM -D NDEBUG -DSTDC_HEADERS -DNO_PLUGINS
CFLAGS_ARM += -DFASTEST -DNO_ERRNO_H 
CFLAGS_ARM += -DFIXED_POINT
CFLAGS_ARM += -DFPM_ARM -DOPT_SPEED -DASO_INTERLEAVE1 
CFLAGS_ARM += -D BIG_ENDIAN=0 -D BYTE_ORDER=1 -D LITTLE_ENDIAN=1 -D _LOW_ACCURACY_ 
CFLAGS_ARM += -DLC_ONLY_DECODER
CFLAGS_ARM += -DLIBA52_FIXED
CFLAGS_ARM += -DMPC_FIXED_POINT -DMPC_LITTLE_ENDIAN -Impc/libmusepack/include -Icross-compile/libc-palmos
CFLAGS_ARM += -D CONFIG_H264_DECODER -D TCPMP -Iffmpeg/libavutil

#*************
# palmOne SDK
#*************
CFLAGS_ARM += -I../SDK/palmOne-SDK -DHAVE_PALMONE_SDK

#*************
# Sony SDK
#*************
CFLAGS_ARM += -I../SDK/Sony-SDK/Incs -I../SDK/Sony-SDK/Incs/System -DHAVE_SONY_SDK

SFLAGS_ARM = -march=armv4

LDFLAGS_ARM = -Wl,--split-by-file=63000 -Wl,--emit-relocs -nostartfiles
LDFLAGS_M68K = -L/usr/local/share/palmdev/sdk/lib/m68k-palmos-coff -lPalmOSGlue -lc -lgcc

LANG_TXT = lang/lang_en.txt lang/lang_std.txt

SRCFILES_M68K = common/palmos/pilotmain_m68k.c
SRCFILES_M68K += $(PEALDIR)m68k/peal.c

SRCFILES_ARM = $(PEALDIR)arm/pealstub.c
SRCFILES_ARM += common/advanced.c
SRCFILES_ARM += common/audio.c 
SRCFILES_ARM += common/bitstrm.c
SRCFILES_ARM += common/buffer.c
SRCFILES_ARM += common/codec.c
SRCFILES_ARM += common/color.c
SRCFILES_ARM += common/context.c
SRCFILES_ARM += common/equalizer.c
SRCFILES_ARM += common/flow.c 
SRCFILES_ARM += common/format.c 
SRCFILES_ARM += common/format_base.c 
SRCFILES_ARM += common/format_subtitle.c
SRCFILES_ARM += common/id3tag.c
SRCFILES_ARM += common/idct.c 
SRCFILES_ARM += common/node.c 
SRCFILES_ARM += common/nulloutput.c
SRCFILES_ARM += common/overlay.c
SRCFILES_ARM += common/parser2.c
SRCFILES_ARM += common/platform.c
SRCFILES_ARM += common/player.c
SRCFILES_ARM += common/playlist.c
SRCFILES_ARM += common/probe.c
SRCFILES_ARM += common/rawaudio.c
SRCFILES_ARM += common/rawimage.c
SRCFILES_ARM += common/str.c 
SRCFILES_ARM += common/streams.c 
SRCFILES_ARM += common/tchar.c 
SRCFILES_ARM += common/timer.c 
SRCFILES_ARM += common/tools.c 
SRCFILES_ARM += common/helper_base.c 
SRCFILES_ARM += common/helper_video.c 
SRCFILES_ARM += common/video.c 
SRCFILES_ARM += common/vlc.c
SRCFILES_ARM += common/waveout.c 
SRCFILES_ARM += common/cpu/arm.s 
SRCFILES_ARM += common/cpu/cpu.c 
SRCFILES_ARM += common/playlist/m3u.c 
SRCFILES_ARM += common/playlist/pls.c 
SRCFILES_ARM += common/playlist/asx.c
SRCFILES_ARM += common/dyncode/dyncode.c 
SRCFILES_ARM += common/dyncode/dyncode_arm.c 
SRCFILES_ARM += common/palmos/association_palmos.c
SRCFILES_ARM += common/palmos/context_palmos.c 
SRCFILES_ARM += common/palmos/node_palmos.c 
SRCFILES_ARM += common/palmos/platform_palmos.c 
SRCFILES_ARM += common/palmos/str_palmos.c 
SRCFILES_ARM += common/palmos/mem_palmos.c 
SRCFILES_ARM += common/palmos/dia.c 
SRCFILES_ARM += common/palmos/pace.c 
SRCFILES_ARM += common/palmos/native.s
SRCFILES_ARM += common/palmos/waveout_palmos.c
SRCFILES_ARM += common/palmos/file_palmos.c 
SRCFILES_ARM += common/palmos/filedb_palmos.c 
SRCFILES_ARM += common/palmos/vfs_palmos.c 
SRCFILES_ARM += common/libc/rand.c 
SRCFILES_ARM += common/libc/vsprintf.c 
SRCFILES_ARM += common/libc/qsort.c 
SRCFILES_ARM += common/libc/sincos.c 
SRCFILES_ARM += common/overlay/overlay_hires.c
SRCFILES_ARM += common/pcm/pcm_arm.c
SRCFILES_ARM += common/pcm/pcm_soft.c
SRCFILES_ARM += common/blit/blit_arm_fix.c 
SRCFILES_ARM += common/blit/blit_arm_rgb16.c 
SRCFILES_ARM += common/blit/blit_arm_yuv.c
SRCFILES_ARM += common/blit/blit_arm_packed_yuv.c
SRCFILES_ARM += common/blit/blit_arm_gray.c 
SRCFILES_ARM += common/blit/blit_arm_half.c 
SRCFILES_ARM += common/blit/blit_arm_stretch.c
SRCFILES_ARM += common/blit/blit_soft.c 
SRCFILES_ARM += common/blit/blit_wmmx_fix.c 
SRCFILES_ARM += common/softidct/block_c.c 
SRCFILES_ARM += common/softidct/block_half.c 
SRCFILES_ARM += common/softidct/idct_c.c 
SRCFILES_ARM += common/softidct/idct_half.c 
SRCFILES_ARM += common/softidct/idct_arm.s 
SRCFILES_ARM += common/softidct/block_mx1.c 
SRCFILES_ARM += common/softidct/mcomp4x4_c.c 
SRCFILES_ARM += common/softidct/mcomp_c.c 
SRCFILES_ARM += common/softidct/mcomp_arm.s 
SRCFILES_ARM += common/softidct/softidct.c

SRCFILES_ARM += common/zlib/adler32.c
SRCFILES_ARM += common/zlib/crc32.c 
SRCFILES_ARM += common/zlib/inffast.c 
SRCFILES_ARM += common/zlib/inflate.c
SRCFILES_ARM += common/zlib/inftrees.c 
SRCFILES_ARM += common/zlib/uncompr.c 
SRCFILES_ARM += common/zlib/zutil.c

SRCFILES_ARM += splitter/avi.c 
SRCFILES_ARM += splitter/asf.c 
SRCFILES_ARM += splitter/wav.c
SRCFILES_ARM += splitter/mov.c
SRCFILES_ARM += splitter/mpg.c
SRCFILES_ARM += splitter/nsv.c 

#SRCFILES_ARM += interface/about.c
#SRCFILES_ARM += interface/benchresult.c
#SRCFILES_ARM += interface/mediainfo.c
#SRCFILES_ARM += interface/settings.c
#SRCFILES_ARM += interface/palmos/win_palmos.c 
SRCFILES_ARM += interface/palmos/keys.c 
SRCFILES_ARM += interface/skin.c

SRCFILES_ARM += matroska/matroska.c 
SRCFILES_ARM += matroska/MatroskaParser/MatroskaParser.c 

SRCFILES_ARM += mpeg1/mpeg_decode.c 
SRCFILES_ARM += mpeg1/mves.c

SRCFILES_ARM += camera/mjpeg.c
SRCFILES_ARM += camera/tiff.c
SRCFILES_ARM += camera/png.c
SRCFILES_ARM += camera/adpcm.c
SRCFILES_ARM += camera/law.c
SRCFILES_ARM += camera/g726/g726_16.c
SRCFILES_ARM += camera/g726/g726_24.c
SRCFILES_ARM += camera/g726/g726_32.c
SRCFILES_ARM += camera/g726/g726_40.c
SRCFILES_ARM += camera/g726/g72x.c

SRCFILES_ARM += libmad/libmad.c
SRCFILES_ARM += libmad/libmad/bit.c 
SRCFILES_ARM += libmad/libmad/fixed.c
SRCFILES_ARM += libmad/libmad/frame.c 
SRCFILES_ARM += libmad/libmad/huffmanmad.c 
SRCFILES_ARM += libmad/libmad/layer12.c
SRCFILES_ARM += libmad/libmad/layer3.c 
SRCFILES_ARM += libmad/libmad/stream.c 
SRCFILES_ARM += libmad/libmad/synth.c 

SRCFILES_ARM += vorbislq/ogg.c 
SRCFILES_ARM += vorbislq/oggembedded.c 
SRCFILES_ARM += vorbislq/oggpacket.c 
SRCFILES_ARM += vorbislq/vorbis.c
SRCFILES_ARM += vorbislq/tremor/bitwise.c 
SRCFILES_ARM += vorbislq/tremor/block.c 
SRCFILES_ARM += vorbislq/tremor/codebook.c
SRCFILES_ARM += vorbislq/tremor/floor0.c 
SRCFILES_ARM += vorbislq/tremor/floor1.c 
SRCFILES_ARM += vorbislq/tremor/framing.c
SRCFILES_ARM += vorbislq/tremor/info.c 
SRCFILES_ARM += vorbislq/tremor/mapping0.c 
SRCFILES_ARM += vorbislq/tremor/mdctvorbis.c
SRCFILES_ARM += vorbislq/tremor/registry.c 
SRCFILES_ARM += vorbislq/tremor/res012.c 
SRCFILES_ARM += vorbislq/tremor/sharedbook.c
SRCFILES_ARM += vorbislq/tremor/synthesis.c 
SRCFILES_ARM += vorbislq/tremor/window.c 

SRCFILES_ARM += ffmpeg/ffmpeg.c 
SRCFILES_ARM += ffmpeg/libavcodec/bitstream.c 
SRCFILES_ARM += ffmpeg/libavcodec/cabac.c 
SRCFILES_ARM += ffmpeg/libavcodec/cinepak.c 
SRCFILES_ARM += ffmpeg/libavcodec/dsputil.c 
SRCFILES_ARM += ffmpeg/libavcodec/error_resilience.c 
SRCFILES_ARM += ffmpeg/libavcodec/golomb.c 
SRCFILES_ARM += ffmpeg/libavcodec/h263.c 
SRCFILES_ARM += ffmpeg/libavcodec/h263dec.c 
SRCFILES_ARM += ffmpeg/libavcodec/h264.c 
SRCFILES_ARM += ffmpeg/libavcodec/h264idct.c 
SRCFILES_ARM += ffmpeg/libavcodec/jrevdct.c 
SRCFILES_ARM += ffmpeg/libavcodec/mem.c 
SRCFILES_ARM += ffmpeg/libavcodec/mpeg12.c 
SRCFILES_ARM += ffmpeg/libavcodec/mpegvideo.c 
SRCFILES_ARM += ffmpeg/libavcodec/msmpeg4.c 
SRCFILES_ARM += ffmpeg/libavcodec/msvideo1.c 
SRCFILES_ARM += ffmpeg/libavcodec/parser.c 
SRCFILES_ARM += ffmpeg/libavcodec/simple_idct.c 
SRCFILES_ARM += ffmpeg/libavcodec/tscc.c 
SRCFILES_ARM += ffmpeg/libavcodec/utils.c 
SRCFILES_ARM += ffmpeg/libavcodec/vp3dsp.c 
#SRCFILES_ARM += ffmpeg/libavcodec/armv4l/mpegvideo_arm.c
#SRCFILES_ARM += ffmpeg/libavcodec/armv4l/dsputil_arm.c
#SRCFILES_ARM += ffmpeg/libavcodec/armv4l/dsputil_arm_s.S
#SRCFILES_ARM += ffmpeg/libavcodec/armv4l/simple_idct_arm.S
#SRCFILES_ARM += ffmpeg/libavcodec/armv4l/jrevdct_arm.S
SRCFILES_ARM += ffmpeg/libavutil/integer.c 
SRCFILES_ARM += ffmpeg/libavutil/rational.c 
SRCFILES_ARM += ffmpeg/libavutil/mathematics.c

SRCFILES_ARM += mpc/mpc.c
SRCFILES_ARM += mpc/libmusepack/src/huffsv46.c
SRCFILES_ARM += mpc/libmusepack/src/huffsv7.c
SRCFILES_ARM += mpc/libmusepack/src/idtag.c
SRCFILES_ARM += mpc/libmusepack/src/mpc_decoder.c
SRCFILES_ARM += mpc/libmusepack/src/requant.c
SRCFILES_ARM += mpc/libmusepack/src/streaminfo.c
SRCFILES_ARM += mpc/libmusepack/src/synth_filter.c

SRCFILES_ARM += ac3/ac3.c 
SRCFILES_ARM += ac3/liba52/bit_allocate.c 
SRCFILES_ARM += ac3/liba52/bitstream2.c 
SRCFILES_ARM += ac3/liba52/crc.c 
SRCFILES_ARM += ac3/liba52/downmix.c 
SRCFILES_ARM += ac3/liba52/imdct.c 
SRCFILES_ARM += ac3/liba52/parse.c 

SRCFILES_ARM += aac/faad.c 
SRCFILES_ARM += aac/libpaac/aac_imdct.c
SRCFILES_ARM += aac/faad2/libfaad/bits.c aac/faad2/libfaad/cfft.c
SRCFILES_ARM += aac/faad2/libfaad/common.c aac/faad2/libfaad/decoder.c
SRCFILES_ARM += aac/faad2/libfaad/drc.c aac/faad2/libfaad/drm_dec.c
SRCFILES_ARM += aac/faad2/libfaad/filtbank.c
SRCFILES_ARM += aac/faad2/libfaad/hcr.c aac/faad2/libfaad/huffman.c
SRCFILES_ARM += aac/faad2/libfaad/ic_predict.c aac/faad2/libfaad/is.c
SRCFILES_ARM += aac/faad2/libfaad/lt_predict.c aac/faad2/libfaad/mdct.c
SRCFILES_ARM += aac/faad2/libfaad/ms.c aac/faad2/libfaad/mp4.c
SRCFILES_ARM += aac/faad2/libfaad/output.c aac/faad2/libfaad/pns.c
SRCFILES_ARM += aac/faad2/libfaad/ps_dec.c aac/faad2/libfaad/ps_syntax.c
SRCFILES_ARM += aac/faad2/libfaad/rvlc.c aac/faad2/libfaad/pulse.c
SRCFILES_ARM += aac/faad2/libfaad/sbr_dct.c aac/faad2/libfaad/sbr_dec.c
SRCFILES_ARM += aac/faad2/libfaad/sbr_e_nf.c aac/faad2/libfaad/sbr_fbt.c
SRCFILES_ARM += aac/faad2/libfaad/sbr_hfadj.c aac/faad2/libfaad/sbr_hfgen.c
SRCFILES_ARM += aac/faad2/libfaad/sbr_huff.c aac/faad2/libfaad/sbr_qmf.c
SRCFILES_ARM += aac/faad2/libfaad/sbr_syntax.c aac/faad2/libfaad/sbr_tf_grid.c
SRCFILES_ARM += aac/faad2/libfaad/specrec.c aac/faad2/libfaad/ssr.c
SRCFILES_ARM += aac/faad2/libfaad/ssr_fb.c aac/faad2/libfaad/ssr_ipqf.c
SRCFILES_ARM += aac/faad2/libfaad/syntax.c aac/faad2/libfaad/tns.c

SRCFILES_ARM += sonyhhe/ge2d.c 
SRCFILES_ARM += zodiac/ati4200.c 

SRCFILES_ARM += sample/noplugins.c
SRCFILES_ARM += sample/events.c
SRCFILES_ARM += sample/sample_palmos.c
RESFILES = sample/sample.rcp

OBJS_M68K=$(SRCFILES_M68K:%.c=$(OUTDIR_M68K)%.o)
OBJS_ARM1=$(SRCFILES_ARM:%.c=$(OUTDIR_ARM)%.o)
OBJS_ARM2=$(OBJS_ARM1:%.s=$(OUTDIR_ARM)%.o)
OBJS_ARM=$(OBJS_ARM2:%.S=$(OUTDIR_ARM)%.o)
RESOURCES=$(RESFILES:%.rcp=$(OUTDIR_ARM)%.ro)
LANG_BIN=$(LANG_TXT:%.txt=%.bin)

all: $(APP)-all.prc

$(OUTDIR_M68K)$(APP): $(OBJS_M68K)
	@echo linking m68k
	@$(CC_M68K) $(CFLAGS_M68K) -static $(OBJS_M68K) $(LDFLAGS_M68K) -o $@

$(APP)-all.prc: $(OUTDIR_M68K)$(APP) $(OUTDIR_ARM)$(APP).ro $(APP).def $(LANG_BIN) $(RESOURCES)
	@build-prc $(APP).def -o $@ $(OUTDIR_ARM)$(APP).ro $(OUTDIR_M68K)$(APP) $(RESOURCES)

$(OUTDIR_ARM)$(APP).bin: $(OBJS_ARM)
	@echo linking arm
	@$(CC_ARM) $(CFLAGS_ARM) -Wl,-Map=${OUTDIR_ARM}/$(APP).map $(LDFLAGS_ARM) $(OBJS_ARM) -o $@ 

$(OUTDIR_ARM)$(APP).ro: $(OUTDIR_ARM)$(APP).bin $(PEAL)
	@$(PEAL) $(PEALFLAGS) -o $@ $(OUTDIR_ARM)$(APP).bin

$(OUTDIR_M68K)%.o: %.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_M68K)$(*D)  
	@$(CC_M68K) $(CFLAGS_M68K) -c $< -o $@

$(OUTDIR_ARM)aac/faad2/%.o: aac/faad2/%.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)aac/faad2/$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) -w -c $< -o $@

$(OUTDIR_ARM)vorbislq/tremor/%.o: vorbislq/tremor/%.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)vorbislq/tremor/$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) -w -c $< -o $@

$(OUTDIR_ARM)matroska/MatroskaParser/%.o: matroska/MatroskaParser/%.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)matroska/MatroskaParser/$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) -Os -c $< -o $@

$(OUTDIR_ARM)mpc/libmusepack/src/%.o: mpc/libmusepack/src/%.c
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)mpc/libmusepack/src/$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) -w -c $< -o $@

$(OUTDIR_ARM)ffmpeg/libavcodec/%.o: ffmpeg/libavcodec/%.c
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)ffmpeg/libavcodec/$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) -w -c $< -o $@

$(OUTDIR_ARM)%.o: %.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) -c $< -o $@

$(OUTDIR_ARM)%.o: %.s
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)$(*D)  
	@$(CC_ARM) $(SFLAGS_ARM) -c $< -o $@

$(OUTDIR_ARM)%.o: %.S
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)$(*D)  
	@$(CC_ARM) $(SFLAGS_ARM) -c $< -o $@

$(OUTDIR_ARM)%.ro: %.rcp
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)$(*D)  
	@pilrc -I $(*D) -q -ro $< $@

$(PEAL): $(PEALSRC)
	@echo compiling peal-postlink
	@$(CPP_HOST) $(PEALSRC) -o $(PEAL)
	@chmod u+x $(PEAL)

$(OUTDIR_ARM)langtobin: lang/langtobin.c
	@echo compiling langtobin
	@$(CC_HOST) lang/langtobin.c -o $(OUTDIR_ARM)langtobin
	@chmod u+x $(OUTDIR_ARM)langtobin

%.bin: %.txt $(OUTDIR_ARM)langtobin
	@echo convert $<
	@$(OUTDIR_ARM)langtobin $< $@

clean:
	@rm -rf $(OUTDIR_M68K) $(OUTDIR_ARM)
	@rm -f $(APP)-all.prc
	@rm -f lang/*.bin
