# Viewer

# Install client data. Hope that none of these need to be executable
install(DIRECTORY data/client DESTINATION ${ECCE_APPS}/data)

#
install(
    DIRECTORY
        scripts
    DESTINATION ${ECCE_APPS}
)

# Make sure the scripts have proper permissions
install(
    FILES
        scripts/ebuilder
        scripts/ecce
        scripts/ecce_checks
        scripts/epmf
        scripts/eprp
        scripts/eviewer
        scripts/gbsDAVConverter
        scripts/gbsDescriber
        scripts/gbsNWChemConverter
        scripts/gensub
        scripts/processmachine
        scripts/sysdir
        scripts/viewer
    DESTINATION ${ECCE_APPS}/scripts
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_EXECUTE
)
install(DIRECTORY siteconfig DESTINATION ${ECCE_APPS})
install(DIRECTORY java/lib DESTINATION ${ECCE_APPS}/java)
# install(FILES ${ACTIVEMQ_INSTALL}/activemq-all-5.1.0.jar DESTINATION ${ECCE_APPS}/java/lib)

# Server
install(DIRECTORY data/admin DESTINATION ${ECCE_SERVER}/data)
