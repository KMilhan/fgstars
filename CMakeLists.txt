cmake_minimum_required (VERSION 3.16.0 FATAL_ERROR)

PROJECT(kstars
        VERSION 3.8.3
        DESCRIPTION "A Desktop Planetarium for KDE"
        HOMEPAGE_URL "https://edu.kde.org/kstars"
        LANGUAGES CXX C)

set (KStars_VERSION_MAJOR ${kstars_VERSION_MAJOR})
set (KStars_VERSION_MINOR ${kstars_VERSION_MINOR})
set (KStars_VERSION_REVISION ${kstars_VERSION_PATCH})
set (KSTARS_BUILD_RELEASE "Beta")
set (CMAKE_CXX_STANDARD 26)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

set(QT_MIN_VERSION 6.6.0)
set(KF6_MIN_VERSION 6.6.0)
set(QT_MAJOR_VERSION 6)

# Build KStars Lite with -DBUILD_KSTARS_LITE=ON
option(BUILD_KSTARS_LITE "Build KStars Lite" OFF)

# Build the Python bindings for the SkyMesh indexation
option(BUILD_PYKSTARS "Build SkyMesh Python Bindings" OFF)

# Enable Sentry crash reporting (desktop only)
if(NOT ANDROID AND NOT BUILD_KSTARS_LITE)
    option(ENABLE_SENTRY "Enable Sentry crash reporting" OFF)
else()
    set(ENABLE_SENTRY OFF)
endif()

# KStars Timestap build
string(TIMESTAMP KSTARS_BUILD_TS UTC)

# add EVERYTHING to the compile commands
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Large static archives are linked by many test targets in parallel. Generate
# the archive index as part of archive creation so dependents never observe an
# intermediate, unindexed archive while the build graph fan-outs.
if(UNIX AND NOT MSVC)
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH "${CMAKE_COMMAND} -E true")
    set(CMAKE_CXX_ARCHIVE_FINISH "${CMAKE_COMMAND} -E true")
endif()

# Ccache support
IF (ANDROID OR UNIX OR APPLE)
    FIND_PROGRAM(CCACHE_FOUND ccache)
    SET(CCACHE_SUPPORT OFF CACHE BOOL "Enable ccache support")
    IF ((CCACHE_FOUND OR ANDROID) AND CCACHE_SUPPORT MATCHES ON)
        SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    ENDIF ()
ENDIF ()

if(CMAKE_SYSTEM_NAME STREQUAL Android)
    add_definitions(-DANDROID -D__STDC_LIMIT_MACROS)
    set(ANDROID TRUE)
endif()

if("${CMAKE_TOOLCHAIN_FILE}" MATCHES "android.toolchain.cmake$")
    include(${CMAKE_BINARY_DIR}/kf6/kde/install/share/ECM/cmake/ECMConfig.cmake)
endif()

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

# N.B. The includes must be always before find_package(Qt6 ...)
include(ExternalProject)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(KDEInstallDirs6)
include(MacroBoolTo01)
include(ECMQtDeclareLoggingCategory)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(FeatureSummary)

# KStars and some required third-party headers still rely on a few legacy Qt
# conveniences. KDECompilerSettings enables opt-out macros for them, but that
# currently prevents this tree from compiling at all. Disable only the specific
# opt-outs that are known to break the build so the rest of the strict Qt checks
# remain enabled.
remove_definitions(
    -DQT_NO_KEYWORDS
    -DQT_NO_FOREACH
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
)

if(BUILD_KSTARS_LITE)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
        Gui
        Qml
        Quick
        QuickControls2
        Xml
        Svg
        Sql
        Network
        Positioning
        PositioningQuick
        Concurrent
        WebSockets
        Core5Compat
    )
else()
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
        Gui
        Qml
        Quick
        Xml
        Sql
        Svg
        SvgWidgets
        Network
        PrintSupport
        Concurrent
        WebSockets
        DBus
    )
endif()

# Load the frameworks we need
if(BUILD_KSTARS_LITE)
    # Find optional NotifyConfig for desktop notifications.
    find_package(KF6 ${KF6_MIN_VERSION} COMPONENTS NotifyConfig)
    MACRO_BOOL_TO_01(KF6NotifyConfig_FOUND HAVE_NOTIFYCONFIG)

    find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
        Config
        ConfigGui
        WidgetsAddons
        NewStuff
        I18n
        KIO
        XmlGui
        Plotting
        Notifications
    )
