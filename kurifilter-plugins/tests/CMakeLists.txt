if(KDE4_BUILD_TESTS)

  set(kurifiltertest_SRCS kurifiltertest.cpp)

  kde4_automoc(${kurifiltertest_SRCS})

  kde4_add_executable(kurifiltertest NOGUI RUN_UNINSTALLED ${kurifiltertest_SRCS})

  target_link_libraries(kurifiltertest ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})

  add_test(kurifiltertest kurifiltertest)

endif(KDE4_BUILD_TESTS)
