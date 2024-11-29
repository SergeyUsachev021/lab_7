cmake_minimum_required(VERSION 3.10)
set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})
project(lab7 VERSION ${PROJECT_VERSION})
add_executable(lab7 main.cpp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")    # Флаг для компилятора MSVC (Windows)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    else()
    # Флаг для g++ и clang++ (Linux, MacOS)    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()
install(TARGETS lab7 DESTINATION bin)
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_NAME "lab7")
set(CPACK_PACKAGE_DESCRIPTION "Описание вашего проекта")
set(CPACK_PACKAGE_VENDOR "Ваше имя или компания")
set(CPACK_PACKAGE_CONTACT "Ваш контактный email")
include(CPack)

set(BOOST_ROOT "${PROJECT_SOURCE_DIR}/boost") # Относительный путь
set(Boost_INCLUDE_DIRS "${BOOST_ROOT}/boost/spirit/include/")
set(Boost_LIBRARIES "${BOOST_ROOT}/libs") 

# Проверка, существуют ли указанные директории
if(NOT EXISTS "${Boost_INCLUDE_DIRS}")
  message(FATAL_ERROR "Boost include directory not found: ${Boost_INCLUDE_DIRS}")
endif()
if(NOT EXISTS "${Boost_LIBRARIES}")
  message(FATAL_ERROR "Boost libraries directory not found: ${Boost_LIBRARIES}")
endif()

# Находим Boost и проверяем, что он найден
find_package(Boost REQUIRED COMPONENTS filesystem program_options)

# Проверяем, что Boost найден успешно
if(Boost_FOUND)
 # Добавляем необходимые include директории
 target_include_directories(lab7 PRIVATE ${Boost_INCLUDE_DIRS})
 # Линкуем необходимые библиотеки Boost
 target_link_libraries(lab7 PRIVATE ${Boost_LIBRARIES})
else()
 message(FATAL_ERROR "Boost not found! Please install Boost and make sure it's in your system's include and library paths.")
endif()
