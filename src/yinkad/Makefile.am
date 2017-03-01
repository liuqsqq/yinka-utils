bin_PROGRAMS = yinkad

yinkad_SOURCES = \
	yinkad.c

yinkad_CFLAGS = \
	-I$(top_srcdir)/src	\
	$(PTHREAD_CFLAGS)	\
	$(NULL)

yinkad_LDADD = \
	$(top_builddir)/src/utils/libyinkad.la \
	$(PTHREAD_LIBS)	\
	$(NULL)
