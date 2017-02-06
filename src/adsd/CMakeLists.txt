set (adsd_src daemon-ads.c
		../common/daemon.c
	)

add_executable (adsd ${adsd_src})

install (TARGETS adsd
		RUNTIME
		DESTINATION bin)
