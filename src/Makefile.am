## Process this file with automake to produce Makefile.in
bin_PROGRAMS = transtreamproxy
transtreamproxy_SOURCES =  main.cpp
transtreamproxy_SOURCES += eFilePumpThread.cpp
transtreamproxy_SOURCES += eNetworkPumpThread.cpp
transtreamproxy_SOURCES += eURIDecoder.cpp
transtreamproxy_SOURCES += uThread.cpp

transtreamproxy_LDADD = $(LIBLTDL) @PTHREAD_LIBS@


