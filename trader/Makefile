# Generated automatically from Makefile.in by configure.

INCDIR  =	-I$(QTDIR)/include -I$(KDEDIR)/include -I../
CXXFLAGS =	$(INCDIR) -g -O2
LDFLAGS =       -L$(QTDIR)/lib -L/usr/X11/lib -L$(KDEDIR)/lib -lkio -lkdeutil -lkdeui -lkdecore -lqt -lX11 -lXext -lmico2.2.1 -ldl
CXX     =       g++
MOC     =       /usr/bin/moc
HARDCODED =	-DKDE_DATADIR=\"$(KDEDIR)/share/apps\" -DKDE_BINDIR=\"$(KDEDIR)/bin\"

####### Files

SOURCES =	main.cc trader.cc typerepo.cc
HEADERS =	main.h trader.h typerepo.h
OBJECTS =	main.o trader.o typerepo.o
SRCMETA =	
TARGET	=	kio_trader

####### Implicit rules

.SUFFIXES: .cc

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(HARDCODED) -DSTDC_HEADERS -DHAVE_UNISTD_H $(INCDIR) $<

%.moc: %.h
	$(MOC) $< -o $@      

####### Install

INSTALL = install -m 0755 -s
INSTALL_SHELL = install -m 0755
INSTALL_DATA = install -m 0644
INSTALL_DIR = install -d -m 0755
INSTALL_DIR_PRIVATE = install -d -m 0700

####### Build rules

all: $(TARGET)

$(TARGET): $(SRCMETA) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

depend:
	@makedepend $(INCDIR) $(SOURCES) 2> /dev/null

showfiles:
	@echo $(SOURCES) Makefile

clean:
	-rm -f *.o *.bak *~ *% #*
	-rm -f $(SRCMETA) $(TARGET)

install:
	rm -f $(KDEDIR)/bin/$(TARGET)
	$(INSTALL) $(TARGET) $(KDEDIR)/bin

####### Meta objects

# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o: main.h /opt/kde/include/kio_interface.h
main.o: /opt/kde/include/kio_connection.h /usr/include/stdio.h
main.o: /usr/include/libio.h /usr/include/_G_config.h
main.o: /usr/include/sys/cdefs.h /opt/kde/include/kio_base.h typerepo.h
main.o: trader.h /opt/kde/include/kio_manager.h /usr/include/signal.h
main.o: /usr/include/features.h /usr/include/sys/types.h
main.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
main.o: /usr/include/asm/posix_types.h /usr/include/asm/types.h
main.o: /usr/include/sys/bitypes.h /usr/include/linux/signal.h
main.o: /usr/include/asm/signal.h /usr/include/sys/wait.h
main.o: /usr/include/gnu/types.h /usr/include/waitflags.h
main.o: /usr/include/waitstatus.h /usr/include/endian.h
main.o: /usr/include/bytesex.h /usr/include/sys/stat.h
main.o: /usr/include/linux/stat.h /usr/include/dirent.h
main.o: /usr/include/linux/limits.h /usr/include/linux/dirent.h
main.o: /usr/include/posix1_lim.h /usr/include/time.h /usr/include/sys/time.h
main.o: /usr/include/linux/time.h /usr/include/unistd.h
main.o: /usr/include/posix_opt.h /usr/include/confname.h
main.o: /usr/include/string.h /usr/include/pwd.h /usr/include/grp.h
main.o: /usr/include/assert.h /opt/kde/include/k2url.h
main.o: /opt/kde/include/kapp.h /opt/kde/include/kconfig.h
main.o: /opt/kde/include/kconfigbase.h /usr/local/qt/include/qcolor.h
main.o: /usr/local/qt/include/qwindowdefs.h /usr/local/qt/include/qobjectdefs.h
main.o: /usr/local/qt/include/qglobal.h /usr/local/qt/include/qfont.h
main.o: /usr/local/qt/include/qstring.h /usr/local/qt/include/qarray.h
main.o: /usr/local/qt/include/qgarray.h /usr/local/qt/include/qshared.h
main.o: /usr/local/qt/include/qgeneric.h /usr/local/qt/include/qstrlist.h
main.o: /usr/local/qt/include/qlist.h /usr/local/qt/include/qglist.h
main.o: /usr/local/qt/include/qcollection.h /usr/local/qt/include/qdatastream.h
main.o: /usr/local/qt/include/qiodevice.h /opt/kde/include/kconfigdata.h
main.o: /usr/local/qt/include/qdict.h /usr/local/qt/include/qgdict.h
main.o: /usr/local/qt/include/qtstream.h /usr/local/qt/include/qtextstream.h
main.o: /usr/local/qt/include/qfile.h /usr/local/qt/include/qobject.h
main.o: /usr/local/qt/include/qevent.h /usr/local/qt/include/qrect.h
main.o: /usr/local/qt/include/qsize.h /usr/local/qt/include/qpoint.h
main.o: /opt/kde/include/kdebug.h /opt/kde/include/klocale.h
main.o: /usr/local/qt/include/qintdict.h /opt/kde/include/drag.h
main.o: /usr/local/qt/include/qapp.h /usr/local/qt/include/qapplication.h
main.o: /usr/local/qt/include/qwidget.h /usr/local/qt/include/qpaintdevice.h
main.o: /usr/local/qt/include/qpalette.h /usr/local/qt/include/qcursor.h
main.o: /usr/local/qt/include/qfontmetrics.h /usr/local/qt/include/qfontinfo.h
main.o: /usr/local/qt/include/qpixmap.h /usr/include/X11/X.h
main.o: /usr/local/qt/include/qpopmenu.h /usr/local/qt/include/qpopupmenu.h
main.o: /usr/local/qt/include/qtableview.h /usr/local/qt/include/qframe.h
main.o: /usr/local/qt/include/qmenudata.h /opt/kde/include/kregexp.h
main.o: /opt/kde/include/k2config.h
trader.o: trader.h
typerepo.o: typerepo.h trader.h