else()
    # Find optional NotifyConfig for desktop notifications.
    find_package(KF6 ${KF6_MIN_VERSION} COMPONENTS NotifyConfig)
    MACRO_BOOL_TO_01(KF6NotifyConfig_FOUND HAVE_NOTIFYCONFIG)

    find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
        Config
        Crash
        WidgetsAddons
        NewStuff
        I18n
        KIO
        XmlGui
        Plotting
        Notifications
    )
endif()
set(KFX_VERSION ${KF6_VERSION})

## Eigen3 Library
find_package(Eigen3 REQUIRED CONFIG)

## CFITSIO Library
if(ANDROID)
    if("${CMAKE_TOOLCHAIN_FILE}" MATCHES "android.toolchain.cmake$")
        set(extra_cmake "-DM_LIB=-lm")
    endif()

    find_program(DOS2UNIX dos2unix)
    if (NOT DOS2UNIX)
        message(FATAL_ERROR "Could not find dos2unix")
    endif()

    externalproject_add(cfitsio SOURCE_DIR "${CMAKE_BINARY_DIR}/packaging/android/3rdparty/cfitsio"
        URL https://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio3370.tar.gz
        PATCH_COMMAND bash -c "cd ${CMAKE_BINARY_DIR}/packaging/android/3rdparty/cfitsio && ${DOS2UNIX} ${CMAKE_BINARY_DIR}/packaging/android/3rdparty/cfitsio/CMakeLists.txt && patch -p1 < ${CMAKE_SOURCE_DIR}/packaging/android/3rdparty/cfitsio.patch"
        CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${extra_cmake}
        BUILD_COMMAND make cfitsio
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND "")

    set(CFITSIO_FOUND TRUE)
    set(CFITSIO_INCLUDE_DIR PUBLIC ${CMAKE_BINARY_DIR}/packaging/android/3rdparty/cfitsio)
    set(CFITSIO_LIBRARIES ${CMAKE_BINARY_DIR}/packaging/android/3rdparty/cfitsio/libcfitsio.a)
else()
    if(BUILD_KSTARS_LITE)
        find_package(CFitsio REQUIRED)
    else()
        find_package(CFitsio)
    endif()
endif()
MACRO_BOOL_TO_01(CFITSIO_FOUND HAVE_CFITSIO)
set_package_properties(CFitsio PROPERTIES DESCRIPTION "FITS IO Library" URL "https://heasarc.gsfc.nasa.gov/fitsio/fitsio.html" TYPE OPTIONAL PURPOSE "Support for the FITS (Flexible Image Transport System) data format in KStars.")

## cfitsio cURL dependency
if(CFITSIO_FOUND)
    find_package(CURL)
    set(CFITSIO_LIBRARIES ${CFITSIO_LIBRARIE} CURL::libcurl)
endif(CFITSIO_FOUND)

