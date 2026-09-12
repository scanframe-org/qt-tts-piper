cmake_minimum_required(VERSION 3.29...4.4)

function(Sf_FetchContent_MakeAvailable)
	cmake_parse_arguments(PARSE_ARGV 0 _ARG "NO_INSTALL" "" "")

	if (_ARG_NO_INSTALL)
		set(CMAKE_SKIP_INSTALL_RULES TRUE)
	endif ()

	FetchContent_MakeAvailable(${_ARG_UNPARSED_ARGUMENTS})
endfunction()

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

# FetchContent added in CMake 3.11, downloads during the configure step.
include(FetchContent)
# Import piper1-gpl library.
FetchContent_Declare(
	"piper-v1.6.0"
	#URL "https://codeload.github.com/Scanframe/piper1-gpl/zip/refs/heads/main"
	URL https://github.com/Scanframe/piper1-gpl/archive/refs/tags/1.6.0-rc.1.zip
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	EXCLUDE_FROM_ALL
)
# Save current library output directory.
set(__TMP__ "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/texttospeech")

# Force PIC compilation for the Piper library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

Sf_FetchContent_MakeAvailable("piper-v1.6.0" NO_INSTALL)
#[[
# Piper is linked by this project and must not contribute its own install rules.
# Otherwise FetchContent adds Piper's generated cmake_install.cmake files to the
# top-level install graph used by CPack.
if (DEFINED CMAKE_SKIP_INSTALL_RULES)
	set(__TMP_SKIP_INSTALL_RULES "${CMAKE_SKIP_INSTALL_RULES}")
	set(__TMP_SKIP_INSTALL_RULES_UNDEFINED FALSE)
else ()
	set(__TMP_SKIP_INSTALL_RULES_UNDEFINED TRUE)
endif ()
set(CMAKE_SKIP_INSTALL_RULES TRUE)
FetchContent_MakeAvailable("piper-v1.6.0")
if (__TMP_SKIP_INSTALL_RULES_UNDEFINED)
	unset(CMAKE_SKIP_INSTALL_RULES)
else ()
	set(CMAKE_SKIP_INSTALL_RULES "${__TMP_SKIP_INSTALL_RULES}")
endif ()
unset(__TMP_SKIP_INSTALL_RULES)
unset(__TMP_SKIP_INSTALL_RULES_UNDEFINED)
]]

#[[
if (TARGET "piper")
	# Set the target SF flag to pack it. (Should not be needed due to dependency checking when packing.)
	set_target_properties("piper" PROPERTIES SF_FLAGS "pack")
endif ()
]]

# Location of the piper models.
set(SF_PIPER_MODELS_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/models" CACHE STRING INTERNAL)
# Restore library output directory.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${__TMP__}")
unset(__TMP__)

# Downloads Piper voice model files using models.py.
#
# Downloads the .onnx and .onnx.json files for one voice into MODELS_DIR
# (defaults to SF_PIPER_MODELS_DIRECTORY).  Already-present files are skipped.
#
# Signatures:
#   sfDownloadPiperModels(DEFAULT [MODELS_DIR <dir>])
#   sfDownloadPiperModels(
#       LANGUAGE <lang>    # e.g. "en"
#       REGION   <region>  # e.g. "US"
#       DATASET  <dataset> # e.g. "hfc_female"
#       QUALITY  <quality> # e.g. "medium"
#       [MODELS_DIR <dir>]
#   )
function(sfDownloadPiperModels)
	cmake_parse_arguments(_ARG "DEFAULT" "LANGUAGE;REGION;DATASET;QUALITY;MODELS_DIR" "" ${ARGN})
	find_package(Python3 REQUIRED COMPONENTS Interpreter)
	# models.py lives one level above the cmake/ directory (i.e. the project root).
	set(_script "${CMAKE_CURRENT_LIST_DIR}/models.py")
	if (NOT EXISTS "${_script}")
		message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}: models.py not found at '${_script}'.")
	endif ()
	if (NOT _ARG_MODELS_DIR)
		set(_ARG_MODELS_DIR "${SF_PIPER_MODELS_DIRECTORY}")
	endif ()
	set(_cmd "${Python3_EXECUTABLE}" "${_script}" --models-directory "${_ARG_MODELS_DIR}")

	if (_ARG_DEFAULT)
		list(APPEND _cmd --default)
	else ()
		foreach (_required LANGUAGE REGION DATASET QUALITY)
			if (NOT _ARG_${_required})
				message(FATAL_ERROR
					"${CMAKE_CURRENT_FUNCTION}: ${_required} is required "
					"(or use DEFAULT to download the built-in voice list).")
			endif ()
		endforeach ()
		list(APPEND _cmd
			--language "${_ARG_LANGUAGE}"
			--region   "${_ARG_REGION}"
			--dataset  "${_ARG_DATASET}"
			--quality  "${_ARG_QUALITY}")
	endif ()

	message(STATUS "${CMAKE_CURRENT_FUNCTION}: running models.py ...")
	execute_process(
		COMMAND         ${_cmd}
		RESULT_VARIABLE _result
		OUTPUT_VARIABLE _output
		ERROR_VARIABLE  _error
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_STRIP_TRAILING_WHITESPACE
	)
	if (_output)
		message(STATUS "${_output}")
	endif ()
	if (_result)
		message(WARNING "${CMAKE_CURRENT_FUNCTION}: models.py exited with code ${_result}.\n${_error}")
	endif ()
endfunction()
