cmake_minimum_required(VERSION 3.29...4.4)
###
### This libray compiles content of the required `espeak-ng-data` directory.
### It uses the native compiler to build the actual compiler.
###

### Lines to add to a project using this library.
#[[

# Make the library available using the initial set version.
find_package(EspeakNg CONFIG REQUIRED)

# Make a specific version of the library available.
find_package(EspeakNg 1.52.0 CONFIG REQUIRED)

]]

# Bailout when the target is already defined.
if (TARGET "espeak-ng-data")
	# Already defined. Verify if the version is the same.
	if (SfEspeakNgData_FIND_VERSION VERSION_EQUAL SfEspeakNgData_VERSION)
		return()
	endif ()
	message(SEND_ERROR
		"Already loaded version ${SfEspeakNgData_VERSION} "
		"but version ${PACKAGE_FIND_VERSION} was requested.")
endif ()

# Include the file to have the variable `FETCHCONTENT_BASE_DIR` set so it can be used.
include(FetchContent)
# Import for the ExternalProject_Add() function.
include(ExternalProject)
# Check if required base directory for copying the compiled data in has been defined.
if (NOT DEFINED SF_ESPEAK_NG_DATA_BASE_DIR)
	# Set the default location using the by Qt named plugin subdirectory 'texttospeech' used for TTS plugins.
	set(SF_ESPEAK_NG_DATA_BASE_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/texttospeech")
	message(STATUS "Variable `SF_ESPEAK_NG_DATA_BASE_DIR` not defined using: ${SF_ESPEAK_NG_DATA_BASE_DIR}")
endif ()

# Create a target from the external project.
ExternalProject_Add(
	"espeak-ng-data"
	#[[
		GIT_REPOSITORY "https://github.com/espeak-ng/espeak-ng.git"
		GIT_TAG "${SfEspeakNgData_VERSION}"
		GIT_SHALLOW 1
	)
	]]
	# This is a much faster option to get the source then using Git.
	URL "https://github.com/espeak-ng/espeak-ng/archive/refs/tags/${SfEspeakNgData_VERSION}.tar.gz"
	DOWNLOAD_EXTRACT_TIMESTAMP true
	# Put the project also in the same directory as 'FetchContent_MakeAvailable()' uses.
	PREFIX "${FETCHCONTENT_BASE_DIR}/espeak_ng-${SfEspeakNg_VERSION}"
	# Override build to only run the 'data' target.
	# This will automatically build the necessary internal dependency targets as `espeak-ng-bin` to compile the data.
	BUILD_COMMAND "${CMAKE_COMMAND}" --build . --target data
	# Disable default installation to prevent building everything else.
	INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_directory "<BINARY_DIR>/espeak-ng-data" "${SF_ESPEAK_NG_DATA_BASE_DIR}/espeak-ng-data"
	CMAKE_ARGS
	# Unset/Clear the toolchain file to force a native build.
	"-DCMAKE_TOOLCHAIN_FILE="
	# Recommended flags for a data-only build.
	-DBUILD_SHARED_LIBS=OFF
	-DUSE_ASYNC:BOOL=OFF
	-DUSE_MBROLA:BOOL=OFF
	-DUSE_LIBSONIC:BOOL=OFF
	-DUSE_LIBPCAUDIO:BOOL=OFF
	-DUSE_KLATT:BOOL=OFF
	-DUSE_SPEECHPLAYER:BOOL=OFF
)


