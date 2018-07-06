TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    password_parameters_handler.cpp \
    genl.c \
    base64.cpp \
    sm3.cpp \
    capture.cpp \
    base_pwd_handler.cpp \
    master_ap_pwd_hander.cpp \
    slave_ap_pwd_handler.cpp \
    supplicant_pwd_handler.cpp

SUBDIRS += \
    supplicant.pro

DISTFILES +=

HEADERS += \
    password_parameters_handler.h \
    typedef.h \
    genl.h \
    utils.h \
    base_pwd_handler.h \
    master_ap_pwd_hander.h \
    slave_ap_pwd_handler.h \
    supplicant_pwd_handler.h
