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
# Every .c (not .C) file in this tree (33 total, under src/comm/expect,
# src/inv/dbnodes, src/inv/flclient, src/inv/image, src/wxgui/wxplotctrl)
# is vendored ~1990s third-party C, riddled with K&R-style empty-parens
# forward declarations (e.g. `char *exp_printify();`, later defined and
# called with real arguments) -- valid, unambiguous C when empty parens
# mean "unspecified arguments" (K&R/C89 through GNU's own default
# extensions), a hard error under newer C standard defaults where empty
# parens instead mean "takes no arguments". Confirmed via real Fedora CI
# build failures (newer default GCC) not reproducible on Debian/Ubuntu
# (older default GCC) -- pin C compilation tree-wide to gnu17 rather than
# rewriting decades-old third-party prototypes file by file as each one
# surfaces on whichever platform's default happens to be newest this
# month. Only affects .c compilation -- CMAKE_CXX_STANDARD above is
# separate and unaffected.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ------------------------------------------------------------------------
# Road to 9.0, step 1: split the single monolithic package into
# ecce-client (GUI apps, input generators/parsers, codereg dialogs, the
# job-side scripts the Launcher copies to compute hosts, the per-session
# JMSDispatcher relay, ecce-remote-setup) and ecce-server (the per-user
# or central WebDAV data server and its content, the ActiveMQ broker
# config and its start/stop). Every install() rule below carries a
# COMPONENT tag either way -- that costs nothing when this is OFF, since
# plain `cpack`/`cpack -G DEB` ignores COMPONENT entirely and produces
# one monolithic package exactly as before (CPACK_DEB_COMPONENT_INSTALL
# defaults OFF). Turning this ON is what makes CPack actually split them.
# Default OFF so 8.x releases (one .deb, everything in it) are
# unaffected; flip it on with `cmake -DECCE_SPLIT_PACKAGES=ON`.
option(ECCE_SPLIT_PACKAGES "Package ecce-client and ecce-server as separate .deb/.rpm files instead of one monolithic package" OFF)

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
# Homebrew's freetype2 package lands at /opt/homebrew (Apple Silicon) or
# /usr/local (Intel Mac) instead of /usr -- neither hardcoded Linux path
# above exists there at all, and find_path() silently returns
# "...-NOTFOUND" rather than erroring, which only surfaces later as a
# baffling "Found relative path while evaluating include directories"
# CMake generate-step error. Confirmed via a real macOS CI run.
find_path(FREETYPE_UNQUALIFIED_INCLUDE_DIR ftoutln.h
  PATHS /usr/include/freetype2/freetype /usr/include/freetype
        /opt/homebrew/include/freetype2/freetype
        /usr/local/include/freetype2/freetype)
if(NOT FREETYPE_UNQUALIFIED_INCLUDE_DIR)
  message(FATAL_ERROR "FreeType's freetype/ include subdir (ftoutln.h) not found -- searched the usual Linux and Homebrew locations")
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${FREETYPE_UNQUALIFIED_INCLUDE_DIR}
)

if(APPLE)
  # ~90 files across the tree spell OpenGL headers the Linux/X11 way
  # ("GL/gl.h", "GL/glu.h") -- Apple's own OpenGL framework headers live
  # under "OpenGL/" instead. A forwarding shim here (packaging/macos-
  # compat/GL/{gl,glu}.h) avoids editing every one of those call sites;
  # placed last so it never shadows a real GL/ dir if one somehow exists.
  include_directories(AFTER ${CMAKE_SOURCE_DIR}/packaging/macos-compat)
endif()

set(ECCE_VERSION_RAW "")
if(EXISTS ${CMAKE_SOURCE_DIR}/data/client/config/Version)
  file(STRINGS ${CMAKE_SOURCE_DIR}/data/client/config/Version ECCE_VERSION_RAW LIMIT_COUNT 1)
  # file(STRINGS ...) is a configure-time read, not a build dependency --
  # CMake won't notice this file changed on a later `cmake --build` alone.
  # Registering it here makes an edit to the Version file trigger an
  # automatic reconfigure the next time anything is built.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/data/client/config/Version)
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

# ------------------------------------------------------------------------
# Where an installed ECCE lives, and where its ecce-<app> wrappers go.
#
# Both default to what the .deb ships (/opt/ecce and /usr/bin), so packaging
# is unaffected. They are cache variables so that a developer can install to
# somewhere writable and skip sudo entirely:
#
#   cmake -B build-user -GNinja \
#         -DCMAKE_INSTALL_PREFIX=$HOME/.local/ecce \
#         -DECCE_HOME_DIR=$HOME/.local/ecce \
#         -DECCE_WRAPPER_DESTINATION=$HOME/.local/bin
#   cmake --build build-user --target install
#
# That matters for more than convenience: tests/apps can only exercise an
# INSTALLED build, so without this every test of a C++ change needs root.
# ------------------------------------------------------------------------
set(ECCE_HOME_DIR "/opt/ecce" CACHE PATH
    "Directory an installed ECCE runs from; wrappers export it as ECCE_HOME")
set(ECCE_WRAPPER_DESTINATION "/usr/bin" CACHE PATH
    "Where the ecce-<app> wrapper scripts are installed")


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
# (K&R empty-parens prototype conflicts in this vendored code are handled
# by the tree-wide CMAKE_C_STANDARD gnu17 setting above, not a per-target
# override here -- the same bug shape turned out to also be present in
# src/inv/image's vendored C, not just here.)

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
# Same Homebrew-path gap as FREETYPE_UNQUALIFIED_INCLUDE_DIR above.
find_path(FREETYPE_ROOT_INCLUDE_DIR ft2build.h
  PATHS /usr/include/freetype2 /usr/include
        /opt/homebrew/include/freetype2
        /usr/local/include/freetype2)
if(NOT FREETYPE_ROOT_INCLUDE_DIR)
  message(FATAL_ERROR "FreeType's ft2build.h not found -- searched the usual Linux and Homebrew locations")
endif()
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

# ewxApp.C installs a g_log_set_writer_func() filter for a known-benign
# wxWidgets/GTK3 backend warning (see the comment there) -- needs GLib's
# own headers directly, which wxWidgets_INCLUDE_DIRS doesn't expose.
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
target_include_directories(eccewxgui PRIVATE ${GLIB2_INCLUDE_DIRS})
target_link_libraries(eccewxgui PRIVATE ${GLIB2_LIBRARIES})

# WxDavAuth.C calls gtk_window_present_with_time()/gdk_x11_get_server_time()
# directly (#120: Mutter's focus-stealing prevention silently drops the
# auth dialog's window-manager activation unless it's requested with a
# fresh, real X server timestamp) -- same gtk+-3.0 need as eccewxplotctrl
# below, headers AND the library this time since it's a real call, not
# just the log filter's headers-only use above.
pkg_check_modules(GTK3 QUIET gtk+-3.0)
if(GTK3_FOUND)
  target_include_directories(eccewxgui PRIVATE ${GTK3_INCLUDE_DIRS})
  target_link_libraries(eccewxgui PRIVATE ${GTK3_LIBRARIES})
endif()

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
  # eccercmd for RCommand::localityNote (#144), the same decision the launch makes.
  LINK eccewxgui eccewxplotctrl eccewxthings eccedsi eccedav eccecipc eccefaces eccexml eccercmd ecceexp eccetdat ecceutil)
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
# Added for the symmetry analysis behind the MO diagrams (#132): gensym
# already turns a group name into its operation matrices, and nothing
# outside these Fortran tools could reach them.
ecce_fortran_app(symops
  SOURCES ${_sym_dir}/symops.f ${_sym_dir}/spnames.f90 ${_sym_dir}/spgen.f90
          ${_sym_dir}/deter3.f ${_sym_dir}/dctr.f ${_sym_dir}/gensym.f ${_sym_dir}/errquit.f)
ecce_fortran_app(cleansym
  SOURCES ${_sym_dir}/cleansym.f ${_sym_dir}/sym_avg.f ${_sym_dir}/spnames.f90 ${_sym_dir}/spgen.f90
          ${_sym_dir}/deter3.f ${_sym_dir}/dctr.f ${_sym_dir}/gensym.f ${_sym_dir}/errquit.f)

