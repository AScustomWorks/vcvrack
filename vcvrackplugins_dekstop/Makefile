# this flag selects the rack version to compile against.
# 
# possible values are v040 v_050_dev

# FLAGS += -D v040
# FLAGS += -D v_050_dev

SLUG = dekstop 
VERSION = 0.6.0dev


SOURCES = $(wildcard src/*.cpp portaudio/*.c)

include ../../plugin.mk

FLAGS += -Iportaudio

dist: all
	mkdir -p dist/dekstop
	cp LICENSE* dist/dekstop/
	cp plugin.* dist/dekstop/
	cp -R res dist/dekstop/
