cmake_minimum_required(VERSION 3.5)

project(T-Event VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS SerialPort)
find_package(Qt6 REQUIRED COMPONENTS Network)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/img/icon.rc")


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(T-Event
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

        forminfo.h forminfo.cpp forminfo.ui
        resource.qrc
        qsportevent.h qsportevent.cpp
        race.h race.cpp
        course.h course.cpp
        group.h group.cpp
        organization.h organization.cpp
        person.h person.cpp
        result.h result.cpp
        tpersonmodel.h tpersonmodel.cpp
        tcoursemodel.cpp tcoursemodel.h
        torganizationmodel.cpp torganizationmodel.h
        tgroupmodel.cpp tgroupmodel.h
        tresultmodel.cpp tresultmodel.h

        ${app_icon_resource_windows}
        settingsdialog.cpp settingsdialog.h settingsdialog.ui
        postrequestsender.h postrequestsender.cpp
        formonline.h formonline.cpp formonline.ui

        formperson.h formperson.cpp formperson.ui
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET T-Event APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(T-Event SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(T-Event
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(T-Event PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::SerialPort)
target_link_libraries(T-Event PRIVATE Qt6::Network)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.T-Event)
endif()
set_target_properties(T-Event PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS T-Event
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(T-Event)
endif()

