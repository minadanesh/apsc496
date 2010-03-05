ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

PSTAG_64 = .64
LIB_VARIANT = $(subst .o,,a.$(COMPOUND_VARIANT))$(PSTAG_$(PADDR_SIZE))

LINKER_TYPE=BOOTSTRAP
INSTALLDIR = boot/sys
LIBS = startup$(subst .,-,$(PSTAG_$(PADDR_SIZE))) lzo ucl drvr

NAME = startup-$(SECTION)
EXTRA_SILENT_VARIANTS+=$(subst -, ,$(SECTION))
USEFILE = 

EXTRA_SRCVPATH +=	$(SECTION_ROOT)/overrides

EXTRA_INCVPATH +=	$(SECTION_ROOT)/overrides \
					$(LIBSTARTUP_ROOT)/$(CPU)/$(LIB_VARIANT) \
					$(LIBSTARTUP_ROOT)/$(CPU) \
					$(LIBSTARTUP_ROOT) \
					$(LIBSTARTUP_ROOT)/public

EXTRA_LIBVPATH +=	$(LIBSTARTUP_ROOT)/$(CPU)/$(LIB_VARIANT) \
					$(USE_ROOT_LIB) \
					$(QNX_TARGET)/$(CPUDIR)/lib \
					$(QNX_TARGET)/$(CPUDIR)/usr/lib

EXTRA_INCVPATH+=$(INSTALL_ROOT_nto)/usr/include/xilinx


#LDBOOTSTRAPPOST_nto_x86_gcc_qcc:=$(subst -lc, -L$(QNX_TARGET)/x86/lib -lc, $(LDBOOTSTRAPPOST_nto_x86_gcc_qcc))

CCFLAG_64 = -D_PADDR_BITS=64
CCFLAGS_gcc_ = -O2 -fomit-frame-pointer
CCFLAGS_gcc_qcc = -O2 -Wc,-fomit-frame-pointer 
CCFLAGS_$(BUILDENV) = -DBUILDENV_$(BUILDENV)
CCFLAGS = $(CCFLAGS_$(COMPILER_TYPE)_$(COMPILER_DRIVER)) $(CCFLAG_$(PADDR_SIZE)) $(CCFLAGS_$(BUILDENV))

LDFLAGS_gcc_qcc = -M
LDFLAGS = $(LDFLAGS_$(COMPILER_TYPE)_$(COMPILER_DRIVER))

EXTRA_ICLEAN=$(SECTION_ROOT)/*.pinfo

define POST_INSTALL
	-$(foreach build,$(EXAMPLE_BUILDFILES) $(EXAMPLE_READMES), $(CP_HOST) $(build) $(INSTALL_ROOT_nto)/$(CPUDIR)/boot/build/$(SECTION).$(notdir $(build));)
endef

include $(MKFILES_ROOT)/qmacros.mk
ADD_USAGE=

-include $(PROJECT_ROOT)/roots.mk
#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../../install
   USE_INSTALL_ROOT=1
##############################################################

ifndef LIBSTARTUP_ROOT
LIBSTARTUP_ROOT=$(PRODUCT_ROOT)/lib
endif

EXAMPLE_BUILDFILES:=$(wildcard *build $(SECTION_ROOT)/*build)
EXAMPLE_READMES:=$(wildcard $(SECTION_ROOT)/*readme)

EBF_PINFOS:=$(addsuffix .pinfo, $(EXAMPLE_BUILDFILES))
ERM_PINFOS:=$(addsuffix .pinfo, $(EXAMPLE_READMES))

EXTRA_DEPS = $(EBF_PINFOS) $(ERM_PINFOS)

include $(SECTION_ROOT)/pinfo.mk

include $(MKFILES_ROOT)/qtargets.mk

-include $(PROJECT_ROOT)/announce.mk


define DO_PINFO
	@$(ECHO_HOST)  >$@ STATE=Experimental 
	@$(ECHO_HOST) >>$@ INSTALLDIR=$(CPUDIR)/boot/build/
	@$(ECHO_HOST) >>$@ INSTALLNAME=$(SECTION).$(patsubst %.pinfo,%,$(@F))
	@$(ECHO_HOST) >>$@ NAME=$(patsubst %.pinfo,%,$(@F))
	@$(ECHO_HOST) >>$@ USER=$(shell $(USER_HOST))
	@$(ECHO_HOST) >>$@ HOST=$(shell $(HOST_HOST))
	@$(ECHO_HOST) >>$@ DATE=$(shell $(DATE_HOST))
endef

$(EBF_PINFOS): $(EXAMPLE_BUILDFILES)
	$(DO_PINFO)
	@$(ECHO_HOST) >>$@ DESCRIPTION=Example build file for $(NAME)

$(ERM_PINFOS): $(EXAMPLE_READMES)
	$(DO_PINFO)
	@$(ECHO_HOST) >>$@ DESCRIPTION=Readme file for $(SECTION)

#
# This particular little kludge is to stop GCC from using F.P. instructions
# to move 8 byte quantities around. 
#
CC_nto_ppc_gcc += -msoft-float
CC_nto_ppc_gcc_qcc += -Wc,-msoft-float

# Prevent building with debug for the standard build.
# Temporary until linker has been changed.
CCOPTS:=$(filter-out -g,$(CCOPTS))

