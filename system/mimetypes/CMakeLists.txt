kde4_header()



########### install files ###############

install_files( ${MIME_INSTALL_DIR}/inode FILES system_directory.desktop )

kde4_footer()



#original Makefile.am contents follow:

#mimetypedir = $(kde_mimedir)/inode
#
#mimetype_DATA = system_directory.desktop
#
#
#EXTRA_DIST = $(mimetype_DATA)