# ------------------------------------------------------------------------
# The JMS gateway: Java JMSDispatcher relay + ActiveMQ broker.
#
# Previously excluded entirely (see the old comment below, now corrected).
# This wires up the single-machine "client and server both here" case
# scripts/ecce/install_ecce used to configure interactively:
#   - java/ is built with the existing Ant script (java/build/build.xml)
#     into java/lib/ecce_jms.jar, installed alongside its activemq-all
#     client jar under <prefix>/java/lib.
#   - the ActiveMQ broker itself is the Debian-packaged `activemq` (not
#     vendored -- see the "Migrate 3rd Party Libraries" project history
#     for why this changed from a bundled 5.1.0 binary distribution: the
#     package's own /usr/bin/activemq launcher script respects the same
#     ACTIVEMQ_HOME/ACTIVEMQ_BASE overrides the old vendored one did,
#     with no root-owned single-instance service involved, so the
#     per-user launch model below didn't need to change, just which
#     broker it launches). Our own known-working config
#     (packaging/gateway/activemq.xml.ecce, installed to
#     server/activemq-conf/activemq.xml) is copied into place per-user by
#     ecce-gateway-start rather than relying on the package's own default
#     (/etc/activemq/, root-owned, wrong port) -- see that script for the
#     full rationale.
#   - siteconfig/jndi.properties (the JNDI/topic config JMSDispatcher and
#     every JMS-using C++ app read via $ECCE_HOME/siteconfig) is installed
#     with its provider URL repointed at localhost instead of the
#     hardcoded PNNL host it ships with.
#   - packaging/gateway/ecce-gateway-{start,stop,status} manage the broker
#     + JMSDispatcher as a per-user, per-machine background pair, replacing
#     the old ecce-admin/start_ecce_server csh scripts and the old
#     "ecce" launcher's one-gateway-per-toolbar-process model (this
#     package has no single controlling toolbar process - each app is
#     independently launchable - so the gateway is instead started lazily,
#     idempotently, by whichever app needs it first, and left running).
# ------------------------------------------------------------------------
find_program(ANT_EXECUTABLE ant)
if(ANT_EXECUTABLE)
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/java/lib/ecce_jms.jar
    COMMAND ${ANT_EXECUTABLE} -f build.xml all
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/java/build
    DEPENDS
      ${CMAKE_SOURCE_DIR}/java/src/gov/pnnl/emsl/ecce/jms/JMSDispatcher.java
      ${CMAKE_SOURCE_DIR}/java/src/gov/pnnl/emsl/ecce/jms/MessageHandler.java
      ${CMAKE_SOURCE_DIR}/java/src/gov/pnnl/emsl/ecce/jms/MessageHelper.java
      ${CMAKE_SOURCE_DIR}/java/src/gov/pnnl/emsl/ecce/jms/Execer.java
      ${CMAKE_SOURCE_DIR}/java/src/gov/pnnl/emsl/ecce/jms/Sender.java
      ${CMAKE_SOURCE_DIR}/java/src/gov/pnnl/emsl/ecce/jms/Target.java
    COMMENT "Building ecce_jms.jar (ant)"
  )
  add_custom_target(ecce_jms ALL DEPENDS ${CMAKE_SOURCE_DIR}/java/lib/ecce_jms.jar)
  install(FILES ${CMAKE_SOURCE_DIR}/java/lib/ecce_jms.jar
                ${CMAKE_SOURCE_DIR}/java/lib/activemq-all-5.1.0.jar
          DESTINATION java/lib
          COMPONENT client)
  set(ECCE_GATEWAY_JAVA_AVAILABLE TRUE)
else()
  message(WARNING "ant not found - the JMS gateway (java/) will not be built "
                   "or packaged; ecce-gateway-start will have nothing to run.")
  set(ECCE_GATEWAY_JAVA_AVAILABLE FALSE)
endif()

# The broker itself now comes from the Debian `activemq` package (see
# CPACK_DEBIAN_PACKAGE_DEPENDS below) rather than a vendored tarball --
# nothing to extract/install here, just our own config template (installed
# above, alongside the ecce-gateway-* scripts).
set(ECCE_GATEWAY_BROKER_AVAILABLE TRUE)

# Help content: BrowserHelp::initialize() (src/util/genutil/BrowserHelp.C)
# throws NullPointerException -- uncaught, crashing the whole app -- if
# ECCE_HELP isn't set in the environment when any Help button is clicked.
# None of the app wrapper scripts set it (an old gap, not a regression --
# the old scripts/runtime_setup this replaces apparently did). Extract the
# vendored help content this package already ships
# (data/admin/dataserver/help/eccehelp.tar) into the layout help.urls'
# entries expect (an "EcceHelp/" subdirectory containing homepage.html,
# etc. -- confirmed against the tarball's actual contents and against
# BrowserHelp::initialize()'s own p_filePrefix construction, which is
# hardcoded to ecceDataPath()/client/WebHelp/), then point ECCE_HELP at it
# in the wrapper scripts below. ECCE_HELP is an http:// URL served by the
# data server's Apache instance (see packaging/dataserver/httpd.conf.ecce's
# "/EcceHelp" Alias), not a raw file:// path -- the help content's own HTML
# uses absolute paths like "/EcceHelp/media/foo.gif" for every image and
# many links, which only resolve correctly against an HTTP server where
# "/EcceHelp" is a real URL root; opened as file://, a browser resolves
# that same absolute path against the filesystem root instead, breaking
# every embedded image. Entries in help.urls that need the CGI help
# backend (cgi-bin/help/cshelp, toolhelp) now work too -- see the
# ecce_help_cgi_patch step below, which patches the vendored Perl scripts'
# shebang and hardcoded PNNL path for this install, and
# ecce-dataserver-start, which seeds them into the per-user cgi-bin/
# alongside the pre-existing (still-unimplemented) account-creation CGI
# slot.
set(_ecce_help_tarball ${CMAKE_SOURCE_DIR}/data/admin/dataserver/help/eccehelp.tar)
if(EXISTS ${_ecce_help_tarball})
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp/homepage.html
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp
            ${CMAKE_COMMAND} -E tar xf ${_ecce_help_tarball}
    # The vendored tarball itself ships 3 genuinely corrupt entries under
    # basis/ and builder/ -- filenames containing literal HTML markup
    # (quotes, "<", ">", spaces, e.g. `overview_top"> <!-- #begineditable
    # "title.html`), not real help pages. Confirmed via `tar tf` against
    # the tarball directly -- this is decades-old corruption in the
    # vendored PNNL content itself, not something this build introduced.
    # Harmless for a DEB build (dpkg-deb tolerates the odd filename) but
    # a real, reproducible CI failure for RPM packaging specifically
    # ("CPack Error: Problem compressing the directory" -- rpmbuild's
    # file-list handling rejects them outright), and not guaranteed safe
    # on every filesystem/locale a DEB build might run under either.
    # None of these 3 files are reachable from help.urls or any real
    # navigation path (their "content" is a stray HTML comment fragment,
    # not a page), so deleting them costs nothing.
    COMMAND bash -c "find '${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp' -name '*begineditable*' -delete"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Extracting vendored ECCE help content"
  )
  add_custom_target(ecce_help ALL
    DEPENDS ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp/homepage.html)

  # The vendored toolhelp/cshelp CGI scripts (context-sensitive help,
  # invoked from within ECCE apps) hardcode PNNL's own 1990s server path
  # and Perl location. Neither script depends on CGI.pm (unlike the data
  # server's still-unported account-creation CGI) -- just the ancient but
  # still-bundled getopts.pl -- so patching these two values is enough to
  # make them work as-is against this install.
  set(_help_cgi_dir ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp/cgi-bin)
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/ecce-help-extracted/.help-cgi-patched
    COMMAND bash ${CMAKE_SOURCE_DIR}/packaging/dataserver/patch-help-cgi.sh ${_help_cgi_dir}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/ecce-help-extracted/.help-cgi-patched
    DEPENDS ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp/homepage.html
            ${CMAKE_SOURCE_DIR}/packaging/dataserver/patch-help-cgi.sh
    COMMENT "Patching vendored help CGI scripts (shebang, help_dir) for this install"
  )
  add_custom_target(ecce_help_cgi_patch ALL
    DEPENDS ${CMAKE_BINARY_DIR}/ecce-help-extracted/.help-cgi-patched)

  # Lives under data/client/ on disk (BrowserHelp's rare file:// fallback
  # path expects that layout - see BrowserHelp::initialize()) but this is
  # server content: served over HTTP by the data server's Apache instance
  # (httpd.conf.ecce's "/EcceHelp" Alias) and the source of the help CGI
  # scripts ecce-dataserver-start seeds into its own cgi-bin/ -- so it
  # goes in the server component, not client, when packages are split.
  install(DIRECTORY ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp
          DESTINATION data/client/WebHelp
          COMPONENT server)
  set(ECCE_HELP_AVAILABLE TRUE)
