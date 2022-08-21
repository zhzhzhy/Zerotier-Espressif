# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(zt)

if (USE_SANITIZER STREQUAL "Address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif (USE_SANITIZER STREQUAL "Thread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
elseif (USE_SANITIZER STREQUAL "Undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif (USE_SANITIZER STREQUAL "Leak")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
elseif (USE_SANITIZER STREQUAL "Memory")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
endif()

# ------------------------------------------------------------------------------
# |                             SOURCE DIRECTORIES                             |
# ------------------------------------------------------------------------------

set(PROJ_DIR ${PROJECT_SOURCE_DIR})
set(LWIP_SRC_DIR "${PROJ_DIR}/ext/lwip/src")
set(ZTO_SRC_DIR "${PROJ_DIR}/ext/ZeroTierOne")
set(LIBZT_SRC_DIR "${PROJ_DIR}/src")
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(INTERMEDIATE_LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib/intermediate)
if(UNIX)
    set(LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/unix/port)
endif()

if(BUILD_WIN)
    set(LWIP_PORT_DIR ${PROJ_DIR}/ext/lwip-contrib/ports/win32)
endif()


# ------------------------------------------------------------------------------
# |                             INCLUDE DIRECTORIES                            |
# ------------------------------------------------------------------------------
# ZeroTier
include_directories(${ZTO_SRC_DIR})
include_directories(${ZTO_SRC_DIR}/include)
include_directories(${ZTO_SRC_DIR}/node)
include_directories(${ZTO_SRC_DIR}/osdep)
# ZeroTier (ext)
#include_directories(${ZTO_SRC_DIR}/ext/miniupnpc)
include_directories(${ZTO_SRC_DIR}/ext/libnatpmp)
# libzt
include_directories(${PROJ_DIR}/src)
include_directories(${PROJ_DIR}/include)
# libzt (ext)
include_directories(${PROJ_DIR}/ext/concurrentqueue)
include_directories(${LWIP_SRC_DIR}/include)
include_directories(${LWIP_PORT_DIR}/include)

# ------------------------------------------------------------------------------
# |                        TARGET AND VARIANT SELECTION                        |
# ------------------------------------------------------------------------------

# Defaults
option(ALLOW_INSTALL_TARGET     "Enable the install target"   TRUE)
option(BUILD_STATIC_LIB         "Build static library"        TRUE)
option(BUILD_SHARED_LIB         "Build shared libary"         TRUE)
option(BUILD_HOST_SELFTEST      "Build host selftest binary"  TRUE)
option(ZTS_DISABLE_CENTRAL_API  "Disable central API"         TRUE)


# All native targets for this host
if(BUILD_HOST)
    set(BUILD_STATIC_LIB        TRUE)
    set(BUILD_SHARED_LIB        TRUE)
    set(BUILD_HOST_EXAMPLES     TRUE)
    set(ALLOW_INSTALL_TARGET    TRUE)
    set(BUILD_HOST_SELFTEST    FALSE)
    set(ZTS_ENABLE_STATS        TRUE)
endif()

# ------------------------------------------------------------------------------
# |                    HACKS TO GET THIS TO WORK ON WINDOWS                    |
# ------------------------------------------------------------------------------

if(BUILD_WIN)
    # Possibly a CMake limitation? -- Can't share target output names
    set(STATIC_LIB_NAME ${PROJECT_NAME}-static)
    set(STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME}-static)
    set(DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
    set(DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME}-shared)
else()
    set(STATIC_LIB_NAME ${PROJECT_NAME}-static)
    set(STATIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
    set(DYNAMIC_LIB_NAME ${PROJECT_NAME}-shared)
    set(DYNAMIC_LIB_OUTPUT_NAME ${PROJECT_NAME})
endif()


# ------------------------------------------------------------------------------
# |                                BUILD TYPES                                 |
# ------------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "debug")
    set(BUILD_DEBUG ON)
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(DEBUG_OPTIMIZATION "-O3")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "release")
    set(BUILD_RELEASE ON)
    set(CMAKE_VERBOSE_MAKEFILE OFF)
    set(RELEASE_OPTIMIZATION "-O3")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -w")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -w")
    if(NOT MSVC)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wno-everything")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-everything")
    endif()
endif()



# -------------------------------------------------------------------------------
# |                                 EXAMPLES                                    |
# -------------------------------------------------------------------------------

#    add_executable(pingable-node
#       ${PROJ_DIR}/main/pingable-node.c)
#    target_link_libraries(pingable-node ${STATIC_LIB_NAME})



# ------------------------------------------------------------------------------
# |                                    FLAGS                                   |
# ------------------------------------------------------------------------------

