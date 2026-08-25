cmake_minimum_required(VERSION 3.16)
project(ECCE CXX C Fortran)

# ------------------------------------------------------------------------
# This replaces the wx-independent portion of the old build_ecce / recursive
# Makefile system (build/build_ecce, build/make/Makefile.*). It targets the
# distro-packaged versions of wxWidgets/Xerces-C/Mesa rather than building
# vendored copies from build/3rdparty-dists/ as the old system did.
#
# Status: covers src/util, src/tdat, src/dsm, src/viz, src/comm, src/inv
# (minus src/inv/wxinv, which needs wxWidgets and hasn't been ported to
# wx 3.2 yet). src/apps, src/wxgui, src/wxviz are not yet included here -
# they need the wx 2.8->3.2 port first. See
# claude/ecce-modernization-status.md in the project docs for full status.
# ------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wno-deprecated -Wno-sign-compare -Wno-non-virtual-dtor)

find_package(XercesC REQUIRED)
find_package(OpenGL REQUIRED)
find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base gl aui)
include(${wxWidgets_USE_FILE})
find_package(PkgConfig)

# Old build used "-I/usr/include/freetype2/freetype" as a baseline include
# dir everywhere, because most FreeType headers this codebase includes
# unqualified (e.g. <ftoutln.h>) live one level below the freetype2 package's
# normal include root. Keep that same include shape rather than switching to
# find_package(Freetype), to stay behavior-identical to the verified Make
# build. (One file, flclient/flfreetype.c, also needs <ft2build.h> itself,
# which lives at the freetype2 root one level up - handled separately below,
# on just the ecceinv target, matching the old per-directory Makefile.)
find_path(FREETYPE_UNQUALIFIED_INCLUDE_DIR ftoutln.h
  PATHS /usr/include/freetype2/freetype /usr/include/freetype)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${FREETYPE_UNQUALIFIED_INCLUDE_DIR}
)

set(ECCE_VERSION_RAW "")
if(EXISTS ${CMAKE_SOURCE_DIR}/data/client/config/Version)
  file(STRINGS ${CMAKE_SOURCE_DIR}/data/client/config/Version ECCE_VERSION_RAW LIMIT_COUNT 1)
endif()

