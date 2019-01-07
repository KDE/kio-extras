add_executable( copytester main.cpp copytester.cpp)

target_link_libraries( copytester KF5::KIOCore )

install( TARGETS copytester ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}  )
