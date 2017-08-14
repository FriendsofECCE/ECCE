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
add_library(ewxaui SHARED ${ewxaui_SOURCE})
target_include_directories(ewxaui PUBLIC ${WX_INCLUDE} ${GTK2_INCLUDE_DIRS})
target_compile_options(ewxaui PUBLIC ${WX_FLAGS})
add_dependencies(ewxaui xerces wxpython)

#expect TODO: LOOK INTO THIS AND SEE IF THEY MAY BE A SHARED LIB
find_library(libutil NAMES util)
file(GLOB expect_SOURCE ${LIB_SRC_DIR}/expect/*.c)
add_library(expect STATIC ${expect_SOURCE})
target_link_libraries(expect ${libutil})

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
        ${FREETYPE_INCLUDE_DIR_ft2build}
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

file(GLOB rcommand_SOURCE ${LIB_SRC_DIR}/rcommand/*.C)
add_library(rcommand SHARED ${rcommand_SOURCE})


#tdat
set(tdat_SRC_DIR ${LIB_SRC_DIR}/tdat)
file(GLOB tdat_SOURCE
    ${tdat_SRC_DIR}/calcorg/*.C
    ${tdat_SRC_DIR}/chemistry/*.C
    ${tdat_SRC_DIR}/ffield/*.C
    ${tdat_SRC_DIR}/nwmdtask/*.C
    ${tdat_SRC_DIR}/properties/*.C
    ${tdat_SRC_DIR}/reaction/*.C
    ${tdat_SRC_DIR}/resources/*.C
)
add_library(tdat SHARED ${tdat_SOURCE})

#util
set(util_SRC_DIR ${LIB_SRC_DIR}/util)
file(GLOB util_SOURCE
    ${util_SRC_DIR}/chemutil/*.C
    ${util_SRC_DIR}/command/*.C
    ${util_SRC_DIR}/events/*.C
    ${util_SRC_DIR}/exceptions/*.C
    ${util_SRC_DIR}/genutil/*.C
    ${util_SRC_DIR}/jms/*.C
    ${util_SRC_DIR}/units/*.C
)
add_library(util SHARED ${util_SOURCE})
target_include_directories(util PUBLIC ${XERCES_INCLUDE})
add_dependencies(util xerces)
target_compile_options(util PUBLIC -DECCE_VERSION="${ECCE_VERSION}")

#vizsg
file(GLOB_RECURSE vizsg_SOURCE ${LIB_SRC_DIR}/vizsg/*.C)
add_library(vizsg SHARED ${vizsg_SOURCE})
target_include_directories(vizsg PUBLIC ${XERCES_INCLUDE})
add_dependencies(vizsg xerces)

#wxgui
set(wxgui_SRC_DIR ${LIB_SRC_DIR}/wxgui)
file(GLOB wxgui_SOURCE
    ${wxgui_SRC_DIR}/ewxClasses/*.C
    ${wxgui_SRC_DIR}/jms/*.C
    ${wxgui_SRC_DIR}/pertable/*.C
    ${wxgui_SRC_DIR}/wxdialogs/*.C
    ${wxgui_SRC_DIR}/wxtools/*.C
)
add_library(wxgui SHARED ${wxgui_SOURCE})
target_include_directories(wxgui PUBLIC ${XERCES_INCLUDE} ${WX_INCLUDE})
target_compile_options(wxgui PUBLIC ${WX_FLAGS})
add_dependencies(wxgui xerces wxpython)

#wxguicomm
add_library(wxguicomm SHARED ${LIB_SRC_DIR}/wxguicomm/WxCalcImport.C)
target_include_directories(wxguicomm PUBLIC ${XERCES_INCLUDE} ${WX_INCLUDE})
target_compile_options(wxguicomm PUBLIC ${WX_FLAGS})
add_dependencies(wxguicomm xerces wxpython)

#wxinv
file(GLOB_RECURSE wxinv_SOURCE ${LIB_SRC_DIR}/wxinv/*.C)
add_library(wxinv SHARED ${wxinv_SOURCE})
target_include_directories(wxinv PUBLIC ${WX_INCLUDE})
target_compile_options(wxinv PUBLIC ${WX_FLAGS})
add_dependencies(wxinv wxpython)

#wxmdtools
file(GLOB_RECURSE wxmdtools_SOURCE ${LIB_SRC_DIR}/wxmdtools/*.C)
add_library(wxmdtools SHARED ${wxmdtools_SOURCE})
target_include_directories(wxmdtools PUBLIC ${XERCES_INCLUDE} ${WX_INCLUDE})
target_compile_options(wxmdtools PUBLIC ${WX_FLAGS})
add_dependencies(wxmdtools xerces wxpython)

#wxplotctrl
file(GLOB_RECURSE wxplotctrl_SOURCE ${LIB_SRC_DIR}/wxplotctrl/*.C)
add_library(wxplotctrl SHARED ${wxplotctrl_SOURCE})
target_include_directories(wxplotctrl PUBLIC ${WX_INCLUDE})
target_compile_options(wxplotctrl PUBLIC ${WX_FLAGS})
add_dependencies(wxplotctrl wxpython)

#wxthings
file(GLOB_RECURSE wxthings_SOURCE ${LIB_SRC_DIR}/wxthings/*.C)
add_library(wxthings SHARED ${wxthings_SOURCE})
target_include_directories(wxthings PUBLIC ${WX_INCLUDE})
target_compile_options(wxthings PUBLIC ${WX_FLAGS})
add_dependencies(wxthings wxpython)

#wxviz
set(wxviz_SRC_DIR ${LIB_SRC_DIR}/wxviz)
file(GLOB wxviz_SOURCE
    ${wxviz_SRC_DIR}/viewer/*.C
    ${wxviz_SRC_DIR}/viztools/*.C
)
add_library(wxviz SHARED ${wxviz_SOURCE})
target_include_directories(wxviz PUBLIC ${XERCES_INCLUDE} ${WX_INCLUDE})
target_compile_options(wxviz PUBLIC ${WX_FLAGS})
add_dependencies(wxviz xerces wxpython)

#xml
set(xml_SRC_DIR ${LIB_SRC_DIR}/xml)
file(GLOB xml_SOURCE
    ${xml_SRC_DIR}/chemistry/*.C
    ${xml_SRC_DIR}/xml/*.C
)
add_library(xml SHARED ${xml_SOURCE})
target_include_directories(xml PUBLIC ${XERCES_INCLUDE})
add_dependencies(xml xerces)

# Add the right libecce_ prefix
set_target_properties(
        cipc
        commtools
        commxt
        dav
        edsiimpl
        ewxaui
        expect
        faces
        inv
        moiv
        rcommand
        tdat
        util
        vizsg
        wxgui
        wxguicomm
        wxinv
        wxmdtools
        wxplotctrl
        wxthings
        wxviz
        xml
    PROPERTIES PREFIX libecce_)

# Install the the library directory
install(
    TARGETS
        cipc
        commtools
        commxt
        dav
        edsiimpl
        ewxaui
        expect
        faces
        inv
        moiv
        rcommand
        tdat
        util
        vizsg
        wxgui
        wxguicomm
        wxinv
        wxmdtools
        wxplotctrl
        wxthings
        wxviz
        xml
    DESTINATION lib)