# ------------------------------------------------------------------------
# Helper: define a static library from an explicit list of source
# directories (matches the old build's grouping of multiple src/<mod>/<sub>
# directories into one libecceXXX.a - see per-directory LIBRARY= lines in
# the old Makefiles for the mapping this replicates).
# ------------------------------------------------------------------------
function(ecce_library name)
  set(_srcs)
  foreach(dir ${ARGN})
    file(GLOB _dir_srcs CONFIGURE_DEPENDS
      ${CMAKE_SOURCE_DIR}/${dir}/*.C
      ${CMAKE_SOURCE_DIR}/${dir}/*.c)
    list(APPEND _srcs ${_dir_srcs})
  endforeach()
  add_library(${name} STATIC ${_srcs})
  target_include_directories(${name} PRIVATE ${XercesC_INCLUDE_DIRS})
endfunction()

# --- src/util -> libecceutil.a ---
ecce_library(ecceutil
  src/util/chemutil
  src/util/command
  src/util/events
  src/util/exceptions
  src/util/genutil
  src/util/jms
  src/util/units
)
target_compile_definitions(ecceutil PRIVATE ECCE_VERSION=\"${ECCE_VERSION_RAW}\")

# --- src/tdat -> libeccetdat.a ---
ecce_library(eccetdat
  src/tdat/calcorg
  src/tdat/chemistry
  src/tdat/ffield
  src/tdat/nwmdtask
  src/tdat/properties
  src/tdat/reaction
  src/tdat/resources
)

# --- src/dsm -> 5 separate libraries, matching the old per-subdir mapping ---
ecce_library(eccexml    src/dsm/chemistry src/dsm/xml)
ecce_library(eccecipc   src/dsm/cipc)
ecce_library(eccedav    src/dsm/dav)
ecce_library(eccedsi    src/dsm/edsiimpl)
ecce_library(eccefaces  src/dsm/faces)

# --- src/viz -> libeccevizsg.a ---
ecce_library(eccevizsg
  src/viz/atomnodes
  src/viz/nodes
  src/viz/propsgcommands
  src/viz/sgcommands
)

# --- src/comm -> 4 separate libraries ---
ecce_library(eccecomm   src/comm/commtools)
ecce_library(eccecommxt src/comm/commxt)
ecce_library(eccercmd   src/comm/rcommand)
ecce_library(ecceexp    src/comm/expect)
# Old build's src/comm/expect/Makefile: CCFLAGS += -DNO_UNION_WAIT (this vendored
# Tcl/Expect code's WAIT_STATUS_TYPE macro defaults to the BSD "union wait" type,
# which glibc has never provided on Linux - NO_UNION_WAIT selects plain "int").
target_compile_definitions(ecceexp PRIVATE NO_UNION_WAIT)

# --- src/inv -> libecceinv.a + libeccemoiv.a (libeccewxinv.a needs wx, TODO) ---
ecce_library(ecceinv
  src/inv/dbelements
  src/inv/dbengines
  src/inv/dbfields
  src/inv/dbnodes
  src/inv/dbsb
  src/inv/dbso
  src/inv/dbupgraders
  src/inv/flclient
  src/inv/image
  src/inv/interaction
  src/inv/nodekits
)
target_compile_definitions(ecceinv PRIVATE NDEBUG)
target_link_libraries(ecceinv PRIVATE OpenGL::GL OpenGL::GLU)
# Old build's src/inv/flclient/Makefile adds `pkg-config --cflags-only-I freetype2`
# on top of the baseline include dir: some FreeType headers (ft2build.h) live at
# the top of the freetype2 include tree, others (ftoutln.h etc, used elsewhere in
# this codebase) live one level down - both are needed.
find_path(FREETYPE_ROOT_INCLUDE_DIR ft2build.h
  PATHS /usr/include/freetype2 /usr/include)
target_include_directories(ecceinv PRIVATE ${FREETYPE_ROOT_INCLUDE_DIR})

ecce_library(eccemoiv src/inv/moiv)
target_compile_definitions(eccemoiv PRIVATE NDEBUG)
target_link_libraries(eccemoiv PRIVATE OpenGL::GL OpenGL::GLU)

# ------------------------------------------------------------------------
# wxWidgets 2.8 -> 3.2 port (in progress). See
# claude/ecce-modernization-status.md in the project docs for what's done.
# ------------------------------------------------------------------------

# --- src/inv/wxinv -> libeccewxinv.a (Open Inventor / wxWidgets bridge) ---
ecce_library(eccewxinv src/inv/wxinv)
target_compile_definitions(eccewxinv PRIVATE NDEBUG ${wxWidgets_DEFINITIONS})
target_include_directories(eccewxinv PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(eccewxinv PRIVATE ${wxWidgets_LIBRARIES} OpenGL::GL OpenGL::GLU)

# --- src/wxviz -> libeccewxviz.a ---
ecce_library(eccewxviz src/wxviz/viewer src/wxviz/viztools)
target_compile_definitions(eccewxviz PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(eccewxviz PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(eccewxviz PRIVATE ${wxWidgets_LIBRARIES} OpenGL::GL OpenGL::GLU)

# --- src/wxgui -> 5 separate libraries, matching the old per-subdir mapping ---
ecce_library(eccewxgui
  src/wxgui/ewxClasses
  src/wxgui/jms
  src/wxgui/pertable
  src/wxgui/wxdialogs
  src/wxgui/wxtools
)
target_compile_definitions(eccewxgui PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(eccewxgui PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(eccewxgui PRIVATE ${wxWidgets_LIBRARIES})

ecce_library(eccewxguicomm src/wxgui/comm)
target_compile_definitions(eccewxguicomm PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(eccewxguicomm PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(eccewxguicomm PRIVATE ${wxWidgets_LIBRARIES})

ecce_library(eccewxguimd src/wxgui/mdtools)
target_compile_definitions(eccewxguimd PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(eccewxguimd PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(eccewxguimd PRIVATE ${wxWidgets_LIBRARIES})

ecce_library(eccewxplotctrl src/wxgui/wxplotctrl)
target_compile_definitions(eccewxplotctrl PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(eccewxplotctrl PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(eccewxplotctrl PRIVATE ${wxWidgets_LIBRARIES})
# Old build's src/wxgui/wxplotctrl/Makefile adds `pkg-config --cflags-only-I
# gtk+-2.0` - this build targets wx3.2's GTK3 backend, so gtk+-3.0 is the
# equivalent (the old gtk+-2.0 dev package isn't even installed here).
pkg_check_modules(GTK3 QUIET gtk+-3.0)
if(GTK3_FOUND)
  target_include_directories(eccewxplotctrl PRIVATE ${GTK3_INCLUDE_DIRS})
endif()

ecce_library(eccewxthings src/wxgui/wxthings)
target_compile_definitions(eccewxthings PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(eccewxthings PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(eccewxthings PRIVATE ${wxWidgets_LIBRARIES})

# TODO: src/apps targets and final executables.
