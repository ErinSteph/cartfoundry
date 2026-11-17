# CMake generated Testfile for 
# Source directory: /Users/erin/Dev/CartFoundry
# Build directory: /Users/erin/Dev/CartFoundry/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test("project-and-build" "/Users/erin/Dev/CartFoundry/build/cf-tests")
set_tests_properties("project-and-build" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;33;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("runtime-flow" "/Users/erin/Dev/CartFoundry/build/cf-flow-tests")
set_tests_properties("runtime-flow" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;36;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("runtime-world" "/Users/erin/Dev/CartFoundry/build/cf-world-tests")
set_tests_properties("runtime-world" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;39;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("emulator-integration" "/Users/erin/Dev/CartFoundry/build/cf-emulator-tests")
set_tests_properties("emulator-integration" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;42;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("runtime-behavior" "/Users/erin/Dev/CartFoundry/build/cf-behavior-tests")
set_tests_properties("runtime-behavior" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;45;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("runtime-animation" "/Users/erin/Dev/CartFoundry/build/cf-animation-tests")
set_tests_properties("runtime-animation" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;48;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("game-systems" "/Users/erin/Dev/CartFoundry/build/cf-systems-tests")
set_tests_properties("game-systems" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;52;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("pokemon-adventure" "/Users/erin/Dev/CartFoundry/build/cf-pokemon-tests")
set_tests_properties("pokemon-adventure" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;55;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
add_test("versatility-and-rendering" "/Users/erin/Dev/CartFoundry/build/cf-versatility-tests")
set_tests_properties("versatility-and-rendering" PROPERTIES  _BACKTRACE_TRIPLES "/Users/erin/Dev/CartFoundry/CMakeLists.txt;59;add_test;/Users/erin/Dev/CartFoundry/CMakeLists.txt;0;")
subdirs("third_party/mgba")
