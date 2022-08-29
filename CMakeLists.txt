cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(libzt_esp32)
idf_build_get_property(idf_target IDF_TARGET)
idf_build_get_property(python PYTHON)

set(priv_requires soc esp_hw_support)

if(NOT BOOTLOADER_BUILD)
    list(APPEND priv_requires esp_pm)
endif()

set(libzt_srcs "")
set(libzt_include_dirs "port/include" "mbedtls/include" "./mbedtls/library")


idf_component_register(SRCS "${libzt_srcs}"
    INCLUDE_DIRS "${libzt_include_dirs}"
    PRIV_REQUIRES "${priv_requires}"
    )

# Determine the type of libzt component library
if(libztsrcs STREQUAL "")
    # For no sources in component library we must use "INTERFACE"
    set(linkage_type INTERFACE)
else()
    set(linkage_type PUBLIC)
endif()




# Make sure mbedtls finds the same Python interpreter as IDF uses
idf_build_get_property(python PYTHON)
set(Python3_EXECUTABLE ${python})

# Needed to for include_next includes to work from within mbedtls
include_directories("${COMPONENT_DIR}/port/include")

# Import mbedtls library targets
add_subdirectory(libzt)


