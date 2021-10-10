cmake_minimum_required(VERSION 3.20.0)
project(CuteChart
    VERSION 0.0.1
    LANGUAGES CXX)

set(QT_DEFAULT_MAJOR_VERSION 6)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 6.1 COMPONENTS Core Widgets Charts)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Core Widgets Charts REQUIRED)
endif()

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

include_directories(SYSTEM ${QT_INCLUDES})

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-reorder -pedantic \
                                           -Wextra -Wcast-align -Wcast-qual  -Wchar-subscripts  \
                                           -Wcomment -Wdisabled-optimization \
                                           -Wformat  -Wformat=2 -Wformat-nonliteral -Wformat-security\
                                           -Wformat-y2k  -Wimport  -Winit-self  -Winline -Winvalid-pch\
                                           -Wunsafe-loop-optimizations  -Wmissing-braces\
                                           -Wmissing-field-initializers -Wmissing-format-attribute  \
                                           -Wmissing-include-dirs -Wmissing-noreturn -Wpacked  -Wparentheses\
                                           -Wpointer-arith -Wredundant-decls -Wreturn-type -Wsequence-point\
                                           -Wsign-compare  -Wstack-protector -Wstrict-aliasing\
                                           -Wstrict-aliasing=2 -Wswitch \
                                           -Wtrigraphs  -Wuninitialized -Wunknown-pragmas  -Wunreachable-code\
                                           -Wunused -Wunused-function  -Wunused-label  -Wunused-parameter\
                                           -Wunused-value  -Wunused-variable  -Wvariadic-macros\
                                           -Wvolatile-register-var  -Wwrite-strings -Wno-deprecated-declarations\
                                           -Wno-error=unused-local-typedefs -Wno-error=enum-compare -Wno-narrowing")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

set(cutechart_SRC
    src/peakcallout.cpp
    src/chartconfig.cpp
    src/chartviewprivate.cpp
    src/chartview.cpp
    src/listchart.cpp
    src/series.cpp
    )

set(example_SRC
    src/example.cpp)

add_library(cutechart ${cutechart_SRC})

set_property(TARGET cutechart PROPERTY CXX_STANDARD 17)
target_link_libraries(cutechart Qt::Core Qt::Widgets Qt::Charts)


add_executable(example ${example_SRC})
target_link_libraries(example cutechart)

