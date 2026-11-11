TARGET		:= slitherio_offline
TITLE_ID	:= SLITH0001
VENDOR		:= OFFLINE
TITLE		:= Slither.io Offline PS3
APP_VERSION	:= 01.00

include $(PS3DEV)/ps3dev.mk

OBJS = main.o

CFLAGS	+= -O2 -Wall
LDFLAGS	+= 

include $(PS3DEV)/ppu.mk
