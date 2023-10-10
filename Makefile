#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	source

TARGET_AUTOBOOT			:=	nintendont_default_autobooter
TARGET_FORCE43_INTERLACED	:=	nintendont_force_43_interlaced_autobooter
TARGET_FORCE43			:=	nintendont_force_4_by_3_autobooter
TARGET_INTERLACED		:=	nintendont_force_interlaced_autobooter
TARGET_FW			:=	nintendont_forwarder

BUILD_AUTOBOOT			:=	build_autoboot
BUILD_FORCE43_INTERLACED	:=	build_force_43_interlaced
BUILD_FORCE43			:=	build_force_43
BUILD_INTERLACED		:=	build_interlaced
BUILD_FW			:=	build_fw

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CFLAGS	= -Ofast -Wall -flto=auto -fno-fat-lto-objects -use-flinker-plugin

ifeq ($(strip $(FW_AUTOBOOT)), 1)
	CFLAGS		+=	-DFW_AUTOBOOT
	ifeq ($(strip $(FORCE_43)), 1)
		CFLAGS	+=	-DFORCE_43
		ifeq ($(strip $(FORCE_INTERLACED)), 1)
			CFLAGS	+=	-DFORCE_INTERLACED
			TARGET	:=	$(TARGET_FORCE43_INTERLACED)
			BUILD	:=	$(BUILD_FORCE43_INTERLACED)
		else
			TARGET	:=	$(TARGET_FORCE43)
			BUILD	:=	$(BUILD_FORCE43)
		endif
	else
		ifeq ($(strip $(FORCE_INTERLACED)), 1)
			CFLAGS	+=	-DFORCE_INTERLACED
			TARGET	:=	$(TARGET_INTERLACED)
			BUILD	:=	$(BUILD_INTERLACED)
		else
			TARGET	:=	$(TARGET_AUTOBOOT)
			BUILD	:=	$(BUILD_AUTOBOOT)
		endif
	endif
else
	TARGET	:=	$(TARGET_FW)
	BUILD	:=	$(BUILD_FW)
endif

CFLAGS		+=	$(MACHDEP) $(INCLUDE)
CXXFLAGS	=	$(CFLAGS)
LDFLAGS		=	$(CFLAGS) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lfat -logc

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(DEVKITPPC)/lib $(CURDIR)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(CURDIR)/$(BUILD_AUTOBOOT) $(CURDIR)/$(TARGET_AUTOBOOT).elf $(CURDIR)/$(TARGET_AUTOBOOT).dol \
		$(CURDIR)/$(BUILD_FORCE43_INTERLACED) $(CURDIR)/$(TARGET_FORCE43_INTERLACED).elf $(CURDIR)/$(TARGET_FORCE43_INTERLACED).dol \
		$(CURDIR)/$(BUILD_FORCE43) $(CURDIR)/$(TARGET_FORCE43).elf $(CURDIR)/$(TARGET_FORCE43).dol \
		$(CURDIR)/$(BUILD_INTERLACED) $(CURDIR)/$(TARGET_INTERLACED).elf $(CURDIR)/$(TARGET_INTERLACED).dol \
		$(CURDIR)/$(BUILD_FW) $(CURDIR)/$(TARGET_FW).elf $(CURDIR)/$(TARGET_FW).dol


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the .bin extension
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
