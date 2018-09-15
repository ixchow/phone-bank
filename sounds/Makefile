.PHONY : all

DIR=../dist/samples

all : \
	$(DIR)/bgm.wav \
	$(DIR)/A-check-1.wav $(DIR)/A-check-2.wav \
	$(DIR)/B-check-1.wav $(DIR)/B-check-2.wav \
	$(DIR)/C-check-1.wav $(DIR)/C-check-2.wav \
	$(DIR)/A-task-1.wav $(DIR)/A-task-2.wav $(DIR)/A-task-3.wav $(DIR)/A-task-4.wav \
	$(DIR)/B-task-1.wav $(DIR)/B-task-2.wav $(DIR)/B-task-3.wav $(DIR)/B-task-4.wav \
	$(DIR)/C-task-1.wav $(DIR)/C-task-2.wav $(DIR)/C-task-3.wav $(DIR)/C-task-4.wav \
	$(DIR)/A-say-1.wav $(DIR)/A-say-2.wav $(DIR)/A-say-3.wav $(DIR)/A-say-4.wav \
	$(DIR)/A-say-5.wav $(DIR)/A-say-6.wav $(DIR)/A-say-7.wav $(DIR)/A-say-8.wav \
	$(DIR)/A-say-9.wav $(DIR)/A-say-10.wav $(DIR)/A-say-11.wav $(DIR)/A-say-12.wav \
	$(DIR)/A-say-13.wav $(DIR)/A-say-14.wav $(DIR)/A-say-15.wav $(DIR)/A-say-16.wav \
	$(DIR)/B-say-1.wav $(DIR)/B-say-2.wav $(DIR)/B-say-3.wav $(DIR)/B-say-4.wav \
	$(DIR)/B-say-5.wav $(DIR)/B-say-6.wav $(DIR)/B-say-7.wav $(DIR)/B-say-8.wav \
	$(DIR)/B-say-9.wav $(DIR)/B-say-10.wav $(DIR)/B-say-11.wav $(DIR)/B-say-12.wav \
	$(DIR)/B-say-13.wav $(DIR)/B-say-14.wav $(DIR)/B-say-15.wav $(DIR)/B-say-16.wav \
	$(DIR)/C-say-1.wav $(DIR)/C-say-2.wav $(DIR)/C-say-3.wav $(DIR)/C-say-4.wav \
	$(DIR)/C-say-5.wav $(DIR)/C-say-6.wav $(DIR)/C-say-7.wav $(DIR)/C-say-8.wav \
	$(DIR)/C-say-9.wav $(DIR)/C-say-10.wav $(DIR)/C-say-11.wav $(DIR)/C-say-12.wav \
	$(DIR)/C-say-13.wav $(DIR)/C-say-14.wav $(DIR)/C-say-15.wav $(DIR)/C-say-16.wav \
	$(DIR)/ring-1.wav $(DIR)/ring-1-strong.wav $(DIR)/ring-1-end.wav \
	$(DIR)/ring-2.wav $(DIR)/ring-2-strong.wav $(DIR)/ring-2-end.wav \
	$(DIR)/ring-3.wav $(DIR)/ring-3-strong.wav $(DIR)/ring-3-end.wav \
	$(DIR)/ring-4.wav $(DIR)/ring-4-strong.wav $(DIR)/ring-4-end.wav \
	$(DIR)/click-1.wav $(DIR)/click-2.wav $(DIR)/click-3.wav $(DIR)/click-4.wav \

$(DIR)/bgm.wav : bgm.wav
	sox '$<' -c 1 -b 16 '$@'

%.norm.wav : %.wav
	sox '$<' -c 1 -b 16 '$@' norm

temp.A-check.wav : check-in.wav
	sox '$<' '$@' trim 0 8

temp.B-check.wav : check-in.wav
	sox '$<' '$@' trim 8 8

temp.C-check.wav : check-in.wav
	sox '$<' '$@' trim 16 8

temp.A-task.wav : new-task.wav
	sox '$<' '$@' trim 0 16

