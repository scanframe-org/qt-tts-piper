# Set the target name using the project name as a base name and prefixed.
set(TEST_TARGET "${SF_TEST_NAME_PREFIX}${PROJECT_NAME}")

# Make Catch2::Catch2 library available.
find_package(SfCatch2 3.14.0 CONFIG REQUIRED)

# Find the needed packages.
find_package(Qt6 COMPONENTS Core Gui Widgets TextToSpeech REQUIRED)

# Import target 'espeak-ng-data' for generating/compiling the required 'espeak-ng-data' directory.
find_package(SfEspeakNgData 1.52.0 CONFIG REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(TEST_LIBRARIES Catch2::Catch2 Qt::Core Qt::TextToSpeech)

# Find the test sources.
file(GLOB TEST_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "gen/test-*")

# Tests need to be added as executables first
Sf_AddExecutable("${TEST_TARGET}")

target_sources("${TEST_TARGET}" PRIVATE main.cpp "${TEST_SOURCES}")

# Adds the compiler and linker options for coverage.
Sf_AddTargetForCoverage("${TEST_TARGET}")

# Sets the extension of the generated binary.
Sf_SetTargetSuffix("${TEST_TARGET}")

if (SF_BUILD_QT)
	# Explicitly tell the compiled code the QT libraries are included.
	target_compile_definitions("${TEST_TARGET}" PRIVATE TARGET_QT)
endif ()

# Should be linked to the main library, as well as the Catch2 testing library
target_link_libraries("${TEST_TARGET}" PRIVATE "${TEST_LIBRARIES}")

# Add this application as a test for all possible builds.
Sf_AddTest(${TEST_TARGET} "${PROJECT_NAME}")

# Make sure this test is executed before coverage test.
Sf_AddAsCoverageTest("${TEST_TARGET}")

if (SF_BUILD_QT AND SF_BUILD_GUI_TESTING)
	Sf_GetSubDirectories(_TestApps "${CMAKE_CURRENT_LIST_DIR}/app/qt" ".+")
	foreach (_TestApp ${_TestApps})
		add_subdirectory("app/qt/${_TestApp}")
	endforeach ()
endif ()