## INDI Library
if (ANDROID)
    set(QT_ANDROID $ENV{QT_ANDROID})
    if("${CMAKE_TOOLCHAIN_FILE}" MATCHES "android.toolchain.cmake$")
        set(extra_cmake -DCMAKE_AR=${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar)
    endif()
    externalproject_add(indi SOURCE_DIR "${CMAKE_BINARY_DIR}/packaging/android/indi"
        URL https://github.com/indilib/indi/archive/master.zip
        CONFIGURE_COMMAND cmake . -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \\
             -DINDI_BUILD_POSIX_CLIENT=OFF -DINDI_BUILD_SERVER=OFF -DINDI_BUILD_DRIVERS=OFF -DINDI_BUILD_UNITTESTS=OFF \\
             -DINDI_BUILD_DRIVERS=OFF -DINDI_BUILD_QT5_CLIENT=ON -DINDI_CALCULATE_MINMAX=ON \\
             -DCFITSIO_DIR=${CMAKE_BINARY_DIR}/packaging/android/3rdparty/cfitsio -DQT_ANDROID=${QT_ANDROID} -DCMAKE_PREFIX_PATH=${QT_ANDROID} ${extra_cmake}
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND "")

    set(INDI_FOUND TRUE)
    set(INDI_INCLUDE_DIR ${CMAKE_BINARY_DIR}/packaging/android/indi/libs/indibase ${CMAKE_BINARY_DIR}/packaging/android/indi ${CMAKE_BINARY_DIR}/packaging/android/indi/libs)
    set(INDI_CLIENT_ANDROID_LIBRARIES ${CMAKE_BINARY_DIR}/packaging/android/indi/libindiclientqt.a)
else ()
    find_package(INDI 2.0.0)
endif ()
MACRO_BOOL_TO_01(INDI_FOUND HAVE_INDI)
set_package_properties(INDI PROPERTIES DESCRIPTION "Astronomical instrumentation control" URL "https://www.indilib.org" TYPE OPTIONAL PURPOSE "Support for controlling astronomical devices on Linux with KStars.")

option(KSTARS_BUILD_VENDORED_INDI_TEST_RUNTIME "Build a vendored INDI server and simulator runtime for UI tests" ON)
option(KSTARS_BUILD_VENDORED_INDI_3RDPARTY_RUNTIME
       "Build an optional indi-3rdparty source tree against the vendored INDI runtime" OFF)
set(KSTARS_INDI_3RDPARTY_SOURCE_DIR "" CACHE PATH
    "Path to an indi-3rdparty checkout used only during builds")
set(KSTARS_INDI_3RDPARTY_SUBDIR "" CACHE STRING
    "Optional subdirectory inside KSTARS_INDI_3RDPARTY_SOURCE_DIR to build")
option(KSTARS_INDI_3RDPARTY_BUILD_LIBS
       "Build indi-3rdparty vendor libraries instead of drivers when using the optional runtime" OFF)
if(BUILD_TESTING AND UNIX AND NOT APPLE AND KSTARS_BUILD_VENDORED_INDI_TEST_RUNTIME
   AND EXISTS "${CMAKE_SOURCE_DIR}/submodules/indi/CMakeLists.txt")
    set(KSTARS_VENDORED_INDI_BINARY_DIR "${CMAKE_BINARY_DIR}/submodules/indi-runtime")
    set(KSTARS_VENDORED_INDI_PREFIX "${KSTARS_VENDORED_INDI_BINARY_DIR}/install")
    set(KSTARS_VENDORED_INDI_SERVER "${KSTARS_VENDORED_INDI_PREFIX}/bin/indiserver")

    externalproject_add(kstars_indi_test_runtime
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/submodules/indi"
        BINARY_DIR "${KSTARS_VENDORED_INDI_BINARY_DIR}"
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DINDI_BUILD_SERVER=ON
            -DINDI_BUILD_CLIENT=OFF
            -DINDI_BUILD_QT_CLIENT=OFF
            -DINDI_BUILD_DRIVERS=ON
            -DINDI_BUILD_COMMON=ON
            -DINDI_BUILD_UNITTESTS=OFF
            -DINDI_BUILD_INTEGTESTS=OFF
            -DINDI_BUILD_EXAMPLES=OFF
            -DINDI_BUILD_SHARED=ON
            -DINDI_BUILD_STATIC=OFF
            -DINDI_INSTALL_UDEV_RULES=OFF
        BUILD_COMMAND ${CMAKE_COMMAND} --build . --target
            indiserver
            indilx200
            indi_simulator_telescope
            indi_simulator_ccd
            indi_simulator_focus
            indi_simulator_wheel
            indi_simulator_guide
            indi_simulator_weather
            indi_simulator_rotator
            indi_simulator_dome
            indi_simulator_lightpanel
        BUILD_BYPRODUCTS
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indiserver"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_telescope"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_ccd"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_focus"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_wheel"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_guide"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_weather"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_rotator"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_dome"
            "${KSTARS_VENDORED_INDI_PREFIX}/bin/indi_simulator_lightpanel"
            "${KSTARS_VENDORED_INDI_PREFIX}/lib/libindidriver.so"
            "${KSTARS_VENDORED_INDI_PREFIX}/lib/libindiAlignmentDriver.so"
            "${KSTARS_VENDORED_INDI_PREFIX}/lib/libindilx200.so"
        INSTALL_COMMAND
            ${CMAKE_SOURCE_DIR}/scripts/install_vendored_indi_runtime.sh
                ${CMAKE_SOURCE_DIR}/submodules/indi
                ${KSTARS_VENDORED_INDI_BINARY_DIR}
                ${KSTARS_VENDORED_INDI_PREFIX}
    )

    if(KSTARS_BUILD_VENDORED_INDI_3RDPARTY_RUNTIME)
        if(NOT KSTARS_INDI_3RDPARTY_SOURCE_DIR)
            message(FATAL_ERROR
                    "KSTARS_BUILD_VENDORED_INDI_3RDPARTY_RUNTIME requires KSTARS_INDI_3RDPARTY_SOURCE_DIR. "
                    "Bootstrap a checkout with scripts/bootstrap_indi_3rdparty.sh and point CMake at it.")
        endif()

        get_filename_component(KSTARS_INDI_3RDPARTY_EFFECTIVE_SOURCE_DIR
                               "${KSTARS_INDI_3RDPARTY_SOURCE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        if(KSTARS_INDI_3RDPARTY_SUBDIR)
            set(KSTARS_INDI_3RDPARTY_EFFECTIVE_SOURCE_DIR
                "${KSTARS_INDI_3RDPARTY_EFFECTIVE_SOURCE_DIR}/${KSTARS_INDI_3RDPARTY_SUBDIR}")
        endif()

        if(NOT EXISTS "${KSTARS_INDI_3RDPARTY_EFFECTIVE_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR
                    "KSTARS_INDI_3RDPARTY_SOURCE_DIR does not contain a buildable CMake project: "
                    "${KSTARS_INDI_3RDPARTY_EFFECTIVE_SOURCE_DIR}")
        endif()

        if(KSTARS_INDI_3RDPARTY_BUILD_LIBS)
            set(KSTARS_INDI_3RDPARTY_BUILD_LIBS_FLAG 1)
        else()
            set(KSTARS_INDI_3RDPARTY_BUILD_LIBS_FLAG 0)
        endif()

        set(KSTARS_VENDORED_INDI_3RDPARTY_BINARY_DIR "${CMAKE_BINARY_DIR}/submodules/indi-3rdparty-runtime")
        externalproject_add(kstars_indi_3rdparty_runtime
            DEPENDS kstars_indi_test_runtime
            SOURCE_DIR "${KSTARS_INDI_3RDPARTY_EFFECTIVE_SOURCE_DIR}"
            BINARY_DIR "${KSTARS_VENDORED_INDI_3RDPARTY_BINARY_DIR}"
            CMAKE_ARGS
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_INSTALL_PREFIX=${KSTARS_VENDORED_INDI_PREFIX}
                -DINDI_ROOT=${KSTARS_VENDORED_INDI_PREFIX}
                -DINDI_DIR=${KSTARS_VENDORED_INDI_PREFIX}
                -DCMAKE_PREFIX_PATH=${KSTARS_VENDORED_INDI_PREFIX}
                -DBUILD_LIBS=${KSTARS_INDI_3RDPARTY_BUILD_LIBS_FLAG}
                -DINDI_INSTALL_UDEV_RULES=OFF
            BUILD_COMMAND ${CMAKE_COMMAND} --build .
            INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
        )
    endif()
endif()

if (NOT ANDROID)
    find_package(StellarSolver REQUIRED 2.6)
endif(NOT ANDROID)
MACRO_BOOL_TO_01(StellarSolver_FOUND HAVE_STELLARSOLVER)

find_package(Nova)
MACRO_BOOL_TO_01(NOVA_FOUND HAVE_LIBNOVA)
if (NOVA_FOUND)
    include_directories(${NOVA_INCLUDE_DIR})
endif()

if(BUILD_TESTING)
    # Find liberfa, which we check our computation against
    find_package(ERFA)
    MACRO_BOOL_TO_01(ERFA_FOUND HAVE_LIBERFA)
    set_package_properties(ERFA PROPERTIES TYPE OPTIONAL PURPOSE "BSD-compatible version of SOFA, used to test core computations of KStars (only needed by tests)")
endif ()

## Libraw
if(BUILD_KSTARS_LITE AND ANDROID)
    # OpenMP needed for LibRaw
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp -DLIBRAW_USE_OPENMP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -DLIBRAW_USE_OPENMP")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgomp")

    externalproject_add(raw SOURCE_DIR "${CMAKE_BINARY_DIR}/packaging/android/3rdparty/libraw"
        URL https://www.libraw.org/data/LibRaw-0.17.2.tar.gz
        PATCH_COMMAND bash -c "cd ${CMAKE_BINARY_DIR}/packaging/android/3rdparty/libraw && wget -qO- https://github.com/LibRaw/LibRaw-cmake/archive/master.zip | jar xvf /dev/stdin && rm -rf cmake && mv -f LibRaw-cmake-master/CMakeLists.txt LibRaw-cmake-master/cmake . && patch -p1 < ${CMAKE_SOURCE_DIR}/packaging/android/3rdparty/libraw.patch"
        CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DENABLE_OPENMP=OFF -DENABLE_LCMS=OFF
        BUILD_COMMAND make raw
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND "")
    include_directories(${CMAKE_BINARY_DIR}/packaging/android/3rdparty/libraw)
    set(LIBRAW_LIBRARIES ${CMAKE_BINARY_DIR}/packaging/android/3rdparty/libraw/libraw.a)
    set(LibRaw_FOUND true)
else()
    find_package(LibRaw)
endif()
MACRO_BOOL_TO_01(LibRaw_FOUND HAVE_LIBRAW)
set_package_properties(LibRaw PROPERTIES DESCRIPTION "Library for reading RAW files" URL "https://www.libraw.org/" TYPE OPTIONAL PURPOSE "Support for reading and displaying RAW files in KStars.")

## WCS Library
find_package(WCSLIB)
set_package_properties(WCSLIB PROPERTIES DESCRIPTION "World Coordinate System library" URL "https://www.atnf.csiro.au/people/mcalabre/WCS" TYPE REQUIRED PURPOSE "WCS enables KStars to read and process world coordinate systems in FITS header.")

## XPlanet
find_package(Xplanet)
set_package_properties(Xplanet PROPERTIES DESCRIPTION "Renders an image of all the major planets and most satellites" URL "http://xplanet.sourceforge.net" TYPE RUNTIME PURPOSE "Gives KStars support for xplanet.")

# Qt6 Data Visualization
find_package(Qt6DataVisualization)
MACRO_BOOL_TO_01(Qt6DataVisualization_FOUND HAVE_DATAVISUALIZATION)

# LibXISF
find_package(LibXISF)
set_package_properties(LibXISF PROPERTIES DESCRIPTION "Library for loading and saving XISF images" URL "https://nouspiro.space" TYPE OPTIONAL)
MACRO_BOOL_TO_01(LibXISF_FOUND HAVE_XISF)

## Astrometry.net
#find_package(AstrometryNet)
#set_package_properties(AstrometryNet PROPERTIES DESCRIPTION "Astrometrics Library" URL "http://www.astrometry.net" TYPE RUNTIME PURPOSE "Support for plate solving in KStars.")

## Key Chain
find_package(Qt6Keychain)
MACRO_BOOL_TO_01(Qt6Keychain_FOUND HAVE_KEYCHAIN)

## OpenCV
# Ensure we have at least version 4.6.0
find_package(OpenCV 4.6.0 REQUIRED COMPONENTS core imgcodecs calib3d photo)
set_package_properties(OpenCV PROPERTIES DESCRIPTION "OpenCV library" URL "https://opencv.org" TYPE REQUIRED PURPOSE "Standard library of image processing functions.")
include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS})

