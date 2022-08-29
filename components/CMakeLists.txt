idf_build_get_property(idf_target IDF_TARGET)
idf_build_get_property(python PYTHON)

set(priv_requires soc esp_hw_support lwip)

if(NOT BOOTLOADER_BUILD)
    list(APPEND priv_requires esp_pm)
endif()

set(libzt_srcs "")
set(libzt_include_dirs "libzt/include")


idf_component_register(SRCS "${libzt_srcs}"
    INCLUDE_DIRS "${libzt_include_dirs}"
    PRIV_REQUIRES "${priv_requires}"
    )




# Make sure mbedtls finds the same Python interpreter as IDF uses
idf_build_get_property(python PYTHON)
set(Python3_EXECUTABLE ${python})

# Needed to for include_next includes to work from within libzt
#include_directories("${COMPONENT_DIR}/port/include")

# Import libzt library targets
add_subdirectory(libzt)


