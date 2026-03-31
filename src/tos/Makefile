
DIRS = kernel test

all:
	for i in $(DIRS); do $(MAKE) -C $$i || exit 1; done

tests:
	$(MAKE) -C kernel
	$(MAKE) -C test $@

clean:
	for i in $(DIRS); do $(MAKE) -C $$i clean || exit 1; done

depend:
	for i in $(DIRS); do $(MAKE) -C $$i .depend || exit 1; done

.depend:
	for i in $(DIRS); do $(MAKE) -C $$i .depend || exit 1; done


.PHONY: doc
doc:
	@doxygen
