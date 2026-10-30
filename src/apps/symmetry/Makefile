
include $(ECCE_MAKEINC_DIR)/Makefile.defs

.NOTPARALLEL:

FC = gfortran
CXX = g++
LDFLAGS = $(ECCE_LDFLAGS) $(ECCE_LFLAGS)

default: $(OBJ_PATH) autosym autosym_cpp getfrag genmol genmollat cleansym cleansym_cpp getfrag_cpp genmol_cpp genmollat_cpp symops_cpp

testnames : $(OBJ_PATH)/testnames.o $(OBJ_PATH)/spnames.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/testnames.o $(OBJ_PATH)/spnames.o -o $(BIN_PATH)/$@

autosym : $(OBJ_PATH)/autosym.o $(OBJ_PATH)/geom_hnd.o $(OBJ_PATH)/errquit.o $(OBJ_PATH)/deter3.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/autosym.o $(OBJ_PATH)/geom_hnd.o $(OBJ_PATH)/deter3.o $(OBJ_PATH)/errquit.o -o $(BIN_PATH)/$@

autosym_cpp: $(OBJ_PATH)/autosym_cpp.o
	$(CXX) $(LDFLAGS) $^ -o $(BIN_PATH)/$@

getfrag : $(OBJ_PATH)/getfrag.o $(OBJ_PATH)/findfrag.o $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o $(OBJ_PATH)/errquit.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/getfrag.o $(OBJ_PATH)/findfrag.o\
    $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o\
    $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o\
    $(OBJ_PATH)/errquit.o -o $(BIN_PATH)/$@

genmol : $(OBJ_PATH)/genmol.o $(OBJ_PATH)/dosymops.o $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o $(OBJ_PATH)/errquit.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/genmol.o $(OBJ_PATH)/dosymops.o\
    $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o\
    $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o\
    $(OBJ_PATH)/errquit.o -o $(BIN_PATH)/$@

genmollat : $(OBJ_PATH)/genmollat.o $(OBJ_PATH)/dosymops.o $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o $(OBJ_PATH)/errquit.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/genmollat.o $(OBJ_PATH)/dosymops.o\
    $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o\
    $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o\
    $(OBJ_PATH)/errquit.o -o $(BIN_PATH)/$@

cleansym : $(OBJ_PATH)/cleansym.o $(OBJ_PATH)/sym_avg.o $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o $(OBJ_PATH)/errquit.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/cleansym.o $(OBJ_PATH)/sym_avg.o\
    $(OBJ_PATH)/spnames.o $(OBJ_PATH)/spgen.o\
    $(OBJ_PATH)/deter3.o $(OBJ_PATH)/dctr.o $(OBJ_PATH)/gensym.o\
    $(OBJ_PATH)/errquit.o -o $(BIN_PATH)/$@

# Parallel C++ implementation used when ECCE_CXX_SYMMETRY=1.
cleansym_cpp : $(OBJ_PATH)/cleansym_cpp.o $(OBJ_PATH)/sym_avg_cpp.o $(OBJ_PATH)/spnames_cpp.o $(OBJ_PATH)/spgen_cpp.o $(OBJ_PATH)/gensym_cpp.o $(OBJ_PATH)/deter3_cpp.o $(OBJ_PATH)/errquit_cpp.o
	$(CXX) $(LDFLAGS) $^ -o $(BIN_PATH)/$@

getfrag_cpp: $(OBJ_PATH)/getfrag_cpp.o $(OBJ_PATH)/symmetry_fragments_cpp.o $(OBJ_PATH)/spnames_cpp.o $(OBJ_PATH)/spgen_cpp.o $(OBJ_PATH)/spgen_periodic_cpp.o $(OBJ_PATH)/gensym_cpp.o $(OBJ_PATH)/deter3_cpp.o $(OBJ_PATH)/errquit_cpp.o
	$(CXX) $(LDFLAGS) $^ -o $(BIN_PATH)/$@
