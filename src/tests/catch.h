#pragma once

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS

/*
 * This file is used to import the Catch2 library headers indirectly
 * to prevent future changes to many files when ti changes.
 *
 * For faster compiling it is better to import only the includes needed instead all of them.
 * Like:
 *   #include <catch2/catch_test_macros.hpp>
 *   #include <catch2/catch_template_test_macros.hpp>
 *   #include <catch2/catch_approx.hpp>
 */
#include <catch2/catch_all.hpp>
