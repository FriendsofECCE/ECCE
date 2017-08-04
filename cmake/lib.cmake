# Building the C Library files for ECCE

# ewxaui
file(GLOB_RECURSE ewxaui_SOURCE ${CMAKE_SOURCE_DIR}/src/ewxaui/*.C)
set(ewxaui_INCLUDE ${CMAKE_SOURCE_DIR}/rc/ewxaui/include)

add_library(ewxaui SHARED ${ewxaui_SOURCE})
add_dependencies(ewxaui third_party)
target_compile_options(ewxaui PUBLIC ${wxWidgets_CXX_FLAGS})
target_include_directories(ewxaui PUBLIC ${wxWidgets_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS})
# Does this need to be in here?
# target_link_libraries(ewxaui PUBLIC ${wxWidgets_LIBRARIES})
install(TARGETS ewxaui DESTINATION lib)
