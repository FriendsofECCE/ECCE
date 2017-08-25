# Fortran
set(f_SRC ${CMAKE_SOURCE_DIR}/src/f/symmetry)

add_custom_target(f_code)
add_dependencies(f_code autosym cleansym genmol genmollat getfrag testnames)

add_executable(autosym
    ${f_SRC}/autosym.f
    ${f_SRC}/geom_hnd.f
    ${f_SRC}/errquit.f
    ${f_SRC}/deter3.f
)

add_executable(cleansym
    ${f_SRC}/cleansym.f
    ${f_SRC}/sym_avg.f
    ${f_SRC}/spnames.f90
    ${f_SRC}/spgen.f90
    ${f_SRC}/deter3.f
    ${f_SRC}/dctr.f
    ${f_SRC}/gensym.f
    ${f_SRC}/errquit.f
)

add_executable(genmol
    ${f_SRC}/genmol.f
    ${f_SRC}/dosymops.f
    ${f_SRC}/spnames.f90
    ${f_SRC}/spgen.f90
    ${f_SRC}/deter3.f
    ${f_SRC}/dctr.f
    ${f_SRC}/gensym.f
    ${f_SRC}/errquit.f
)

add_executable(genmollat
    ${f_SRC}/genmollat.f
    ${f_SRC}/dosymops.f
    ${f_SRC}/spnames.f90
    ${f_SRC}/spgen.f90
    ${f_SRC}/deter3.f
    ${f_SRC}/dctr.f
    ${f_SRC}/gensym.f
    ${f_SRC}/errquit.f
)

add_executable(getfrag
    ${f_SRC}/getfrag.f
    ${f_SRC}/findfrag.f
    ${f_SRC}/spnames.f90
    ${f_SRC}/spgen.f90
    ${f_SRC}/deter3.f
    ${f_SRC}/dctr.f
    ${f_SRC}/gensym.f
    ${f_SRC}/errquit.f
)

add_executable(testnames
    ${f_SRC}/testnames.f
    ${f_SRC}/spnames.f90
)

install(TARGETS autosym cleansym genmol genmollat getfrag testnames DESTINATION ${ECCE_APPS}/bin)
