if(KArchive_HAVE_BZIP2)
  add_subdirectory(bzip2)
endif()
add_subdirectory(fish)
add_subdirectory(gzip)
add_subdirectory(info)
add_subdirectory(man)
add_subdirectory(sftp)
if(NOT WIN32)
  add_subdirectory(smb)
endif()
add_subdirectory(tar)
add_subdirectory(thumbnail)
if(KArchive_HAVE_LZMA)
  add_subdirectory(xz)
endif()
if(KArchive_HAVE_ZSTD)
  add_subdirectory(zstd)
endif()
