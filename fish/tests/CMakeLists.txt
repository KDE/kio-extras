ADD_EXECUTABLE( copytester main.cpp copytester.cpp)

TARGET_LINK_LIBRARIES( copytester KF5::KDE4Support KF5::KIOCore )

install( TARGETS copytester ${INSTALL_TARGETS_DEFAULT_ARGS}  )
