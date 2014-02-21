PROJECT( copytester )
FIND_PACKAGE( KDE4 REQUIRED )
INCLUDE_DIRECTORIES( ${KDE4_INCLUDES} . )


SET( copytesterSources main.cpp copytester.cpp )

ADD_EXECUTABLE( copytester ${copytesterSources} )

TARGET_LINK_LIBRARIES( copytester ${KDE4_KDEUI_LIBS} KF5::KIOCore )

install( TARGETS copytester ${INSTALL_TARGETS_DEFAULT_ARGS}  )