genmol_cpp: $(OBJ_PATH)/genmol_cpp.o $(OBJ_PATH)/symmetry_fragments_cpp.o $(OBJ_PATH)/spnames_cpp.o $(OBJ_PATH)/spgen_cpp.o $(OBJ_PATH)/spgen_periodic_cpp.o $(OBJ_PATH)/gensym_cpp.o $(OBJ_PATH)/deter3_cpp.o $(OBJ_PATH)/errquit_cpp.o
	$(CXX) $(LDFLAGS) $^ -o $(BIN_PATH)/$@
genmollat_cpp: $(OBJ_PATH)/genmollat_cpp.o $(OBJ_PATH)/symmetry_fragments_cpp.o $(OBJ_PATH)/spnames_cpp.o $(OBJ_PATH)/spgen_cpp.o $(OBJ_PATH)/spgen_periodic_cpp.o $(OBJ_PATH)/gensym_cpp.o $(OBJ_PATH)/deter3_cpp.o $(OBJ_PATH)/errquit_cpp.o
	$(CXX) $(LDFLAGS) $^ -o $(BIN_PATH)/$@
symops_cpp: $(OBJ_PATH)/symops_cpp.o $(OBJ_PATH)/spnames_cpp.o $(OBJ_PATH)/spgen_cpp.o $(OBJ_PATH)/spgen_periodic_cpp.o $(OBJ_PATH)/gensym_cpp.o $(OBJ_PATH)/deter3_cpp.o $(OBJ_PATH)/errquit_cpp.o
	$(CXX) $(LDFLAGS) $^ -o $(BIN_PATH)/$@

$(OBJ_PATH)/testnames.o: testnames.f
	$(FC) $(ECCE_FFLAGS) -c testnames.f -o $@
$(OBJ_PATH)/genmollat.o: genmollat.f
	$(FC) $(ECCE_FFLAGS) -c genmollat.f -o $@
$(OBJ_PATH)/spnames.o: spnames.f90
	$(FC) $(ECCE_FFLAGS) -c spnames.f90 -o $@
$(OBJ_PATH)/autosym.o: autosym.f
	$(FC) $(ECCE_FFLAGS) -c autosym.f -o $@
$(OBJ_PATH)/autosym_cpp.o: autosym.cpp
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c autosym.cpp -o $@
$(OBJ_PATH)/geom_hnd.o : geom_hnd.f
	$(FC) $(ECCE_FFLAGS) -c geom_hnd.f -o $@
$(OBJ_PATH)/getfrag.o : getfrag.f
	$(FC) $(ECCE_FFLAGS) -c getfrag.f -o $@
$(OBJ_PATH)/findfrag.o : findfrag.f
	$(FC) $(ECCE_FFLAGS) -c findfrag.f -o $@
$(OBJ_PATH)/genmol.o : genmol.f
	$(FC) $(ECCE_FFLAGS) -c genmol.f -o $@
$(OBJ_PATH)/dosymops.o : dosymops.f
	$(FC) $(ECCE_FFLAGS) -c dosymops.f -o $@
$(OBJ_PATH)/cleansym.o : cleansym.f
	$(FC) $(ECCE_FFLAGS) -c cleansym.f -o $@
$(OBJ_PATH)/sym_avg.o : sym_avg.f
	$(FC) $(ECCE_FFLAGS) -c sym_avg.f -o $@
$(OBJ_PATH)/spgen.o : spgen.f90
	$(FC) $(ECCE_FFLAGS) -c spgen.f90 -o $@
$(OBJ_PATH)/deter3.o : deter3.f
	$(FC) $(ECCE_FFLAGS) -c deter3.f -o $@
$(OBJ_PATH)/dctr.o : dctr.f
	$(FC) $(ECCE_FFLAGS) -c dctr.f -o $@
$(OBJ_PATH)/gensym.o : gensym.f
	$(FC) $(ECCE_FFLAGS) -c gensym.f -o $@
$(OBJ_PATH)/errquit.o : errquit.f
	$(FC) $(ECCE_FFLAGS) -c errquit.f -o $@
