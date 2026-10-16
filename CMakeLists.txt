cmake_minimum_required(VERSION 3.25)

# MUST be set before project() so the vcpkg toolchain sees it early
#set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "" FORCE)

# set build directories and options
if(WIN32)
    # place the vcpkg_installed folder outside of the build directory
    set(VCPKG_INSTALLED_DIR "$ENV{USERPROFILE}/vcpkg_installed_cache/WeeklyGiving" CACHE PATH "vcpkg install dir")
    # use C:\b for the buildtrees to avoid long path errors with qtdeclarative
    set(VCPKG_INSTALL_OPTIONS "--x-buildtrees-root=C:/b")
    # set the executable directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "$ENV{USERPROFILE}/CDist/WeeklyGiving/Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "$ENV{USERPROFILE}/CDist/WeeklyGiving/Release")
endif()
if(UNIX AND NOT APPLE)
    # place the vcpkg_installed folder outside of the build directory
    set(VCPKG_INSTALLED_DIR "$ENV{HOME}/.cache/vcpkg_installed/WeeklyGiving" CACHE PATH "vcpkg install dir")
    # set the executable directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "$ENV{HOME}/CDist/WeeklyGiving/Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "$ENV{HOME}/CDist/WeeklyGiving/Release")
endif()

project(WeeklyGiving)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ============================================
#            WINDOWS BUILD OPTIONS
# ============================================

if(WIN32)
    # Protect against Windows min/max macros breaking std::min / std::max
    add_compile_definitions(NOMINMAX)

    # add the app resources file
    list(APPEND APP_SOURCES app_icon.rc)

    find_package(Qt6 REQUIRED COMPONENTS
            Svg
            Widgets
            PrintSupport
            Core
            Network
            Sql
    )

    # Find sqlite3 from vcpkg
    find_package(unofficial-sqlite3 REQUIRED)

    add_executable(WeeklyGiving WIN32
            main.cpp
            gui.cpp
            gui.h
            autoSelectWidgets.cpp
            autoSelectWidgets.h
            db.cpp
            db.h
            printDialog.cpp
            printDialog.h
            resources.qrc
            app_icon.rc
    )

    target_link_libraries(WeeklyGiving PRIVATE
            Qt6::Svg
            Qt6::Widgets
            Qt6::PrintSupport
            Qt6::Core
            Qt6::Network
            Qt6::Sql
            unofficial::sqlite3::sqlite3
    )

    # Locate windeployqt automatically from Qt6 installation
    find_package(Qt6 COMPONENTS Core REQUIRED)
    get_target_property(_qt_qmake_location Qt6::qmake LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake_location}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # prevent vcpkg from copying DLLs automatically and copy them through a command (prevents icu*.dll file conflicts
    # with windeployqt).

    # Run windeployqt to deploy ALL Qt DLLs, plugins, styles, and dependencies automatically
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET WeeklyGiving POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-compiler-runtime
                --verbose 1
                "$<TARGET_FILE:WeeklyGiving>"

                COMMENT "Running windeployqt to stage Qt runtime environment...\\n"
        )
    endif()

    # add the other necessary dlls
    add_custom_command(TARGET WeeklyGiving POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:WeeklyGiving>
            $<TARGET_FILE_DIR:WeeklyGiving>
            COMMAND_EXPAND_LISTS
    )

    # Add resources directory
    add_custom_command(TARGET WeeklyGiving POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:WeeklyGiving>/resources"

            COMMENT "Staging resources directory..."
    )

endif()

# ============================================
#            LINUX BUILD OPTIONS
# ============================================

if(UNIX AND NOT APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/home/jeremy/.vcpkg-clion/vcpkg/installed/x64-linux")
    find_package(unofficial-mariadb-connector-cpp CONFIG REQUIRED)
    find_package(BZip2 REQUIRED)
    find_package(absl CONFIG REQUIRED)
    find_package(EXPAT REQUIRED)
    find_package(Fontconfig REQUIRED)
    find_package(OpenJPEG CONFIG REQUIRED)
    find_package(Qt6 REQUIRED COMPONENTS
            Svg
            Widgets
            PrintSupport
            Core
            Network
            Sql
    )
    add_executable(WeeklyGiving
            main.cpp
            gui.cpp
            gui.h
            autoSelectWidgets.cpp
            autoSelectWidgets.h
            resources.qrc
            db.cpp
            db.h
            printDialog.cpp
            printDialog.h
    )

    # static build optimization for openjp2
    target_compile_definitions(WeeklyGiving PRIVATE
            OPJ_STATIC
            XML_STATIC
    )

    target_link_libraries(WeeklyGiving PRIVATE
            Qt6::Svg
            Qt6::Widgets
            Qt6::PrintSupport
            Qt6::Core
            Qt6::Network
            Qt6::Sql
            unofficial::mariadb-connector-cpp::mariadbcpp
            absl::flat_hash_set
            absl::hash
            absl::raw_hash_set
            absl::base
            absl::strings

            # Force every symbol from these static libs into the link
            # Use --whole-archive to prevent openjp2/xml linking error
            -Wl,--whole-archive
            Fontconfig::Fontconfig
            EXPAT::EXPAT
            openjp2
            -Wl,--no-whole-archive
    )

    target_include_directories(WeeklyGiving PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include/yams-symspell-main/include
    )

    # Build for distribution:
    find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)

    if(LINUXDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET WeeklyGiving POST_BUILD
                COMMAND "${LINUXDEPLOYQT_EXECUTABLE}"
                "$<TARGET_FILE:WeeklyGiving>"
                -unsupported-allow-new-glibc
                COMMENT "Running linuxdeployqt to bundle Qt dependencies..."
        )

        if(MARIADB_SO_PATH)
            add_custom_command(TARGET WeeklyGiving POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MARIADB_SO_PATH}"
                    "$<TARGET_FILE_DIR:WeeklyGiving>"
                    COMMENT "Copying MariaDB shared library..."
            )
        endif()
    endif()

    add_custom_command(TARGET WeeklyGiving POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:WeeklyGiving>/resources"
            COMMENT "Staging resources directory..."
    )
endif()