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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wno-deprecated -Wno-sign-compare -Wno-non-virtual-dtor)

find_package(XercesC REQUIRED)
find_package(OpenGL REQUIRED)
find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base gl aui net)
include(${wxWidgets_USE_FILE})
find_package(PkgConfig)
find_package(X11 REQUIRED)
if(NOT X11_Xt_FOUND)
  message(FATAL_ERROR "libXt (X Toolkit Intrinsics) not found - needed by src/comm/commxt/JobStore.C")
endif()

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
# JobStore.C in this directory calls raw X Toolkit Intrinsics APIs
# (XtAppAddSignal, XtCreateApplicationContext, etc.) directly - not part of
# the wx port, this is the old build's async I/O/event-loop scaffolding for
# eccejobstore. Link PUBLIC so every consumer of this static lib (jobstore,
# launcher, mdprepare, organizer) picks up the Xt (and its X11 dependency)
# linkage automatically without needing to list it themselves.
target_link_libraries(eccecommxt PUBLIC X11::Xt X11::X11)
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

# ------------------------------------------------------------------------
# src/apps -> the final executables. Each old src/apps/<name>/Makefile
# defined PROGRAMS (usually one, matching the directory name) built from
# every .C file in that directory, plus an ECCE_LIBS list of which of the
# static libraries above to link against - see src/apps/<name>/Makefile in
# git history for the exact old lists these were transcribed from. A few
# apps (davutils, ecmd, jobstore) never included Makefile.wx at all and
# have no wx dependency; the rest all do. builder and vizthumbnail also
# pull in Makefile.gl (they link the Open Inventor/visualization stack).
# ------------------------------------------------------------------------

