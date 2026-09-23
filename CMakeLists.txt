cmake_minimum_required(VERSION 3.20)

project(GameInferDesktop VERSION 0.2.0 LANGUAGES C CXX)

set(GAMEINFER_DESKTOP_VERSION ${PROJECT_VERSION})

option(BUILD_TESTS "Build GameInfer tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (WIN32)
    set(ONNXRUNTIME_ENABLE_DML ON CACHE BOOL "Enable ONNX Runtime DirectML" FORCE)
endif ()

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /manifest:no")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif ()

set(PROJECT_CMAKE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake)

find_package(QT NAMES Qt6 COMPONENTS Core Widgets Svg Concurrent LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Svg Concurrent LinguistTools REQUIRED)

add_custom_target(DeployedTargets)

set(AUDIO_UTIL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(AUDIO_UTIL_INSTALL OFF CACHE BOOL "" FORCE)
set(GAME_INFER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GAME_INFER_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(src/libs/audio-util)
add_subdirectory(src/libs/qsmedia)
add_subdirectory(src/libs/sdlplayback)
add_subdirectory(src/libs/game-infer)
add_subdirectory(src/apps/GameInfer)

if (BUILD_TESTS)
    add_subdirectory(src/tests)
endif ()

if (NOT DEFINED QT_QMAKE_EXECUTABLE)
    get_target_property(QT_QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
endif ()

if (NOT EXISTS "${QT_QMAKE_EXECUTABLE}" OR NOT CMAKE_INSTALL_PREFIX)
    return()
endif ()

cmake_path(GET QT_QMAKE_EXECUTABLE PARENT_PATH _qt_bin_dir)
find_program(QT_DEPLOY_EXECUTABLE NAMES windeployqt macdeployqt HINTS "${_qt_bin_dir}")

if (NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
    message(WARNING "deploy_qt_runtime: cannot locate the Qt deployment tool")
    return()
endif ()

get_target_property(_deployed_targets DeployedTargets TARGETS)

if (WIN32)
    string(REPLACE "\\" "/" _install_dir ${CMAKE_INSTALL_PREFIX})
    install(CODE "
        file(GLOB _files \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.dll\" \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.exe\")
        foreach(_file \${_files})
            file(COPY \${_file} DESTINATION \"${_install_dir}\")
        endforeach()
    ")

    foreach (_target ${_deployed_targets})
        install(CODE "
            set(ENV{PATH} \"${_qt_bin_dir};\$ENV{PATH}\")
            execute_process(
                COMMAND \"${QT_DEPLOY_EXECUTABLE}\"
                --libdir \"${_install_dir}\"
                --plugindir \"${_install_dir}/plugins\"
                --no-translations
                --no-system-d3d-compiler
                --no-compiler-runtime
                --no-opengl-sw
                --force
                --verbose 0
                \"${_install_dir}/$<TARGET_FILE_NAME:${_target}>\"
                WORKING_DIRECTORY \"${_install_dir}\"
            )
        ")
    endforeach ()
elseif (APPLE)
    foreach (_target ${_deployed_targets})
        install(CODE "file(COPY \"$<TARGET_BUNDLE_DIR:${_target}>\" DESTINATION \"${CMAKE_INSTALL_PREFIX}\")")
    endforeach ()
endif ()
