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
# The JMS gateway: Java JMSDispatcher relay + vendored ActiveMQ broker.
#
# Previously excluded entirely (see the old comment below, now corrected).
# This wires up the single-machine "client and server both here" case
# scripts/ecce/install_ecce used to configure interactively:
#   - java/ is built with the existing Ant script (java/build/build.xml)
#     into java/lib/ecce_jms.jar, installed alongside its activemq-all
#     client jar under <prefix>/java/lib.
#   - the vendored ActiveMQ 5.1.0 broker distribution
#     (build/3rdparty-dists/apache-activemq-5.1.0-bin.tar.bz2) is
#     extracted at build time and installed under <prefix>/server/activemq,
#     using its stock localhost-only config (see packaging/gateway/
#     ecce-gateway-start for the port this assumes).
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
          DESTINATION java/lib)
  set(ECCE_GATEWAY_JAVA_AVAILABLE TRUE)
else()
  message(WARNING "ant not found - the JMS gateway (java/) will not be built "
                   "or packaged; ecce-gateway-start will have nothing to run.")
  set(ECCE_GATEWAY_JAVA_AVAILABLE FALSE)
endif()

set(_activemq_tarball ${CMAKE_SOURCE_DIR}/build/3rdparty-dists/apache-activemq-5.1.0-bin.tar.bz2)
if(EXISTS ${_activemq_tarball})
  # WORKING_DIRECTORY applies to every COMMAND line in this rule, including
  # the one that's supposed to create it -- on a from-scratch build tree
  # (no stale activemq-extracted/ left over from a previous run) that made
  # the very first command `cd` into a directory that doesn't exist yet.
  # Use a WORKING_DIRECTORY that's guaranteed to already exist (the build
  # dir itself) and explicitly `cmake -E chdir` into the subdir only for
  # the extraction step, after make_directory has created it.
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/activemq-extracted/activemq/bin/activemq
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/activemq-extracted
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/activemq-extracted
            ${CMAKE_COMMAND} -E tar xjf ${_activemq_tarball}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Extracting vendored ActiveMQ 5.1.0 broker"
  )
  add_custom_target(ecce_activemq ALL
    DEPENDS ${CMAKE_BINARY_DIR}/activemq-extracted/activemq/bin/activemq)
  # linux-x86-32 is the vendored 32-bit Java Service Wrapper binary; this
  # package only targets amd64 and dpkg-shlibdeps can't resolve a 32-bit
  # ELF's deps without i386 multiarch libs present on the build host, so
  # drop it rather than fight package-time shlibdeps for an arch we don't
  # ship for. activemq/bin/activemq picks linux-x86-64 automatically.
  install(DIRECTORY ${CMAKE_BINARY_DIR}/activemq-extracted/activemq
          DESTINATION server
          USE_SOURCE_PERMISSIONS
          PATTERN "linux-x86-32" EXCLUDE)
  set(ECCE_GATEWAY_BROKER_AVAILABLE TRUE)
else()
  message(WARNING "No vendored ActiveMQ tarball at ${_activemq_tarball} - "
                   "the JMS broker will not be packaged.")
  set(ECCE_GATEWAY_BROKER_AVAILABLE FALSE)
