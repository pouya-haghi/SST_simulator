if (NOT TARGET SST::SSTCore)
  add_library(SST::SSTCore INTERFACE IMPORTED)
  set_target_properties(SST::SSTCore
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES /work2/07431/tg867305/stampede2/sstcore-11.0.0/install/include
    INTERFACE_COMPILE_FEATURES    cxx_std_11
  )
endif()

if (NOT TARGET SST::SST)
  add_executable(SST::SST IMPORTED)
  set_target_properties(SST::SST PROPERTIES
    IMPORTED_LOCATION /work2/07431/tg867305/stampede2/sstcore-11.0.0/install/bin/sst
  )
endif()
macro(add_sst_library NAME)
  if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_MODULE_LINKER_FLAGS "-undefined dynamic_lookup")
  endif()
  #ARGN is source list
  add_library(${NAME} MODULE ${ARGN})
  #ensure at least C++11, this can be overriden with higher
  target_link_libraries(${NAME} PUBLIC SST::SSTCore)
endmacro()
