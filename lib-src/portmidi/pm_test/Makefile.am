check_PROGRAMS = pm_test

pm_test_CPPFLAGS = $(PORTAUDIO_CFLAGS)
pm_test_LDADD = $(top_builddir)/src/libportmidi.la $(PORTAUDIO_LIBS)
pm_test_SOURCES = latency.c \
	midiclock.c \
	midithread.c \
	midithru.c \
	mm.c \
	qtest.c \
	sysex.c \
	test.c
