# Required first entry checking the cmake version.
cmake_minimum_required(VERSION 3.29...4.4)

# Make it so our own packages are found and also the ones in the sub-module library.
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" "${CMAKE_CURRENT_LIST_DIR}/cmake/lib")

# Package needed for Sf_GetGitTagVersion().
find_package(SfBase CONFIG REQUIRED)
# Package setting the correct tool chain according the host and SF_COMPILER.
find_package(SfToolChain CONFIG REQUIRED)

# Get the Git versions from the repository of this files directory.
Sf_GetGitTagVersion(_Versions "${CMAKE_CURRENT_LIST_DIR}")

# Report the found Git tag found version.
Sf_ReportGitTagVersion("${_Versions}")
# Split the list into separate values.
list(GET _Versions 0 SF_GIT_TAG_VERSION)
list(GET _Versions 1 SF_GIT_TAG_RC)
list(GET _Versions 2 SF_GIT_TAG_COMMITS)
# Convert the versions list to a string.
Sf_VersionToString(SF_VERSION "${_Versions}")


set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

# Set the global project name.
project("qt-tts-piper"
	VERSION "${SF_GIT_TAG_VERSION}"
	DESCRIPTION "Piper1GPL Text to Speech Qt6 Plugin"
	HOMEPAGE_URL "https://git.scanframe.com/shared/qt-tts-piper"
	LANGUAGES C CXX
)

# Check if the cmake is available and include it to maybe overrule CMakePresets.json cache variables.
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/user.cmake")
	include("${CMAKE_CURRENT_LIST_DIR}/user.cmake")
endif ()

# Add top target for displaying info on the compiled target where Sf_AddExifTarget() is called on.
add_custom_target("exif" ALL)

# Use faster linker for Windows maybe?
if (WIN32)
	# Adding option '-mwindows' as a linker flag will remove the console.
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=bfd")
	# Prevent error when configuring for cross compile for Windows in Linux.
	if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
		# Make some cmake files happy so they do report "Not found".
		set(Vulkan_INCLUDE_DIR "/tmp")
	endif ()
	# Needed to be able to set debug breaks using MSVC
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
	# When using the release runtime library, the QT_NO_DEBUG must be defined
	# to prevent the "Cannot mix debug and release libraries" error when loading plugins.
	if (CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL$|Static$")
		add_compile_definitions(QT_NO_DEBUG)
	endif ()
endif ()

# Make sure builds do not wind up in the source directory.
find_package(SfMacros CONFIG REQUIRED)
# Make sure builds do not wind up in the source directory.
find_package(SfBuildCheck CONFIG REQUIRED)
if (SF_BUILD_QT)
	# Install/fetch the Qt Libraries when ENV{QT_VER_DIR} is not set.
	find_package(SfQtLibrary 6.10.1 CONFIG REQUIRED)
endif ()

# Set the C++ standard to 17 for all projects which is required for the SfCompiler package.
set(CMAKE_CXX_STANDARD 17)

if (SF_BUILD_TESTING)
	# Prevents Catch2 from adding targets.
	set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
	# Enable the tests added with add_test.
	enable_testing()
	# Include CDash dashboard testing module and it sets the BUILD_TESTING to 'ON'.
	include(CTest)
endif ()

# Satisfy cmake to prevent warning.
if (CMAKE_VERBOSE_MAKEFILE)
	message(STATUS "Verbosity enabled.")
endif ()

# Add Sub Projects in the right order of dependencies.
add_subdirectory(src)

if (SF_BUILD_TESTING)
	sfDownloadPiperModels(DEFAULT)
endif ()

# Windows 11 need onnxruntime library next to the executable since itself has an older version present.
if (WIN32)
	file(GLOB _Files "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/onnxruntime.*")
else ()
	file(GLOB _Files "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/texttospeech/libonnxruntime.*")
endif ()
list(LENGTH _Files _FilesLength)
if (_FilesLength EQUAL 0)
	message(SEND_ERROR "Need onnxruntime library to be present in the build directory.")
else ()
	if (WIN32)
		install(FILES ${_Files} DESTINATION "." COMPONENT "${SF_DEFAULT_COMPONENT_NAME}")
	else ()
		install(FILES ${_Files} DESTINATION "lib/texttospeech/" COMPONENT "${SF_DEFAULT_COMPONENT_NAME}")
	endif ()
endif ()
# Get the real path of the models directory since it is a symlink (development only to save space).
get_filename_component(_ModelsDir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/texttospeech/models/" REALPATH)
# Install the models files.
install(DIRECTORY "${_ModelsDir}" DESTINATION "lib/texttospeech" COMPONENT "${SF_DEFAULT_COMPONENT_NAME}")
# Install the espeak-ng data files.
install(DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/texttospeech/espeak-ng-data"
	DESTINATION "lib/texttospeech" COMPONENT "${SF_DEFAULT_COMPONENT_NAME}")
# Install the TTS plugin.
install(TARGETS "sf-tts-piper"
	# Windows uses the runtime destination to install the plugin.
	# Linux uses the library destination to install the plugin.
	# The plugin is installed in the 'lib/texttospeech' directory.
	RUNTIME DESTINATION "lib/texttospeech" COMPONENT "${SF_DEFAULT_COMPONENT_NAME}"
	LIBRARY DESTINATION "lib/texttospeech" COMPONENT "${SF_DEFAULT_COMPONENT_NAME}"
)

# Add Doxygen document project.
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/doc")
	add_subdirectory(doc)
endif ()

# Set the 'RUNPATH' for all targets and report each target's values.
sf_SetRunPath(REPORT)

# Coverage report generator in the form af a test is added.
# Only when testing is enabled and the build type is 'Coverage'.
# This must be the last test added since it relies on previous the calls
# to 'Sf_AddAsCoverageTest()'.
Sf_AddTestCoverageReport("coverage-report" "${CMAKE_CURRENT_LIST_DIR}/bin/gcov" "--html flat --json --cleanup --verbose" "src")

# Add package build config when not building coverage.
if (NOT CMAKE_BUILD_TYPE STREQUAL "Coverage")
	if (EXISTS "${SF_CPACK_PREPARE_FILE}")
		include("${SF_CPACK_PREPARE_FILE}")
	endif ()
endif ()
