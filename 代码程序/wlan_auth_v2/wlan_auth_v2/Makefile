PKG_CONFIG ?= pkg-config
CC ?= gcc
CXX ?= g++

LIBS += -lnl-genl-3
LIBS += $(shell $(PKG_CONFIG) --libs libnl-3.0)
CFLAGS += -DCONFIG_LIBNL30
CFLAGS += $(shell $(PKG_CONFIG) --cflags libnl-3.0)

OBJS = genl.o capture.o base64.o sm3.o sm4.o byte2hex.o base_pwd_handler.o

MASTER_AP_OBJS = $(OBJS) master_ap_pwd_handler.o master_ap.o

SLAVE_AP_OBJS = $(OBJS) slave_ap_pwd_handler.o slave_ap.o

SUPPLICANT_OBJS = $(OBJS) supplicant_pwd_handler.o supplicant.o

ALL = wlan_auth master_ap slave_ap supplicant

ALL : main.o
	$(CXX) main.o -o wlan_auth

master_ap : $(MASTER_AP_OBJS)
	$(CXX) $(MASTER_AP_OBJS) $(LIBS) -o master_ap

slave_ap : $(SLAVE_AP_OBJS)
	$(CXX) $(SLAVE_AP_OBJS) $(LIBS) -o slave_ap

supplicant : $(SUPPLICANT_OBJS)
	$(CXX) $(SUPPLICANT_OBJS) $(LIBS) -o supplicant

genl.o : genl.h

capture.o : capture.h

base64.o : utils.h

sm3.o : utils.h

sm4.o : utils.h

byte2hex.o : utils.h

base_pwd_handler.o : base_pwd_handler.h

master_ap_pwd_handler.o : master_ap_pwd_handler.h

slave_ap_pwd_handler.o : slave_ap_pwd_handler.h

supplicant_pwd_handler.o : supplicant_pwd_handler.h

master_ap.o : master_ap_pwd_handler.h

slave_ap.o : slave_ap_pwd_handler.h

supplicant.o : supplicant_pwd_handler.h

clean:
	rm -rf $(MASTER_AP_OBJS) $(SLAVE_AP_OBJS) $(SUPPLICANT_OBJS) $(ALL) main.o

