
# Run 'make SYS=<target>'; or, set a SYS env.
# var. to build for another target system.
SYS ?= supervision

# Just the usual way to find out if we're
# using cmd.exe to execute make rules.
ifneq ($(shell echo),)
  CMD_EXE = 1
endif

ifdef CMD_EXE
  NULLDEV = nul:
  DEL = -del /f
  RMDIR = rmdir /s /q
else
  NULLDEV = /dev/null
  DEL = $(RM)
  RMDIR = $(RM) -r
endif

ifdef CC65_HOME
  AS = $(CC65_HOME)/bin/ca65
  CC = $(CC65_HOME)/bin/cc65
  CL = $(CC65_HOME)/bin/cl65
  LD = $(CC65_HOME)/bin/ld65
  SP = $(CC65_HOME)/bin/sp65
else
  AS := $(if $(wildcard ../../bin/ca65*),../../bin/ca65,ca65)
  CC := $(if $(wildcard ../../bin/cc65*),../../bin/cc65,cc65)
  CL := $(if $(wildcard ../../bin/cl65*),../../bin/cl65,cl65)
  LD := $(if $(wildcard ../../bin/ld65*),../../bin/ld65,ld65)
  SP := $(if $(wildcard ../../bin/sp65*),../../bin/sp65,sp65)
endif

EXELIST_supervision = \
        menu

ifneq ($(EXELIST_$(SYS)),)
samples: $(EXELIST_$(SYS))
else
samples: notavailable
endif

# empty target used to skip systems that will not work with any program in this dir
notavailable:
ifeq ($(MAKELEVEL),0)
	@echo "info: supervision samples not available for" $(SYS)
else
# suppress the "nothing to be done for 'samples' message
	@echo > $(NULLDEV)
endif

menu: menu.c
	$(CL) -t $(SYS) -O -o menu.sv -m menu.map menu.c
	
clean:
	@$(DEL) $(EXELIST_supervision) 2>$(NULLDEV)
	@$(DEL) *.map 2>$(NULLDEV)
start:
	retroarch -L ~/.config/retroarch/cores/potator_libretro.so  menu.sv
