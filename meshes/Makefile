.PHONY : all

HOSTNAME := $(shell hostname)

ifeq ($(HOSTNAME), incepchow)
	BLENDER = ~/blender-2.79b-linux-glibc219-x86_64/blender
else
	BLENDER = blender
endif

DIST=../dist

all : \
	$(DIST)/menu.p \
	$(DIST)/title.pc \
	$(DIST)/phone-bank.pnc \
	$(DIST)/phone-bank.scene \
	$(DIST)/meshes.pnc \
	$(DIST)/crates.pnc \
	$(DIST)/crates.scene \

$(DIST)/%.p : %.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- '$<' '$@'

$(DIST)/%.pnc : %.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- '$<' '$@'

$(DIST)/title.pc : phone-bank.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- '$<':12 '$@'

$(DIST)/phone-bank.pnc : phone-bank.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- '$<':1 '$@'

$(DIST)/phone-bank.scene : phone-bank.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- '$<':1 '$@'

$(DIST)/%.scene : %.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- '$<' '$@'
