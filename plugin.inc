PLUGIN_NAME=tcpmp_$(PLUGIN)_plugin.prc

LIBS_ARM=objplugin.arm/tcpmp.lib

CC_ARM = arm-tcpmp-palmos-gcc
OUTDIR_ARM=objplugin.arm/$(PLUGIN)/
OUTDIR=

PEAL = objplugin.arm/peal-postlink
PFLAGS = -k DLLRegister -k DLLUnRegister -s 1000
CFLAGS_ARM = -pipe -Wall -Wno-multichar -fshort-enums -O3 -mno-apcs-frame -fomit-frame-pointer -fsigned-char 
#CFLAGS_ARM += -mcpu=xscale 
CFLAGS_ARM += -march=armv4 -mtune=xscale 
CFLAGS_ARM += $(DEFS) -D ARM -D NDEBUG -Icross-compile/libc-palmos
#SFLAGS_ARM = -march=armv4 
SFLAGS_ARM = -mcpu=iwmmxt
LDFLAGS_ARM = -Wl,--split-by-file=64000 -Wl,--emit-relocs -nostartfiles

OBJS_ARM_=$(SRCFILES_ARM:%.c=$(OUTDIR_ARM)%.o)
OBJS_ARM2=$(OBJS_ARM_:%.s=$(OUTDIR_ARM)%.o)
OBJS_ARM=$(OBJS_ARM2:%.S=$(OUTDIR_ARM)%.o)

all: $(OUTDIR)$(PLUGIN_NAME)

$(OUTDIR)$(PLUGIN_NAME): $(OUTDIR_ARM)$(PLUGIN).ro $(DEFFILE)
	@build-prc $(DEFFILE) -o $@ $(OUTDIR_ARM)$(PLUGIN).ro 

$(OUTDIR_ARM)$(PLUGIN).bin: $(OBJS_ARM) ${LIBS_ARM}
	@echo linking arm
	@$(CC_ARM) $(CFLAGS_ARM) -Wl,-Map=${OUTDIR_ARM}/$(PLUGIN).map $(LDFLAGS_ARM) $(OBJS_ARM) ${LIBS_ARM} -o $@ 

$(OUTDIR_ARM)$(PLUGIN).ro: $(OUTDIR_ARM)$(PLUGIN).bin
	@$(PEAL) $(PFLAGS) -o $@ $(OUTDIR_ARM)$(PLUGIN).bin

$(OUTDIR_ARM)$(SPECDIR)%.o: $(SPECDIR)%.c 
	@echo compiling $<
	@mkdir -p $(OUTDIR_ARM)$(SPECDIR)$(*D)  
	@$(CC_ARM) $(CFLAGS_ARM) $(SPECFLAGS) -c $< -o $@

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

clean:
	@rm -rf $(OUTDIR_ARM)
	@rm -f $(OUTDIR)$(PLUGIN_NAME)
