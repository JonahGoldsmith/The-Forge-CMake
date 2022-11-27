cmake_minimum_required(VERSION 3.22)

project(IMGUI)

set(SOURCES
        imgui_tables.cpp
        imgui_widgets.cpp
        imgui_draw.cpp
        imgui_demo.cpp
        imgui.cpp
        imgui_internal.h
        imgui.h)

add_library(IMGUI STATIC ${SOURCES})