ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

HOST_MKIFS := mkifs

SUFFIXES := .build .ifs .raw

.PHONY: all clean

all: ifs-omap3530-beagle.bin 

clean: 
	$(RM_HOST) ifs-omap3530-beagle.bin 

ifs-omap3530-beagle.bin: omap3530.beagle.build
	$(HOST_MKIFS) -r../install -v $(MKIFSFLAGS) $^ $@

