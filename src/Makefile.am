bin_PROGRAMS = \
	mensa-ctrl mensa-serv

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include
	$(ZMQ_CFLAGS)

#
# binary
#
ledctrl_SOURCES = \
	mensactrl.c

ledctrl_LDADD = \
	$(ZMQ_LIBS)

ledserv_SOURCES = \
	mensaserv.c

ledserv_LDADD = \
	$(ZMQ_LIBS)