endif()

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

  install(DIRECTORY ${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp
          DESTINATION data/client/WebHelp)
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
install(DIRECTORY ${CMAKE_SOURCE_DIR}/siteconfig/ DESTINATION siteconfig
        PATTERN "jndi.properties" EXCLUDE
        PATTERN "RemoteServer" EXCLUDE
        PATTERN "DataServers" EXCLUDE
        PATTERN "site_runtime" EXCLUDE)
install(FILES ${CMAKE_BINARY_DIR}/siteconfig-local/jndi.properties
              ${CMAKE_BINARY_DIR}/siteconfig-local/DataServers
              ${CMAKE_BINARY_DIR}/siteconfig-local/site_runtime
        DESTINATION siteconfig)

install(PROGRAMS
          ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-gateway-start
          ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-gateway-stop
          ${CMAKE_SOURCE_DIR}/packaging/gateway/ecce-gateway-status
        DESTINATION bin)

# Also put the gateway control scripts on PATH directly (like the ecce-<app>
# wrappers) so `ecce-gateway-status` etc. work without needing $ECCE_HOME.
foreach(_gw start stop status)
  set(_gw_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-gateway-${_gw})
  file(WRITE ${_gw_wrapper}
"#!/bin/bash
export ECCE_HOME=/opt/ecce
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
exec \"$ECCE_HOME/bin/ecce-gateway-${_gw}\" \"$@\"
")
  install(PROGRAMS ${_gw_wrapper} DESTINATION /usr/bin)
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
install(PROGRAMS
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-start
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-stop
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-status
          ${CMAKE_SOURCE_DIR}/packaging/dataserver/ecce-dataserver-adduser
        DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/packaging/dataserver/httpd.conf.ecce
        DESTINATION server/httpd-conf)

foreach(_ds start stop status adduser)
  set(_ds_wrapper ${CMAKE_BINARY_DIR}/wrappers/ecce-dataserver-${_ds})
  file(WRITE ${_ds_wrapper}
"#!/bin/bash
export ECCE_HOME=/opt/ecce
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
export ECCE_REALUSER=\${ECCE_REALUSER:-\$(id -un)}
exec \"$ECCE_HOME/bin/ecce-dataserver-${_ds}\" \"$@\"
")
  install(PROGRAMS ${_ds_wrapper} DESTINATION /usr/bin)
endforeach()

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
          USE_SOURCE_PERMISSIONS)
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
    set(_gateway_start "if [ -z \"$ECCE_NO_MESSAGING\" ]; then \"$ECCE_HOME/bin/ecce-gateway-start\" || true; fi")
    # Same app set as messaging: apps that need the JMS gateway also need
    # data-server (EDSI/WebDAV) access, per EDSIServerCentral::
    # checkServerSetup() -- see ecce-dataserver-start.
    set(_dataserver_start "if [ -z \"$ECCE_NO_DATASERVER\" ]; then \"$ECCE_HOME/bin/ecce-dataserver-start\" || true; fi")
  else()
    set(_gateway_start "")
    set(_dataserver_start "")
  endif()
  file(WRITE ${_wrapper}
"#!/bin/bash
export ECCE_HOME=/opt/ecce
export ECCE_REALUSER=\${ECCE_REALUSER:-$(id -un)}
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
export HOST=\${HOST:-$(hostname)}
export DISPLAY=\${DISPLAY:-:0}
export ECCE_HELP=\${ECCE_HELP:-http://localhost:8096/}
${_gateway_start}
${_dataserver_start}
exec \"$ECCE_HOME/bin/${_app}\" \"$@\"
")
  install(PROGRAMS ${_wrapper} DESTINATION /usr/bin)
endforeach()

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
export ECCE_HOME=/opt/ecce
export ECCE_REALUSER=\${ECCE_REALUSER:-$(id -un)}
export ECCE_REALUSERHOME=\${ECCE_REALUSERHOME:-$HOME}
export HOST=\${HOST:-$(hostname)}
export DISPLAY=\${DISPLAY:-:0}
export ECCE_HELP=\${ECCE_HELP:-http://localhost:8096/}
export ECCE_INVOKE_VIEWER=1
export ECCE_INVOKE_FROMECCE=1
if [ -z \"$ECCE_NO_MESSAGING\" ]; then \"$ECCE_HOME/bin/ecce-gateway-start\" || true; fi
if [ -z \"$ECCE_NO_DATASERVER\" ]; then \"$ECCE_HOME/bin/ecce-dataserver-start\" || true; fi
exec \"$ECCE_HOME/bin/builder\" \"$@\"
")
install(PROGRAMS ${_viewer_wrapper} DESTINATION /usr/bin)

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
set(CPACK_DEBIAN_PACKAGE_DEPENDS "apache2, apache2-utils")
# RelWithDebInfo keeps -g everywhere (useful while porting - see the
# CMAKE_BUILD_TYPE default above), which makes for a needlessly huge .deb;
# strip debug symbols out of the packaged copies only, not the build tree.
set(CPACK_STRIP_FILES ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
include(CPack)
