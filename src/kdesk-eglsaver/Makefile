#
#  Makefile - To build kdesk-eglsaver
#
#  Change OPTDIR to point to your RaspberryPI /opt firmware root directory - for include and lib files
#

OPTDIR=/opt/vc

INCLUDES=-I$(OPTDIR)/include -I$(OPTDIR)/include/interface/vcos/pthreads -I$(OPTDIR)/include/interface/vmcs_host/linux

CFLAGS+=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

#LDFLAGS+=-L$(OPTDIR)/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -L../libs/ilclient -L../libs/vgfont

LDFLAGS+=-L$(OPTDIR)/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt 

#-L../libs/ilclient -L../libs/vgfont

APP=kdesk-eglsaver

all: $(APP)
debug: all

$(APP): $(APP).o
	$(CC) -o $@ -Wl,--whole-archive $(APP).o $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

$(APP).o: $(APP).c
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations



