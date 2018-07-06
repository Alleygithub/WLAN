TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    byte2hex.cpp \
    main.cpp \
    sm3.cpp \
    master_ap.cpp \
    slave_ap.cpp

HEADERS += \
    typedef.h \
    utils.h \
    master_ap.h \
    slave_ap.h
