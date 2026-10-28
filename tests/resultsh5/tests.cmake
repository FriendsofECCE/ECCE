# Included from the root CMakeLists when ecce-results is built.
add_test(NAME resultsh5 COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/resultsh5/run_tests.py $<TARGET_FILE:ecce-results>)
