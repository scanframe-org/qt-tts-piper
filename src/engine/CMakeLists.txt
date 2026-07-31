# Required first entry checking the cmake version.
cmake_minimum_required(VERSION 3.18)
# Set the project name.
project("sf-tts-piper" LANGUAGES CXX)

# Add the directory to find this source packages.
Sf_AppendCmakePrefixPath("../../cmake")

# Project for creating the piper library from source.
find_package(SfPiperLib REQUIRED)

# Find the needed packages.
find_package(Qt6 COMPONENTS Widgets Multimedia TextToSpeech REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Add shared library and also sets the dynamic library version.
Sf_AddSharedLibrary("${PROJECT_NAME}")

# Set the correct plugin directory.
if (WIN32)
	get_target_property(_LibOutDir "${PROJECT_NAME}" RUNTIME_OUTPUT_DIRECTORY)
else ()
	get_target_property(_LibOutDir "${PROJECT_NAME}" LIBRARY_OUTPUT_DIRECTORY)
endif ()
set(_LibOutDir "${_LibOutDir}/texttospeech")
if (WIN32)
	set_target_properties("${PROJECT_NAME}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${_LibOutDir}")
else ()
	set_target_properties("${PROJECT_NAME}" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${_LibOutDir}")
endif ()

# Add custom target to report resource stored versions.
Sf_AddExifTarget("${PROJECT_NAME}")

target_sources(${PROJECT_NAME} PRIVATE
	TextToSpeechSynthesizer.cpp TextToSpeechSynthesizer.h
	TextToSpeechEngine.cpp TextToSpeechEngine.h
	TextToSpeechPlugin.cpp TextToSpeechPlugin.h
	AudioStream.cpp AudioStream.h
	helpers.cpp helpers.h
	logging.cpp logging.h
	plugin.json
)

# Adds the compiler and linker options for coverage.
Sf_AddTargetForCoverage("${PROJECT_NAME}")

# Link the needed interface libraries.
target_link_libraries(${PROJECT_NAME} PRIVATE
	Qt6::Core
	Qt::Multimedia
	Qt6::TextToSpeech
	piper::static
)

# Explicitly tell the compiled code the QT libraries are included.
target_compile_definitions(${PROJECT_NAME} PRIVATE
	# Tell the compiled code a dynamic library (DL) is being build.
	TARGET_DYNAMIC_LIB
	# The 'DEBUG_LEVEL' controls the debug output macro's.
	_DEBUG_LEVEL=1
	# When enabled the 'target.h' file enables reporting of the current target being build using 'pragma message'.
	#REPORT_TARGET
)
