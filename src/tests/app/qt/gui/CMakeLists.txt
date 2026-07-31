# Set the target name using the current directory name and the test prefix.
get_filename_component(SF_PROJECT_SUFFIX ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TEST_APP_TARGET "${SF_TEST_NAME_PREFIX}${PROJECT_NAME}-${SF_PROJECT_SUFFIX}")

# Find the needed packages.
find_package(Qt6 COMPONENTS Gui Widgets TextToSpeech REQUIRED)

# Import target 'espeak-ng-data' for generating/compiling the required 'espeak-ng-data' directory.
find_package(SfEspeakNgData 1.52.0 CONFIG REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find the test sources.
file(GLOB TEST_APP_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test-*")

# Tests need to be added as executables first
Sf_AddExecutable("${TEST_APP_TARGET}")

target_sources("${TEST_APP_TARGET}" PRIVATE main.cpp "${TEST_APP_SOURCES}")

# Add a dependency to the target.
add_dependencies("${TEST_APP_TARGET}" espeak-ng-data)

# Sets the extension of the generated binary.
Sf_SetTargetSuffix("${TEST_APP_TARGET}")

# Should be linked to the main library.
target_link_libraries("${TEST_APP_TARGET}" PRIVATE Qt::Gui Qt::Widgets Qt::TextToSpeech)
