all : out/tm-nsfplay

CC = gcc
LD = gcc

NEZPLUG_SRC = ./nezplug/src

INCLUDES = -I$(NEZPLUG_SRC) \
	   -Ikbhit

CFLAGS =  -pipe -Wall -O2
#CFLAGS += -DENABLE_BUFFERED_PCM
#CFLAGS += -DDEFAULT_FREQ=44100
CFLAGS += -DDEFAULT_TIME=180
CFLAGS += $(INCLUDES)
LFLAGS =  -lm -lz -laoss


VPATH = $(NEZPLUG_SRC)/nezvm/device \
	$(NEZPLUG_SRC)/nezvm/device/nes \
	$(NEZPLUG_SRC)/nezvm/device/opl \
	$(NEZPLUG_SRC)/nezvm/cpu/kmz80 \
	$(NEZPLUG_SRC)/nezvm/machine \
	$(NEZPLUG_SRC)/nezvm \
	$(NEZPLUG_SRC)/common/nsfsdk \
	$(NEZPLUG_SRC)/common/unix \
	$(NEZPLUG_SRC)/common/nez \
	kbhit

OBJS = nsfplay.o \
	audiosys.o handler.o m_nsf.o nsf6502.o \
	logtable.o s_logtbl.o s_apu.o s_fds.o s_fds1.o s_fds2.o s_fds3.o \
	s_fme7.o s_psg.o s_opl.o s_opltbl.o s_n106.o s_deltat.o \
	s_vrc6.o s_vrc7.o \
	s_mmc5.o s_hes.o s_hesad.o s_sng.o s_scc.o s_dmg.o \
	kmdmg.o kmr800.o kmz80.o kmz80c.o kmz80t.o kmevent.o \
	songinfo.o m_kss.o m_hes.o m_zxay.o m_gbr.o m_nsd.o m_sgc.o \
	nsfsdk.o csounix.o \
	memzip.o nez.o nezplug.o \
	kbhit.o


all: out/tm-nsfplay

.SUFFIXES: .c .o

out/%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(addprefix out/, $(OBJS)) : out

out/tm-nsfplay: $(addprefix out/, $(OBJS))
	$(LD) -o $@ $^ $(LFLAGS)

clean:
	rm -f out/tm-nsfplay
	rm -f $(addprefix out/, $(OBJS))

out:
	mkdir out

deps:
	rm -rf nezplug
	git clone https://github.com/AoiMoe/nezplug.git
	cd nezplug && git checkout master && git pull
