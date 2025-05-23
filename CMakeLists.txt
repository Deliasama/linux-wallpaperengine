cmake_minimum_required(VERSION 3.12)
project(linux-wallpaperengine)

# prevent installing to /usr by default so we don't bork anything
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/${PROJECT_NAME}" CACHE PATH "..." FORCE)
endif()

set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
set(OpenGL_GL_PREFERENCE "LEGACY")
set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
set(PATCHESDIR ${DATADIR}/patches/)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT ERRORONLY)
    set(ERRORONLY 0)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

# if you're developing you might find this debug option useful for shader output, although RenderDoc is encouraged
add_compile_definitions(ERRORONLY=${ERRORONLY})
add_compile_definitions(DATADIR="${DATADIR}")

find_package(X11)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SDL2 REQUIRED)
find_package(MPV REQUIRED)
find_package(LZ4 REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(PulseAudio REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Download CEF of specified version for current platform
# Specify the CEF distribution version.
set(CEF_VERSION "120.1.10+g3ce3184+chromium-120.0.6099.129")
# Determine the platform.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    if("${PROJECT_ARCH}" STREQUAL "arm64")
        set(CEF_PLATFORM "macosarm64")
        set(CEF_ARCH_DETECTION "${PROJECT_ARCH}")
    elseif("${PROJECT_ARCH}" STREQUAL "x86_64")
        set(CEF_PLATFORM "macosx64")
        set(CEF_ARCH_DETECTION "${PROJECT_ARCH}")
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "arm64")
        set(PROJECT_ARCH "arm64")
        set(CEF_PLATFORM "macosarm64")
        set(CEF_ARCH_DETECTION "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    else()
        set(PROJECT_ARCH "x86_64")
        set(CEF_PLATFORM "macosx64")
        set(CEF_ARCH_DETECTION "Default for Darwin")
    endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm")
        set(CEF_PLATFORM "linuxarm")
        set(CEF_ARCH_DETECTION "${CMAKE_SYSTEM_PROCESSOR}")
    elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
        set(PROJECT_ARCH "arm64")
        set(CEF_PLATFORM "linuxarm64")
        set(CEF_ARCH_DETECTION "${CMAKE_SYSTEM_PROCESSOR}")
    elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
        set(PROJECT_ARCH "arm64")
        set(CEF_PLATFORM "linuxarm64")
        set(CEF_ARCH_DETECTION "${CMAKE_SYSTEM_PROCESSOR}")
    elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
        set(CEF_PLATFORM "linux64")
        set(CEF_ARCH_DETECTION "Based on void size ${CMAKE_SIZEOF_VOID_P} (${CMAKE_SYSTEM_PROCESSOR})")
    else()
        message(FATAL_ERROR "Linux x86 32-bit builds are discontinued.")
    endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    if("${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
        set(CEF_PLATFORM "windowsarm64")
        set(CEF_ARCH_DETECTION "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
    elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
        set(CEF_PLATFORM "windows64")
        set(CEF_ARCH_DETECTION "Based on void size ${CMAKE_SIZEOF_VOID_P}")
    else()
        set(CEF_PLATFORM "windows32")
        set(CEF_ARCH_DETECTION "Default for Windows")
    endif()
endif()
message(STATUS "Using CEF for ${CMAKE_SYSTEM_NAME} - ${CEF_PLATFORM} (${CEF_ARCH_DETECTION})")
include(DownloadCEF)
DownloadCEF("${CEF_PLATFORM}" "${CEF_VERSION}" "${CMAKE_SOURCE_DIR}/third_party/cef")

find_package(CEF REQUIRED)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/build
    )

set(
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/lib
    )

set(
    TARGET_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/build
    )

add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

include_directories(
    ${MPV_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${LZ4_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIR}
    ${PULSEAUDIO_INCLUDE_DIR}
    src
    ${CEF_INCLUDE_PATH}
    ${CMAKE_SOURCE_DIR}
    include)


add_library(ceflib SHARED IMPORTED)
set_target_properties(ceflib
    PROPERTIES IMPORTED_LOCATION ${TARGET_OUTPUT_DIRECTORY}/libcef.so)

ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
# SET_CEF_TARGET_OUT_DIR()
include_directories(${_CEF_ROOT})

# try to enable wayland builds when possible
pkg_check_modules(WAYLAND_SUPPORT wayland-cursor wayland-protocols egl wayland-egl)

if(WAYLAND_SUPPORT_FOUND)
    find_program(WaylandScanner NAMES wayland-scanner)
    message(STATUS "Found WaylandScanner at ${WaylandScanner}")
    execute_process(
        COMMAND pkg-config --variable=pkgdatadir wayland-protocols
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Found wayland-protocols at ${WAYLAND_PROTOCOLS_DIR}")

    message(STATUS "Building protocols...")
    execute_process(
        COMMAND ${WaylandScanner} client-header ${CMAKE_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml wlr-foreign-toplevel-management-unstable-v1-protocol.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(
        COMMAND ${WaylandScanner} private-code ${CMAKE_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml wlr-foreign-toplevel-management-unstable-v1-protocol.c
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(
        COMMAND ${WaylandScanner} client-header ${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml wlr-layer-shell-unstable-v1-protocol.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(
        COMMAND ${WaylandScanner} private-code ${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml wlr-layer-shell-unstable-v1-protocol.c
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(
        COMMAND ${WaylandScanner} client-header ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(
        COMMAND ${WaylandScanner} private-code ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_compile_definitions(ENABLE_WAYLAND)
    set(WAYLAND_SOURCES
            "src/WallpaperEngine/Render/Drivers/CWaylandOpenGLDriver.h"
            "src/WallpaperEngine/Render/Drivers/CWaylandOpenGLDriver.cpp"
            "src/WallpaperEngine/Render/Drivers/Detectors/CWaylandFullScreenDetector.cpp"
            "src/WallpaperEngine/Render/Drivers/Detectors/CWaylandFullScreenDetector.h"
            "src/WallpaperEngine/Render/Drivers/Output/CWaylandOutput.cpp"
            "src/WallpaperEngine/Render/Drivers/Output/CWaylandOutput.h"
            "src/WallpaperEngine/Render/Drivers/Output/CWaylandOutputViewport.cpp"
            "src/WallpaperEngine/Render/Drivers/Output/CWaylandOutputViewport.h"
            "src/WallpaperEngine/Input/Drivers/CWaylandMouseInput.cpp"
            "src/WallpaperEngine/Input/Drivers/CWaylandMouseInput.h"
            "xdg-shell-protocol.c"
            "wlr-foreign-toplevel-management-unstable-v1-protocol.c"
            "wlr-layer-shell-unstable-v1-protocol.c")
endif()

if(X11_FOUND)
    set(X11_SUPPORT_FOUND TRUE)

    if(NOT X11_Xrandr_FOUND)
        message(WARNING "X11 support disabled. Xrandr package is missing")
        set(X11_SUPPORT_FOUND FALSE)
    endif()
    if(NOT X11_xf86vmode_FOUND)
        message(WARNING "X11 support disabled. Xxf86vm package is missing")
        set(X11_SUPPORT_FOUND FALSE)
    endif()
endif()

if(X11_SUPPORT_FOUND)
    include_directories(${X11_INCLUDE_DIR} ${XRANDR_INCLUDE_DIR})
    add_compile_definitions(ENABLE_X11)

    set(X11_SOURCES
            "src/WallpaperEngine/Render/Drivers/Output/CX11Output.cpp"
            "src/WallpaperEngine/Render/Drivers/Output/CX11Output.h"
            "src/WallpaperEngine/Render/Drivers/Detectors/CX11FullScreenDetector.cpp"
            "src/WallpaperEngine/Render/Drivers/Detectors/CX11FullScreenDetector.h")
endif()

add_executable(
    linux-wallpaperengine
    main.cpp
    
    src/Qt/UIWindow.h
    src/Qt/UIWindow.cpp

    src/External/Android/fft.cpp
    src/External/Android/fft.h

    src/Steam/FileSystem/FileSystem.h
    src/Steam/FileSystem/FileSystem.cpp

    src/WallpaperEngine/Logging/CLog.cpp
    src/WallpaperEngine/Logging/CLog.h

    src/WallpaperEngine/Application/CApplicationContext.cpp
    src/WallpaperEngine/Application/CApplicationContext.h
    src/WallpaperEngine/Application/CWallpaperApplication.cpp
    src/WallpaperEngine/Application/CWallpaperApplication.h

    src/WallpaperEngine/Assets/CPackageLoadException.cpp
    src/WallpaperEngine/Assets/CPackageLoadException.h
    src/WallpaperEngine/Assets/CAssetLoadException.cpp
    src/WallpaperEngine/Assets/CAssetLoadException.h
    src/WallpaperEngine/Assets/CContainer.h
    src/WallpaperEngine/Assets/CContainer.cpp
    src/WallpaperEngine/Assets/CVirtualContainer.h
    src/WallpaperEngine/Assets/CVirtualContainer.cpp
    src/WallpaperEngine/Assets/CCombinedContainer.h
    src/WallpaperEngine/Assets/CCombinedContainer.cpp
    src/WallpaperEngine/Assets/CDirectory.h
    src/WallpaperEngine/Assets/CDirectory.cpp
    src/WallpaperEngine/Assets/CPackage.h
    src/WallpaperEngine/Assets/CPackage.cpp
    src/WallpaperEngine/Assets/ITexture.h
    src/WallpaperEngine/Assets/CTexture.h
    src/WallpaperEngine/Assets/CTexture.cpp

    src/WallpaperEngine/Core/Core.h
    src/WallpaperEngine/Core/Core.cpp

    src/WallpaperEngine/Audio/Drivers/Recorders/CPulseAudioPlaybackRecorder.cpp
    src/WallpaperEngine/Audio/Drivers/Recorders/CPulseAudioPlaybackRecorder.h
    src/WallpaperEngine/Audio/Drivers/Recorders/CPlaybackRecorder.cpp
    src/WallpaperEngine/Audio/Drivers/Recorders/CPlaybackRecorder.h

    src/WallpaperEngine/Audio/Drivers/Detectors/CPulseAudioPlayingDetector.cpp
    src/WallpaperEngine/Audio/Drivers/Detectors/CPulseAudioPlayingDetector.h
    src/WallpaperEngine/Audio/Drivers/Detectors/CAudioPlayingDetector.cpp
    src/WallpaperEngine/Audio/Drivers/Detectors/CAudioPlayingDetector.h

    src/WallpaperEngine/Audio/Drivers/CAudioDriver.cpp
    src/WallpaperEngine/Audio/Drivers/CAudioDriver.h
    src/WallpaperEngine/Audio/Drivers/CSDLAudioDriver.cpp
    src/WallpaperEngine/Audio/Drivers/CSDLAudioDriver.h
    src/WallpaperEngine/Audio/CAudioContext.cpp
    src/WallpaperEngine/Audio/CAudioContext.h
    src/WallpaperEngine/Audio/CAudioStream.cpp
    src/WallpaperEngine/Audio/CAudioStream.h

    src/WallpaperEngine/Input/CInputContext.cpp
    src/WallpaperEngine/Input/CInputContext.h
    src/WallpaperEngine/Input/CMouseInput.cpp
    src/WallpaperEngine/Input/CMouseInput.h
    src/WallpaperEngine/Input/Drivers/CGLFWMouseInput.cpp
    src/WallpaperEngine/Input/Drivers/CGLFWMouseInput.h

    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariable.h
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariable.cpp
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableFloat.h
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableFloat.cpp
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableInteger.h
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableInteger.cpp
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableVector2.h
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableVector2.cpp
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableVector3.h
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableVector3.cpp
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableVector4.h
    src/WallpaperEngine/Render/Shaders/Variables/CShaderVariableVector4.cpp

    src/WallpaperEngine/Render/Shaders/Compiler.h
    src/WallpaperEngine/Render/Shaders/Compiler.cpp

    src/WallpaperEngine/Render/Helpers/CContextAware.cpp
    src/WallpaperEngine/Render/Helpers/CContextAware.h

    src/WallpaperEngine/Render/Drivers/Detectors/CFullScreenDetector.cpp
    src/WallpaperEngine/Render/Drivers/Detectors/CFullScreenDetector.h

    src/WallpaperEngine/Render/Drivers/Output/COutput.cpp
    src/WallpaperEngine/Render/Drivers/Output/COutput.h
    src/WallpaperEngine/Render/Drivers/Output/CGLFWWindowOutput.cpp
    src/WallpaperEngine/Render/Drivers/Output/CGLFWWindowOutput.h
    src/WallpaperEngine/Render/Drivers/Output/CGLFWOutputViewport.cpp
    src/WallpaperEngine/Render/Drivers/Output/CGLFWOutputViewport.h
    src/WallpaperEngine/Render/Drivers/Output/COutputViewport.cpp
    src/WallpaperEngine/Render/Drivers/Output/COutputViewport.h
    src/WallpaperEngine/Render/Drivers/CGLFWOpenGLDriver.h
    src/WallpaperEngine/Render/Drivers/CGLFWOpenGLDriver.cpp
    src/WallpaperEngine/Render/Drivers/CVideoDriver.h
    src/WallpaperEngine/Render/Drivers/CVideoDriver.cpp
    src/WallpaperEngine/Render/CRenderContext.h
    src/WallpaperEngine/Render/CRenderContext.cpp
    src/WallpaperEngine/Render/CTextureCache.h
    src/WallpaperEngine/Render/CTextureCache.cpp

    src/WallpaperEngine/Render/Helpers/CContextAware.cpp
    src/WallpaperEngine/Render/Helpers/CContextAware.h

    src/WallpaperEngine/Render/CWallpaper.h
    src/WallpaperEngine/Render/CWallpaper.cpp
    src/WallpaperEngine/Render/CWallpaperState.h
    src/WallpaperEngine/Render/CWallpaperState.cpp
    src/WallpaperEngine/Render/Wallpapers/CScene.h
    src/WallpaperEngine/Render/Wallpapers/CScene.cpp
    src/WallpaperEngine/Render/Wallpapers/CVideo.h
    src/WallpaperEngine/Render/Wallpapers/CVideo.cpp
    src/WallpaperEngine/Render/Wallpapers/CWeb.h
    src/WallpaperEngine/Render/Wallpapers/CWeb.cpp
    src/WallpaperEngine/Render/CCamera.h
    src/WallpaperEngine/Render/CCamera.cpp
    src/WallpaperEngine/Render/CObject.h
    src/WallpaperEngine/Render/CObject.cpp

    src/WallpaperEngine/Render/Objects/CImage.h
    src/WallpaperEngine/Render/Objects/CImage.cpp
    src/WallpaperEngine/Render/Objects/CSound.h
    src/WallpaperEngine/Render/Objects/CSound.cpp
    src/WallpaperEngine/Render/Objects/CEffect.h
    src/WallpaperEngine/Render/Objects/CEffect.cpp

    src/WallpaperEngine/Render/CFBO.h
    src/WallpaperEngine/Render/CFBO.cpp
    src/WallpaperEngine/Render/Objects/Effects/CPass.h
    src/WallpaperEngine/Render/Objects/Effects/CPass.cpp
    src/WallpaperEngine/Render/Objects/Effects/CMaterial.h
    src/WallpaperEngine/Render/Objects/Effects/CMaterial.cpp

    src/WallpaperEngine/WebBrowser/CWebBrowserContext.cpp
    src/WallpaperEngine/WebBrowser/CWebBrowserContext.h

    src/WallpaperEngine/FileSystem/FileSystem.cpp
    src/WallpaperEngine/FileSystem/FileSystem.h

    src/WallpaperEngine/Core/UserSettings/CUserSettingValue.cpp
    src/WallpaperEngine/Core/UserSettings/CUserSettingValue.h
    src/WallpaperEngine/Core/UserSettings/CUserSettingBoolean.cpp
    src/WallpaperEngine/Core/UserSettings/CUserSettingBoolean.h
    src/WallpaperEngine/Core/UserSettings/CUserSettingVector3.cpp
    src/WallpaperEngine/Core/UserSettings/CUserSettingVector3.h
    src/WallpaperEngine/Core/UserSettings/CUserSettingFloat.cpp
    src/WallpaperEngine/Core/UserSettings/CUserSettingFloat.h

    src/WallpaperEngine/Core/CProject.cpp
    src/WallpaperEngine/Core/CProject.h
    src/WallpaperEngine/Core/CWallpaper.cpp
    src/WallpaperEngine/Core/CWallpaper.h
    src/WallpaperEngine/Core/Wallpapers/CScene.cpp
    src/WallpaperEngine/Core/Wallpapers/CScene.h
    src/WallpaperEngine/Core/Wallpapers/CVideo.cpp
    src/WallpaperEngine/Core/Wallpapers/CVideo.h
    src/WallpaperEngine/Core/Wallpapers/CWeb.cpp
    src/WallpaperEngine/Core/Wallpapers/CWeb.h
    src/WallpaperEngine/Core/CObject.cpp
    src/WallpaperEngine/Core/CObject.h

    src/WallpaperEngine/Core/Projects/CProperty.h
    src/WallpaperEngine/Core/Projects/CProperty.cpp
    src/WallpaperEngine/Core/Projects/CPropertyColor.h
    src/WallpaperEngine/Core/Projects/CPropertyColor.cpp
    src/WallpaperEngine/Core/Projects/CPropertyBoolean.h
    src/WallpaperEngine/Core/Projects/CPropertyBoolean.cpp
    src/WallpaperEngine/Core/Projects/CPropertySlider.h
    src/WallpaperEngine/Core/Projects/CPropertySlider.cpp
    src/WallpaperEngine/Core/Projects/CPropertyCombo.h
    src/WallpaperEngine/Core/Projects/CPropertyCombo.cpp
    src/WallpaperEngine/Core/Projects/CPropertyText.h
    src/WallpaperEngine/Core/Projects/CPropertyText.cpp

    src/WallpaperEngine/Core/Scenes/CCamera.cpp
    src/WallpaperEngine/Core/Scenes/CCamera.h
    src/WallpaperEngine/Core/Scenes/CProjection.cpp
    src/WallpaperEngine/Core/Scenes/CProjection.h

    src/WallpaperEngine/Core/Objects/CImage.cpp
    src/WallpaperEngine/Core/Objects/CImage.h
    src/WallpaperEngine/Core/Objects/CSound.cpp
    src/WallpaperEngine/Core/Objects/CSound.h
    src/WallpaperEngine/Core/Objects/CEffect.cpp
    src/WallpaperEngine/Core/Objects/CEffect.h
    src/WallpaperEngine/Core/Objects/CParticle.cpp
    src/WallpaperEngine/Core/Objects/CParticle.h

    src/WallpaperEngine/Core/Objects/Effects/CFBO.h
    src/WallpaperEngine/Core/Objects/Effects/CFBO.cpp
    src/WallpaperEngine/Core/Objects/Effects/CBind.h
    src/WallpaperEngine/Core/Objects/Effects/CBind.cpp

    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstant.h
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstant.cpp
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantFloat.h
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantFloat.cpp
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantVector3.h
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantVector3.cpp
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantVector4.h
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantVector4.cpp
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantInteger.h
    src/WallpaperEngine/Core/Objects/Effects/Constants/CShaderConstantInteger.cpp

    src/WallpaperEngine/Core/Objects/Particles/CControlPoint.cpp
    src/WallpaperEngine/Core/Objects/Particles/CControlPoint.h
    src/WallpaperEngine/Core/Objects/Particles/CEmitter.cpp
    src/WallpaperEngine/Core/Objects/Particles/CEmitter.h
    src/WallpaperEngine/Core/Objects/Particles/CInitializer.cpp
    src/WallpaperEngine/Core/Objects/Particles/CInitializer.h

    src/WallpaperEngine/Core/Objects/Particles/Initializers/CLifeTimeRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CLifeTimeRandom.h
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CSizeRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CSizeRandom.h
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CRotationRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CRotationRandom.h
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CVelocityRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CVelocityRandom.h
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CColorRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CColorRandom.h
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CAlphaRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CAlphaRandom.h
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CAngularVelocityRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CAngularVelocityRandom.h
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CTurbulentVelocityRandom.cpp
    src/WallpaperEngine/Core/Objects/Particles/Initializers/CTurbulentVelocityRandom.h

    src/WallpaperEngine/Core/Objects/Images/CMaterial.cpp
    src/WallpaperEngine/Core/Objects/Images/CMaterial.h

    src/WallpaperEngine/Core/Objects/Images/Materials/CPass.cpp
    src/WallpaperEngine/Core/Objects/Images/Materials/CPass.h

    ${WAYLAND_SOURCES}
    ${X11_SOURCES}

)

COPY_FILES(linux-wallpaperengine "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${TARGET_OUTPUT_DIRECTORY}")
COPY_FILES(linux-wallpaperengine "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${TARGET_OUTPUT_DIRECTORY}")

SET_EXECUTABLE_TARGET_PROPERTIES(linux-wallpaperengine)
add_dependencies(linux-wallpaperengine libcef_dll_wrapper)

# Need to remove libvulkan, otherwise will get error on  linking:
# /usr/bin/ld: /usr/lib/libmpv.so: undefined reference to `vkCreateXlibSurfaceKHR'
file(REMOVE "${CEF_BINARY_DIR_DEBUG}/libvulkan.so.1" "${CEF_BINARY_DIR_RELEASE}/libvulkan.so.1")
target_link_libraries (linux-wallpaperengine PUBLIC
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LZ4_LIBRARY}
    ${SDL2_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${MPV_LIBRARY}
    ${PULSEAUDIO_LIBRARY}
    glfw
    libcef_lib libcef_dll_wrapper
    Qt5::Widgets)


if (WAYLAND_SUPPORT_FOUND)
    target_link_libraries(linux-wallpaperengine PUBLIC
        pthread
        wayland-cursor
        wayland-client
        wayland-egl
        ${OPENGL_egl_LIBRARY})
endif()

if(X11_SUPPORT_FOUND)
    target_link_libraries (linux-wallpaperengine PUBLIC
            ${X11_LIBRARIES} ${X11_Xrandr_LIB})


    set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})

    # make sure some of the X11 functions we'll use are available
    check_function_exists(XSetIOErrorExitHandler HAVE_XSETIOERROREXITHANDLER)

    if(HAVE_XSETIOERROREXITHANDLER)
        add_compile_definitions(HAVE_XSETIOERROREXITHANDLER=1)
    endif()
endif()

if(NOT WAYLAND_SUPPORT_FOUND AND NOT X11_SUPPORT_FOUND)
    message(WARNING "No window server detected at build time. You will only be able to preview backgrounds")
endif()

# set some install parameters if not in debug mode
install(DIRECTORY ${TARGET_OUTPUT_DIRECTORY}/ DESTINATION .)
install(FILES ${TARGET_OUTPUT_DIRECTORY}/${PROJECT_NAME} PERMISSIONS OWNER_READ OWNER_WRITE WORLD_EXECUTE WORLD_READ GROUP_READ DESTINATION .)
install(DIRECTORY share/ DESTINATION ./share)
