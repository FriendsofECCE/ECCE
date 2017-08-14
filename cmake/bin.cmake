# Let's build some executables
set(BIN_SRC_DIR ${CMAKE_SOURCE_DIR}/src/c/bin)
set(WX_LIBS -L/home/vagrant/install/wxpython/lib -pthread   -L/usr/lib64 -lwx_gtk2_adv-2.8 -lwx_gtk2_richtext-2.8 -lwx_gtk2_aui-2.8 -lwx_gtk2_xrc-2.8 -lwx_gtk2_qa-2.8 -lwx_gtk2_html-2.8 -lwx_gtk2_adv-2.8 -lwx_gtk2_core-2.8 -lwx_base_xml-2.8 -lwx_base_net-2.8 -lwx_base-2.8)

# basistool
# set(basistool_SRC_DIR ${BIN_SRC_DIR}/basistool)
# file(GLOB basistool_SOURCE ${basistool_SRC_DIR}/*.C)
# add_executable(basistool ${basistool_SOURCE})
# target_include_directories(basistool PUBLIC ${WXPYTHON_INCLUDE})
# target_include_directories(basistool PRIVATE ${basistool_SRC_DIR})
# target_link_libraries(basistool
#     ${wxWidgets_LIBRARIES}
#     xerces
#     ewxaui
#     wxgui
#     wxplotctrl
#     wxthings
#     edsiimpl
#     dav
#     cipc
#     faces
#     xml
#     tdat
#     util
# )

# builder
# calced
# davutils
# dirdyed

# ecmd
find_library(libutil NAMES util)
link_directories(${XERCES_LIB_DIR} /usr/lib64)
include_directories(${XERCES_INCLUDE})
add_executable(ecmd ${BIN_SRC_DIR}/ecmd/ecmd.C)
target_link_libraries(ecmd
    ${XERCES_LIBRARIES}
    rcommand
    expect
    edsiimpl
    dav
    cipc
    faces
    xml
    tdat
    util 
)

# gateway
# jobstore
# launcher
# machbrowser
# machregister
# mddynamics
# mdenergy
# mdoptimize
# mdprepare
# messagedialog
# metadyn
# organizer
# passdialog
# pertable
# polyrate
# solvate
# vizthumbnail

# basistool
# builder
# calced
# davutils
# dirdyed
# ecmd
# gateway
# jobstore
# launcher
# machbrowser
# machregister
# mddynamics
# mdenergy
# mdoptimize
# mdprepare
# messagedialog
# metadyn
# organizer
# passdialog
# pertable
# polyrate
# solvate
# vizthumbnail
