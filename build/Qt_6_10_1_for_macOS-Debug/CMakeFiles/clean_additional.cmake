# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/K4Controller_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/K4Controller_autogen.dir/ParseCache.txt"
  "K4Controller_autogen"
  )
endif()
