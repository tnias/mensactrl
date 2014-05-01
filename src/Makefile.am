bin_PROGRAMS = \
	mensactrl mensaserv

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include
	$(ZMQ_CFLAGS)

#
# binary
#
mensactrl_SOURCES = \
	mensactrl.c

mensactrl_LDADD = \
	$(ZMQ_LIBS)

mensaserv_SOURCES = \
	mensaserv.c

mensaserv_LDADD = \
	$(ZMQ_LIBS)

