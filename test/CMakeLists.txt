cmake_minimum_required(VERSION 3.5)

project(test LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Test)
find_package(Qt6 REQUIRED COMPONENTS Test)
find_package(Qt6 REQUIRED COMPONENTS SerialPort)
find_package(Qt6 REQUIRED COMPONENTS Network)
find_package(Qt6 COMPONENTS Gui REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(qtests tst_test.cpp
    ../app/src/course.cpp ../app/src/course.h
    ../app/src/forminfo.cpp ../app/src/forminfo.h ../app/src/forminfo.ui
    ../app/src/formonline.cpp ../app/src/formonline.h ../app/src/formonline.ui
    ../app/src/formperson.cpp ../app/src/formperson.h ../app/src/formperson.ui
    ../app/src/formprepar.cpp ../app/src/formprepar.h ../app/src/formprepar.ui
    ../app/src/formresult.cpp ../app/src/formresult.h ../app/src/formresult.ui
    ../app/src/functions.cpp ../app/src/functions.h
    ../app/src/group.cpp ../app/src/group.h
    ../app/src/mainwindow.cpp ../app/src/mainwindow.h ../app/src/mainwindow.ui
    ../app/src/organization.cpp ../app/src/organization.h
    ../app/src/person.cpp ../app/src/person.h
    ../app/src/postrequestsender.cpp ../app/src/postrequestsender.h
    ../app/src/qsportevent.cpp ../app/src/qsportevent.h
    ../app/src/race.cpp ../app/src/race.h
    ../app/src/racesettings.cpp ../app/src/racesettings.h
    ../app/src/recentfile.cpp
    ../app/src/result.cpp ../app/src/result.h
    ../app/src/settingsdialog.cpp ../app/src/settingsdialog.h ../app/src/settingsdialog.ui
    ../app/src/tcoursemodel.cpp ../app/src/tcoursemodel.h
    ../app/src/tgroupmodel.cpp ../app/src/tgroupmodel.h
    ../app/src/torganizationmodel.cpp ../app/src/torganizationmodel.h
    ../app/src/tpersonmodel.cpp ../app/src/tpersonmodel.h
    ../app/src/tresultmodel.cpp ../app/src/tresultmodel.h
)
add_test(NAME test COMMAND test)

include_directories(../app/src)

target_link_libraries(qtests PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Network
)
