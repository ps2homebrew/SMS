EE_OBJS =\
	chase.o \
	cl_demo.o \
	cl_input.o \
	cl_main.o \
	cl_parse.o \
	cl_tent.o \
	cmd.o \
	common.o \
	console.o \
	crc.o \
	cvar.o \
	draw.o \
	d_edge.o \
	d_fill.o \
	d_init.o \
	d_modech.o \
	d_part.o \
	d_polyse.o \
	d_scan.o \
	d_sky.o \
	d_sprite.o \
	d_surf.o \
	d_vars.o \
	d_zpoint.o \
	host.o \
	host_cmd.o \
	keys.o \
	menu.o \
	mathlib.o \
	model.o \
	nonintel.o \
	pr_cmds.o \
	pr_edict.o \
	pr_exec.o \
	r_aclip.o \
	r_alias.o \
	r_bsp.o \
	r_light.o \
	r_draw.o \
	r_efrag.o \
	r_edge.o \
	r_misc.o \
	r_main.o \
	r_sky.o \
	r_sprite.o \
	r_surf.o \
	r_part.o \
	r_vars.o \
	screen.o \
	sbar.o \
	sv_main.o \
	sv_phys.o \
	sv_move.o \
	sv_user.o \
	zone.o	\
	view.o	\
	wad.o \
	world.o \
	cd_null.o \
	net_vcr.o \
	net_main.o \
	net_loop.o \
	net_none.o \
	sys_ps2.o \
	snd_null.o \
	in_ps2.o \
	ps2_gs.o \
	vid_ps2.o

EE_BIN = quake.elf

EE_LIBS = $(EE)/ee/lib/libc.a -lmc -lkbd -lmouse -lm -lc

EE_CFLAGS = -g -Dstricmp=strcasecmp -funroll-loops -fomit-frame-pointer -fexpensive-optimizations

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run:
	ps2client execee host:$(EE_BIN)

include Makefile.pref
include Makefile.eeglobal
