PKG_CONFIG ?= pkg-config
CC ?= gcc
CXX ?= g++

LIBS += -lnl-genl-3
LIBS += $(shell $(PKG_CONFIG) --libs libnl-3.0)
CFLAGS += -DCONFIG_LIBNL30
CFLAGS += $(shell $(PKG_CONFIG) --cflags libnl-3.0)

OBJS = sm3.o byte2hex.o

MASTER_AP_OBJS = $(OBJS) master_ap.o

SLAVE_AP_OBJS = $(OBJS) slave_ap.o

ALL :
	$(CXX) -g main.cpp -o wlan_auth

master_ap : $(MASTER_AP_OBJS)
	$(CXX) -D MASTER_AP -c main.cpp -o main.o
	$(CXX) $(MASTER_AP_OBJS) main.o $(LIBS) -o master_ap

slave_ap : $(SLAVE_AP_OBJS)
	$(CXX) -D SLAVE_AP -c main.cpp -o main.o
	$(CXX) $(SLAVE_AP_OBJS) main.o $(LIBS) -o slave_ap

sm3.o : utils.h

byte2hex.o : utils.h

master_ap.o : master_ap.h

slave_ap.o : slave_ap.h

clean:
	rm -rf $(MASTER_AP_OBJS) $(SLAVE_AP_OBJS) main.o wlan_auth master_ap slave_ap