## OpenGL
find_package(OpenGL)
set_package_properties(OpenGL PROPERTIES DESCRIPTION "Open Graphics Library" URL "https://www.opengl.org/" TYPE OPTIONAL PURPOSE "Support for hardware rendering in KStars.")

## Sentry
if(ENABLE_SENTRY)
    find_package(Sentry)    
    MACRO_BOOL_TO_01(Sentry_FOUND HAVE_SENTRY)
    set_package_properties(Sentry PROPERTIES DESCRIPTION "Sentry crash reporting SDK" URL "https://sentry.io" TYPE OPTIONAL PURPOSE "Provides crash reporting and error tracking for KStars.")
endif()

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32)
  add_definitions(-DQT_STRICT_ITERATORS)
endif()
add_definitions(-DQT_NO_CAST_TO_ASCII)
if (BUILD_TESTING)
  add_definitions(-DKSTARS_BUILD_TESTING=1)
endif()

# Needed for htmesh, and libraw
kde_enable_exceptions()

if (UNIX)
  # Optimize binary size by dropping unneeded symbols at linking stage
  if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections -ffunction-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
  endif()
endif(UNIX)

# Optimize binary size by dropping unneeded symbols at linking stage
if (ANDROID)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections -ffunction-sections -fvisibility=hidden -fvisibility-inlines-hidden")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
endif ()

