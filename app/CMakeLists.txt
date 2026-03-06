cmake_minimum_required(VERSION 3.20)
project(T-Event VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Sql SerialPort Network
)

# Включаем текущую директорию для заголовков
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/models)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/widgets)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/recentfile.cpp
        src/functions.h src/functions.cpp
        src/postrequestsender.h src/postrequestsender.cpp

        src/core/DatabaseManager.cpp
        src/core/DatabaseManager.h
        src/core/Document.cpp
        src/core/Document.h
        src/core/CsvImporter.cpp
        src/core/CsvImporter.h
        src/core/UndoStack.cpp
        src/core/UndoStack.h
        src/core/ResultProcessor.cpp
        src/core/ResultProcessor.h

        src/models/SqlTableModel.h
        src/models/SqlTableModel.cpp

        src/models/ResultColumns.h
        src/models/AbstractProxyModel.h
        src/models/ResultsProxyModel.h
        src/models/ResultsProxyModel.cpp

        src/dialogs/EditDialogBase.h
        src/dialogs/EditDialogBase.cpp

        src/dialogs/PersonEditDialog.h
        src/dialogs/PersonEditDialog.cpp
        #src/ui/person_edit_dialog.ui

        src/dialogs/DelegationEditDialog.cpp
        src/dialogs/DelegationEditDialog.h

        src/dialogs/DistanceEditDialog.cpp
        src/dialogs/DistanceEditDialog.h

        src/dialogs/AgeGroupEditDialog.cpp
        src/dialogs/AgeGroupEditDialog.h

        src/serial/isportident_interface.h
        src/serial/sportident_constants.h
        src/serial/sportident_cardmanager.h
        src/serial/sportident_cardmanager.cpp
        src/serial/sportident_crc.h
        src/serial/sportident_decoder.h
        src/serial/sportident_station.h
        src/serial/sportident_station.cpp
        src/serial/sportident_types.h

        #src/qsportevent.h src/qsportevent.cpp
        #src/race.h src/race.cpp
        #src/course.h src/course.cpp
        #src/group.h src/group.cpp
        #src/organization.h src/organization.cpp
        #src/person.h src/person.cpp
        #src/result.h src/result.cpp
        #src/tpersonmodel.h src/tpersonmodel.cpp
        #src/tcoursemodel.cpp src/tcoursemodel.h
        #src/torganizationmodel.cpp src/torganizationmodel.h
        #src/tgroupmodel.cpp src/tgroupmodel.h
        #src/tresultmodel.cpp src/tresultmodel.h
        #src/racesettings.h src/racesettings.cpp

        src/forminfo.h src/forminfo.cpp
        src/formonline.h src/formonline.cpp
        src/formperson.h src/formperson.cpp
        src/formresult.h src/formresult.cpp
        src/formprepar.h src/formprepar.cpp
        src/formfilter.h src/formfilter.cpp
        src/settingsdialog.cpp src/settingsdialog.h
)

# Формы Qt Designer
set(FORMS
    src/mainwindow.ui
    src/forminfo.ui
    src/formonline.ui
    src/formperson.ui
    src/formresult.ui
    src/formprepar.ui
    src/formfilter.ui
    src/settingsdialog.ui
    src/ui/person_edit_dialog.ui
    # ui/delegation_edit_dialog.ui
)

# Ресурсы
set(RESOURCES
    "img/icon.rc"
    resource.qrc
)

qt_add_executable(T-Event
    ${PROJECT_SOURCES}
    ${FORMS}
    ${RESOURCES}
)

target_link_libraries(T-Event PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::SerialPort
    Qt6::Network
)

#target_include_directories(T-Event PUBLIC
#    ${Qt6Sql_INCLUDE_DIRS}
#)

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

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# Включаем предупреждения в зависимости от компилятора
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(T-Event PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message("MSVC - ON")
    set(CMAKE_MSVC_WARNING_LEVEL 4)
    target_compile_options(T-Event PRIVATE
        /permissive- # Более строгий режим (C++17+)
        /WX        # Предупреждения = ошибки
    )
endif()

message(STATUS "Qt6 SQL include dir: ${Qt6Sql_INCLUDE_DIRS}")
message(STATUS "Qt6 SQL libraries: ${Qt6Sql_LIBRARIES}")
# Проверка что компоненты найдены
if(TARGET Qt6::Sql)
    message(STATUS "Qt6::Sql target FOUND")
    get_target_property(sql_lib Qt6::Sql LOCATION)
    message(STATUS "Qt6::Sql library: ${sql_lib}")
else()
    message(FATAL_ERROR "Qt6::Sql target NOT FOUND!")
endif()