$(OBJ_PATH)/cleansym_cpp.o: cleansym.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c cleansym.cpp -o $@
$(OBJ_PATH)/sym_avg_cpp.o: sym_avg.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c sym_avg.cpp -o $@
$(OBJ_PATH)/spnames_cpp.o: spnames.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c spnames.cpp -o $@
$(OBJ_PATH)/spgen_cpp.o: spgen.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c spgen.cpp -o $@
$(OBJ_PATH)/spgen_periodic_cpp.o: spgen_periodic.cpp spgen_tables.inc symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c spgen_periodic.cpp -o $@
$(OBJ_PATH)/gensym_cpp.o: gensym.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c gensym.cpp -o $@
$(OBJ_PATH)/deter3_cpp.o: deter3.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c deter3.cpp -o $@
$(OBJ_PATH)/errquit_cpp.o: errquit.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c errquit.cpp -o $@
$(OBJ_PATH)/symmetry_fragments_cpp.o: symmetry_fragments.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c symmetry_fragments.cpp -o $@
$(OBJ_PATH)/getfrag_cpp.o: getfrag.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c getfrag.cpp -o $@
$(OBJ_PATH)/genmol_cpp.o: genmol.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c genmol.cpp -o $@
$(OBJ_PATH)/genmollat_cpp.o: genmollat.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c genmollat.cpp -o $@
$(OBJ_PATH)/symops_cpp.o: symops.cpp symmetry_cpp.h
	$(CXX) -std=c++11 $(ECCE_CXXFLAGS) -c symops.cpp -o $@

clean:
	rm -f\
    $(OBJ_PATH)/testnames.o\
    $(OBJ_PATH)/genmollat.o\
    $(OBJ_PATH)/spnames.o spnames.mod\
    $(OBJ_PATH)/autosym.o\
    $(OBJ_PATH)/autosym_cpp.o\
    $(OBJ_PATH)/geom_hnd.o\
    $(OBJ_PATH)/getfrag.o\
    $(OBJ_PATH)/findfrag.o\
    $(OBJ_PATH)/genmol.o\
    $(OBJ_PATH)/dosymops.o\
    $(OBJ_PATH)/cleansym.o\
    $(OBJ_PATH)/sym_avg.o\
    $(OBJ_PATH)/spgen.o\
    $(OBJ_PATH)/deter3.o\
    $(OBJ_PATH)/dctr.o\
    $(OBJ_PATH)/gensym.o\
    $(OBJ_PATH)/errquit.o\
    $(OBJ_PATH)/cleansym_cpp.o\
    $(OBJ_PATH)/sym_avg_cpp.o\
    $(OBJ_PATH)/spnames_cpp.o\
    $(OBJ_PATH)/spgen_cpp.o\
    $(OBJ_PATH)/spgen_periodic_cpp.o\
    $(OBJ_PATH)/gensym_cpp.o\
    $(OBJ_PATH)/deter3_cpp.o\
    $(OBJ_PATH)/errquit_cpp.o\
    $(OBJ_PATH)/symmetry_fragments_cpp.o\
    $(OBJ_PATH)/getfrag_cpp.o\
    $(OBJ_PATH)/genmol_cpp.o\
    $(OBJ_PATH)/genmollat_cpp.o\
    $(OBJ_PATH)/symops_cpp.o\
    $(BIN_PATH)/testnames\
    $(BIN_PATH)/autosym\
    $(BIN_PATH)/autosym_cpp\
    $(BIN_PATH)/getfrag\
    $(BIN_PATH)/genmol\
    $(BIN_PATH)/genmollat\
    $(BIN_PATH)/cleansym\
    $(BIN_PATH)/cleansym_cpp\
    $(BIN_PATH)/getfrag_cpp\
    $(BIN_PATH)/genmol_cpp\
    $(BIN_PATH)/genmollat_cpp\
    $(BIN_PATH)/symops_cpp

$(OBJ_PATH): 
	@mkdir -p $@