# Add security (hardening flags)
IF (UNIX OR APPLE OR ANDROID)
    SET(SEC_COMP_FLAGS "${SEC_COMP_FLAGS} -fstack-protector-all -Wcast-align -fPIE")
    # This is ill-defined, but I searched for a solution for quite a long time to no avail
    # - FORTIFY_SOURCE generates a warning when there is no code optimization, and warnings will become errors at some point
    # - Checking the build type in CMakeLists.txt is a bad practice, it is better to use generator expressions when importing targets
    # - CMake modules do not expand cmake generator expressions when testing for a system/compiler feature
    # - Testing CMAKE_BUILD_TYPE does not work well with multi-configuration generators, but those do not fill CMAKE_BUILD_TYPE
    # - The "right" way to write this is "$<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=2>", but it fails the build configuration
    IF (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        SET(SEC_COMP_FLAGS "${SEC_COMP_FLAGS} -D_FORTIFY_SOURCE=2")
    ENDIF (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    IF (NOT ANDROID AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT APPLE)
        SET(SEC_COMP_FLAGS "${SEC_COMP_FLAGS} -Wa,--noexecstack")
    ENDIF ()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SEC_COMP_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SEC_COMP_FLAGS}")
    SET(SEC_LINK_FLAGS "")

    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/CheckNodump.cmake")
    check_nodump(NODUMP_FLAGS)

    IF (NOT APPLE)
        SET(SEC_LINK_FLAGS "${SEC_LINK_FLAGS} ${NODUMP_FLAGS} -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")
    ENDIF ()
    IF (NOT ANDROID AND NOT APPLE)
        SET(SEC_LINK_FLAGS "${SEC_LINK_FLAGS} -pie")
    ENDIF ()
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SEC_LINK_FLAGS}")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SEC_LINK_FLAGS}")
ENDIF ()

