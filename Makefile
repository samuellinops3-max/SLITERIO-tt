TARGET		:= slither
TITLE		:= Slither.io Offline
APPID		:= SLIT00001
CONTENTID	:= UP0001-SLIT00001_00-0000000000000000
SOURCES		:= .
INCLUDES	:= .
CFILES		:= $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
OBJS		:= $(CFILES:.c=.o)
LIBS		:= -ltiny3d -lrsx -lio -lsysutil -lm
include $(PSL1GHT)/ppu_rules