# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/zzy/esp/esp-idf/components/bootloader/subproject"
  "/home/zzy/PROJ/Zerotier-Espressif/build/bootloader"
  "/home/zzy/PROJ/Zerotier-Espressif/build/bootloader-prefix"
  "/home/zzy/PROJ/Zerotier-Espressif/build/bootloader-prefix/tmp"
  "/home/zzy/PROJ/Zerotier-Espressif/build/bootloader-prefix/src/bootloader-stamp"
  "/home/zzy/PROJ/Zerotier-Espressif/build/bootloader-prefix/src"
  "/home/zzy/PROJ/Zerotier-Espressif/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/zzy/PROJ/Zerotier-Espressif/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
