INCLUDES = $(all_includes) -I../mwn  -I../mwn/qt2compat -I../../libusermang/src 

bin_PROGRAMS = netserv

netserv_SOURCES = netserv.cpp \
		    table.cpp

noinst_HEADERS = table.h

netserv_LDADD = -L../mwn  -L../../ccui -L../../libusermang/src \
                -lmwn -lusermang -lccui -lqt $(LIB_KDECORE) $(LIB_KDEUI)

install-data-local:
