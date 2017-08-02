cmake_minimum_required(VERSION 2.8)
project(ecce)

include(ExternalProject)


# THIRD PARTY PACKAGES

add_custom_target(third_party)
add_dependencies(third_party xerces wxpython)

# XERCES
set(XERCES_REPO http://svn.apache.org/repos/asf/xerces/c/tags/Xerces-C_2_8_0)
set(XERCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/xerces)
set(XERCES_INSTALL ${XERCES_DIR}/install)
set(XERCES_ROOT ${XERCES_DIR}/source)
set(XERCES_SRC ${XERCES_ROOT}/src/xercesc)

ExternalProject_Add(xerces
    SVN_REPOSITORY ${XERCES_REPO}
    PREFIX ${XERCES_DIR}
    INSTALL_DIR ${XERCES_INSTALL}
    SOURCE_DIR ${XERCES_ROOT}
    CONFIGURE_COMMAND
        cd ${XERCES_SRC} &&
        pwd && ls &&
        export XERCESCROOT=${XERCES_ROOT} &&
        ./runConfigure -p linux -b 64 -P ${XERCES_INSTALL} &&
        make -C ${XERCES_SRC} &&
        make -C ${XERCES_SRC} install
    # Lumping the build process into the CONFIGURE_COMMAND because of Xerces'
    #   crippling addiction environment variables.
    BUILD_COMMAND
        echo -n ""
    INSTALL_COMMAND
        echo -n ""
)

# wxPython
set(WXPYTHON_URL
    http://cfhcable.dl.sourceforge.net/project/wxpython/wxPython/2.8.12.1/wxPython-src-2.8.12.1.tar.bz2)
set(WXPYTHON_DIR ${CMAKE_CURRENT_BINARY_DIR}/wxpython)
set(WXPYTHON_INSTALL ${WXPYTHON_DIR}/install)
set(WXPYTHON_SRC ${WXPYTHON_DIR}/source)

ExternalProject_Add(wxpython
    URL ${WXPYTHON_URL}
    PREFIX ${WXPYTHON_DIR}
    SOURCE_DIR ${WXPYTHON_SRC}
    INSTALL_DIR ${WXPYTHON_INSTALL}
    CONFIGURE_COMMAND
        sh ${WXPYTHON_SRC}/configure --prefix=${WXPYTHON_INSTALL} --with-gtk --with-gnomeprint --with-opengl --enable-geometry --enable-graphics_ctx --enable-sound --with-sdl --enable-mediactrl --enable-display
    BUILD_COMMAND
        make
    INSTALL_COMMAND
        make install
)
