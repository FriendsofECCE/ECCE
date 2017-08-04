
include $(ECCE_MAKEINC_DIR)/Makefile.defs

.NOTPARALLEL:

FC = gfortran
LDFLAGS = $(ECCE_LDFLAGS) $(ECCE_LFLAGS)

default: $(OBJ_PATH) autosym getfrag genmol genmollat cleansym

testnames : $(OBJ_PATH)/testnames.o $(OBJ_PATH)/spnames.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/testnames.o $(OBJ_PATH)/spnames.o -o $(BIN_PATH)/$@

autosym : $(OBJ_PATH)/autosym.o $(OBJ_PATH)/geom_hnd.o $(OBJ_PATH)/errquit.o $(OBJ_PATH)/deter3.o
	$(FC) $(ECCE_FFLAGS) $(LDFLAGS) $(OBJ_PATH)/autosym.o $(OBJ_PATH)/geom_hnd.o $(OBJ_PATH)/deter3.o $(OBJ_PATH)/errquit.o -o $(BIN_PATH)/$@

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

$(OBJ_PATH)/testnames.o: testnames.f
	$(FC) $(ECCE_FFLAGS) -c testnames.f -o $@
$(OBJ_PATH)/genmollat.o: genmollat.f
	$(FC) $(ECCE_FFLAGS) -c genmollat.f -o $@
$(OBJ_PATH)/spnames.o: spnames.f90
	$(FC) $(ECCE_FFLAGS) -c spnames.f90 -o $@
$(OBJ_PATH)/autosym.o: autosym.f
	$(FC) $(ECCE_FFLAGS) -c autosym.f -o $@
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

clean:
	rm -f\
    $(OBJ_PATH)/testnames.o\
    $(OBJ_PATH)/genmollat.o\
    $(OBJ_PATH)/spnames.o spnames.mod\
    $(OBJ_PATH)/autosym.o\
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
    $(BIN_PATH)/testnames\
    $(BIN_PATH)/autosym\
    $(BIN_PATH)/getfrag\
    $(BIN_PATH)/genmol\
    $(BIN_PATH)/genmollat\
    $(BIN_PATH)/cleansym

$(OBJ_PATH): 
	@mkdir -p $@
