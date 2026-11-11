# ecce-mlp-driver (#728): ECCE's optimisers, cell relaxation, dynamics and
# vibrations for machine-learned potentials; the network runs in a Python
# worker (scripts/ecce-mlp --worker).  POSIX pipes, so not on Windows.
add_library(eccemlp STATIC mlpdriver.cpp)
target_include_directories(eccemlp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(eccemlp PUBLIC cxx_std_17)
add_executable(ecce-mlp-driver main.cpp)
target_link_libraries(ecce-mlp-driver PRIVATE eccemlp)
# Beside the other programs, where the tests' tree install finds it.
set_target_properties(ecce-mlp-driver PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# One self-contained source, which ecce-mlp-setup compiles on a machine
# that has no ECCE installation (a cluster).
file(READ mlpdriver.hpp _mlp_hpp)
file(READ mlpdriver.cpp _mlp_cpp)
file(READ main.cpp _mlp_main)
string(REPLACE "#include \"mlpdriver.hpp\"" "" _mlp_cpp "${_mlp_cpp}")
string(REPLACE "#include \"mlpdriver.hpp\"" "" _mlp_main "${_mlp_main}")
string(REPLACE "#pragma once" "" _mlp_hpp "${_mlp_hpp}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ecce-mlp-driver.cpp
     "// Generated from src/mlp by CMake; compile with c++ -O2 -std=c++17.\n"
     "${_mlp_hpp}${_mlp_cpp}${_mlp_main}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
             mlpdriver.hpp mlpdriver.cpp main.cpp)
