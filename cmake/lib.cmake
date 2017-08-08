# Building the C Library files for ECCE


set(LIB_SRC_DIR ${CMAKE_SOURCE_DIR}/src/c/lib)



#cipic
file(GLOB_RECURSE cipc_SOURCE ${LIB_SRC_DIR}/cipc/*.C)
add_library(cipc SHARED ${cipc_SOURCE})

#commtools
file(GLOB_RECURSE commtools_SOURCE ${LIB_SRC_DIR}/commtools/*.C)
add_library(commtools SHARED ${commtools_SOURCE})
target_include_directories(commtools PUBLIC ${XERCES_INCLUDE})
add_dependencies(commtools xerces)

#commxt
file(GLOB_RECURSE commxt_SOURCE ${LIB_SRC_DIR}/commxt/*.C)
add_library(commxt SHARED ${commxt_SOURCE})
target_include_directories(commxt PUBLIC ${XERCES_INCLUDE})

#dav
file(GLOB dav_SOURCE ${LIB_SRC_DIR}/dav/*.C)
add_library(dav SHARED ${dav_SOURCE})
target_include_directories(dav PUBLIC ${XERCES_INCLUDE})
add_dependencies(dav xerces)

#edsiimpl
file(GLOB edsiimpl_SOURCE ${LIB_SRC_DIR}/edsiimpl/*.C)
add_library(edsiimpl SHARED ${edsiimpl_SOURCE})
target_include_directories(edsiimpl PUBLIC ${XERCES_INCLUDE})
add_dependencies(edsiimpl xerces)

# ewxaui
file(GLOB_RECURSE ewxaui_SOURCE ${LIB_SRC_DIR}/ewxaui/*.C)
set(ewxaui_INCLUDE ${LIB_SRC_DIR}/ewxaui/include)
add_library(ewxaui SHARED ${ewxaui_SOURCE})
target_compile_options(ewxaui PUBLIC ${wxWidgets_CXX_FLAGS})
target_include_directories(ewxaui PUBLIC ${wxWidgets_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS})
add_dependencies(ewxaui xerces)

#expect TODO: LOOK INTO THIS AND SEE IF THEY MAY BE A SHARED LIB
file(GLOB expect_SOURCE ${LIB_SRC_DIR}/expect/*.c)
add_library(expect STATIC ${expect_SOURCE})

#faces
file(GLOB faces_SOURCE ${LIB_SRC_DIR}/faces/*.C)
add_library(faces SHARED ${faces_SOURCE})
target_include_directories(faces PUBLIC ${XERCES_INCLUDE})
add_dependencies(faces xerces)

#inv
set(inv_SOURCE_DIR ${LIB_SRC_DIR}/inv)
file(GLOB_RECURSE inv_cpp_SOURCE ${inv_SOURCE_DIR}/*.C)
file(GLOB_RECURSE inv_c_SOURCE ${inv_SOURCE_DIR}/*.c)
add_library(inv SHARED ${inv_c_SOURCE} ${inv_cpp_SOURCE})
target_include_directories(inv
    PUBLIC
        ${FREETYPE_INCLUDE_DIRS}
    PRIVATE
        ${inv_SOURCE_DIR}/dbelements
        ${inv_SOURCE_DIR}/dbengines
        ${inv_SOURCE_DIR}/dbfields
        ${inv_SOURCE_DIR}/dbnodes
        ${inv_SOURCE_DIR}/dbso
        ${inv_SOURCE_DIR}/dbupgraders
        ${inv_SOURCE_DIR}/flcient
        ${inv_SOURCE_DIR}/image
        ${inv_SOURCE_DIR}/interaction
)
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(inv PUBLIC -DNDEBUG)
endif(NOT CMAKE_BUILD_TYPE MATCHES Debug)

#moiv
file(GLOB_RECURSE moiv_SOURCE ${LIB_SRC_DIR}/moiv/*.C)
add_library(moiv SHARED ${moiv_SOURCE})
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(moiv PUBLIC -DNDEBUG)
endif(NOT CMAKE_BUILD_TYPE MATCHES Debug)

#rcommand


# Add the right libecce_ prefix
set_target_properties(
    cipc
    ewxaui
    PROPERTIES
    PREFIX libecce_
)

# Install the the library directory
install(
    TARGETS
        cipc
        ewxaui
    DESTINATION lib)
