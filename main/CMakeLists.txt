idf_component_register(
	SRCS "main.c"
	INCLUDE_DIRS "."
	# These are component names, expected to exist as subdirectories of components/.
	REQUIRES "LcdManager" "ButtonManager"
	)