SET(FIX_WARNINGS OFF CACHE BOOL "Enable strict compilation mode to turn compiler warnings to errors")
# Warning, debug and linker flags
IF (UNIX OR APPLE)
    SET(COMP_FLAGS "")
    SET(LINKER_FLAGS "")
    # Verbose warnings and turns all to errors
    SET(COMP_FLAGS "${COMP_FLAGS} -Wall -Wextra")
    IF (FIX_WARNINGS)
        SET(COMP_FLAGS "${COMP_FLAGS} -Werror")
    ENDIF ()
    # Omit problematic warnings
    SET(COMP_FLAGS "${COMP_FLAGS} -Wno-deprecated")
    SET(COMP_FLAGS "${COMP_FLAGS} -Wno-error=pedantic")
    SET(COMP_FLAGS "${COMP_FLAGS} -Wno-error=zero-as-null-pointer-constant")
    SET(COMP_FLAGS "${COMP_FLAGS} -Wno-zero-as-null-pointer-constant")
    SET(COMP_FLAGS "${COMP_FLAGS} -Wno-error=deprecated-declarations")
    SET(COMP_FLAGS "${COMP_FLAGS} -Wno-error=format")
    SET(COMP_FLAGS "${COMP_FLAGS} -Wno-deprecated-declarations")
    IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-unused-but-set-variable")
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-stringop-truncation")
    ENDIF ()
    IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.9.9)
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-format-truncation")
    ENDIF ()
    IF (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-nonnull -Wno-deprecated-declarations")
    ENDIF ()
    IF (BUILD_KSTARS_LITE AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-overloaded-virtual")
    ENDIF ()

    SET(COMP_FLAGS "${COMP_FLAGS} -g")

    # Note: The following flags are problematic on older systems with gcc 4.8
    IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9.9))
        IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
            SET(COMP_FLAGS "${COMP_FLAGS} -Wno-unused-command-line-argument")
        ENDIF ()
        FIND_PROGRAM(LDGOLD_FOUND ld.gold)
        SET(LDGOLD_SUPPORT OFF CACHE BOOL "Enable ld.gold support")
        # Optional ld.gold is 2x faster than normal ld
        IF (LDGOLD_FOUND AND LDGOLD_SUPPORT MATCHES ON AND NOT APPLE AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES arm)
            SET(LINKER_FLAGS "${LINKER_FLAGS} -fuse-ld=gold")
            # We have Gsl library what is a special case for linking:
            # The gsl library must be linked with cblas. There are two alternatives for this: libcblas or libgslcblas.
            # For example, CMake gets the GSL_LIBRARIES linking flags from the pkgconfig (gsl.pc) file on Ubuntu.
            # This file defines -lgsl -lglscblas for linking flags and if KStars is compiled with Clang, the linker
            # finds out magically that KStars must be linked against glscblas library, but gslcblas is omitted in linking
            # stage if KStars is built with gcc. The linker must be instructed explicitly to link against all libraries
            # passed on command line by -Wl,--no-as-needed.
            SET(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--no-as-needed")
            # Use Identical Code Folding
            SET(COMP_FLAGS "${COMP_FLAGS} -ffunction-sections")
            SET(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--icf=safe")
            # Compress the debug sections
            # Note: Before valgrind 3.12.0, patch should be applied for valgrind (https://bugs.kde.org/show_bug.cgi?id=303877)
            IF (NOT APPLE AND NOT ANDROID AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES arm AND NOT CMAKE_CXX_CLANG_TIDY)
                SET(COMP_FLAGS "${COMP_FLAGS} -Wa,--compress-debug-sections")
                SET(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--compress-debug-sections=zlib")
            ENDIF ()
        ENDIF ()
    ENDIF ()

    # Apply the flags. Strip any previously cached C++-only warning options from
    # C flags so re-configuring an existing build directory stays clean.
    string(REPLACE "-Wno-deprecated-literal-operator" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMP_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMP_FLAGS} -Wno-deprecated-literal-operator")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
ENDIF ()

# Sanitizer support
SET(SANITIZERS OFF CACHE BOOL "Sanitizer support for gcc and Clang")
IF (SANITIZERS AND
    ((UNIX AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR CMAKE_COMPILER_IS_GNUCXX) OR
     (APPLE AND (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_COMPILER_IS_GNUCXX))))
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
ENDIF ()

# Unity build support
SET(UNITY_BUILD OFF CACHE BOOL "Enable Unity Build")
INCLUDE(UnityBuild)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config-kstars.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/config-kstars.h
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/kstars/version.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/kstars/version.h )

# let our config.h be found first in any case
include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})

