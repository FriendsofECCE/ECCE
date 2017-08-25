# THIRD PARTY PACKAGES
set(THIRD_PARTY_INSTALL_DIR ${CMAKE_BINARY_DIR}/3rdparty)
# set(THIRD_PARTY_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}) # TODO: Remove this line

# add_custom_target(third_party)
# add_dependencies(third_party xerces wxpython httpd)

# XERCES
set(XERCES_REPO
    http://svn.apache.org/repos/asf/xerces/c/tags/Xerces-C_2_8_0)
set(XERCES_URL
    http://archive.apache.org/dist/xerces/c/2/sources/xerces-c-src_2_8_0.tar.gz)
# set(XERCES_REPO https://svn.apache.org/repos/asf/xerces/c/trunk)
set(XERCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/xerces)
set(XERCES_INSTALL ${THIRD_PARTY_INSTALL_DIR}/xerces)
set(XERCES_ROOT ${XERCES_DIR}/source)
set(XERCES_SRC ${XERCES_ROOT}/src/xercesc)

ExternalProject_Add(xerces
    URL ${XERCES_URL}
    PREFIX ${XERCES_DIR}
    INSTALL_DIR ${XERCES_INSTALL}
    SOURCE_DIR ${XERCES_ROOT}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
        # Build steps lumped together because XercesC environment variables
        # Using bash script prevents rebuilding if .installed file exists
        # I dislike this, but...
        sh ${CMAKE_SOURCE_DIR}/cmake/build_xerces.sh ${XERCES_SRC} ${XERCES_ROOT} ${XERCES_INSTALL}
)
set(XERCES_INCLUDE ${XERCES_INSTALL}/include)
set(XERCES_LIB_DIR ${XERCES_INSTALL}/lib)
set(XERCES_LIBRARIES -lxerces-c -lxerces-depdom)

# wxPython
set(WXPYTHON_URL
    http://cfhcable.dl.sourceforge.net/project/wxpython/wxPython/2.8.12.1/wxPython-src-2.8.12.1.tar.bz2)
set(WXPYTHON_DIR ${CMAKE_CURRENT_BINARY_DIR}/wxpython)
set(WXPYTHON_INSTALL ${THIRD_PARTY_INSTALL_DIR}/wxpython)
set(WXPYTHON_SRC ${WXPYTHON_DIR}/source)

ExternalProject_Add(wxpython
    URL ${WXPYTHON_URL}
    PREFIX ${WXPYTHON_DIR}
    SOURCE_DIR ${WXPYTHON_SRC}
    INSTALL_DIR ${WXPYTHON_INSTALL}
    CONFIGURE_COMMAND
        sh ${WXPYTHON_SRC}/configure --prefix=${WXPYTHON_INSTALL} --with-gtk --with-gnomeprint --with-opengl --enable-geometry --enable-graphics_ctx --enable-sound --with-sdl --enable-mediactrl --enable-display
    BUILD_COMMAND
        make &&
        make -C contrib/src/gizmos &&
        make -C contrib/src/stc
    INSTALL_COMMAND
        make install &&
        make -C contrib/src/gizmos install &&
        make -C contrib/src/stc install &&
        cd ${WXPYTHON_SRC}/wxPython &&
        export LD_LIBRARY_PATH=${WX_INSTALL}/lib:$ENV{LD_LIBRARY_PATH} &&
        export WXWIN=${WXPYTHON_SRC} &&
        export PATH=${WX_INSTALL}/bin:$ENV{PATH} &&
        export CFLAGS="-I${WX_INSTALL}/include/wx-2.8 $ENV{CFLAGS}" &&
        export CXXFLAGS="-I${WX_INSTALL}/include/wx-2.8 $ENV{CCFLAGS}" &&
        python setup.py build_ext --inplace UNICODE=0 BUILD_GLCANVAS=0 BUILD_OGL=0 &&
        python setup.py install UNICODE=0 --home=${WXPYTHON_INSTALL}
)
set(WX_INCLUDE ${WXPYTHON_INSTALL}/include/wx-2.8 ${WXPYTHON_INSTALL}/lib/wx/include/gtk2-ansi-release-2.8)
set(WX_LIB_DIR ${WXPYTHON_INSTALL}/lib)
set(WX_FLAGS -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -pthread)
set(WX_LIBRARIES ewxaui -lwx_gtk2_adv-2.8 -lwx_gtk2_richtext-2.8 -lwx_gtk2_aui-2.8 -lwx_gtk2_xrc-2.8 -lwx_gtk2_qa-2.8 -lwx_gtk2_html-2.8 -lwx_gtk2_adv-2.8 -lwx_gtk2_core-2.8 -lwx_base_xml-2.8 -lwx_base_net-2.8 -lwx_base-2.8)

# HTTPD
set(HTTPD_URL
    http://apache.mirrors.pair.com//httpd/httpd-2.2.34.tar.gz)
set(HTTPD_DIR ${CMAKE_CURRENT_BINARY_DIR}/httpd)
set(HTTPD_INSTALL ${THIRD_PARTY_INSTALL_DIR}/httpd)
set(HTTPD_SRC ${HTTPD_DIR}/source)

ExternalProject_Add(httpd
    URL ${HTTPD_URL}
    PREFIX ${HTTPD_DIR}
    SOURCE_DIR ${HTTPD_SRC}
    INSTALL_DIR ${HTTPD_INSTALL}
    CONFIGURE_COMMAND
        ${HTTPD_SRC}/configure --prefix=${HTTPD_INSTALL} --enable-rewrite --enable-dav
    BUILD_COMMAND
        make
    INSTALL_COMMAND
        make install
)

# ActiveMQ
#set(ACTIVEMQ_URL
#    https://github.com/FriendsofECCE/ECCE/raw/develop/build/3rdparty-dists/apache-activemq-5.1.0-bin.tar.bz2)
set(ACTIVEMQ_URL
    http://archive.apache.org/dist/activemq/apache-activemq/5.1.0/apache-activemq-5.1.0-bin.tar.gz)
set(ACTIVEMQ_DIR ${CMAKE_CURRENT_BINARY_DIR}/activemq)
set(ACTIVEMQ_INSTALL ${THIRD_PARTY_INSTALL_DIR}/activemq)
set(ACTIVEMQ_SRC ${ACTIVEMQ_DIR}/source)

ExternalProject_Add(activemq
    URL ${ACTIVEMQ_URL}
    PREFIX ${ACTIVEMQ_DIR}
    SOURCE_DIR ${ACTIVEMQ_INSTALL}
    CONFIGURE_COMMAND
        echo "It's a bin distrobution, so no build."
    BUILD_COMMAND
        echo "No build either."
    INSTALL_COMMAND
        echo "And no install."
)

install(DIRECTORY
        ${WXPYTHON_INSTALL}
        ${WX_INSTALL}
        ${XERCES_INSTALL}
    DESTINATION
        ${ECCE_APPS}/3rdparty)

install(DIRECTORY
        ${ACTIVEMQ_INSTALL}
        ${HTTPD_INSTALL}
    DESTINATION
        ${ECCE_SERVER})
