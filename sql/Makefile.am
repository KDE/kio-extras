INCLUDES = -I$(top_builddir)/lib $(all_includes) 

METASOURCES = AUTO


kde_module_LTLIBRARIES = kio_sql.la

kio_sql_la_SOURCES = kio_sql.cpp

kio_sql_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module $(KDE_PLUGIN)

kio_sql_la_LIBADD = $(LIB_KDBCORE) $(LIB_KIO) -lkdbcore

#libkdbcore.la        libkdb_mysql.la      libkdbui.so.0
#libkdbcore.so        libkdb_mysql.so      libkdbui.so.0.0.1
#libkdbcore.so.0      libkdbui.la
#libkdbcore.so.0.0.1  libkdbui.so


datadir = $(kde_servicesdir)
data_DATA = sql.protocol

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_sql_la_SOURCES) linktest.cpp

linktest.cpp:
	echo "int main() {};" > linktest.cpp

linktest_LDFLAGS  = $(all_libraries)
linktest_LDADD = $(kio_sql_la_LIBADD)
