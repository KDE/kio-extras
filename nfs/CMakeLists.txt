remove_definitions(-DQT_NO_CAST_FROM_ASCII)

## Check for XDR functions
include(CheckCXXSymbolExists)

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${TIRPC_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${TIRPC_INCLUDE_DIRS})
check_cxx_symbol_exists(xdr_u_int64_t "rpc/xdr.h" HAVE_XDR_U_INT64_T)
check_cxx_symbol_exists(xdr_uint64_t "rpc/xdr.h" HAVE_XDR_UINT64_T)
check_cxx_symbol_exists(xdr_u_hyper "rpc/xdr.h" HAVE_XDR_U_HYPER)
check_cxx_symbol_exists(xdr_u_longlong_t "rpc/xdr.h" HAVE_XDR_U_LONGLONG_T)

if (NOT HAVE_XDR_U_INT64_T AND NOT HAVE_XDR_UINT64_T AND NOT HAVE_XDR_U_HYPER AND NOT HAVE_XDR_U_LONGLONG_T)
    message(FATAL_ERROR "Could not find 64-bit XDR datatype functions!")
endif (NOT HAVE_XDR_U_INT64_T AND NOT HAVE_XDR_UINT64_T AND NOT HAVE_XDR_U_HYPER AND NOT HAVE_XDR_U_LONGLONG_T)

if (HAVE_XDR_U_INT64_T)
    add_definitions(-DHAVE_XDR_U_INT64_T)
endif (HAVE_XDR_U_INT64_T)
if (HAVE_XDR_UINT64_T)
    add_definitions(-DHAVE_XDR_UINT64_T)
endif (HAVE_XDR_UINT64_T)
if (HAVE_XDR_U_HYPER)
    add_definitions(-DHAVE_XDR_U_HYPER)
endif (HAVE_XDR_U_HYPER)
if (HAVE_XDR_U_LONGLONG_T)
    add_definitions(-DHAVE_XDR_U_LONGLONG_T)
endif (HAVE_XDR_U_LONGLONG_T)

# silence warnings for rpcgen generated code having lots of unused variables
set(AVOID_UNUSED_VARIABLE_WARNING_FLAG)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(_flag "-Wno-unused-variable")
    check_c_compiler_flag(${_flag} _flag_present)
    if(_flag_present)
        set(AVOID_UNUSED_VARIABLE_WARNING_FLAG ${_flag})
    endif()
endif()

# silence warnings for use of xdr_void as no-op method in calls of clnt_call()
# It's method signature "bool_t xdr_void(void)" does not match the xdrproc_t definition
# which is "bool_t (*)(XDR*, ...)", and the compiler correctly points that out as issue.
# While no-one has come up so far with a nice way to tell the compiler that cast is fine,
# for now just disable the warning completely for the compilation unit.
set(AVOID_CAST_FUNCTION_TYPE_WARNING_FLAG)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(_flag "-Wno-cast-function-type")
    check_cxx_compiler_flag(${_flag} _flag_present)
    if(_flag_present)
        set(AVOID_CAST_FUNCTION_TYPE_WARNING_FLAG ${_flag})
    endif()
endif()

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_nfs\")
include_directories(${TIRPC_INCLUDE_DIRS})

add_library(kio_nfs MODULE kio_nfs.cpp nfsv2.cpp nfsv3.cpp rpc_nfs3_prot_xdr.c rpc_nfs2_prot_xdr.c)
target_link_libraries(kio_nfs KF5::KIOCore KF5::I18n Qt5::Network ${TIRPC_LIBRARIES})
set_target_properties(kio_nfs PROPERTIES OUTPUT_NAME "nfs")

if(AVOID_UNUSED_VARIABLE_WARNING_FLAG)
    set_source_files_properties(rpc_nfs2_prot_xdr.c rpc_nfs3_prot_xdr.c PROPERTIES
        COMPILE_OPTIONS "${AVOID_UNUSED_VARIABLE_WARNING_FLAG}"
    )
endif()

if(AVOID_CAST_FUNCTION_TYPE_WARNING_FLAG)
    set_source_files_properties(nfsv2.cpp nfsv3.cpp PROPERTIES
        COMPILE_OPTIONS "${AVOID_CAST_FUNCTION_TYPE_WARNING_FLAG}"
    )
endif()

ecm_qt_declare_logging_category(kio_nfs
    HEADER kio_nfs_debug.h
    IDENTIFIER LOG_KIO_NFS
    CATEGORY_NAME kf.kio.slaves.nfs
    OLD_CATEGORY_NAMES kde.kio_nfs
    DESCRIPTION "KIO nfs"
    EXPORT KIO_EXTRAS
)

install(TARGETS kio_nfs DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### install files ###############
