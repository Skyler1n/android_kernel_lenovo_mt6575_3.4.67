# Run board preparation once before Kbuild parses generated includes/config.
# Recursive make keeps the normal jobserver and all command-line assignments.
.DEFAULT_GOAL := a690-kernel
.PHONY: a690-kernel $(MAKECMDGOALS)
a690-kernel:
	+@MAKE="$(MAKE)" /bin/bash ../mediatek/build/kernel/a690-prepare.sh $(MAKECMDGOALS)
ifneq ($(strip $(MAKECMDGOALS)),)
$(MAKECMDGOALS): a690-kernel
	@:
endif
