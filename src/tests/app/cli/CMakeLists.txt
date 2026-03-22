# Set the target name using the current directory name and the test prefix.
get_filename_component(SF_PROJECT_SUFFIX ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TEST_APP_TARGET "${SF_TEST_NAME_PREFIX}${PROJECT_NAME}-${SF_PROJECT_SUFFIX}")

# Find the test sources.
file(GLOB TEST_APP_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test-*")

# Tests need to be added as executables first
Sf_AddExecutable("${TEST_APP_TARGET}")

target_sources("${TEST_APP_TARGET}" PRIVATE main.cpp "${TEST_APP_SOURCES}")

# Sets the extension of the generated binary.
Sf_SetTargetSuffix("${TEST_APP_TARGET}")

# Should be linked to the main library.
target_link_libraries("${TEST_APP_TARGET}" PRIVATE piper::static)

# Required for the location of 'libonnxruntime.so'.
set_target_properties("${TEST_APP_TARGET}" PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH};\${ORIGIN}/lib/texttospeech")


# Add this application as a test for all possible builds.
Sf_AddTest(${TEST_APP_TARGET} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..")

# Make sure this test is executed before coverage test.
Sf_AddAsCoverageTest("${TEST_APP_TARGET}")