else()
  message(WARNING "No vendored help tarball at ${_ecce_help_tarball} - "
                   "Help will still work (won't crash) but ECCE_HELP will "
                   "point at a location with no content.")
  set(ECCE_HELP_AVAILABLE FALSE)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/siteconfig/jndi.properties)
  file(READ ${CMAKE_SOURCE_DIR}/siteconfig/jndi.properties _jndi_contents)
  # The checked-in template points at PNNL's own broker; repoint it at the
  # broker this package installs and starts locally (see ecce-gateway-start
  # for the matching port).
  string(REGEX REPLACE
    "java\\.naming\\.provider\\.url[ \t]*=[ \t]*[^\r\n]*"
    "java.naming.provider.url = tcp://localhost:8088"
    _jndi_contents "${_jndi_contents}")
  file(WRITE ${CMAKE_BINARY_DIR}/siteconfig-local/jndi.properties "${_jndi_contents}")
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/siteconfig/DataServers)
  file(READ ${CMAKE_SOURCE_DIR}/siteconfig/DataServers _dataservers_contents)
  # Same idea as jndi.properties above, for the data server (see
  # packaging/dataserver/ below): the checked-in template points at the
  # real PNNL production server; repoint it at the data server this
  # package installs and starts locally (see ecce-dataserver-start for
  # the matching port). Both the default <Url> and the <BasisSet> URL
  # share this hostname, and nothing else in the file does, so one
  # substring replace covers both.
  string(REPLACE "eccetera.emsl.pnl.gov" "localhost"
    _dataservers_contents "${_dataservers_contents}")
  file(WRITE ${CMAKE_BINARY_DIR}/siteconfig-local/DataServers "${_dataservers_contents}")
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/siteconfig/site_runtime)
  file(READ ${CMAKE_SOURCE_DIR}/siteconfig/site_runtime _site_runtime_contents)
  # ECCE_AUTO_ACCOUNTS drives EDSIServerCentral::checkServerSetup()'s
  # self-service account-creation POST against cgi-bin/ecce_accounts,
  # which this package doesn't install (the Perl CGI auto-provisioning
  # flow is deliberately deferred, see packaging/dataserver/ -- a missing
  # CGI degrades gracefully in the client, but there's no reason to
  # attempt a request that can never succeed). Flip it off; accounts are
  # created with ecce-dataserver-adduser instead.
  string(REGEX REPLACE
    "ECCE_AUTO_ACCOUNTS[ \t]+yes"
    "ECCE_AUTO_ACCOUNTS no"
    _site_runtime_contents "${_site_runtime_contents}")
  file(WRITE ${CMAKE_BINARY_DIR}/siteconfig-local/site_runtime "${_site_runtime_contents}")
endif()

# Single directory install for everything in siteconfig/ *except* the
# four files rewritten above, which install from their resolved
# siteconfig-local/ copies instead. (Deliberately one combined
# install(DIRECTORY) rather than one per rewritten file above -- two
# separate install(DIRECTORY siteconfig/ ...) calls with different
# exclude sets would both run at install time and race on which version
# of the non-excluded files wins.)
# siteconfig/ is entirely client-side config: it tells a CLIENT install
# where to find servers (DataServers, RemoteServer/, jndi.properties),
# what compute resources/queues exist (Machines, Queues, CONFIG-Examples),
# and how it should behave (site_runtime). Nothing under server/ reads
# any of it -- the data server and broker are configured from
# packaging/dataserver and packaging/gateway instead. So it goes whole
# into the client component with no split needed.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/siteconfig/ DESTINATION siteconfig
        COMPONENT client
        PATTERN "jndi.properties" EXCLUDE
        PATTERN "RemoteServer" EXCLUDE
        PATTERN "DataServers" EXCLUDE
        PATTERN "site_runtime" EXCLUDE)
install(FILES ${CMAKE_BINARY_DIR}/siteconfig-local/jndi.properties
              ${CMAKE_BINARY_DIR}/siteconfig-local/DataServers
              ${CMAKE_BINARY_DIR}/siteconfig-local/site_runtime
        DESTINATION siteconfig
        COMPONENT client)

# ecce-gateway-{start,stop,status,reap}: manage the per-session
# JMSDispatcher relay (always client-side, needs Java) AND, when it finds
# server/activemq-conf/activemq.xml installed alongside it, the local
# ActiveMQ broker too. A client-only install still needs these scripts
# (the relay is unconditional) -- see ecce-gateway-start's own check for
# what it does when the broker half of that config is missing.
install(PROGRAMS
          ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-gateway-start
          ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-gateway-stop
          ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-gateway-status
          ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-gateway-reap
          ${CMAKE_SOURCE_DIR}/packaging/nwchem/ecce-nwchem-datadir
        DESTINATION bin
        COMPONENT client)
# The broker's own config: only meaningful on a machine running the
# broker, i.e. only needed by ecce-server. ecce-gateway-start checks for
# this exact path to decide whether a local broker exists to start.
install(FILES ${CMAKE_SOURCE_DIR}/packaging/gateway/activemq.xml.ecce
        DESTINATION server/activemq-conf
        RENAME activemq.xml
        COMPONENT server)

# Deployment mode 3, the site's shared broker (GETTING_STARTED.md,
# "Deployment modes"). ecce-broker-setup writes siteconfig/SharedBroker,
# which the clients read, so it goes with them; ecce-broker-run and the
# unit run the broker, so they go with its config. The unit is installed
# beside that config rather than into /usr/lib/systemd/system, so a
# non-root prefix install still works; ecce-broker-setup prints the
# `systemctl link` for it.
install(PROGRAMS ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-broker-setup
        DESTINATION bin
        COMPONENT client)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-broker-run
        DESTINATION bin
        COMPONENT server)
