cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 17)
# Required for static libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Force static linkage for all dependencies
set(BUILD_SHARED_LIBS OFF)

# Prevent a CMake error.
cmake_policy(PUSH)
cmake_policy(SET CMP0169 OLD)

include(FetchContent)

set(_SpdLogVer "1.12.0")
FetchContent_Declare(
	"spdlog-${_SpdLogVer}"
	GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
	GIT_SHALLOW 1
	GIT_TAG "v${_SpdLogVer}"
)
FetchContent_MakeAvailable("spdlog-${_SpdLogVer}")

set(_PiperVer "2023.11.14-2")

set(_PiperPhonemizeSrc "piper-phonemize-${_PiperVer}")
FetchContent_Declare(
	"${_PiperPhonemizeSrc}"
	GIT_REPOSITORY "https://github.com/rhasspy/piper-phonemize.git"
	GIT_SHALLOW 1
	# Matching the Piper version
	GIT_TAG "${_PiperVer}"
)
# Prevent an warning message.
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
FetchContent_MakeAvailable("${_PiperPhonemizeSrc}")
unset(CMAKE_POLICY_DEFAULT_CMP0135)


# Fix the non-existing 'src/piper-phonemize' directory referenced by the 'piper.hpp' file.
if (NOT EXISTS "${${_PiperPhonemizeSrc}_SOURCE_DIR}/src/piper-phonemize")
	file(GLOB HPP_FILES "${${_PiperPhonemizeSrc}_SOURCE_DIR}/src/*.hpp")
	file(MAKE_DIRECTORY "${${_PiperPhonemizeSrc}_SOURCE_DIR}/src/piper-phonemize")
	file(COPY ${HPP_FILES} DESTINATION "${${_PiperPhonemizeSrc}_SOURCE_DIR}/src/piper-phonemize")
endif ()

# Fix a header file missing a define.
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	set(_SrcDir "${${_PiperPhonemizeSrc}_SOURCE_DIR}")
	# Fixing header 'lib/onnxruntime-win-x64-1.14.1/include/onnxruntime_c_api.h' for Windows.
	file(GLOB_RECURSE _Headers "${_SrcDir}/onnxruntime_c_api.h")
	foreach (_Header IN LISTS _Headers)
		message(VERBOSE "Fixing header: ${_Header}")
		# Check if an original file exists which is used as a sentry.
		if (NOT EXISTS "${_Header}-original")
			# Read the file content into a variable
			file(READ "${_Header}" _FileContent)
			# Rename the file to original.
			file(RENAME "${_Header}" "${_Header}-original")
			# Define the line to find and the line to insert
			set(FIND_LINE "#include <specstrings.h>")
			# Use string(REPLACE) to insert the new line after the found line
			# We include the original line in the replacement to keep it
			string(REPLACE "${FIND_LINE}" "${FIND_LINE}\n#define _Frees_ptr_opt_" _FileContent "${_FileContent}")
			# Write the updated content back to the file
			file(WRITE "${_Header}" "${_FileContent}")
		endif ()
	endforeach ()
	# Add include file for MinGW compiler.
	set(_File "${_SrcDir}/src/main.cpp")
	# Check if an original file exists which is used as a sentry.
	if (NOT EXISTS "${_File}-original" AND EXISTS "${_File}")
		# Read the file content into a variable
		file(READ "${_File}" _FileContent)
		# Rename the file to original.
		file(RENAME "${_File}" "${_File}-original")
		# Define the line to find and the line to insert
		set(FIND_LINE "#ifdef _MSC_VER")
		# Use string(REPLACE) to insert the new line after the found line
		# We include the original line in the replacement to keep it
		string(REPLACE "${FIND_LINE}" "#ifdef __MINGW32__\n#include <windows.h>\n#endif\n\n${FIND_LINE}" _FileContent "${_FileContent}")
		# Write the updated content back to the file
		file(WRITE "${_File}" "${_FileContent}")
	endif ()

#[[
	#ifdef __MINGW32__
	#include <windows.h>
	#endif
]]

endif ()

#
# Dynamic libraries un Windows need to be in the executable directory when not in the path and are created as symlinks.
#
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	# Get the assembled espeak source directory.
	set(_EspeakDir "${${_PiperPhonemizeSrc}_BINARY_DIR}/e/src/espeak_ng_external-build")
	message(VERBOSE "Symbolic linking DLL's: ${_EspeakDir}/src")
	# Files to symlink which do not exists yet.
	set(_Files "libespeak-ng/libespeak-ng.dll;ucd-tools/libucd.dll")
	file(MAKE_DIRECTORY "${_EspeakDir}/src")
	foreach (_File IN LISTS _Files)
		get_filename_component(_SubDir "${_File}" DIRECTORY)
		get_filename_component(_Link "${_File}" NAME)
		if (EXISTS "${_EspeakDir}/src/${_File}")
			message(VERBOSE "Copying file: '${_Link}' > '${_File}'")
			file(COPY_FILE "${_EspeakDir}/src/${_File}" "${_EspeakDir}/src/${_Link}")
		endif ()
#[[
		# Symlinked DDL's in the executable directory are considered not to be private DLL's somehow.
		if (NOT IS_SYMLINK "${_EspeakDir}/src/${_Link}")
			file(MAKE_DIRECTORY "${_EspeakDir}/src/${_SubDir}")
			message(VERBOSE "Creating Symlink: '${_Link}' > '${_File}'")
			#file(TOUCH "${_EspeakDir}/src/${_File}")
			file(CREATE_LINK "${_File}" "${_EspeakDir}/src/${_Link}" SYMBOLIC)
		endif ()
]]
	endforeach ()
endif ()

set(_PiperSrc "piper-src-${_PiperVer}")
FetchContent_Declare(
	"${_PiperSrc}"
	GIT_REPOSITORY "https://github.com/rhasspy/piper.git"
	GIT_SHALLOW 1
	GIT_TAG "${_PiperVer}"
)
FetchContent_Populate("${_PiperSrc}")

# Turn policies changes back.
cmake_policy(POP)

file(GLOB PIPER_SOURCES "${${_PiperSrc}_SOURCE_DIR}/src/cpp/*.cpp")
list(REMOVE_ITEM PIPER_SOURCES "${${_PiperSrc}_SOURCE_DIR}/src/cpp/main.cpp")

# Define the Static Library: libpiper.a
add_library(piper STATIC ${PIPER_SOURCES})
target_include_directories(piper PUBLIC "${${_PiperSrc}_SOURCE_DIR}/src/cpp")

# Link all dependencies.
target_link_libraries(piper
	PRIVATE
	spdlog::spdlog
	PUBLIC
	piper_phonemize
)

# Create an ALIAS target for consumer projects (piper::piper)
add_library(piper::piper ALIAS piper)

if (PIPER_CLI_ENABLED)
	# Build the Executable (piper_cli).
	# The executable source file is src/cpp/main.cpp.
	add_executable(piper_cli "${${_PiperSrc}_SOURCE_DIR}/src/cpp/main.cpp")
	# Link the executable against the static library we just built
	target_link_libraries(piper_cli PRIVATE piper::piper spdlog::spdlog)
endif ()