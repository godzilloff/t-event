cmake_minimum_required(VERSION 3.5)

project(T-Event VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Sql
    SerialPort
    Network
)

set(app_icon_resource_windows "img/icon.rc")

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui

        src/forminfo.h src/forminfo.cpp src/forminfo.ui
        src/qsportevent.h src/qsportevent.cpp
        src/race.h src/race.cpp
        src/course.h src/course.cpp
        src/group.h src/group.cpp
        src/organization.h src/organization.cpp
        src/person.h src/person.cpp
        src/result.h src/result.cpp
        src/tpersonmodel.h src/tpersonmodel.cpp
        src/tcoursemodel.cpp src/tcoursemodel.h
        src/torganizationmodel.cpp src/torganizationmodel.h
        src/tgroupmodel.cpp src/tgroupmodel.h
        src/tresultmodel.cpp src/tresultmodel.h
        src/settingsdialog.cpp src/settingsdialog.h src/settingsdialog.ui
        src/postrequestsender.h src/postrequestsender.cpp
        src/formonline.h src/formonline.cpp src/formonline.ui
        src/formperson.h src/formperson.cpp src/formperson.ui
        src/recentfile.cpp
        src/formresult.h src/formresult.cpp src/formresult.ui
        src/functions.h src/functions.cpp
        src/formprepar.h src/formprepar.cpp src/formprepar.ui
        src/racesettings.h src/racesettings.cpp
        src/formfilter.h src/formfilter.cpp src/formfilter.ui
)


qt_add_executable(T-Event
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    resource.qrc
    ${app_icon_resource_windows}
)

target_link_libraries(T-Event PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::SerialPort
    Qt6::Network
)

set_target_properties(T-Event PROPERTIES
    ${BUNDLE_ID_OPTION}
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

