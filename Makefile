#---------------------------------------------------------------
# Cobra Arena - Makefile (PS3 homebrew, PSL1GHT + tiny3d)
#---------------------------------------------------------------
include $(PSL1GHT)/ppu_rules

TARGET     := cobra_arena
CONTENT_ID := UP0001-COBR00001_00-COBRAARENA000001
PKG_DIR    := pkg_root

SOURCES := .
CFLAGS  := -O2 -Wall -Wno-unused-function
LIBS    := -ltiny3d -lsysutil -lio -lgcm_sys -lsysmodule -lrt -llv2 -lm

# 'make' gera $(TARGET).elf e $(TARGET).self (regras do ppu_rules)
all: $(TARGET).self $(TARGET).pkg

# Monta a pasta do pacote: EBOOT.BIN, PARAM.SFO e ICON0.PNG
$(PKG_DIR)/USRDIR/EBOOT.BIN: $(TARGET).self
	mkdir -p $(PKG_DIR)/USRDIR
	cp $(TARGET).self $@

$(PKG_DIR)/PARAM.SFO: sfo.xml
	mkdir -p $(PKG_DIR)
	$(PS3DEV)/bin/sfo.py -f $< $@

# Gera o .pkg (ferramenta pkg.py do PS3Dev)
$(TARGET).pkg: $(PKG_DIR)/USRDIR/EBOOT.BIN $(PKG_DIR)/PARAM.SFO
	cp ICON0.PNG $(PKG_DIR)/ICON0.PNG
	python3 $(PS3DEV)/bin/pkg.py --contentid $(CONTENT_ID) $(PKG_DIR) $@

clean:
	rm -rf $(PKG_DIR) *.elf *.self *.pkg

.PHONY: all clean