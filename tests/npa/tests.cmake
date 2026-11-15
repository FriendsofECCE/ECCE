# Included from the root CMakeLists when ecce-npa is built (#1054).
add_test(NAME npa_nbo COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/npa/run_tests.py $<TARGET_FILE:ecce-npa>)
