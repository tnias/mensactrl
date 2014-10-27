# -*-makefile-*-
#
# Copyright (C) 2014 by Daniel Willmann <daniel@totalueberwachung.de>
#
# See CREDITS for details about who has contributed to this project.
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
PACKAGES-$(PTXCONF_MENSA_CONTROL) += mensa-control

#
# Paths and names
#
MENSA_CONTROL_VERSION	:= 0
MENSA_CONTROL		:= mensa-control
MENSA_CONTROL_URL	:= file://$(PTXDIST_WORKSPACE)/local_src/$(MENSA_CONTROL)
MENSA_CONTROL_DIR	:= $(BUILDDIR)/$(MENSA_CONTROL)
MENSA_CONTROL_BUILD_OOT	:= YES
MENSA_CONTROL_LICENSE	:= unknown

# ----------------------------------------------------------------------------
# Extract
# ----------------------------------------------------------------------------

ifdef PTXCONF_MENSA_CONTROL_TRUNK
$(STATEDIR)/mensa-control.extract: $(STATEDIR)/autogen-tools
endif

$(STATEDIR)/mensa-control.extract:
	@$(call targetinfo)
	@$(call clean, $(MENSA_CONTROL_DIR))
	@$(call extract, MENSA_CONTROL)
ifdef PTXCONF_MENSA_CONTROL_TRUNK
	cd $(MENSA_CONTROL_DIR) && sh autogen.sh
else
	cd $(MENSA_CONTROL_DIR) && [ -f configure ] || sh autogen.sh
endif
	@$(call patchin, MENSA_CONTROL)
	@$(call touch)

# ----------------------------------------------------------------------------
# Prepare
# ----------------------------------------------------------------------------

#MENSA_CONTROL_CONF_ENV	:= $(CROSS_ENV)

#
# autoconf
#
MENSA_CONTROL_CONF_TOOL	:= autoconf
MENSA_CONTROL_CONF_OPT	:= \
	$(CROSS_AUTOCONF_USR) \
	--disable-debug

#$(STATEDIR)/mensa-control.prepare:
#	@$(call targetinfo)
#	@$(call world/prepare, MENSA_CONTROL)
#	@$(call touch)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/mensa-control.targetinstall:
	@$(call targetinfo)

	@$(call install_init, mensa-control)
	@$(call install_fixup, mensa-control, PRIORITY, optional)
	@$(call install_fixup, mensa-control, SECTION, base)
	@$(call install_fixup, mensa-control, AUTHOR, "Daniel Willmann <daniel@totalueberwachung.de>")
	@$(call install_fixup, mensa-control, DESCRIPTION, missing)

#	#
#	# example code:; copy all libraries, links and binaries
#	#

	@for i in $(shell cd $(MENSA_CONTROL_PKGDIR) && find bin sbin usr/bin usr/sbin -type f); do \
		$(call install_copy, mensa-control, 0, 0, 0755, -, /$$i); \
	done
	@for i in $(shell cd $(MENSA_CONTROL_PKGDIR) && find lib usr/lib -name "*.so*"); do \
		$(call install_copy, mensa-control, 0, 0, 0644, -, /$$i); \
	done
	@links="$(shell cd $(MENSA_CONTROL_PKGDIR) && find lib usr/lib -type l)"; \
	if [ -n "$$links" ]; then \
		for i in $$links; do \
			from="`readlink $(MENSA_CONTROL_PKGDIR)/$$i`"; \
			to="/$$i"; \
			$(call install_link, mensa-control, $$from, $$to); \
		done; \
	fi

	@$(call install_alternative, mensa-control, 0, 0, 0755, \
		/usr/bin/mensactrl.sh)

	@$(call install_alternative, mensa-control, 0, 0, 0644, \
		/lib/systemd/system/mensadisplay.service)
	@$(call install_link, mensa-control, ../mensadisplay.service, \
		/lib/systemd/system/multi-user.target.wants/mensadisplay.service)

	@$(call install_finish, mensa-control)

	@$(call touch)

# vim: syntax=make
