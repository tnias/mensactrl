# -*-makefile-*-
#
# Copyright (C) 2014 by Jan Luebbe <jluebbe@debian.org>
#
# See CREDITS for details about who has contributed to this project.
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
PACKAGES-$(PTXCONF_SSH_KEYS) += ssh-keys

SSH_KEYS_VERSION	:= 1

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/ssh-keys.targetinstall:
	@$(call targetinfo)

	@$(call install_init, ssh-keys)
	@$(call install_fixup,ssh-keys,PRIORITY,optional)
	@$(call install_fixup,ssh-keys,SECTION,base)
	@$(call install_fixup,ssh-keys,AUTHOR,"Jan Luebbe <jluebbe@debian.org>")
	@$(call install_fixup,ssh-keys,DESCRIPTION,missing)

	@$(call install_alternative, ssh-keys, 0, 0, 0644, \
		/home/.ssh/authorized_keys)

	@$(call install_finish,ssh-keys)

	@$(call touch)

# vim: syntax=make
