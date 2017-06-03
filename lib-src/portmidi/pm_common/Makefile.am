lib_LTLIBRARIES = libportmidi.la

libportmidi_la_CPPFLAGS = $(PORTAUDIO_CFLAGS)
libportmidi_la_SOURCES = \
	pminternal.h \
	pmutil.c \
	pmutil.h \
	portmidi.c \
	..\porttime\porttime.c \
	$(NULL)

if USE_LINUX_ALSA
libportmixer_la_SOURCES += ..\pm_linux\finddefault.c \
	..\pm_linux\pmlinux.c \
	..\pm_linux\pmlinux.h \
	..\pm_linux\pmlinuxalsa.c \
	..\pm_linux\pmlinuxalsa.h \
	..\porttime\ptlinux.c
endif
