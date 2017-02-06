set (yinkad_src daemon-yinka.c
		../common/daemon.c
	)

add_executable (yinkad ${yinkad_src})

install (TARGETS yinkad
		RUNTIME
		DESTINATION bin)
