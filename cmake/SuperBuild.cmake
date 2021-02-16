include(ExternalProject)

#----- YAAP External project 
set( YAAP_BUILD_PATH "${THIRD_PARTY_BUILD_DIR}/yaap" )
set( YAAP_INSTALL_PATH "${THIRD_PARTY_INSTALL_DIR}/yaap" )

ExternalProject_Add ( yaap_ep
    PREFIX "${YAAP_BUILD_PATH}"
    GIT_REPOSITORY "https://github.com/basileMarchand/yaap"
    INSTALL_DIR ${YAAP_INSTALL_PATH}
    CMAKE_ARGS ${thirdparties_forward_options} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    LOG_DOWNLOAD  ON
    LOG_UPDATE    ON
    LOG_CONFIGURE ON
    LOG_BUILD     ON
    LOG_INSTALL   ON
    LOG_TEST      ON  
)

set(YAAP_INCLUDE_DIR "${YAAP_INSTALL_PATH}/include" CACHE PATH "Yaap include directory" FORCE)

