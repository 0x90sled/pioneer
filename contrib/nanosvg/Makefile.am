#  Process this file with automake to produce Makefile.in
include $(top_srcdir)/Makefile.common

AM_CFLAGS += $(SDL2_CFLAGS)

AM_CPPFLAGS += "-I$(top_srcdir)/contrib/nanosvg"
# noinst_LIBRARIES = libimgui.a
# libimgui_a_SOURCES = imgui.cpp imgui_draw.cpp examples/sdl_opengl3_example/imgui_impl_sdl_gl3.cpp
noinst_HEADERS = nanosvg.h nanosvgrast.h