set(ZT_FLAGS "${ZT_FLAGS} -DZT_SDK=1")
set(ZT_FLAGS "${ZT_FLAGS} -DZT_USE_MINIUPNPC=1")
set(ZT_FLAGS "${ZT_FLAGS} -D_USING_LWIP_DEFINITIONS_=0")

if (NOT BUILD_WIN)
    set(ZT_FLAGS "${ZT_FLAGS} -DOMIT_JSON_SUPPORT=1 -Wno-unused-parameter")
    set(LWIP_FLAGS "${LWIP_FLAGS} -Wno-tautological-constant-out-of-range-compare -Wno-parentheses-equality")
endif()

if(BUILD_DEBUG)
    set(LWIP_FLAGS "${LWIP_FLAGS} -DLWIP_DBG_TYPES_ON=128")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DSOCKETS_DEBUG=128")
    # set (LWIP_FLAGS "${LWIP_FLAGS} -DLWIP_STATS_LARGE=1") set (LWIP_FLAGS
    # "${LWIP_FLAGS} -DLWIP_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DLINK_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DETHARP_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DIPFRAG_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DIP_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DICMP_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DIGMP_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DUDP_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DTCP_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DSYS_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DIP6_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DICMP6_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DIP6_FRAG_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DMLD6_STATS=1")
    set(LWIP_FLAGS "${LWIP_FLAGS} -DND6_STATS=1")
else()
    set(LWIP_FLAGS "${LWIP_FLAGS} -DLWIP_DBG_TYPES_ON=0")
endif()

if(BUILD_WIN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc -DNOMINMAX")
else()
    option(STACK_PROTECTOR "Compile with -fstack-protector" ON)
    if(STACK_PROTECTOR)
      set(STACK_PROTECTOR_FLAGS -fstack-protector)
    endif()

    set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} \
         ${ZT_FLAGS} \
         ${STACK_PROTECTOR_FLAGS}")

    set(CMAKE_C_FLAGS_DEBUG
            "${CMAKE_C_FLAGS_DEBUG} \
         ${DEBUG_OPTIMIZATION} \
         -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1")

    set(CMAKE_C_FLAGS_RELEASE
            "${CMAKE_C_FLAGS_RELEASE} \
         ${RELEASE_OPTIMIZATION} \
         ${STACK_PROTECTOR_FLAGS}")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
         ${ZT_FLAGS} -Wall -Wextra -std=c++11")

    set(CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG} \
         ${DEBUG_OPTIMIZATION} \
         -DLWIP_DEBUG=1 -DLIBZT_DEBUG=1")

    set(CMAKE_CXX_FLAGS_RELEASE
            "${CMAKE_CXX_FLAGS_RELEASE} \
         ${RELEASE_OPTIMIZATION}")
endif()

# WINDOWS-specific MSVC flags and libraries
if(BUILD_WIN)
    # 32-bit
    if(NOT BUILD_WIN64)
        set(WINLIBDIR,
                "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x86")
    endif()
    # 64-bit
    if(BUILD_WIN64)
        set(WINLIBDIR,
                "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.16299.0/um/x64")
    endif()
    # find_library (ws2_32_LIBRARY_PATH NAMES WS2_32 HINTS ${WINLIBDIR})
    # find_library (shlwapi_LIBRARY_PATH NAMES ShLwApi HINTS ${WINLIBDIR})
    set(ws2_32_LIBRARY_PATH "${WINLIBDIR}/WS2_32.Lib")
    set(shlwapi_LIBRARY_PATH "${WINLIBDIR}/ShLwApi.Lib")
    set(iphlpapi_LIBRARY_PATH "${WINLIBDIR}/iphlpapi.Lib")
    message(STATUS ${WINLIBDIR})
    message(STATUS "WS2_32=${ws2_32_LIBRARY_PATH}")
    message(STATUS "ShLwApi=${shlwapi_LIBRARY_PATH}")
    message(STATUS "liphlpapi=${iphlpapi_LIBRARY_PATH}")
    add_definitions(-DADD_EXPORTS=1)
endif()


# ------------------------------------------------------------------------------
# |                              OPTIONAL FEATURES                             |
# ------------------------------------------------------------------------------

if(NOT ZTS_DISABLE_CENTRAL_API)
    set(requiredlibs)
    find_package(CURL)
    if(CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIR})
        set(requiredlibs ${requiredlibs} ${CURL_LIBRARIES})
    else(CURL_FOUND)
        message(
            FATAL_ERROR "Could not find the CURL library and development files.")
    endif(CURL_FOUND)
endif()