# Helper: define an executable from every .C source file in one directory.
# link_libs are this app's own ECCE_LIBS list (as CMake target names, i.e.
# without the old "-l" prefix - note libecceedsi.a is the CMake target
# "eccedsi" here, not "ecceedsi", see the src/dsm section above).
function(ecce_app name dir)
  cmake_parse_arguments(ECCE_APP "WX;GL" "" "LINK" ${ARGN})
  file(GLOB _srcs CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/${dir}/*.C)
  add_executable(${name} ${_srcs})
  target_include_directories(${name} PRIVATE ${XercesC_INCLUDE_DIRS})
  if(ECCE_APP_WX)
    target_compile_definitions(${name} PRIVATE ${wxWidgets_DEFINITIONS})
    target_include_directories(${name} PRIVATE ${wxWidgets_INCLUDE_DIRS})
    target_link_libraries(${name} PRIVATE ${wxWidgets_LIBRARIES})
  endif()
  if(ECCE_APP_GL)
    target_link_libraries(${name} PRIVATE OpenGL::GL OpenGL::GLU)
  endif()
  target_link_libraries(${name} PRIVATE ${ECCE_APP_LINK} ${XercesC_LIBRARIES})
endfunction()

# --- apps with no wx dependency ---
ecce_app(load_tgbs src/apps/davutils
  LINK eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(ecmd src/apps/ecmd
  LINK eccercmd ecceexp eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
# jobstore builds two independent single-file programs from this directory,
# each with its own main() - not combined into one binary (matches the old
# Makefile's "multiple PROGRAMS -> each from its own matching .o" rule).
add_executable(eccejobmaster src/apps/jobstore/eccejobmaster.C)
target_include_directories(eccejobmaster PRIVATE ${XercesC_INCLUDE_DIRS})
target_link_libraries(eccejobmaster PRIVATE
  eccecommxt eccercmd ecceexp eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil
  ${XercesC_LIBRARIES})
add_executable(eccejobstore src/apps/jobstore/eccejobstore.C)
target_include_directories(eccejobstore PRIVATE ${XercesC_INCLUDE_DIRS})
target_link_libraries(eccejobstore PRIVATE
  eccecommxt eccercmd ecceexp eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil
  ${XercesC_LIBRARIES})

# --- wx apps ---
ecce_app(basistool src/apps/basistool WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(calced src/apps/calced WX
  LINK eccecomm eccercmd ecceexp eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(dirdyed src/apps/dirdyed WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(gateway src/apps/gateway WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(launcher src/apps/launcher WX
  LINK eccecommxt eccecomm eccercmd ecceexp eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(machbrowser src/apps/machbrowser WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccercmd ecceexp eccetdat ecceutil)
ecce_app(machregister src/apps/machregister WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(mddynamics src/apps/mddynamics WX
  LINK eccewxguimd eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(mdenergy src/apps/mdenergy WX
  LINK eccewxguimd eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(mdoptimize src/apps/mdoptimize WX
  LINK eccewxguimd eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(mdprepare src/apps/mdprepare WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccecommxt eccecomm eccercmd ecceexp eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(msgdialog src/apps/messagedialog WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(metadyn src/apps/metadyn WX
  LINK eccecomm eccercmd ecceexp eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(organizer src/apps/organizer WX
  LINK eccewxguicomm eccecommxt eccecomm eccercmd ecceexp eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(passdialog src/apps/passdialog WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(pertable src/apps/pertable WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(polyed src/apps/polyrate WX
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)
ecce_app(solvate src/apps/solvate WX
  LINK eccecomm eccercmd ecceexp eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil)

# --- wx + GL apps ---
ecce_app(builder src/apps/builder WX GL
  LINK eccewxguicomm eccewxviz eccewxinv eccevizsg eccemoiv ecceinv eccecommxt eccecomm eccercmd ecceexp
       eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil
       X11::X11 jpeg freetype)
ecce_app(vizthumbnail src/apps/vizthumbnail WX GL
  LINK eccewxviz eccewxinv eccevizsg eccemoiv ecceinv eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccetdat ecceutil X11::X11 jpeg freetype)

# --- src/apps/symmetry -> 5 standalone Fortran executables, no wx involvement.
# Mirrors the old Makefile's "default:" target exactly (testnames is a
# dev/test-only program the old default build skipped - left out here too).
# Each program links only the specific .f/.f90 object files its old Makefile
# rule listed, not every source file in the directory (unlike ecce_app(),
# which links a whole directory - this old Makefile was per-program instead).
function(ecce_fortran_app name)
  cmake_parse_arguments(ECCE_FAPP "" "" "SOURCES" ${ARGN})
  add_executable(${name} ${ECCE_FAPP_SOURCES})
  # Several of these programs independently compile the same spnames.f90 /
  # spgen.f90 module sources - give each target its own .mod output
  # directory so Ninja doesn't see two targets writing the same
  # spnames.mod/spgen.mod file (they're identical content, but CMake/Ninja
  # doesn't know that and refuses to build with a shared output).
  set_target_properties(${name} PROPERTIES
    Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${name}.mod)
endfunction()

set(_sym_dir ${CMAKE_SOURCE_DIR}/src/apps/symmetry)
ecce_fortran_app(autosym
  SOURCES ${_sym_dir}/autosym.f ${_sym_dir}/geom_hnd.f ${_sym_dir}/deter3.f ${_sym_dir}/errquit.f)
ecce_fortran_app(getfrag
  SOURCES ${_sym_dir}/getfrag.f ${_sym_dir}/findfrag.f ${_sym_dir}/spnames.f90 ${_sym_dir}/spgen.f90
          ${_sym_dir}/deter3.f ${_sym_dir}/dctr.f ${_sym_dir}/gensym.f ${_sym_dir}/errquit.f)
ecce_fortran_app(genmol
  SOURCES ${_sym_dir}/genmol.f ${_sym_dir}/dosymops.f ${_sym_dir}/spnames.f90 ${_sym_dir}/spgen.f90
          ${_sym_dir}/deter3.f ${_sym_dir}/dctr.f ${_sym_dir}/gensym.f ${_sym_dir}/errquit.f)
ecce_fortran_app(genmollat
  SOURCES ${_sym_dir}/genmollat.f ${_sym_dir}/dosymops.f ${_sym_dir}/spnames.f90 ${_sym_dir}/spgen.f90
          ${_sym_dir}/deter3.f ${_sym_dir}/dctr.f ${_sym_dir}/gensym.f ${_sym_dir}/errquit.f)
ecce_fortran_app(cleansym
  SOURCES ${_sym_dir}/cleansym.f ${_sym_dir}/sym_avg.f ${_sym_dir}/spnames.f90 ${_sym_dir}/spgen.f90
          ${_sym_dir}/deter3.f ${_sym_dir}/dctr.f ${_sym_dir}/gensym.f ${_sym_dir}/errquit.f)

# ------------------------------------------------------------------------
# Install rules. Everything installs under /opt/ecce (see
# CPACK_PACKAGING_INSTALL_PREFIX below) - the compiled apps still expect
# an ECCE_HOME pointing at a directory with bin/ and data/ as siblings
# (see src/util/genutil/Ecce.C, SFile.C's $ECCE_HOME expansion), which a
# split FHS layout (bin in /usr/bin, data in /usr/share/...) doesn't give
# them without a lot of path-handling surgery this session hasn't done.
# ------------------------------------------------------------------------
set(ECCE_GUI_APPS
  basistool calced dirdyed gateway launcher machbrowser machregister
  mddynamics mdenergy mdoptimize mdprepare msgdialog metadyn organizer
  passdialog pertable polyed solvate builder vizthumbnail)
set(ECCE_CLI_APPS
  load_tgbs ecmd eccejobmaster eccejobstore
  autosym getfrag genmol genmollat cleansym)

install(TARGETS ${ECCE_GUI_APPS} ${ECCE_CLI_APPS} RUNTIME DESTINATION bin)
if(EXISTS ${CMAKE_SOURCE_DIR}/data)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/data DESTINATION .)
endif()

# Thin /usr/bin wrapper scripts for the 19 GUI apps, so they land on PATH
# and get ECCE_HOME set without the user having to source anything. These
# are NEW scripts written for this build's flat single-platform bin/
# layout - not a port of the old scripts/ecce, ebuilder, eviewer etc,
# which assume a vendored 3rdparty Python interpreter, multi-platform
# ECCE_SYSDIR subdirectories, and (for the top-level "ecce" launcher) the
# Java/ActiveMQ JMS messaging server. Those, plus the Perl/Python helper
# scripts in scripts/ (gbs*, eccejobmonitor, processmachine, pmf_gui.py/
# prp_gui.py), are deliberately left out of this package for now - they're
# exactly the peripheral Perl/Python pieces Andy's own forks have already
# diverged on, so packaging this repo's copies would ship the wrong
# version. Named "ecce-<app>" (not the bare old names like "builder") to
# avoid colliding with unrelated commands already on a typical system's
# PATH.
foreach(_app ${ECCE_GUI_APPS})
  set(_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-${_app})
  file(WRITE ${_wrapper}
"#!/bin/sh
export ECCE_HOME=/opt/ecce
exec \"$ECCE_HOME/bin/${_app}\" \"$@\"
")
  install(PROGRAMS ${_wrapper} DESTINATION /usr/bin)
endforeach()

# ------------------------------------------------------------------------
# CPack: build a .deb instead of the old install_ecce self-extracting
# installer. Run `cpack` (or `cpack -G DEB`) after building to produce it.
# Runtime library deps are auto-detected from the actual linked .so's via
# dpkg-shlibdeps (CPACK_DEBIAN_PACKAGE_SHLIBDEPS) rather than hand-listed,
# so this stays correct as the wx/Xerces/Mesa package versions change.
# ------------------------------------------------------------------------
string(REGEX REPLACE "^v" "" _ecce_deb_version "${ECCE_VERSION_RAW}")
string(REPLACE "-" "~" _ecce_deb_version "${_ecce_deb_version}")
if(_ecce_deb_version STREQUAL "")
  set(_ecce_deb_version "0.0.0")
endif()

set(CPACK_PACKAGE_NAME "ecce")
set(CPACK_PACKAGE_VERSION "${_ecce_deb_version}")
set(CPACK_PACKAGE_VENDOR "FriendsofECCE")
set(CPACK_PACKAGE_CONTACT "Andy <caohlin.umu@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "Extensible Computational Chemistry Environment - modeling/workflow GUI tools")
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/ecce")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# RelWithDebInfo keeps -g everywhere (useful while porting - see the
# CMAKE_BUILD_TYPE default above), which makes for a needlessly huge .deb;
# strip debug symbols out of the packaged copies only, not the build tree.
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
include(CPack)
