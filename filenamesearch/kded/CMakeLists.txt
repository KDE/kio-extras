add_library(filenamesearchmodule MODULE)

target_sources(filenamesearchmodule PRIVATE
     filenamesearchmodule.cpp
)

target_link_libraries(filenamesearchmodule
    KF${QT_MAJOR_VERSION}::DBusAddons
    KF${QT_MAJOR_VERSION}::KIOCore
)

install(TARGETS filenamesearchmodule DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kded)
