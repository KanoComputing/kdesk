#
#  Makefile - To build kdesk-eglsaver
#
#  Change OPTDIR to point to your RaspberryPI /opt firmware root directory - for include and lib files
#

OPTDIR=/opt/vc

INCLUDES=-I$(OPTDIR)/include -I$(OPTDIR)/include/interface/vcos/pthreads -I$(OPTDIR)/include/interface/vmcs_host/linux

CFLAGS+=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

LDFLAGS+=-L$(OPTDIR)/lib/ -lGLESv2 -lEGL -lbcm_host

APP=kdesk-eglsaver

# macro to transform a raw bitmap into a C array structure
cstyle_from_raw=$(shell echo "// $1 Array Definition\nconst unsigned char $1[]={\n" > $1.h; cat $1.raw | hexdump -v -e '16/1 "0x%02x, "' -e '"\n"' >> $1.h; echo "};" >> $1.h)

all: $(APP)
debug: all

# dynamically create C bitmap arrays from RAW bitmap files
bitmap_minecraft.h: bitmap_minecraft.raw
	$(call cstyle_from_raw, "bitmap_minecraft")

bitmap_pong.h: bitmap_pong.raw
	$(call cstyle_from_raw, "bitmap_pong")

bitmap_homefolder.h: bitmap_homefolder.raw
	$(call cstyle_from_raw, "bitmap_homefolder")


$(APP): $(APP).o
	$(CC) -o $@ -Wl,--whole-archive $(APP).o $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

$(APP).o: $(APP).c bitmap_minecraft.h bitmap_pong.h bitmap_homefolder.h
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations



