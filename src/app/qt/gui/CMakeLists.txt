# Set the target name using the current directory name and the test prefix.
get_filename_component(SF_PROJECT_SUFFIX ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TEST_APP_TARGET "${PROJECT_NAME}-${SF_PROJECT_SUFFIX}")

# Find the needed packages.
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui Widgets TextToSpeech REQUIRED)

# Import target 'espeak-ng-data' for generating/compiling the required 'espeak-ng-data' directory.
find_package(SfEspeakNgData 1.52.0 CONFIG REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find the test sources.
file(GLOB TEST_APP_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test-*")

# Add shared library and also sets the executable version.
Sf_AddTarget("${TEST_APP_TARGET}" FLAGS pack gui qt pri VERSION EXIF SUFFIX EXCL_COV)

target_sources("${TEST_APP_TARGET}" PRIVATE main.cpp "${TEST_APP_SOURCES}")

# Set version from Git.
target_compile_definitions("${TEST_APP_TARGET}" PRIVATE SF_VERSION=${SF_VERSION})
# Add a dependency to the target.
add_dependencies("${TEST_APP_TARGET}" espeak-ng-data)
# Should be linked to the main library.
target_link_libraries("${TEST_APP_TARGET}" PRIVATE Qt::Gui Qt::Widgets Qt::TextToSpeech)