configure_file(${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-broker.service.in
               ${CMAKE_BINARY_DIR}/systemd/ecce-broker.service @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/systemd/ecce-broker.service
        DESTINATION server/systemd
        COMPONENT server)
set(_bs_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-broker-setup)
file(WRITE ${_bs_wrapper}
"#!/bin/bash
export ECCE_HOME=\${ECCE_HOME:-${ECCE_HOME_DIR}}
exec \"$ECCE_HOME/bin/ecce-broker-setup\" \"$@\"
")
install(PROGRAMS ${_bs_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT client)

# Also put the gateway control scripts on PATH directly (like the ecce-<app>
# wrappers) so `ecce-gateway-status` etc. work without needing $ECCE_HOME.
foreach(_gw start stop status reap)
  set(_gw_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-gateway-${_gw})
  file(WRITE ${_gw_wrapper}
"#!/bin/bash
export ECCE_HOME=\${ECCE_HOME:-${ECCE_HOME_DIR}}
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
exec \"$ECCE_HOME/bin/ecce-gateway-${_gw}\" \"$@\"
")
  install(PROGRAMS ${_gw_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT client)
endforeach()

# ------------------------------------------------------------------------
# ECCE data server (EDSI/WebDAV) - the other half of old ECCE's server
# side, independent of the JMS gateway above. Runs as a per-user,
# non-root Debian apache2 instance under $ECCE_REALUSERHOME/.ECCE/
# dataserver -- see packaging/dataserver/httpd.conf.ecce and
# ecce-dataserver-start for the full rationale. Requires the apache2 and
# apache2-utils packages at runtime (see CPACK_DEBIAN_PACKAGE_DEPENDS
# below); nothing to vendor/extract here since both are ordinary Debian
# packages, unlike ActiveMQ above.
# ------------------------------------------------------------------------
# The dataserver's own start/stop/status/adduser: only meaningful on a
# machine actually running the Apache instance, so these -- and the
# Apache config they resolve -- are server-only.
install(PROGRAMS
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-start
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-stop
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-status
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-adduser
        DESTINATION bin
        COMPONENT server)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/dataserver/httpd.conf.ecce
        DESTINATION server/httpd-conf
        COMPONENT server)
# ecce-remote-setup and ecce-diagnose, by contrast, run on and describe
# the CLIENT installation (ecce-remote-setup writes this client's own
# siteconfig/RemoteServer/; ecce-diagnose collects this install's own
# state) -- they belong with ecce-client even though they live under
# packaging/dataserver/ and packaging/ for historical reasons.
install(PROGRAMS
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-remote-setup
          ${CMAKE_SOURCE_DIR}/packaging/ecce-diagnose
        DESTINATION bin
        COMPONENT client)

foreach(_ds start stop status adduser)
  set(_ds_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-dataserver-${_ds})
  file(WRITE ${_ds_wrapper}
"#!/bin/bash
export ECCE_HOME=\${ECCE_HOME:-${ECCE_HOME_DIR}}
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
export ECCE_REALUSER=\${ECCE_REALUSER:-\$(id -un)}
exec \"$ECCE_HOME/bin/ecce-dataserver-${_ds}\" \"$@\"
")
  install(PROGRAMS ${_ds_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT server)
endforeach()

# The same wrapper for the central-server setup helper, so an
# administrator can type "ecce-remote-setup <host>" without knowing
# where the installation lives.
# Same for the diagnostic collector, which is only ever run by
# somebody who has already hit a problem and should not also have to
# find out where ECCE was installed.
set(_diag_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-diagnose)
file(WRITE ${_diag_wrapper}
"#!/bin/bash
export ECCE_HOME=\${ECCE_HOME:-${ECCE_HOME_DIR}}
exec \"$ECCE_HOME/bin/ecce-diagnose\" \"$@\"
")
install(PROGRAMS ${_diag_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT client)

set(_remote_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-remote-setup)
file(WRITE ${_remote_wrapper}
"#!/bin/bash
export ECCE_HOME=\${ECCE_HOME:-${ECCE_HOME_DIR}}
exec \"$ECCE_HOME/bin/ecce-remote-setup\" \"$@\"
")
install(PROGRAMS ${_remote_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT client)

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
  autosym getfrag genmol genmollat cleansym symops)

install(TARGETS ${ECCE_GUI_APPS} ${ECCE_CLI_APPS} RUNTIME DESTINATION bin
        COMPONENT client)
# data/ splits cleanly along its own top-level directories: data/client
# is exactly what the GUI apps/scripts above read locally via
# ecceDataPath() (edml manifests, templates, pixmaps, the local
# StructureLibrary browsed by New Structure -- see StructLib.C), so it
# is client content; data/admin is exclusively read by
# ecce-dataserver-start (basis-set seed, the data-server seed tar, the
# vendored help tarball/CGI source) to build the server's own document
# root, and nothing else ever opens it, so it is server content.
if(EXISTS ${CMAKE_SOURCE_DIR}/data/client)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/client DESTINATION data
          COMPONENT client)
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/data/admin)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/admin DESTINATION data
          COMPONENT server)
endif()

# scripts/parsers: per-code basis-set translation and I/O-format scripts
# (e.g. NWChem.edml's <BasisTranslationScript>std2NWChem</BasisTranslationScript>,
# looked up via JCode::getScript() -> TGBSConfig.C, which shells out to
# whichever one the active code needs). Genuinely modern, working Perl
# already (use strict, Getopt::Std, confirmed syntax-clean on this
# system's Perl 5.40) -- this was a pure packaging gap, not a porting
# job, unlike scripts/gbs* (real Python 2, still unported -- separate,
# not currently reachable via any wired-up UI path, left alone for now).
# Distinct from the scripts/ecce/ebuilder/eviewer-class launcher scripts
# deliberately not ported (see the wrapper comment below) -- this is
# actual calculation-setup logic, not a launcher.
if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/parsers)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/parsers
          DESTINATION scripts
          USE_SOURCE_PERMISSIONS
          COMPONENT client)
endif()

# scripts/processmachine: Machine Registration's save handler
# (WxMachineRegister::machineChangeButtonClickedCB() shells out to
# $ECCE_HOME/scripts/processmachine, CGI-POST-style via stdin/
# CONTENT_LENGTH). Same story as scripts/parsers above: genuinely modern,
# syntactically clean Perl (no CGI.pm dependency, no hardcoded EMSL
# paths -- its one CVS/svn integration path is already commented out in
# the script itself), just never installed -- a pure packaging gap. Also
# already writes to the correct place for a non-admin user without any
# further changes: it branches on the "siteconfig" form field (site-wide
# vs per-user registration) between $ECCE_HOME/siteconfig (read-only,
# admin-only by design -- matches this build's other shared-vs-per-user
# split, e.g. the JMS broker/data server) and $ECCE_REALUSERHOME/.ECCE
# (always writable) -- Gateway's own "Machine Registration" tool invokes
# `machregister` with no `-admin` flag, so ordinary use already resolves
# to the writable per-user path.
if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/processmachine)
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/scripts/processmachine
          DESTINATION scripts
          COMPONENT client)
endif()

# scripts/gensub: job-submission-script generator, invoked as a bare
# command (Launch.C:2372, `p_localconn->execout("gensub -v  -p subParams",
# ...)`, relying entirely on $PATH -- unlike processmachine/std2NWChem
# above, this one has no full-path fallback at all, so it needs
# $ECCE_HOME/scripts (not just scripts/parsers) on PATH; see the wrapper
# template below. Same diligence as the other scripts/ fixes: perl -c
# clean, no hardcoded EMSL paths, reads only already-installed,
# already-readable package content ($ECCE_HOME/siteconfig,
# $ECCE_HOME/data/client/config/Version) -- a pure packaging gap, not a
# porting job.
if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/gensub)
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/scripts/gensub
          DESTINATION scripts
          COMPONENT client)
endif()


# scripts/eccejobmonitor: found via an audit of every "$ECCE_HOME/scripts/"
# reference in src/ after the gensub fix, prompted by hitting three of
# these packaging gaps in a row live (std2NWChem, processmachine,
# gensub) -- rather than wait to hit the next one, grepped for
# `"/scripts/` across the whole tree and checked every hit. This is the
# one other real gap: Launch.C's moveJobMonitoringFiles()/similar SCPs
# $ECCE_HOME/scripts/eccejobmonitor to the remote machine as part of job
# submission (a full-path reference, not $PATH-dependent, so no wrapper
# change needed) then runs `perl eccejobmonitor -configFile ...` there to
# report job progress/completion back to ECCE -- would have failed the
# same way gensub did, just one step later in the launch sequence.
# Confirmed portable: perl -c clean, only core modules (POSIX, Socket, no
# CPAN deps), no hardcoded EMSL paths despite being a large (4800+ line)
# legacy script.
#
# The rest of that audit came back clean: everything else under scripts/
# (ecce, ebuilder, eviewer, epmf, eprp, ecce_checks, ecce_env,
# runtime_setup*, sysdir, gbsDAVConverter, gbsNWChemConverter,
# gbsDescriber, pmf_gui.py, prp_gui.py) has zero references from any
# compiled C++ code -- they're either top-level launcher scripts this
# build's own ecce-<app> wrappers already supersede (documented rationale
# above), or manual/admin-only tooling invoked by a human, not by ECCE
# itself. scripts/codereg was wrongly written off in an earlier pass of
# this same audit as "a real Python 2/wxPython port, not a packaging
# gap" -- corrected below, it's actually the same packaging-gap shape as
# everything else here, just with one extra piece (a real apt
# dependency).
if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/eccejobmonitor)
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/scripts/eccejobmonitor
          DESTINATION scripts
          COMPONENT client)
endif()

# Named by `ecce --help`, so it has to be on the machine.
install(FILES ${CMAKE_SOURCE_DIR}/docs/ENVIRONMENT.md
        DESTINATION doc
        COMPONENT client)

# scripts/codereg: Theory/Runtype "Details" dialogs (JCode::
# getTheoryRunTypeEditorNames(), src/dsm/xml/JCode.C). Wrongly assessed in
# an earlier pass of the audit above as unported Python 2/wxPython
# Classic and left alone -- actually already Python-3-ported by an
# earlier session (commit 32d2806: all 21 scripts/*.py files, including
# these, confirmed compiling clean under python3, `import wx` not the
# Classic `from wxPython.wx import *`). The two real gaps: (1) this
# directory was never installed (same shape as every other scripts/
# fix above), and (2) JCode.C hardcoded `python` (not `python3` -- fixed
# alongside this) plus a legacy `LD_LIBRARY_PATH=../3rdparty/...` prefix
# left over from the old vendored-3rdparty multi-platform layout, neither
# of which resolves to anything in this build's flat layout (also
# removed -- the distro's own python3-wxgtk4.0 package, added as a real
# apt dependency below, resolves its own shared-library deps normally).
# NOT verified against the actual wxPython Phoenix API surface inside
# these scripts -- python3-wxgtk4.0 isn't installed in this sandbox and
# can't be without sudo, so whether the dialog *logic* itself (not just
# imports) is Classic/Phoenix-compatible is still an open question the
# 32d2806 commit message already flagged and this pass couldn't close.
# PATTERN EXCLUDE __pycache__: install(DIRECTORY) takes whatever is in the
# source tree, so any developer who has run these scripts (or merely
# python3 -m py_compile on them, as a syntax check) leaves a __pycache__
# directory behind and its .pyc files get packaged into the .deb. They are
# interpreter-version-stamped (cpython-313 etc.) and serve no purpose in
# the package; exclude them so the package contents can't depend on
# whether the build host happened to byte-compile anything first.
if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/codereg)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/codereg
          DESTINATION scripts
          USE_SOURCE_PERMISSIONS
          COMPONENT client
          PATTERN "__pycache__" EXCLUDE)
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
# ECCE_REALUSER/ECCE_REALUSERHOME are required (fatal EE_RT_ASSERT if unset
# - see src/util/genutil/Ecce.C realUser()/realUserHome()) by every app, not
# just the messaging-using ones; the previous wrapper never set them, so
# every app would have aborted on first touching preferences/cache paths.
# HOST must also be exported: the Java gateway writes its UDP port file
# keyed by -DECCE_HOST (see JMSDispatcher.saveServerPort), while the C++
# side reads it back keyed by the plain "HOST" env var (DatagramUtil::
# loadServerPort, src/util/jms/JMSMessage.C) - tcsh set this automatically
# for the old launcher; a plain sh/bash wrapper has to do it explicitly or
# the two sides look for different filenames and JMS silently never
# connects.
set(ECCE_MESSAGING_APPS ${ECCE_GUI_APPS} eccejobmaster eccejobstore)

