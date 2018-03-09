SLUG = "Aepelzens Modules"
VERSION = "0.6.0dev"

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..
include $(RACK_DIR)/plugin.mk

LIBDIR += "../../plugins/AepelzensModules/sox/lib"
LDFLAGS += "-L../../plugins/AepelzensModules/soxr/lib -lsoxr-lsr"
