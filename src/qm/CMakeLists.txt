# ecce-qm: small built-in HF/DFT engine for teaching molecules.
# Builds inside the ECCE tree (add_subdirectory) or on its own:
#     cmake -S src/qm -B build-qm -G Ninja && cmake --build build-qm -j2
#     ctest --test-dir build-qm
cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_PROJECT_NAME OR CMAKE_PROJECT_NAME STREQUAL "ecce_qm")
  project(ecce_qm CXX C)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  enable_testing()
  set(ECCE_QM_STANDALONE ON)
endif()

include(FetchContent)
# The fetched projects ask for CMake < 3.5, which CMake 4 (macOS and MSYS2
# runners) refuses.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
# Static dependencies built into ecce-qm; their headers and libraries are not
# part of any package (EXCLUDE_FROM_ALL also drops their install rules).
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
  set(_qm_fc_exclude EXCLUDE_FROM_ALL)
else()
  set(_qm_fc_exclude)
endif()
find_package(OpenMP COMPONENTS CXX)

# --- Eigen (header only, MPL-2.0) ------------------------------------------
find_package(Eigen3 3.3 CONFIG QUIET)
if(NOT TARGET Eigen3::Eigen)
  FetchContent_Declare(eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0 GIT_SHALLOW ON ${_qm_fc_exclude})
  set(EIGEN_BUILD_DOC OFF)
  set(BUILD_TESTING OFF)
  set(EIGEN_BUILD_PKGCONFIG OFF)
  FetchContent_MakeAvailable(eigen)
endif()

# --- libxc (MPL-2.0): system package if present, else vendored -------------
# Debian: apt install libxc-dev
find_package(PkgConfig QUIET)
set(_have_xc OFF)
if(PkgConfig_FOUND AND NOT ECCE_QM_VENDOR_LIBXC)
  pkg_check_modules(XC IMPORTED_TARGET libxc>=5.1)
  if(XC_FOUND)
    add_library(qm_xc INTERFACE)
    target_link_libraries(qm_xc INTERFACE PkgConfig::XC)
    set(_have_xc ON)
    message(STATUS "ecce-qm: using system libxc ${XC_VERSION}")
  endif()
endif()
if(NOT _have_xc)
  message(STATUS "ecce-qm: system libxc not found, vendoring libxc 7.0.0")
  FetchContent_Declare(libxc
    GIT_REPOSITORY https://gitlab.com/libxc/libxc.git
    GIT_TAG 7.0.0 GIT_SHALLOW ON ${_qm_fc_exclude})
  set(BUILD_SHARED_LIBS OFF)
  set(BUILD_TESTING OFF)
  set(ENABLE_FORTRAN OFF)
  set(ENABLE_PYTHON OFF)
  set(ENABLE_XHOST OFF)
  set(DISABLE_KXC ON)
  set(DISABLE_LXC ON)
  FetchContent_MakeAvailable(libxc)
  add_library(qm_xc INTERFACE)
  target_link_libraries(qm_xc INTERFACE xc)
  target_include_directories(qm_xc INTERFACE
    ${libxc_BINARY_DIR}/src ${libxc_BINARY_DIR} ${libxc_SOURCE_DIR}/src)
endif()

# --- libcint (Apache-2.0): not packaged in Debian, always vendored ---------
FetchContent_Declare(libcint
  GIT_REPOSITORY https://github.com/sunqm/libcint.git
  GIT_TAG v6.1.3 GIT_SHALLOW ON ${_qm_fc_exclude})
set(ENABLE_STATIC ON)
set(WITH_RANGE_COULOMB OFF)
set(BUILD_MARCH_NATIVE OFF)
FetchContent_MakeAvailable(libcint)

# --- the library and the command line tool ---------------------------------
add_library(eccems STATIC
  molecule.cpp basislib.cpp basis.cpp integrals.cpp ao.cpp grid.cpp xc.cpp
  scf.cpp input.cpp output.cpp)
target_include_directories(eccems PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${libcint_SOURCE_DIR}/include)
target_link_libraries(eccems PUBLIC Eigen3::Eigen cint qm_xc)
if(OpenMP_CXX_FOUND)
  target_link_libraries(eccems PUBLIC OpenMP::OpenMP_CXX)
endif()
set_target_properties(eccems PROPERTIES POSITION_INDEPENDENT_CODE ON OUTPUT_NAME ecce-qm-lib)
get_filename_component(_ecce_root "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
target_compile_definitions(eccems PRIVATE
  ECCE_QM_DEFAULT_BASIS_DIR="${_ecce_root}/data/admin/basissets")

add_executable(ecce-qm main.cpp)
target_link_libraries(ecce-qm PRIVATE eccems)
# Shipped stripped on every platform (the debug info of the fetched libraries
# makes it 150 MB otherwise).
if(APPLE)
  target_link_options(ecce-qm PRIVATE -Wl,-S -Wl,-x)
else()
  target_link_options(ecce-qm PRIVATE -s)
endif()

# --- tests (oracle numbers: tests/qm/oracle.json, see tests/qm/gen_oracle.py) ---
set(_qm_tests ${_ecce_root}/tests/qm)
add_executable(qm_unit_test ${_qm_tests}/unit_test.cpp)
target_link_libraries(qm_unit_test PRIVATE eccems)
add_test(NAME qm_unit COMMAND qm_unit_test ${_ecce_root}/data/admin/basissets)
find_program(QM_PYTHON3 NAMES python3)
if(QM_PYTHON3)
  add_test(NAME qm_scf_quick COMMAND ${QM_PYTHON3} ${_qm_tests}/run_tests.py $<TARGET_FILE:ecce-qm> --quick)
  add_test(NAME qm_scf COMMAND ${QM_PYTHON3} ${_qm_tests}/run_tests.py $<TARGET_FILE:ecce-qm>)
  set_tests_properties(qm_scf_quick PROPERTIES TIMEOUT 3600)
  set_tests_properties(qm_scf PROPERTIES TIMEOUT 7200)
endif()
