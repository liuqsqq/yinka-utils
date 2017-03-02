yinka-update: yinka-update.sh
		shc -r -f $<
		mv $<.x $@
yinka-network: yinka-network.sh
		shc -r -f $<
		mv $<.x $@
yinka-printer: yinka-printer.sh
		shc -r -f $<
		mv $<.x $@
yinka-input: yinka-input.sh
		shc -r -f $<
		mv $<.x $@

bin_SCRIPTS = \
		yinka-update	\
		yinka-network	\
		yinka-printer	\
		yinka-input		\
		$(NULL)

EXTRA_DIST = \
		yinka-update.sh		\
		yinka-network.sh	\
		yinka-printer.sh	\
		yinka-input.sh		\
		$(NULL)

CLEANFILES = \
		$(bin_SCRIPTS)	\
		*.x.c			\
		$(NULL)
