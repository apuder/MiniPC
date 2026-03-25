
DIRS = kernel tools
ALL_DIRS = $(DIRS) test

FATFORMAT    = tools/fat/fatformat
FATCOPY      = tools/fat/fatcopy
FATDEL       = tools/fat/fatdel
FATSYS       = tools/fat/fatsys
LOADER       = tools/loader/load.exe
DISK_IMAGE   = image/disk_image
KERNEL_IMG   = tos.img
BOOT_STAGE_1 = tools/boot/stage1.bin
BOOT_STAGE_2 = tools/boot/stage2.bin
DEMO_IMG     = image/demo_tos.img


all:
	for i in $(DIRS); do $(MAKE) -C $$i || exit 1; done
	rm -f $(DISK_IMAGE)
	$(FATFORMAT) $(DISK_IMAGE) size=DISC
	$(FATCOPY) $(DISK_IMAGE) $(BOOT_STAGE_2) /`basename $(BOOT_STAGE_2)`
	$(FATCOPY) $(DISK_IMAGE) $(KERNEL_IMG) /`basename $(KERNEL_IMG)`
	$(FATSYS)  $(DISK_IMAGE) $(BOOT_STAGE_1)


demo:
	$(MAKE) -C tools
	rm -f $(DISK_IMAGE)
	$(FATFORMAT) $(DISK_IMAGE) size=DISC
	$(FATCOPY) $(DISK_IMAGE) $(BOOT_STAGE_2) /`basename $(BOOT_STAGE_2)`
	$(FATCOPY) $(DISK_IMAGE) $(DEMO_IMG) /tos.img
	$(FATSYS)  $(DISK_IMAGE) $(BOOT_STAGE_1)

tests frame-test run_ref:
	$(MAKE) -C kernel
	$(MAKE) -C test $@
	$(MAKE) -C tools
	rm -f $(DISK_IMAGE)
	$(FATFORMAT) $(DISK_IMAGE) size=DISC
	$(FATCOPY) $(DISK_IMAGE) $(BOOT_STAGE_2) /`basename $(BOOT_STAGE_2)`
	$(FATCOPY) $(DISK_IMAGE) $(KERNEL_IMG) /`basename $(KERNEL_IMG)`
	$(FATSYS)  $(DISK_IMAGE) $(BOOT_STAGE_1)

host-tests:
	$(MAKE) -C test $@

clean-kernel:
	$(MAKE) -C kernel clean
	$(MAKE) -C test clean
	rm -f $(DISK_IMAGE) $(KERNEL_IMG)

clean:
	for i in $(ALL_DIRS); do $(MAKE) -C $$i clean || exit 1; done
	rm -f $(DISK_IMAGE)
	rm -f image/tos.img
	rm -f $(KERNEL_IMG)
	rm -f tos-debug.img
	rm -f ttc.jar

depend:
	for i in $(DIRS); do $(MAKE) -C $$i .depend || exit 1; done

.depend:
	for i in $(DIRS); do $(MAKE) -C $$i .depend || exit 1; done


# TOS_IFDEF never

# The mkdist script will strip these rules from the Makefile
# in the version of TOS that is distributed to students.

MKDIST  = ./mkdist.py
DISTDIR = ../../www/tos/dist
TMPDIR	= ../tmp

.PHONY: doc
doc:
	@doxygen

dist: dist-initial dist-assn1 dist-assn2 dist-assn3 dist-assn4 \
      dist-assn5 dist-assn6 dist-assn7 dist-assn8 dist-assn9
	rm -rf $(TMPDIR)

library:
	mkdir -p lib
	$(MAKE) -C kernel lib
	$(MAKE) -C test lib

demo_kernel:
	$(MAKE) -C kernel demo
	mv $(KERNEL_IMG) image/demo_tos.img
	$(MAKE) -C test lib.o

dist-prereqs: library demo_kernel
	$(MAKE) -C tools/ttc

dist-initial: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos
	cd $(TMPDIR); tar czf $(DISTDIR)/tos.tar.gz tos/

dist-assn1: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn1-19617.tar.gz tos/

dist-assn2: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn2-26187.tar.gz tos/

dist-assn3: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2 -Dassn3
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn3-16451.tar.gz tos/

dist-assn4: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2 -Dassn3 -Dassn4
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn4-75903.tar.gz tos/

dist-assn5: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2 -Dassn3 -Dassn4 -Dassn5
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn5-84946.tar.gz tos/

dist-assn6: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2 -Dassn3 -Dassn4 -Dassn5 -Dassn6
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn6-69405.tar.gz tos/

dist-assn7: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2 -Dassn3 -Dassn4 -Dassn5 -Dassn6 -Dassn7
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn7-52056.tar.gz tos/

dist-assn8: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2 -Dassn3 -Dassn4 -Dassn5 -Dassn6 -Dassn7 -Dassn8
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn8-42876.tar.gz tos/

dist-assn9: dist-prereqs
	rm -rf $(TMPDIR)/tos
	$(MKDIST) --outdir $(TMPDIR)/tos -Dassn1 -Dassn2 -Dassn3 -Dassn4 -Dassn5 -Dassn6 -Dassn7 -Dassn8 -Dassn9
	cd $(TMPDIR); tar czf $(DISTDIR)/tos-assn9-53210.tar.gz tos/

# TOS_ENDIF never
