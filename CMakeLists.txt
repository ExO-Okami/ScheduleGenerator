cmake_minimum_required(VERSION 3.16)

project(ScheduleGenerator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(PROJECT_SOURCES
    main.cpp

    mainwindow.h
    mainwindow.cpp
    mainwindow.ui

    models.h

    datamanager.h
    datamanager.cpp

    scheduler.h
    scheduler.cpp

    scheduleutils.h

    datatab.h
    datatab.cpp

    generationtab.h
    generationtab.cpp

    resulttab.h
    resulttab.cpp

    heatmapwidget.h
    heatmapwidget.cpp

    barchartwidget.h
    barchartwidget.cpp

    teacheravailabilitydialog.h
    teacheravailabilitydialog.cpp

    subjectdialog.h
    subjectdialog.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ScheduleGenerator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(ScheduleGenerator
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(ScheduleGenerator PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_finalize_executable(ScheduleGenerator)
endif()
