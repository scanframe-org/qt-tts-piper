cmake_minimum_required(VERSION 3.27)

# FetchContent added in CMake 3.11, downloads during the configure step.
include(FetchContent)
# Import piper1-gpl library.
FetchContent_Declare(
	"piper1-gpl-main"
	URL "https://codeload.github.com/Scanframe/piper1-gpl/zip/refs/heads/main"
	DOWNLOAD_EXTRACT_TIMESTAMP true
)
# Save current library output directory.
set(__TMP__ "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/texttospeech")
# Force PIC compilation for the Piper library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_MakeAvailable("piper1-gpl-main")
# Restore library output directory.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${__TMP__}")
unset(__TMP__)