# add some utility headers
include_directories(utils)

add_subdirectory(kstars)
add_subdirectory(datahandlers)

IF (NOT ANDROID)
    IF (BUILD_TESTING)
    enable_testing(true)
    add_subdirectory(Tests)
    ENDIF()
    ki18n_install(po)
ENDIF ()

if(NOT APPLE) #This is not used on MacOS
    install(FILES org.kde.kstars.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
endif(NOT APPLE)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# Code formatting helper (see README.md "Code Style")
find_program(ASTYLE_EXECUTABLE astyle)
if(ASTYLE_EXECUTABLE)
    add_custom_target(astyle
        COMMAND bash ${CMAKE_SOURCE_DIR}/tools/run_astyle.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting sources with astyle"
        VERBATIM
    )
else()
    add_custom_target(astyle
        COMMAND ${CMAKE_COMMAND} -E echo "astyle not found; install Artistic Style to use this target."
        VERBATIM
    )
endif()

# Only build kauth for Linux
# JM 2018-09-26: KAuth is disabled due to reliability issues.
#if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    # This helper file is generated to download astrometry.net index files.
#    add_executable(kauth_kstars_helper kstars/auxiliary/downloadhelper.cpp  ${helper_mocs})
#    target_link_libraries(kauth_kstars_helper Qt::Core)
#    target_link_libraries(kauth_kstars_helper Qt::Widgets)
#    target_link_libraries(kauth_kstars_helper KF6::Auth)
#    install(TARGETS kauth_kstars_helper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
#    kauth_install_helper_files(kauth_kstars_helper org.kde.kauth.kstars root)
#    kauth_install_actions(org.kde.kauth.kstars org.kde.kauth.kstars.actions)
#ENDIF ()

# Final package generation
if ("${CMAKE_TOOLCHAIN_FILE}" MATCHES "android.toolchain.cmake$")
    set(EXPORT_DIR "${CMAKE_BINARY_DIR}/packaging/android/kstars_build_apk/")
    set(ANDROID_APK_DIR "${CMAKE_SOURCE_DIR}/packaging/android/apk/")
    set(EXECUTABLE_DESTINATION_PATH "${EXPORT_DIR}/libs/armeabi-v7a/libkstars.so")
    set(ANDROID_NDK $ENV{CMAKE_ANDROID_NDK})
    set(ANDROID_SDK_ROOT $ENV{ANDROID_SDK_ROOT})
    set(ANDROID_API_LEVEL $ENV{ANDROID_API_LEVEL})
    set(ANDROID_TOOLCHAIN arm-linux-androideabi)
    set(ANDROID_GCC_VERSION 4.9)
    set(ANDROID_ABI armeabi-v7a)
    set(_HOST "${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
    string(TOLOWER "${_HOST}" _HOST)
    set(ANDROID_SDK_BUILD_TOOLS_REVISION 21.1.1)
    set(ANDROID_KEYSTORE $ENV{ANDROID_KEYSTORE})
    set(ANDROID_KEYSTORE_ALIAS $ENV{ANDROID_KEYSTORE_ALIAS})
    configure_file("${CMAKE_SOURCE_DIR}/packaging/android/deployment-file.json.in" "${CMAKE_BINARY_DIR}/packaging/android/kstars-deployment.json.in")


    add_custom_target(create-apk-debug-kstars
        COMMAND cmake -E echo "Generating $<TARGET_NAME:kstars> with $<TARGET_FILE_DIR:Qt6::qmake>/androiddeployqt"
        COMMAND cmake -E remove_directory "${EXPORT_DIR}"
        COMMAND cmake -E copy_directory "${CMAKE_SOURCE_DIR}/packaging/android/apk" "${EXPORT_DIR}"
        COMMAND cmake -E copy "$<TARGET_FILE:kstars>" "${EXECUTABLE_DESTINATION_PATH}"
        COMMAND cmake -DINPUT_FILE="${CMAKE_BINARY_DIR}/packaging/android/kstars-deployment.json.in" -DOUTPUT_FILE="${CMAKE_BINARY_DIR}/packaging/android/kstars-deployment.json" "-DTARGET_DIR=${CMAKE_BINARY_DIR}/kstars" "-DTARGET_NAME=kstars" "-DEXPORT_DIR=${CMAKE_INSTALL_PREFIX}" -P ${CMAKE_SOURCE_DIR}/packaging/android/specifydependencies.cmake
        COMMAND $<TARGET_FILE_DIR:Qt6::qmake>/androiddeployqt --input "${CMAKE_BINARY_DIR}/packaging/android/kstars-deployment.json" --output "${EXPORT_DIR}" --android-platform android-${ANDROID_API_LEVEL} --debug --deployment bundled "\\$(ARGS)"
        )
    add_custom_target(create-apk-release-kstars
        COMMAND cmake -E echo "Generating $<TARGET_NAME:kstars> with $<TARGET_FILE_DIR:Qt6::qmake>/androiddeployqt"
        COMMAND cmake -E remove_directory "${EXPORT_DIR}"
        COMMAND cmake -E copy_directory "${CMAKE_SOURCE_DIR}/packaging/android/apk" "${EXPORT_DIR}"
        COMMAND cmake -E copy "$<TARGET_FILE:kstars>" "${EXECUTABLE_DESTINATION_PATH}"
        COMMAND cmake -DINPUT_FILE="${CMAKE_BINARY_DIR}/packaging/android/kstars-deployment.json.in" -DOUTPUT_FILE="${CMAKE_BINARY_DIR}/packaging/android/kstars-deployment.json" "-DTARGET_DIR=${CMAKE_BINARY_DIR}/kstars" "-DTARGET_NAME=kstars" "-DEXPORT_DIR=${CMAKE_INSTALL_PREFIX}" -P ${CMAKE_SOURCE_DIR}/packaging/android/specifydependencies.cmake
        COMMAND $<TARGET_FILE_DIR:Qt6::qmake>/androiddeployqt --input "${CMAKE_BINARY_DIR}/packaging/android/kstars-deployment.json" --output "${EXPORT_DIR}" --android-platform android-${ANDROID_API_LEVEL} --release --deployment bundled "\\$(ARGS)"
        )
    add_custom_target(install-apk-debug-kstars
        COMMAND adb install -r ${CMAKE_BINARY_DIR}/packaging/android/kstars_build_apk//bin/QtApp-debug.apk
        )

    if (ANDROID_KEYSTORE AND ANDROID_KEYSTORE_ALIAS)
        add_custom_target(sign-apk-kstars
            COMMAND jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ${ANDROID_KEYSTORE} ${CMAKE_BINARY_DIR}/packaging/android/kstars_build_apk/bin/QtApp-release-unsigned.apk ${ANDROID_KEYSTORE_ALIAS}
            COMMAND rm -rf ${CMAKE_BINARY_DIR}/packaging/android/kstars_build_apk/bin/kstars-signed.apk
            COMMAND zipalign -v 4 ${CMAKE_BINARY_DIR}/packaging/android/kstars_build_apk/bin/QtApp-release-unsigned.apk ${CMAKE_BINARY_DIR}/packaging/android/kstars_build_apk/bin/kstars-signed.apk
            )
    endif ()
endif ()
