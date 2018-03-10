SLUG = "Aepelzens Modules"
VERSION = "0.6.0dev"

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..

FLAGS += -I$(RACK_DIR)/plugins/AepelzensModules/soxr/include

LDFLAGS += -L$(RACK_DIR)/plugins/AepelzensModules/soxr/lib -lsoxr-lsr

include $(RACK_DIR)/plugin.mk
