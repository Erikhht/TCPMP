APP=tcpmp

CC_M68K = m68k-palmos-gcc
CC_ARM = arm-tcpmp-palmos-gcc
CC_HOST = gcc
CPP_HOST = g++

OUTDIR_M68K=objplugin.m68k/
OUTDIR_ARM=objplugin.arm/

PEAL = $(OUTDIR_ARM)peal-postlink
PFLAG = -s 1000
PEALDIR = common/palmos/peal/
PEALSRC = $(PEALDIR)postlink/image.cc $(PEALDIR)postlink/postlinker.cc $(PEALDIR)postlink/relocation.cc $(PEALDIR)postlink/section.cc 
PEALSRC += $(PEALDIR)postlink/symbol.cc $(PEALDIR)postlink/symboltable.cc $(PEALDIR)postlink/complain.cc

CFLAGS_M68K = -pipe -Wall -O3 -mnoshort

CFLAGS_ARM = -pipe -Wall -Wno-multichar -march=armv4 -fshort-enums -O3 -mno-apcs-frame -mtune=xscale -fomit-frame-pointer -fsigned-char 
#CFLAGS_ARM += -fno-inline-functions 
CFLAGS_ARM += -D ARM -D NDEBUG -Icross-compile/libc-palmos
CFLAGS_ARM += -DFASTEST -DNO_ERRNO_H -DTCPMP
CFLAGS_ARM += -DFIXED_POINT

#*************
# palmOne SDK
#*************
CFLAGS_ARM += -I../SDK/palmOne-SDK -DHAVE_PALMONE_SDK

#*************
# Sony SDK
#*************
CFLAGS_ARM += -I../SDK/Sony-SDK/Incs -I../SDK/Sony-SDK/Incs/System -DHAVE_SONY_SDK

SFLAGS_ARM = -march=armv4 -mcpu=iwmmxt

LDFLAGS_ARM = -Wl,--split-by-file=63500 -Wl,--emit-relocs -nostartfiles
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
SRCFILES_ARM += common/helper_video.c 
SRCFILES_ARM += common/helper_base.c 
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
SRCFILES_ARM += common/softidct/block_mx1.c 
SRCFILES_ARM += common/softidct/block_wmmx.c 
SRCFILES_ARM += common/softidct/idct_c.c 
SRCFILES_ARM += common/softidct/idct_half.c 
SRCFILES_ARM += common/softidct/idct_arm.s 
SRCFILES_ARM += common/softidct/idct_wmmx.s 
SRCFILES_ARM += common/softidct/mcomp4x4_c.c
SRCFILES_ARM += common/softidct/mcomp_c.c 
SRCFILES_ARM += common/softidct/mcomp_arm.s 
SRCFILES_ARM += common/softidct/mcomp_wmmx.s 
SRCFILES_ARM += common/softidct/softidct.c

SRCFILES_ARM += common/zlib/adler32.c
SRCFILES_ARM += common/zlib/crc32.c 
SRCFILES_ARM += common/zlib/inffast.c 
SRCFILES_ARM += common/zlib/inflate.c
SRCFILES_ARM += common/zlib/inftrees.c 
SRCFILES_ARM += common/zlib/uncompr.c 
SRCFILES_ARM += common/zlib/zutil.c

SRCFILES_ARM += camera/adpcm.c 
SRCFILES_ARM += camera/law.c 
SRCFILES_ARM += camera/g726/g726_16.c
SRCFILES_ARM += camera/g726/g726_24.c
SRCFILES_ARM += camera/g726/g726_32.c
SRCFILES_ARM += camera/g726/g726_40.c
SRCFILES_ARM += camera/g726/g72x.c

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

SRCFILES_ARM += sample/events.c
SRCFILES_ARM += sample/sample_palmos.c
RESFILES = sample/sample.rcp

OBJS_M68K=$(SRCFILES_M68K:%.c=$(OUTDIR_M68K)%.o)
OBJS_ARM_=$(SRCFILES_ARM:%.c=$(OUTDIR_ARM)%.o)
OBJS_ARM=$(OBJS_ARM_:%.s=$(OUTDIR_ARM)%.o)
RESOURCES=$(RESFILES:%.rcp=$(OUTDIR_ARM)%.ro)
LANG_BIN=$(LANG_TXT:%.txt=%.bin)

all: $(APP).prc $(OUTDIR_ARM)$(APP).lib

$(OUTDIR_M68K)$(APP): $(OBJS_M68K)
	@echo linking m68k
	@$(CC_M68K) $(CFLAGS_M68K) -static $(OBJS_M68K) $(LDFLAGS_M68K) -o $@

$(APP).prc: $(OUTDIR_M68K)$(APP) $(OUTDIR_ARM)$(APP).ro $(APP).def $(LANG_BIN) $(RESOURCES)
	@build-prc $(APP).def -o $@ $(OUTDIR_ARM)$(APP).ro $(OUTDIR_M68K)$(APP) $(RESOURCES)

$(OUTDIR_ARM)$(APP).bin: $(OBJS_ARM) 
	@echo linking arm
	@$(CC_ARM) $(CFLAGS_ARM) -Wl,-Map=${OUTDIR_ARM}/$(APP).map $(LDFLAGS_ARM) $(OBJS_ARM) -o $@ 

$(OUTDIR_ARM)$(APP).ro: $(OUTDIR_ARM)$(APP).bin $(PEAL)
	@$(PEAL) $(PFLAG) -o $@ $(OUTDIR_ARM)$(APP).bin

$(OUTDIR_ARM)$(APP).lib: $(OUTDIR_ARM)$(APP).bin $(PEAL)
	@$(PEAL) -l $(APP) -o $@ $(OUTDIR_ARM)$(APP).bin

$(OUTDIR_M68K)%.o: %.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_M68K)$(*D)  
	@$(CC_M68K) $(CFLAGS_M68K) -c $< -o $@

$(OUTDIR_ARM)%.o: %.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) -c $< -o $@

$(OUTDIR_ARM)%.o: %.s
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
	@rm -f $(APP).prc
	@rm -f lang/*.bin
