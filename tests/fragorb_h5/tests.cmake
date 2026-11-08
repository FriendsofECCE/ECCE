# Included from the root CMakeLists when ecce-fragorb is built (#201).
add_executable(fragorb_h5_test tests/fragorb_h5/fragorb_test.C)
target_link_libraries(fragorb_h5_test PRIVATE eccetdat ecceutil Threads::Threads)
add_test(NAME fragorb_analysis
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/fragorb_h5/run_tests.py
          $<TARGET_FILE:ecce-fragorb> $<TARGET_FILE:fragorb_h5_test>)
set_tests_properties(fragorb_analysis PROPERTIES TIMEOUT 600
  ENVIRONMENT "ECCE_HOME=${CMAKE_SOURCE_DIR};ECCE_REALUSER=fragorb;ECCE_REALUSERHOME=${CMAKE_BINARY_DIR}/fragorb-home;HOME=${CMAKE_BINARY_DIR}/fragorb-home;XDG_CACHE_HOME=${CMAKE_BINARY_DIR}/fragorb-home/.cache")
add_test(NAME fragorb_dos_oracle
  COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/fragorb_h5/dos_oracle.py
          $<TARGET_FILE:ecce-fragorb> $<TARGET_FILE:ecce-results>)
set_tests_properties(fragorb_dos_oracle PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77
  ENVIRONMENT "ECCE_HOME=${CMAKE_SOURCE_DIR};ECCE_REALUSER=fragorb;ECCE_REALUSERHOME=${CMAKE_BINARY_DIR}/fragorb-home;HOME=${CMAKE_BINARY_DIR}/fragorb-home;XDG_CACHE_HOME=${CMAKE_BINARY_DIR}/fragorb-home/.cache")
