# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v5.2/components/bootloader/subproject"
  "C:/IDF-Proj/uros/build/bootloader"
  "C:/IDF-Proj/uros/build/bootloader-prefix"
  "C:/IDF-Proj/uros/build/bootloader-prefix/tmp"
  "C:/IDF-Proj/uros/build/bootloader-prefix/src/bootloader-stamp"
  "C:/IDF-Proj/uros/build/bootloader-prefix/src"
  "C:/IDF-Proj/uros/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/IDF-Proj/uros/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/IDF-Proj/uros/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