# ------------------------------------------------------------------------------
# |                                   SOURCES                                  |
# ------------------------------------------------------------------------------

file(GLOB ztcoreSrcGlob ${ZTO_SRC_DIR}/node/*.cpp
         ${ZTO_SRC_DIR}/osdep/OSUtils.cpp ${ZTO_SRC_DIR}/osdep/PortMapper.cpp)

file(GLOB libnatpmpSrcGlob ${ZTO_SRC_DIR}/ext/libnatpmp/natpmp.c
    ${ZTO_SRC_DIR}/ext/libnatpmp/wingettimeofday.c
    ${ZTO_SRC_DIR}/ext/libnatpmp/getgateway.c)
if(NOT BUILD_WIN)
list(REMOVE_ITEM libnatpmpSrcGlob ${ZTO_SRC_DIR}/ext/libnatpmp/wingettimeofday.c)
endif()

file(
    GLOB
    libminiupnpcSrcGlob
    ${ZTO_SRC_DIR}/ext/miniupnpc/connecthostport.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/igd_desc_parse.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/minisoap.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/minissdpc.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/miniupnpc.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/miniwget.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/minixml.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/portlistingparse.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/receivedata.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnpcommands.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnpdev.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnperrors.c
    ${ZTO_SRC_DIR}/ext/miniupnpc/upnpreplyparse.c)

file(GLOB libztSrcGlob ${LIBZT_SRC_DIR}/*.cpp ${LANG_WRAPPER_FILE})

file(
    GLOB
    lwipSrcGlob
    ${LWIP_SRC_DIR}/netif/*.c
    ${LWIP_SRC_DIR}/api/*.c
    ${LWIP_PORT_DIR}/sys_arch.c
    ${LWIP_SRC_DIR}/core/*.c
    ${LWIP_SRC_DIR}/core/ipv4/*.c
    ${LWIP_SRC_DIR}/core/ipv6/*.c)
list(REMOVE_ITEM lwipSrcGlob ${LWIP_SRC_DIR}/netif/slipif.c)


# ------------------------------------------------------------------------------
# |                     OBJECT LIBRARIES (INTERMEDIATE)                        |
# ------------------------------------------------------------------------------

if(BUILD_STATIC_LIB)
    # zto_obj
    add_library(zto_obj OBJECT ${ztcoreSrcGlob})
    set_target_properties(zto_obj PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}")
    if(BUILD_WIN)
        target_link_libraries(zto_obj ws2_32)
        target_link_libraries(zto_obj ${shlwapi_LIBRARY_PATH})
        target_link_libraries(zto_obj ${iphlpapi_LIBRARY_PATH})
    endif()

    # libnatpmp_obj
    add_library(libnatpmp_obj OBJECT ${libnatpmpSrcGlob})
    set_target_properties(libnatpmp_obj PROPERTIES COMPILE_FLAGS "-DNATPMP_EXPORTS")

    # miniupnpc_obj
    add_library(miniupnpc_obj OBJECT ${libminiupnpcSrcGlob})
    target_compile_definitions(
        miniupnpc_obj
        PRIVATE ZT_USE_MINIUPNPC
            MINIUPNP_STATICLIB
            _DARWIN_C_SOURCE
            MINIUPNPC_SET_SOCKET_TIMEOUT
            MINIUPNPC_GET_SRC_ADDR
            _BSD_SOURCE
            _DEFAULT_SOURCE
            MINIUPNPC_VERSION_STRING=\"2.0\"
            UPNP_VERSION_STRING=\"UPnP/1.1\"
            ENABLE_STRNATPMPERR
            OS_STRING=\"Darwin/15.0.0\")
    if(BUILD_DARWIN AND NOT BUILD_IOS_FRAMEWORK)
        target_compile_definitions(miniupnpc_obj PRIVATE MACOSX)
    endif()

    # lwip_obj
    add_library(lwip_obj OBJECT ${lwipSrcGlob})
    set_target_properties(lwip_obj PROPERTIES COMPILE_FLAGS "${LWIP_FLAGS}")

    # libzt_obj
    add_library(libzt_obj OBJECT ${libztSrcGlob})
    set_target_properties(libzt_obj PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}")
endif()

# PIC

# zto_pic
add_library(zto_pic ${ztcoreSrcGlob})
set_target_properties(zto_pic PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}"
    POSITION_INDEPENDENT_CODE ON)

# libnatpmp_pic
add_library(natpmp_pic ${libnatpmpSrcGlob})
set_target_properties(natpmp_pic PROPERTIES COMPILE_FLAGS "-DNATPMP_EXPORTS"
    POSITION_INDEPENDENT_CODE ON)

# miniupnpc_pic
add_library(miniupnpc_pic ${libminiupnpcSrcGlob})
target_compile_definitions(
    miniupnpc_pic
    PRIVATE MACOSX
                    ZT_USE_MINIUPNPC
                    MINIUPNP_STATICLIB
                    _DARWIN_C_SOURCE
                    MINIUPNPC_SET_SOCKET_TIMEOUT
                    MINIUPNPC_GET_SRC_ADDR
                    _BSD_SOURCE
                    _DEFAULT_SOURCE
                    MINIUPNPC_VERSION_STRING=\"2.0\"
                    UPNP_VERSION_STRING=\"UPnP/1.1\"
                    ENABLE_STRNATPMPERR
                    OS_STRING=\"Darwin/15.0.0\")
set_target_properties(miniupnpc_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)

# lwip_pic
add_library(lwip_pic ${lwipSrcGlob})
set_target_properties(lwip_pic PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(lwip_pic PROPERTIES COMPILE_FLAGS "${LWIP_FLAGS}")

# libzt_pic
add_library(zt_pic ${libztSrcGlob})
set_target_properties(zt_pic PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}"
    POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# |                                STATIC LIB                                  |
# ------------------------------------------------------------------------------

if(BUILD_STATIC_LIB)
    # libztcore.a
    add_library(ztcore STATIC $<TARGET_OBJECTS:zto_obj>)
    set_target_properties(
        ztcore PROPERTIES OUTPUT_NAME ztcore LIBRARY_OUTPUT_DIRECTORY
        ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})
    # libzt.a
    add_library(
        ${STATIC_LIB_NAME} STATIC
        $<TARGET_OBJECTS:zt_pic> $<TARGET_OBJECTS:zto_pic>
        $<TARGET_OBJECTS:natpmp_pic> $<TARGET_OBJECTS:miniupnpc_pic>
        $<TARGET_OBJECTS:lwip_pic> ${libztSrcGlob})
    set_target_properties(
        ${STATIC_LIB_NAME}
        PROPERTIES LINKER_LANGUAGE CXX
                OUTPUT_NAME zt
                POSITION_INDEPENDENT_CODE ON
                LIBRARY_OUTPUT_DIRECTORY ${INTERMEDIATE_LIBRARY_OUTPUT_PATH})
    set_target_properties(${STATIC_LIB_NAME} PROPERTIES COMPILE_FLAGS "${ZT_FLAGS}")
    target_link_libraries(${STATIC_LIB_NAME} ${CMAKE_THREAD_LIBS_INIT})

    if(BUILD_WIN)
        target_link_libraries(${STATIC_LIB_NAME} ${ws2_32_LIBRARY_PATH}
            ${shlwapi_LIBRARY_PATH} ${iphlpapi_LIBRARY_PATH})
    endif()
    if(NOT ZTS_DISABLE_CENTRAL_API)
        target_link_libraries(${STATIC_LIB_NAME} ${CURL_LIBRARIES})
    endif()
endif() # BUILD_STATIC_LIB

# ------------------------------------------------------------------------------
# |                                SHARED LIB                                  |
# ------------------------------------------------------------------------------

if(BUILD_SHARED_LIB)
    # libzt.so/dylib/dll
    add_library(${DYNAMIC_LIB_NAME} SHARED ${libztSrcGlob})
    target_link_libraries(${DYNAMIC_LIB_NAME} ${PYTHON_LIBRARIES})
    target_link_libraries(${DYNAMIC_LIB_NAME} zt_pic lwip_pic zto_pic natpmp_pic
                                                miniupnpc_pic)
    set_target_properties(${DYNAMIC_LIB_NAME}
        PROPERTIES COMPILE_FLAGS
            "${ZT_FLAGS}")
    set_target_properties(
        ${DYNAMIC_LIB_NAME} PROPERTIES OUTPUT_NAME ${DYNAMIC_LIB_OUTPUT_NAME}
        WINDOWS_EXPORT_ALL_SYMBOLS true)
    target_link_libraries(
        ${DYNAMIC_LIB_NAME}
        ${CMAKE_THREAD_LIBS_INIT}
        ${ws2_32_LIBRARY_PATH}
        ${shlwapi_LIBRARY_PATH}
        ${iphlpapi_LIBRARY_PATH}
        zt_pic
        lwip_pic
        zto_pic
        natpmp_pic
        miniupnpc_pic)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

   
    if(NOT ZTS_DISABLE_CENTRAL_API)
        target_link_libraries(${DYNAMIC_LIB_NAME} ${CURL_LIBRARIES})
    endif()
endif() # BUILD_SHARED_LIB

