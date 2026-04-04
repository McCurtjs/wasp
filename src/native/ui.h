
#ifndef WASP_NATIVE_UI_
#define WASP_NATIVE_UI_

#ifdef _MSC_VER
#pragma warning ( push )
#pragma warning ( disable : 5287 )
#endif

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_SDL3
#define CIMGUI_USE_OPENGL3
#include "cimgui.h"
#include "cimgui_impl.h"

#ifdef _MSC_VER
#pragma warning ( pop )
#endif

#endif
