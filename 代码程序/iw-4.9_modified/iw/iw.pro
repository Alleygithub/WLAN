TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    bitrate.c \
    coalesce.c \
    connect.c \
    cqm.c \
    event.c \
    genl.c \
    hwsim.c \
    ibss.c \
    info.c \
    interface.c \
    iw.c \
    link.c \
    mesh.c \
    mpath.c \
    mpp.c \
    ocb.c \
    offch.c \
    p2p.c \
    phy.c \
    ps.c \
    reason.c \
    reg.c \
    roc.c \
    scan.c \
    sections.c \
    station.c \
    status.c \
    survey.c \
    util.c \
    vendor.c \
    wowlan.c \
    main.cpp

HEADERS += \
    ieee80211.h \
    iw.h \
    nl80211.h