foreach(_app ${ECCE_GUI_APPS} ${ECCE_CLI_APPS})
  set(_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-${_app})
  list(FIND ECCE_MESSAGING_APPS ${_app} _msg_idx)
  if(_msg_idx GREATER -1)
    # Started under -remote too: every app needs this session's relay, and
    # ecce-gateway-start itself skips the local broker in that mode (#167).
    # It fails only when there is no relay for this display, and every one
    # of these apps would then abort on the missing port file
    # (JMSMessage.C), so stop here with its message instead.
    set(_gateway_start "if [ -z \"$ECCE_NO_MESSAGING\" ]; then
  \"$ECCE_HOME/bin/ecce-gateway-start\" || { _rc=$?; echo \"${_app}: not started -- no messaging relay for this session (see above)\" >&2; exit $_rc; }
fi")
    # Same app set as messaging: apps that need the JMS gateway also need
    # data-server (EDSI/WebDAV) access, per EDSIServerCentral::
    # checkServerSetup() -- see ecce-dataserver-start.
    # Split packaging (ECCE_SPLIT_PACKAGES): ecce-dataserver-start only
    # exists at all when the ecce-server package is installed alongside
    # this ecce-client one. A client-only install (talking to someone
    # else's central server, or none yet configured) must not try to
    # spawn it -- and must say clearly why not, rather than a bare
    # "No such file or directory" swallowed by the trailing `|| true`.
    set(_dataserver_start "if [ -z \"$ECCE_NO_DATASERVER\" ] && [ -z \"$ECCE_REMOTE_SERVER\" ]; then
  if [ -x \"$ECCE_HOME/bin/ecce-dataserver-start\" ]; then
    \"$ECCE_HOME/bin/ecce-dataserver-start\" || true
  else
    echo \"${_app}: no local data server installed ($ECCE_HOME/bin/ecce-dataserver-start missing -- ecce-server not installed) and ECCE_REMOTE_SERVER is not set.\" >&2
    echo \"${_app}: install ecce-server for a local data server, or point this client at a central one (ecce-remote-setup, then 'ecce -remote').\" >&2
  fi
fi")
    # Issue #102: the app is run, not exec'd, so this wrapper shell
    # outlives it and its EXIT trap runs however the app terminated --
    # including the SIGABRT that stranded the broker and a JMSDispatcher
    # for ten days on beryllium. ecce-gateway-reap --if-idle does nothing
    # while any other ECCE app is still up on this display, so only the
    # last one out stops the display's relay, and the user's last session
    # stops a per-user broker (never a server's or the site's shared one).
    # The data server stays up until an explicit Quit and Stop Server.
    # bash's own "Terminated" report for a SIGTERM (Quit and Stop Server
    # ends the apps that way) is noise, so the shell's stderr is muted
    # around the app; any other signal is still reported, by name.
    set(_app_exec "trap 'ECCE_REAP_QUIET=1 \"$ECCE_HOME/bin/ecce-gateway-reap\" --if-idle >/dev/null 2>&1 || true' EXIT
exec 3>&2
{ \"$ECCE_HOME/bin/${_app}\" \"$@\" 2>&3 3>&-; } 2>/dev/null
_status=$?
if [ $_status -gt 128 ] && [ $_status -ne 143 ]; then
  echo \"${_app}: ended by SIG$(kill -l $((_status - 128)) 2>/dev/null)\" >&2
fi
exit $_status")
  else()
    set(_gateway_start "")
    set(_dataserver_start "")
    set(_app_exec "exec \"$ECCE_HOME/bin/${_app}\" \"$@\"")
  endif()
  file(WRITE ${_wrapper}
"#!/bin/bash
export ECCE_HOME=\${ECCE_HOME:-${ECCE_HOME_DIR}}
export ECCE_REALUSER=\${ECCE_REALUSER:-$(id -un)}
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
export HOST=\${HOST:-$(hostname)}
export DISPLAY=\${DISPLAY:-:0}
export ECCE_HELP=\${ECCE_HELP:-http://localhost:8096/}
# Where NWChem's force-field data lives is a per-distribution decision,
# so it is resolved when the app starts rather than written into the
# package at configure time -- siteconfig/DataServers refers to it as
# $ECCE_NWCHEM_DATA and SFile expands any $VAR. Unset (no NWChem, or an
# unrecognised layout) leaves DataServers' paths visibly broken rather
# than pointing somewhere wrong, and `ecce-nwchem-datadir --verbose`
# says where it looked.
export ECCE_NWCHEM_DATA=\${ECCE_NWCHEM_DATA:-$(\"$ECCE_HOME/bin/ecce-nwchem-datadir\" 2>/dev/null)}
# wxWidgets 3.2's GTK3 backend was ported/tested against X11 window-
# sizing semantics (synchronous configure, geometry hints honored on
# first show); under a native GNOME Wayland session, GTK3's Wayland
# backend negotiates initial window size with the compositor
# asynchronously instead, and several of this app's frames (confirmed
# live: the Gateway launcher toolbar, the Authentication dialog) come
# up with a collapsed/garbled size as a result -- forcing XWayland
# (the same rootless X11 compat layer already used for DISPLAY=:0
# above) sidesteps it, same fix commonly used for other legacy GTK
# apps under Wayland. Only takes effect on an actual Wayland session;
# harmless no-op under a plain X11 session, and left overridable.
if [ \"\$XDG_SESSION_TYPE\" = \"wayland\" ]; then
  export GDK_BACKEND=\${GDK_BACKEND:-x11}
fi
export PATH=\"$ECCE_HOME/scripts:$ECCE_HOME/scripts/parsers:$PATH\"
# THE CENTRAL-SERVER MODEL.  ECCE was originally deployed with one
# server for a whole group -- a shared data server and broker that many
# clients and many users connect to -- and the old csh launcher selected
# it with \"-remote\", which set ECCE_REMOTE_SERVER and made the client
# read siteconfig/RemoteServer/{DataServers,site_runtime} instead of the
# local ones. The C++ side still honours that variable in full
# (Ecce::ecceAutoAccounts, ecceStoreTrajectories, EDSIServerCentral); it
# was only these wrappers that did not know about it, and they would
# start a per-user broker and Apache underneath a client that wanted
# neither: under -remote only the session relay starts, pointed at the
# central broker (#167). Recognised here so \"ecce -remote\" means what
# it always did.
for _arg in \"$@\"; do
  if [ \"$_arg\" = \"-remote\" ]; then export ECCE_REMOTE_SERVER=1; fi
done
${_gateway_start}
${_dataserver_start}
${_app_exec}
")
  install(PROGRAMS ${_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT client)
endforeach()

# Plain "ecce" is what people actually expect to type -- gateway is the
# top-level launcher every other app is meant to be reached through (see
# CLAUDE.md's "the two background services" section), but until now only
# "ecce-gateway" existed, named that way to dodge the PATH-collision risk
# every ecce-<app> wrapper above already guards against. This just execs
# that same wrapper, so it inherits its env setup rather than duplicating
# it.
set(_ecce_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce)
configure_file(${CMAKE_SOURCE_DIR}/packaging/ecce.in ${_ecce_wrapper} @ONLY)
install(PROGRAMS ${_ecce_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT client)

# "Viewer" isn't a separate build target -- the legacy scripts/viewer
# csh wrapper (not packaged, along with the rest of scripts/ -- see the
# ECCE_GUI_APPS wrapper comment above) just re-execs builder with two
# extra env vars that put it into a read-only/view-only mode. Gateway's
# "Viewer" toolbar button invokes this by name via ResourceDescriptor.xml's
# <InvokeArg>, which now points at ecce-viewer (not the legacy bare
# "viewer" -- avoids exactly the PATH-collision risk the ecce-<app>
# naming convention above already guards against; see ResourceDescriptor*
# .xml's InvokeArg edits alongside this).
set(_viewer_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-viewer)
file(WRITE ${_viewer_wrapper}
"#!/bin/bash
export ECCE_HOME=\${ECCE_HOME:-${ECCE_HOME_DIR}}
export ECCE_REALUSER=\${ECCE_REALUSER:-$(id -un)}
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
export HOST=\${HOST:-$(hostname)}
export DISPLAY=\${DISPLAY:-:0}
export ECCE_HELP=\${ECCE_HELP:-http://localhost:8096/}
# See the ecce-<app> wrappers above: resolved at runtime because the
# location is a per-distribution decision.
export ECCE_NWCHEM_DATA=\${ECCE_NWCHEM_DATA:-$(\"$ECCE_HOME/bin/ecce-nwchem-datadir\" 2>/dev/null)}
export PATH=\"$ECCE_HOME/scripts:$ECCE_HOME/scripts/parsers:$PATH\"
export ECCE_INVOKE_VIEWER=1
export ECCE_INVOKE_FROMECCE=1
if [ -z \"$ECCE_NO_MESSAGING\" ]; then
  \"$ECCE_HOME/bin/ecce-gateway-start\" || { _rc=$?; echo \"ecce-viewer: not started -- no messaging relay for this session (see above)\" >&2; exit $_rc; }
fi
if [ -z \"$ECCE_NO_DATASERVER\" ] && [ -z \"$ECCE_REMOTE_SERVER\" ]; then
  if [ -x \"$ECCE_HOME/bin/ecce-dataserver-start\" ]; then
    \"$ECCE_HOME/bin/ecce-dataserver-start\" || true
  else
    echo \"ecce-viewer: no local data server installed (ecce-server not installed) and ECCE_REMOTE_SERVER is not set.\" >&2
  fi
fi
trap 'ECCE_REAP_QUIET=1 \"$ECCE_HOME/bin/ecce-gateway-reap\" --if-idle >/dev/null 2>&1 || true' EXIT
exec 3>&2
{ \"$ECCE_HOME/bin/builder\" \"$@\" 2>&3 3>&-; } 2>/dev/null
_status=$?
if [ $_status -gt 128 ] && [ $_status -ne 143 ]; then
  echo \"ecce-viewer: ended by SIG$(kill -l $((_status - 128)) 2>/dev/null)\" >&2
fi
exit $_status
")
install(PROGRAMS ${_viewer_wrapper} DESTINATION ${ECCE_WRAPPER_DESTINATION} COMPONENT client)

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
# dpkg-shlibdeps only catches linked .so's, not a spawned subprocess --
# apache2 (the data server, see ecce-dataserver-start above) is invoked
# as an external binary, not linked, so it has to be listed explicitly.
# python3-wxgtk4.0 is the same story for scripts/codereg's Theory/Runtype
# Details dialogs (see the scripts/codereg install() block above) -- a
# spawned `python3 .../codereg/*.py`, not a linked dependency of any
# ECCE binary, so dpkg-shlibdeps can't see it either.
# csh: ECCE's local job sessions and scripts run csh; bsd-csh and tcsh
# both work.
# perl: the whole job pipeline (ai.<code>, *.expt, gensub,
# eccejobmonitor, processmachine) is Perl, and Getopt::Std and
# Sys::Hostname are in perl-modules, which perl-base alone lacks.
# xterm: editing an input file with the default editor (vi) opens it in
# an xterm (UserEditor.C).
set(CPACK_DEBIAN_PACKAGE_DEPENDS "apache2, apache2-utils, python3-wxgtk4.0, activemq, csh | tcsh, perl, xterm")
# NWChem is a real Debian package (confirmed working: /usr/bin/nwchem,
# symlinked to nwchem.openmpi by the package itself) and the auto-register-
# this-machine flow (RunMgmt::registerLocalMachine()) defaults new machine
# registrations to it -- Recommends (not Depends) so `apt-get install
# ecce`/`apt-get install -f` pulls it in by default, matching that
# auto-registration default, without unconditionally forcing NWChem +
# its MPI/Fortran runtime dependencies onto anyone who explicitly opts
# out via --no-install-recommends.
# openssh-client: RCommand drives ssh/scp/sftp for remote machines;
# a purely local install never uses it.
# imagemagick: optional viewer-snapshot export (ImageConverter.C).
# www-browser: the Help menu (BrowserHelp.C).
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "nwchem, openssh-client")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "imagemagick, www-browser")
# RelWithDebInfo keeps -g everywhere (useful while porting - see the
# CMAKE_BUILD_TYPE default above), which makes for a needlessly huge .deb;
# strip debug symbols out of the packaged copies only, not the build tree.
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# ---------------------------------------------------------------------------
#  RPM, for Fedora/RHEL/Rocky.  Built only when rpmbuild is present, so a
#  Debian box without it still packages exactly as before rather than
#  failing at the end of a long build.  Install it with `apt install rpm`.
#
#  The dependency list is NOT the Debian one with different spellings.
#  Real differences:
#
#    * apache2/apache2-utils are httpd/httpd-tools.
#    * python3-wxgtk4.0 is python3-wxpython4.
#    * ActiveMQ is not packaged for RHEL, Rocky or Fedora at all.  It
#      cannot be a hard Requires without making the package
#      uninstallable everywhere, so it is a Recommends (rpm >= 4.12) and
#      GETTING_STARTED covers installing it by hand.  A missing broker is
#      not silent -- ecce-gateway-start reports it.
#    * openssh-client is openssh-clients.
#
#  perl and xterm are Requires here for the same reasons given beside
#  the Debian CPACK_DEBIAN_PACKAGE_DEPENDS above (the scripts/ Perl
#  pipeline, and UserEditor.C's "vi" default running inside xterm) --
#  both package names are identical on Fedora/RHEL/Rocky.
#
#  nwchem is in Fedora but not in RHEL/Rocky without EPEL, so it stays a
#  weak dependency there too, matching the Debian Recommends. ImageMagick
#  and firefox match the Debian Suggests (ImageConverter.C's optional
#  mogrify/convert export, and BrowserHelp.C's Help-menu fallback).
find_program(RPMBUILD_EXECUTABLE rpmbuild)
if(RPMBUILD_EXECUTABLE)
  list(APPEND CPACK_GENERATOR "RPM")
  set(CPACK_RPM_PACKAGE_LICENSE "ECL-2.0")
  set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
  set(CPACK_RPM_PACKAGE_URL "https://github.com/FriendsofECCE/ECCE")
  set(CPACK_RPM_PACKAGE_REQUIRES "httpd, httpd-tools, python3-wxpython4, tcsh, perl, xterm")
  set(CPACK_RPM_PACKAGE_SUGGESTS "activemq, nwchem, openssh-clients, ImageMagick, firefox")
  set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
  #  /opt/ecce is ours, but /opt itself is owned by filesystem(5).  Without
  #  this the package claims /opt and conflicts with it.
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/opt")
  #  Built on Debian, so let rpmbuild work out library requirements from
  #  the binaries rather than trusting a hand-written list that would be
  #  wrong on both distributions.
  set(CPACK_RPM_PACKAGE_AUTOREQ ON)
  set(CPACK_RPM_PACKAGE_AUTOPROV OFF)
  message(STATUS "rpmbuild found: cpack will also produce an .rpm")
else()
  message(STATUS "rpmbuild not found: cpack will produce a .deb only "
                 "(apt install rpm to enable RPM packaging)")
endif()

# The site configuration an admin edits (sudo ecce -admin, ecce-remote-
# setup's copy of a server's machine list, #188) must survive upgrades,
# so it is a Debian conffile / RPM %config(noreplace). jndi.properties
# too: ecce-remote-setup points it at the central broker, and an upgrade
# resetting it to localhost would silently break every -remote client.
set(_ecce_siteconfig_conffiles
  Machines Queues QueueManagers DataServers site_runtime jndi.properties
  remote_shells.site submit.site AppColors StartupMessage NewUserMessage
  CONFIG.dummy dummy.Q example-slurm.Q
  CONFIG-Examples/CONFIG.loadleveler CONFIG-Examples/CONFIG.lsf
  CONFIG-Examples/CONFIG.mpp2 CONFIG-Examples/CONFIG.pbs
  CONFIG-Examples/CONFIG.sge CONFIG-Examples/CONFIG.workstation)
set(_ecce_conffiles_debian ${CMAKE_BINARY_DIR}/conffiles/conffiles)
set(_ecce_conffiles_content "")
set(_ecce_rpm_filelist "")
foreach(_f ${_ecce_siteconfig_conffiles})
  string(APPEND _ecce_conffiles_content
         "${CPACK_PACKAGING_INSTALL_PREFIX}/siteconfig/${_f}\n")
  list(APPEND _ecce_rpm_filelist
       "%config(noreplace) ${CPACK_PACKAGING_INSTALL_PREFIX}/siteconfig/${_f}")
endforeach()
file(WRITE ${_ecce_conffiles_debian} "${_ecce_conffiles_content}")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${_ecce_conffiles_debian}")
set(CPACK_RPM_USER_FILELIST "${_ecce_rpm_filelist}")
# siteconfig/ is all client-side, so under the split it goes with ecce-client.
set(CPACK_DEBIAN_CLIENT_PACKAGE_CONTROL_EXTRA "${_ecce_conffiles_debian}")
set(CPACK_RPM_CLIENT_USER_FILELIST "${_ecce_rpm_filelist}")

# ---------------------------------------------------------------------------
# Split packaging (ECCE_SPLIT_PACKAGES). Every install() rule above already
# carries COMPONENT client/server; this block only decides whether CPack
# acts on those tags at all, and what each resulting package's own
# metadata is. No ecce-common component: auditing every install() rule
# above by which side reads it, nothing was needed by both -- siteconfig/
# and scripts/ are entirely client-read, data/admin and the Apache/broker
# config are entirely server-read. A common package would have shipped
# empty, so there isn't one.
# ---------------------------------------------------------------------------
if(ECCE_SPLIT_PACKAGES)
  # The version boundary below which a plain "ecce" install is the OLD
  # monolithic package -- ecce-client/ecce-server each Break+Replace it
  # so dpkg can upgrade one straight into the split pair instead of
  # refusing with a file-already-owned-by-another-package conflict. Kept
  # as its own variable (rather than reusing _ecce_deb_version directly
  # in the Breaks string) because the branch is still versioned 8.17.0 --
  # whichever release first ships split packages is the real boundary,
  # and that is a decision for whoever cuts that release, not something
  # to hardcode here now.
  set(ECCE_MONOLITHIC_BREAK_VERSION "${_ecce_deb_version}" CACHE STRING
      "ecce-client/ecce-server Break+Replace the monolithic ecce package for versions strictly below this (Debian: 'ecce (<< VERSION)'; RPM: 'ecce < VERSION'). Defaults to this build's own version, i.e. the first split release breaks/replaces every version older than itself.")

  # CPack falls back to the monolithic-build globals below (Depends,
  # Recommends, Suggests) for any component that doesn't set its own --
  # confirmed live: without this, ecce-server and the ecce metapackage
  # both picked up "Recommends: nwchem" from CPACK_DEBIAN_PACKAGE_RECOMMENDS
  # above, which is wrong for both (a headless server box, and a
  # metapackage that should only Depend on the two real components).
  # CPACK_DEBIAN_PACKAGE_SUGGESTS ("imagemagick, www-browser" -- both
  # client-only, ImageConverter.C's viewer export and the Help menu) leaks
  # the same way if left unset: it showed up as a Suggests on ecce-server
  # and the ecce metapackage too, found the same way as the Recommends
  # case above. Clear all three globals here so every component's
  # metadata is exactly what this block sets and nothing leaks in from
  # the monolithic settings above.
  unset(CPACK_DEBIAN_PACKAGE_DEPENDS)
  unset(CPACK_DEBIAN_PACKAGE_RECOMMENDS)
  unset(CPACK_DEBIAN_PACKAGE_SUGGESTS)

  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_COMPONENTS_ALL client server meta)
  # Each component becomes its own independently installable package
  # rather than CPack's default of one combined "unspecified"-grouped
  # archive; auto-shlibdeps and auto-provides/requires run separately per
  # component package (so e.g. only ecce-client ends up depending on
  # wxWidgets/Xerces, since only its ELF binaries link them).
  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_COMPONENTS_GROUPING IGNORE)
  set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
  set(CPACK_RPM_COMPONENT_INSTALL ON)

  set(CPACK_DEBIAN_CLIENT_PACKAGE_NAME "ecce-client")
  set(CPACK_DEBIAN_CLIENT_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_CLIENT_PACKAGE_DESCRIPTION
    "Extensible Computational Chemistry Environment - client (GUI apps, input generators/parsers, job-launch tooling)")
  # apache2/apache2-utils and the broker move to ecce-server; everything
  # else CPACK_DEBIAN_PACKAGE_DEPENDS listed for the monolithic build is
  # still needed here (python3-wxgtk4.0 for scripts/codereg, csh/perl/
  # xterm for the same reasons given beside the monolithic set above --
  # none of that is server-side).
  #
  # default-jre-headless: the monolithic package never had to list a JVM
  # explicitly because activemq (a hard Depends there) pulls in
  # default-jre-headless transitively -- confirmed via `apt-cache depends
  # activemq`. Splitting moves activemq to ecce-server and demotes it to
  # a client Recommends, which would silently drop the client's own JVM
  # need: ecce-gateway-start's "per-session JMSDispatcher relay" step
  # runs `java gov.pnnl.emsl.ecce.jms.JMSDispatcher` unconditionally,
  # including under -remote against a central broker with no local
  # ecce-server at all, so it cannot be left to arrive only via a
  # Recommends that a remote client has no reason to install.
  set(CPACK_DEBIAN_CLIENT_PACKAGE_DEPENDS
      "python3-wxgtk4.0, csh | tcsh, perl, xterm, default-jre-headless")
  # A client on the SAME machine as a server benefits from it (the
  # default "everything on one box" case this project still ships), but
  # a client of an already-central server (#167/#138 -- the teaching
  # deployment) has no use for a local data server or broker at all, so
  # this is Recommends, not Depends, matching the design brief.
  set(CPACK_DEBIAN_CLIENT_PACKAGE_RECOMMENDS "ecce-server, nwchem, openssh-client")
  set(CPACK_DEBIAN_CLIENT_PACKAGE_SUGGESTS "imagemagick, www-browser")
  # Lets dpkg upgrade a monolithic 8.x "ecce" straight into ecce-client:
  # without this, installing ecce-client while the old ecce package still
  # owns e.g. /opt/ecce/bin/organizer is a file-already-owned conflict,
  # not an upgrade.
  set(CPACK_DEBIAN_CLIENT_PACKAGE_BREAKS "ecce (<< ${ECCE_MONOLITHIC_BREAK_VERSION})")
  set(CPACK_DEBIAN_CLIENT_PACKAGE_REPLACES "ecce (<< ${ECCE_MONOLITHIC_BREAK_VERSION})")

  set(CPACK_DEBIAN_SERVER_PACKAGE_NAME "ecce-server")
  set(CPACK_DEBIAN_SERVER_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_SERVER_PACKAGE_DESCRIPTION
    "Extensible Computational Chemistry Environment - server (WebDAV data server, ActiveMQ broker configuration)")
  set(CPACK_DEBIAN_SERVER_PACKAGE_DEPENDS "apache2, apache2-utils, activemq")
  # Deliberately no CPACK_DEBIAN_SERVER_PACKAGE_RECOMMENDS "ecce-client":
  # the central-server deployment (CLAUDE.md, #138/#167) is exactly a
  # machine that runs ecce-server and nothing else, with every real user
  # on a separate client machine -- recommending the client back would
  # pull GUI/wx onto a headless server box by default.
  set(CPACK_DEBIAN_SERVER_PACKAGE_BREAKS "ecce (<< ${ECCE_MONOLITHIC_BREAK_VERSION})")
  set(CPACK_DEBIAN_SERVER_PACKAGE_REPLACES "ecce (<< ${ECCE_MONOLITHIC_BREAK_VERSION})")

  # The "ecce" metapackage (Andy, 2026-09-28): ships no files of its own
  # -- CPack's Debian generator refuses a truly empty component archive,
  # so it gets one harmless marker doc instead -- and exists purely so
  # `apt install ecce` keeps meaning "everything, one machine", and so a
  # bare `apt upgrade` of the old monolithic 8.x "ecce" pulls in both
  # halves via its own Breaks/Replaces above plus this package's
  # Depends. Not Recommends: the whole point is that installing "ecce"
  # unconditionally gets you both.
  set(_ecce_meta_marker ${CMAKE_BINARY_DIR}/ecce-meta/README)
  file(WRITE ${_ecce_meta_marker}
"This package is empty by design.

It exists only so \"apt install ecce\"/\"dnf install ecce\" keeps meaning
\"the full, one-machine ECCE install\" now that it is split into
ecce-client and ecce-server (see GETTING_STARTED.md's \"Split packages\"
section) -- installing this package pulls in both.
")
  install(FILES ${_ecce_meta_marker}
          DESTINATION share/doc/ecce-meta
          COMPONENT meta)
  set(CPACK_DEBIAN_META_PACKAGE_NAME "ecce")
  set(CPACK_DEBIAN_META_FILE_NAME DEB-DEFAULT)
  set(CPACK_DEBIAN_META_PACKAGE_DESCRIPTION
    "Extensible Computational Chemistry Environment - metapackage (installs ecce-client and ecce-server together, the pre-9.0 default)")
  set(CPACK_DEBIAN_META_PACKAGE_DEPENDS "ecce-client (= ${_ecce_deb_version}), ecce-server (= ${_ecce_deb_version})")
  # dpkg-shlibdeps has nothing to scan in an empty package and errors if
  # asked to try.
  set(CPACK_DEBIAN_META_PACKAGE_SHLIBDEPS OFF)

  if(RPMBUILD_EXECUTABLE)
    set(CPACK_RPM_CLIENT_PACKAGE_NAME "ecce-client")
    set(CPACK_RPM_CLIENT_FILE_NAME RPM-DEFAULT)
    # java-headless: same rationale as CPACK_DEBIAN_CLIENT_PACKAGE_DEPENDS
    # above -- the JMSDispatcher relay needs a JVM unconditionally, and on
    # RPM distros activemq (the only thing that pulled one in transitively
    # for the monolithic package, where it even applied -- see the
    # monolithic CPACK_RPM_PACKAGE_SUGGESTS comment above) isn't even a
    # hard Requires there, so it cannot be relied on at all here.
    set(CPACK_RPM_CLIENT_PACKAGE_REQUIRES
        "python3-wxpython4, tcsh, perl, xterm, java-headless")
    set(CPACK_RPM_CLIENT_PACKAGE_SUGGESTS
        "ecce-server, nwchem, openssh-clients, ImageMagick, firefox")
    # RPM has no direct Breaks/Replaces pair; Obsoletes+Provides is the
    # idiomatic equivalent for "this package supersedes an older one
    # that owned the same paths", and Conflicts blocks the two from
    # being installed side by side (the old monolithic package is
    # exactly what ecce-client+ecce-server now provide between them, so
    # letting both stay installed would leave stale duplicate files).
    set(CPACK_RPM_CLIENT_PACKAGE_OBSOLETES "ecce < ${ECCE_MONOLITHIC_BREAK_VERSION}")
    set(CPACK_RPM_CLIENT_PACKAGE_CONFLICTS "ecce < ${ECCE_MONOLITHIC_BREAK_VERSION}")
    set(CPACK_RPM_SERVER_PACKAGE_NAME "ecce-server")
    set(CPACK_RPM_SERVER_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_SERVER_PACKAGE_REQUIRES "httpd, httpd-tools")
    set(CPACK_RPM_SERVER_PACKAGE_SUGGESTS "activemq")
    set(CPACK_RPM_SERVER_PACKAGE_OBSOLETES "ecce < ${ECCE_MONOLITHIC_BREAK_VERSION}")
    set(CPACK_RPM_SERVER_PACKAGE_CONFLICTS "ecce < ${ECCE_MONOLITHIC_BREAK_VERSION}")
    set(CPACK_RPM_META_PACKAGE_NAME "ecce")
    set(CPACK_RPM_META_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_META_PACKAGE_REQUIRES "ecce-client = ${_ecce_deb_version}, ecce-server = ${_ecce_deb_version}")
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/opt")
    set(CPACK_RPM_CLIENT_PACKAGE_AUTOREQ ON)
    set(CPACK_RPM_SERVER_PACKAGE_AUTOREQ ON)
    set(CPACK_RPM_META_PACKAGE_AUTOREQ OFF)
    set(CPACK_RPM_PACKAGE_AUTOPROV OFF)
  endif()
endif()

include(CPack)

# ---------------------------------------------------------------------------
# Tests
#
# tests/parsers is a standalone, script-level regression suite for the
# parser / property pipeline: it replays checked-in chemistry-code output
# through the real .desc Begin/Skip/Lines/End/Frequency algorithm and the
# real scripts/parsers/* scripts and diffs the extracted properties against
# checked-in expected output.  It needs only python3 and perl -- no build,
# no GUI, no ECCE services, no chemistry codes -- so it is registered here
# purely so `ctest` picks it up; nothing else in the build depends on it,
# and the fixtures are deliberately NOT installed into the package.
#
#     cd build-cmake && ctest -R parsers --output-on-failure
# ---------------------------------------------------------------------------
enable_testing()
find_program(PYTHON3_EXECUTABLE NAMES python3)
if(PYTHON3_EXECUTABLE)
  add_test(NAME parsers
           COMMAND ${PYTHON3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/parsers/run_tests.py
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/parsers)

  # tests/dialogs -- the GUI-side sibling of the above: runs the real
  # scripts/codereg dialogs headlessly (broadwayd, GTK3's displayless
  # backend; falls back to xvfb; SKIPs cleanly with neither) and diffs what
  # each dialog offers against what the matching ai.<code> generator
  # recognises, in both directions.  Needs python3 + wxPython, no build and
  # no ECCE services.  See tests/dialogs/README.md.
  #
  #     cd build-cmake && ctest -R dialogs --output-on-failure
  add_test(NAME dialogs
           COMMAND ${PYTHON3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/dialogs/run_tests.py
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/dialogs)

  # tests/verify -- the input-deck checker (#148).  Runs the real
  # scripts/parsers/verifyinput over decks captured from real runs and
  # checks what it says about each.  Weighted against FALSE ALARMS
  # rather than against misses: a user told their working deck is
  # broken stops believing the light, including the time it is right.
  # Needs only python3 and perl.
  #
  #     cd build-cmake && ctest -R verify --output-on-failure
  add_test(NAME verify
           COMMAND ${PYTHON3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify/run_tests.py
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify)

  # tests/guivalues -- memory sizes saved before the GB-everywhere change
  # (Megawords, Megabytes) are converted by GUIValues::load() before a
  # dialog or a generator reads them.  Needs g++ and libecceutil.a.
  #
  #     cd build-cmake && ctest -R guivalues --output-on-failure
  add_test(NAME guivalues
           COMMAND ${PYTHON3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/guivalues/run_tests.py
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/guivalues)

  # tests/apps -- the third tier: starts every INSTALLED wx-linked ECCE app
  # on a headless X server with software OpenGL (Xvfb + llvmpipe; broadway
  # cannot be used here because these apps link GLX for the Open Inventor
  # viewer), waits for a window, and watches it for a few seconds.  That is
  # the shape of the wx3.2/GTK3 bugs this port keeps hitting -- #78 crashed
  # during construction, the layout-reentrancy class crashes after Show()
  # returns -- and nothing else in the tree notices either.
  #
  # This one tests /opt/ecce, NOT the build tree, so package and install
  # before trusting a green run.  It SKIPs cleanly with nothing installed or
  # no Xvfb.  See tests/apps/README.md.
  #
  #     cd build-cmake && ctest -R apps --output-on-failure
  add_test(NAME apps
           COMMAND ${PYTHON3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/apps/run_tests.py
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/apps)
  # Generous: 20 apps x (window wait + settle), plus broker startup.
  set_tests_properties(apps PROPERTIES TIMEOUT 1800)

  # tests/shells -- does a long command's echo come back byte for byte
  # from csh, tcsh and bash (and through zsh/ksh login shells on a hop)?
  # A line editor redrawing it hangs job monitoring forever (#143, #69).
  # Pty harness plus a driver linked against the real RCommand.  Not
  # installed.  See tests/shells/run_tests.py.
  #
  #     cd build-cmake && ctest -R shells --output-on-failure
  add_executable(testRCommandEcho tests/shells/testRCommandEcho.C)
  target_link_libraries(testRCommandEcho PRIVATE
    eccercmd ecceexp eccedsi eccedav eccecipc eccefaces eccexml eccetdat
    ecceutil ${XercesC_LIBRARIES})
  add_test(NAME shells
           COMMAND ${PYTHON3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests/shells/run_tests.py
                   --driver $<TARGET_FILE:testRCommandEcho>
           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/shells)
  set_tests_properties(shells PROPERTIES TIMEOUT 900)
endif()
