cmake_minimum_required(VERSION 3.19)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "")
endif()

project(WeatherForecastApp LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)
find_package(CURL REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(WeatherForecastApp
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    Models/WeatherData.h
    Models/ForecastData.h
    Network/WeatherApiClient.h
    Network/WeatherApiClient.cpp
    Parser/WeatherParser.h
    Parser/WeatherParser.cpp
    Services/WeatherService.h
    Services/WeatherService.cpp
    Models/HourlyData.h
    Parser/WeatherCodeMapper.h
    Parser/WeatherCodeMapper.cpp
    Models/CitySuggestion.h
)

target_link_libraries(WeatherForecastApp PRIVATE Qt6::Widgets Qt6::Charts CURL::libcurl)
target_include_directories(WeatherForecastApp PRIVATE ${RAPIDJSON_INCLUDE_DIRS})

install(TARGETS WeatherForecastApp
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET WeatherForecastApp
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
