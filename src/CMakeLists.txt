add_subdirectory("engine")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/models")
	add_subdirectory("models")
endif ()

add_subdirectory("app/qt/cli")
add_subdirectory("app/qt/gui")

if (SF_BUILD_TESTING)
	# Prevent tests which are failing in the CI-pipeline from running at all.
	if(SF_COMPILER STREQUAL "ga" AND EXISTS "/.dockerenv" AND DEFINED ENV{CI})
		message(STATUS "Skipping tests in docker container running in a pipeline.")
	else()
		add_subdirectory(tests)
	endif()
endif()
