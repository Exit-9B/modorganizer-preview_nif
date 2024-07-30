cmake_minimum_required(VERSION 3.22)

add_library(preview_nif SHARED)
mo2_configure_plugin(
	preview_nif
	WARNINGS OFF
	PRIVATE_DEPENDS Qt::OpenGLWidgets libbsarch uibase
)
mo2_install_target(preview_nif)
