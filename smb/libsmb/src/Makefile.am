## Install only the library and not the headers.
## This way multiple instances of kio_smb can share the lib memory
## and there is no way another program can include a bad smb.h by error

lib_LTLIBRARIES = libsmb.la
libsmb_la_LDFLAGS = -version-info 1:0:0 -no-undefined

libsmb_la_SOURCES = \
	DES.c++ \
	NMBIO.c++ \
	SMBIO.c++ \
	SessionPacket.c++ \
	DES.h \
	NMBIO.h \
	SMBIO.h \
	SessionPacket.h \
	IODescriptors.c++ \
	NameServicePacket.c++  \
	SMBPacket.c++   \
	IODescriptors.h  \
	NameServicePacket.h \
	SMBPacket.h \
	IOTypes.c++ \
	NetBIOSPacket.c++ \
	SessionCodes.h \
	order.h \
	IOTypes.h  \
	NetBIOSPacket.h \
	SessionIO.c++\
	SMBCodes.h  \
	SessionIO.h  \
	NBHostCache.h \
	NBHostCache.c++ \
	CharCnv.c++

noinst_HEADERS = \
	DES.h \
	NMBIO.h \
	SMBIO.h \
	SessionPacket.h \
	IODescriptors.h  \
	NameServicePacket.h \
	SMBPacket.h \
	SessionCodes.h \
	order.h \
	IOTypes.h  \
	NetBIOSPacket.h \
	SMBCodes.h  \
	SessionIO.h  \
	NBHostCache.h \
	CharCnv.h \
	order.h
