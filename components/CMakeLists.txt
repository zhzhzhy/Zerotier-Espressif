idf_build_get_property(idf_target IDF_TARGET)
idf_build_get_property(python PYTHON)

set(priv_requires soc esp_hw_support lwip)

if(NOT BOOTLOADER_BUILD)
    list(APPEND priv_requires esp_pm)
endif()

set(libzt_srcs "libzt_esp")
set(libzt_include_dirs "libzt_esp/include")


idf_component_register(SRCS "${libzt_srcs}"
    INCLUDE_DIRS "${libzt_include_dirs}"
    PRIV_REQUIRES "${priv_requires}"
    )


add_subdirectory(libzt_esp)

