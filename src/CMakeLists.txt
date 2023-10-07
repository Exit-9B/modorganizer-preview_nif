cmake_minimum_required(VERSION 3.16)

add_library(preview_nif SHARED)
mo2_configure_plugin(
	preview_nif
	WARNINGS OFF
	PRIVATE_DEPENDS
		libbsarch
		Qt::OpenGLWidgets
)
mo2_install_target(preview_nif)
