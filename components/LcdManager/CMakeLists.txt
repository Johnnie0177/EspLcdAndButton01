idf_component_register(
	SRCS "LcdManager.c"
	INCLUDE_DIRS "."
	# A compile error noted that 'driver' was required.
	# See Installs/EspIdf/components/driver/CMakeLists.txt
	PRIV_REQUIRES "driver"
	)
