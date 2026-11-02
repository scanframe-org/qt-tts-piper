add_subdirectory("engine")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/models")
	add_subdirectory("models")
endif ()

if (SF_BUILD_TESTING)
	add_subdirectory(tests)
endif()

add_subdirectory("app/qt/cli")
if (SF_BUILD_GUI_TESTING)
	add_subdirectory("app/qt/gui")
endif ()
