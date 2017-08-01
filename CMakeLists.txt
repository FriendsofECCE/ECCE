cmake_minimum_required(VERSION 2.8)
project(ecce)

include(ExternalProject)

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
