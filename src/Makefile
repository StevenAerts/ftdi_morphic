#TOPDIR  := $(shell cd ..; pwd)
#include $(TOPDIR)/Rules.make

DEPENDENCIES := -lpthread
UNAME := $(shell uname)
# Assume target is Mac OS if build host is Mac OS; any other host targets Linux
ifeq ($(UNAME), Darwin)
	DEPENDENCIES += -lobjc -framework IOKit -framework CoreFoundation
else
	DEPENDENCIES += -lrt
endif

CFLAGS = -Wall -Wextra -Wno-unused-function

DYNAMIC_LINK_OPTIONS := -Wl,-rpath /usr/local/lib

APP = ftdi_morphic
#STATIC_APP = $(APP)-static
#DYNAMIC_APP = $(APP)-dynamic
LOADER = alterald
EEMOD = ft2232h_eeprom_modify

#all: $(APP)-static $(APP)-dynamic 
all: $(APP) $(LOADER) $(EEMOD)

#$(STATIC_APP): main.c 
#	$(CC) main.c -o $(STATIC_APP) ../../build/libftd2xx.a $(CFLAGS) $(DEPENDENCIES)

#$(DYNAMIC_APP): main.c 	
#	$(CC) main.c -o $(DYNAMIC_APP) $(CFLAGS) -lftd2xx $(DEPENDENCIES) $(DYNAMIC_LINK_OPTIONS)
	
$(APP): main.c 	ftc.h
	$(CC) main.c -o $(APP) $(CFLAGS) -lftd2xx $(DEPENDENCIES) $(DYNAMIC_LINK_OPTIONS)

$(LOADER): alterald.c
	$(CC) alterald.c -o $(LOADER) $(CFLAGS) -lftd2xx $(DEPENDENCIES) $(DYNAMIC_LINK_OPTIONS)

$(EEMOD): ft2232h_eeprom_modify.c
	$(CC) ft2232h_eeprom_modify.c -o $(EEMOD) $(CFLAGS) -lftd2xx $(DEPENDENCIES) $(DYNAMIC_LINK_OPTIONS)

clean:
	-rm -f *.o ; rm $(APP) ; rm $(LOADER) ; rm $(EEMOD) ; 

#	-rm -f *.o ; rm $(STATIC_APP); rm $(DYNAMIC_APP)
