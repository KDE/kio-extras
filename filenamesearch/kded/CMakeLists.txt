add_library(filenamesearchmodule MODULE)

target_sources(filenamesearchmodule PRIVATE
     filenamesearchmodule.cpp
)

target_link_libraries(filenamesearchmodule
    KF6::DBusAddons
    KF6::KIOCore
)

install(TARGETS filenamesearchmodule DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kded)
