# Set the target name using the project name as a base name and prefixed.
set(TEST_TARGET "${PROJECT_NAME}-test")

# Make Catch2::Catch2 library available.
find_package(SfCatch2 3.14.0 CONFIG REQUIRED)

# Find the needed packages.
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui Widgets TextToSpeech REQUIRED)
# Import target 'espeak-ng-data' for generating/compiling the required 'espeak-ng-data' directory.
find_package(SfEspeakNgData 1.52.0 CONFIG REQUIRED)
# Enable QT flags for this project.
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# Adds this executable as a test and a coverage information generator using target flags (test, cov).
# The 'COVERAGE' option is only when inlined function coverage is required.
Sf_AddTarget("${TEST_TARGET}" FLAGS test cov VERSION EXIF SUFFIX COVERAGE)
# Find the test sources.
file(GLOB TEST_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "gen/test-*")
target_sources("${TEST_TARGET}" PRIVATE main.cpp "${TEST_SOURCES}")
# Explicitly tell the compiled code the QT libraries are included.
target_compile_definitions("${TEST_TARGET}" PRIVATE TARGET_QT)
# Should be linked to the main library, as well as the Catch2 testing library
target_link_libraries("${TEST_TARGET}" PRIVATE Catch2::Catch2 Qt::Core Qt::TextToSpeech)
# Add the as a test application under the label 'Catch2'.
Sf_AddTest("${TEST_TARGET}" "Catch2")
# Get a DLL fix for Wine11.
Sf_Wine11DllFixForQt("${TEST_TARGET}" "${QT_VERSION}")

add_subdirectory("app/qt/cli")
if (SF_BUILD_GUI_TESTING)
	add_subdirectory("app/qt/gui")
endif ()
