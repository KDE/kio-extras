add_executable(cmdtool)
ecm_mark_nongui_executable(cmdtool)
target_sources(cmdtool PRIVATE cmdtool_main.cpp)
target_link_libraries(cmdtool
    cmdtool_static
    Qt::Core
)
