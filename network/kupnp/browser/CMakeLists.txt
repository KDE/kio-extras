include_directories(
  ../lib
)

set( kupnpbrowser_SRCS
  main.cpp
)

kde4_add_executable( kupnpbrowser  ${kupnpbrowser_SRCS} )

target_link_libraries( kupnpbrowser
  kupnp
  ${KDE4_KDEUI_LIBS}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTCORE_LIBRARY}
)

install( TARGETS kupnpbrowser  ${INSTALL_TARGETS_DEFAULT_ARGS} )
