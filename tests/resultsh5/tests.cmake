# Included from the root CMakeLists when ecce-results is built.
add_test(NAME resultsh5 COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/resultsh5/run_tests.py $<TARGET_FILE:ecce-results>)

# ecce-results links the data-store libraries, which need an ECCE_HOME.
set_tests_properties(resultsh5 PROPERTIES
  ENVIRONMENT "ECCE_HOME=${CMAKE_SOURCE_DIR};ECCE_REALUSER=resultsh5;ECCE_REALUSERHOME=${CMAKE_BINARY_DIR}/resultsh5-home")

# results.h5 written from parsed calculations (#690).
add_executable(results_calc_test tests/resultsh5/results_calc_test.C)
target_include_directories(results_calc_test PRIVATE ${XercesC_INCLUDE_DIRS})
target_link_libraries(results_calc_test PRIVATE
  eccedsi eccedav eccecipc eccefaces eccexml eccetdat eccedsi eccexml ecceutil
  ${XercesC_LIBRARIES} Threads::Threads)
add_test(NAME resultsh5_calc
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/resultsh5/run_calc_tests.py
          $<TARGET_FILE:results_calc_test> $<TARGET_FILE:ecce-results>)
set_tests_properties(resultsh5_calc PROPERTIES TIMEOUT 900)
