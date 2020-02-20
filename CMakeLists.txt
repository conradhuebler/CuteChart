cmake_minimum_required(VERSION 3.0.2)
project(cutechart)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)


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

find_package(Qt5Widgets 5.12 CONFIG REQUIRED)
find_package(Qt5Charts 5.12 CONFIG REQUIRED)

set(cutechart_SRC
    src/peakcallout.cpp
    src/chartconfig.cpp
    src/chartviewprivate.cpp
    src/chartview.cpp
    src/listchart.cpp
    src/series.cpp
    )

add_library(cutechart ${cutechart_SRC})
set_property(TARGET cutechart PROPERTY CXX_STANDARD 14)

target_link_libraries(cutechart Qt5::Core Qt5::Widgets Qt5::Charts)
