# Run in libcint's source directory.  libcint calls include(CPack) itself, and
# inside ECCE that second include overwrites ECCE's CPackConfig.cmake (the
# packages come out as libcint's, of the source tree).
file(READ CMakeLists.txt _text)
string(REPLACE "include(CPack)" "# include(CPack)  (left out by ecce-qm)" _text "${_text}")
file(WRITE CMakeLists.txt "${_text}")
