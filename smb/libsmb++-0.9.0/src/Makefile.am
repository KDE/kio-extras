# Striped down libsmb++ Makefile.am. See http://libsmb.sourceforge.net for
# a full implementation.
# Specifically, we do not distribute here any header, so that this version
# of the library remains private to kio_smb.

lib_LTLIBRARIES = libsmb++.la
libsmb___la_LDFLAGS = -version-info 0:9:0 @EXTRA_LIBSMB_FLAGS@

libsmb___la_SOURCES = types.cpp Options.cpp Util.cpp Resolve.cpp FileManagementInterface.cpp NameQueryInterface.cpp DES.cpp IOTypes.cpp NMBIO.cpp SMBIO.cpp NetBIOSPacket.cpp SMBPacket.cpp SessionIO.cpp CharCnv.cpp IODescriptors.cpp SessionPacket.cpp NBHostCache.cpp NameServicePacket.cpp NativeNMB.cpp SambaLink.cpp SambaNMB.cpp

noinst_HEADERS = \
	smb++.h \
	strtool.h \
	cursesHacks.h
	defines.h \
	Util.h \
	FileManagementInterface.h \
	Options.h \
	types.h \
	smb++.h \
	SelectedNMB.h \
	SelectedSMB.h \
	Resolve.h \
	NameQueryInterface.h \
	SambaLink.h \
	SambaNMB.h \
	SambaDefs.h \
	SambaExterns.h \
	DES.h \
	CharCnv.h \
	IODescriptors.h \
	IOTypes.h \
	NBHostCache.h \
	NMBIO.h \
	NameServicePacket.h \
	NativeNMB.h \
	NetBIOSPacket.h \
	SMBCodes.h \
	SMBIO.h \
	SMBPacket.h \
	SessionCodes.h \
	SessionIO.h \
	SessionPacket.h
