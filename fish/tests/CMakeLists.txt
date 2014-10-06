add_executable( copytester main.cpp copytester.cpp)

target_link_libraries( copytester KF5::KDELibs4Support KF5::KIOCore )

install( TARGETS copytester ${INSTALL_TARGETS_DEFAULT_ARGS}  )
