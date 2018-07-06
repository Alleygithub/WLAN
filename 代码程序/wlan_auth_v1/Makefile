PKG_CONFIG ?= pkg-config
CC ?= gcc
CXX ?= g++

LIBS += -lnl-genl-3
LIBS += $(shell $(PKG_CONFIG) --libs libnl-3.0)
CFLAGS += -DCONFIG_LIBNL30
CFLAGS += $(shell $(PKG_CONFIG) --cflags libnl-3.0)

OBJS = base64.o sm3.o genl.o capture.o password_parameters_handler.o main.o

ALL = supplicant

ALL : $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o supplicant

genl.o : genl.h

sm3.o : utils.h

base64.o : utils.h

capture.o : utils.h

password_parameters_handler.o : password_parameters_handler.h

main.o : password_parameters_handler.h

clean:
	rm -rf $(OBJS) $(ALL)

