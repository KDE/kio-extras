INCLUDES = -I$(top_builddir)/lib $(all_includes) 

METASOURCES = AUTO


lib_LTLIBRARIES = kio_sql.la

kio_sql_la_SOURCES = kio_sql.cpp

kio_sql_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -no-undefined -module -avoid-version

kio_sql_la_LIBADD = $(LIB_KDBCORE) $(LIB_KIO)


datadir = $(kde_servicesdir)
data_DATA = sql.protocol

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_sql_la_SOURCES) linktest.cpp

linktest.cpp:
	echo "int main() {};" > linktest.cpp

linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_sql_la_LIBADD)