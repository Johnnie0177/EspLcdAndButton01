idf_component_register(
	SRCS "ButtonManager.c" "ButtonManagerTest.c"
	INCLUDE_DIRS "."
	# A compile error noted that 'esp_driver_gpio' was required.
	# See Installs/EspIdf/components/esp_driver_gpio/CMakeLists.txt
	PRIV_REQUIRES "esp_driver_gpio"
	)
