# Set the target name using the current directory name and the test prefix.
get_filename_component(SF_PROJECT_SUFFIX ${CMAKE_CURRENT_SOURCE_DIR} NAME)


set(TEST_APP_TARGET "${PROJECT_NAME}-${SF_PROJECT_SUFFIX}")

# Build Qt target.
if (TRUE)

	# Find the needed packages.
	find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets TextToSpeech REQUIRED)
	# Import target 'espeak-ng-data' for generating/compiling the required 'espeak-ng-data' directory.
	find_package(SfEspeakNgData 1.52.0 CONFIG REQUIRED)
	# Enable Qt features.
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	# Find the test sources.
	file(GLOB TEST_APP_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test-*")
	# Add shared library and also sets the executable version.
	Sf_AddTarget("${TEST_APP_TARGET}" FLAGS pack VERSION EXIF SUFFIX)
	# Should be linked to the main library.
	target_link_libraries("${TEST_APP_TARGET}" PRIVATE Qt::Gui Qt::Widgets Qt::TextToSpeech)
	# Add the sources.
	target_sources("${TEST_APP_TARGET}" PRIVATE main-qt.cpp "${TEST_APP_SOURCES}")
	# Add a dependency to the target.
	add_dependencies("${TEST_APP_TARGET}" espeak-ng-data)

else ()

	# Find the test sources.
	file(GLOB TEST_APP_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "test-*")
	# Add shared library and also sets the executable version.
	Sf_AddTarget("${TEST_APP_TARGET}" FLAGS test cov pack VERSION EXIF SUFFIX)
	# Should be linked to the main library.
	target_link_libraries("${TEST_APP_TARGET}" PRIVATE piper::static)
	# Add the sources.
	target_sources("${TEST_APP_TARGET}" PRIVATE main-piper.cpp "${TEST_APP_SOURCES}")
	# Required for the location of 'libonnxruntime.so'.
	set_target_properties("${TEST_APP_TARGET}" PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH};\${ORIGIN}/lib/texttospeech")

endif ()