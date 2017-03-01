bin_PROGRAMS = updater

updater_SOURCES = \
    updater.c			\
	$(NULL)

updater_CFLAGS = \
	-I$(top_srcdir)/src	\
	$(XML_CFLAGS)		\
	$(CURL_CFLAGS)		\
	$(JSON_C_CFLAGS)	\
	$(NULL)

updater_LDADD = \
    $(top_builddir)/src/utils/libyinkad.la \
	$(GENERAL_LIBS)		\
	$(XML_LIBS) 		\
	$(CURL_LIBS)		\
	$(JSON_C_LIBS)		\
	$(NULL)

noinst_HEADERS = \
	updater.h			\
	$(NULL)
