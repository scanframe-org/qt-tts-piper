set(_TargetName "sf-tts-piper")
# Set the project name.
project("${_TargetName}" LANGUAGES CXX)

# Add the directory to find this source packages.
Sf_AppendCmakePrefixPath("../../cmake")

# Project for creating the piper library from source.
find_package(SfPiperLib REQUIRED)

# Find the needed packages and set cache variable 'QT_DIR'.
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Multimedia TextToSpeech REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Add shared library and also sets the dynamic library version.
Sf_AddTarget("${_TargetName}" SHARED FLAGS doc qt VERSION EXIF SUFFIX COVERAGE)

# Set the correct plugin directory.
get_target_property(_LibOutDir "${_TargetName}" LIBRARY_OUTPUT_DIRECTORY)
set_target_properties("${_TargetName}" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${_LibOutDir}/texttospeech")
if (WIN32)
	# Needed for WIN32 build to copy the plugin to the correct directory.
	set_target_properties("${_TargetName}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${_LibOutDir}/texttospeech")
endif()

target_sources(${_TargetName} PRIVATE
	TextToSpeechSynthesizer.cpp TextToSpeechSynthesizer.h
	TextToSpeechEngine.cpp TextToSpeechEngine.h
	TextToSpeechPlugin.cpp TextToSpeechPlugin.h
	AudioStream.cpp AudioStream.h
	helpers.cpp helpers.h
	logging.cpp logging.h
	plugin.json
)

# Link the needed interface libraries.
target_link_libraries(${_TargetName} PRIVATE
	Qt6::Core
	Qt::Multimedia
	Qt6::TextToSpeech
	piper::static
)

# Explicitly tell the compiled code the QT libraries are included.
target_compile_definitions(${_TargetName} PRIVATE
	# Tell the compiled code a dynamic library (DL) is being build.
	TARGET_DYNAMIC_LIB
	# The 'DEBUG_LEVEL' controls the debug output macro's.
	_DEBUG_LEVEL=1
	# When enabled the 'target.h' file enables reporting of the current target being build using 'pragma message'.
	#REPORT_TARGET
)

