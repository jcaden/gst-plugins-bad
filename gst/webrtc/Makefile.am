
plugin_LTLIBRARIES = libgstwebrtc.la

# sources used to compile this plug-in
libgstwebrtc_la_SOURCES = gstwebrtc.c \
			gstcryptopolicy.h gstcryptopolicy.c \
			gstsrtpunprotect.h gstsrtpunprotect.c \
			gstsrtpprotect.h gstsrtpprotect.c \
			gstwebrtcbin.h gstwebrtcbin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstwebrtc_la_CFLAGS = $(GST_CFLAGS) $(WEBRTC_CFLAGS)
libgstwebrtc_la_LIBADD = $(GST_LIBS) $(WEBRTC_LIBS)
libgstwebrtc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstwebrtc_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstwebrtcbin.h gstsrtpprotect.h gstsrtpunprotect.h gstcryptopolicy.h