temp.B-task.wav : new-task.wav
	sox '$<' '$@' trim 16 16

temp.C-task.wav : new-task.wav
	sox '$<' '$@' trim 32 16

temp.A-say.wav : say.wav
	sox '$<' '$@' trim 0 64

temp.B-say.wav : say.wav
	sox '$<' '$@' trim 64 64

temp.C-say.wav : say.wav
	sox '$<' '$@' trim 128 64

$(DIR)/ring-1.wav : rings.norm.wav
	sox '$<' '$@' trim 0 2

$(DIR)/ring-1-strong.wav : rings.norm.wav
	sox '$<' '$@' trim 2 2

$(DIR)/ring-1-end.wav : rings.norm.wav
	sox '$<' '$@' trim 4 2 reverse silence 1 1 0 reverse

$(DIR)/ring-2.wav : rings.norm.wav
	sox '$<' '$@' trim 8 2

$(DIR)/ring-2-strong.wav : rings.norm.wav
	sox '$<' '$@' trim 10 2

$(DIR)/ring-2-end.wav : rings.norm.wav
	sox '$<' '$@' trim 12 2 reverse silence 1 1 0 reverse

$(DIR)/ring-3.wav : rings.norm.wav
	sox '$<' '$@' trim 16 2

$(DIR)/ring-3-strong.wav : rings.norm.wav
	sox '$<' '$@' trim 18 2

$(DIR)/ring-3-end.wav : rings.norm.wav
	sox '$<' '$@' trim 20 2 reverse silence 1 1 0 reverse

$(DIR)/ring-4.wav : rings.norm.wav
	sox '$<' '$@' trim 24 2

$(DIR)/ring-4-strong.wav : rings.norm.wav
	sox '$<' '$@' trim 26 2

$(DIR)/ring-4-end.wav : rings.norm.wav
	sox '$<' '$@' trim 28 2 reverse silence 1 1 0 reverse


$(DIR)/click-1.wav : clicks.norm.wav
	sox '$<' '$@' trim 0 2 reverse silence 1 1 0 reverse

$(DIR)/click-2.wav : clicks.norm.wav
	sox '$<' '$@' trim 2 2 reverse silence 1 1 0 reverse

$(DIR)/click-3.wav : clicks.norm.wav
	sox '$<' '$@' trim 4 2 reverse silence 1 1 0 reverse

$(DIR)/click-4.wav : clicks.norm.wav
	sox '$<' '$@' trim 6 2 reverse silence 1 1 0 reverse


DURATION=1
THRESH='0.5%'

$(DIR)/%-1.wav : temp.%.norm.wav
	sox '$<' '$@' trim 0 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-2.wav : temp.%.norm.wav
	sox '$<' '$@' trim 4 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-3.wav : temp.%.norm.wav
	sox '$<' '$@' trim 8 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-4.wav : temp.%.norm.wav
	sox '$<' '$@' trim 12 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-5.wav : temp.%.norm.wav
	sox '$<' '$@' trim 16 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-6.wav : temp.%.norm.wav
	sox '$<' '$@' trim 20 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-7.wav : temp.%.norm.wav
	sox '$<' '$@' trim 24 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-8.wav : temp.%.norm.wav
	sox '$<' '$@' trim 28 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-9.wav : temp.%.norm.wav
	sox '$<' '$@' trim 32 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-10.wav : temp.%.norm.wav
	sox '$<' '$@' trim 36 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-11.wav : temp.%.norm.wav
	sox '$<' '$@' trim 40 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-12.wav : temp.%.norm.wav
	sox '$<' '$@' trim 44 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-13.wav : temp.%.norm.wav
	sox '$<' '$@' trim 48 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-14.wav : temp.%.norm.wav
	sox '$<' '$@' trim 52 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-15.wav : temp.%.norm.wav
	sox '$<' '$@' trim 56 2 reverse silence 1 $(DURATION) $(THRESH) reverse

$(DIR)/%-16.wav : temp.%.norm.wav
	sox '$<' '$@' trim 60 2 reverse silence 1 $(DURATION) $(THRESH) reverse
