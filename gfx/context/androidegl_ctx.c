/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2012 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2012 - Daniel De Matteis
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../driver.h"
#include "../gl_common.h"

#include <EGL/egl.h> /* Requires NDK r5 or newer */
#include <GLES/gl.h>

#include <stdint.h>

enum RenderThreadMessage {
   MSG_NONE = 0,
   MSG_WINDOW_SET,
   MSG_RENDER_LOOP_EXIT
};

enum RenderThreadMessage _msg;
   
ANativeWindow *window;   /* Requires NDK r5 or newer */

static EGLContext g_egl_ctx;
static EGLSurface g_egl_surf;
static EGLDisplay g_egl_dpy;
static EGLConfig g_config;

int _width;
int _height;

static enum gfx_ctx_api g_api;

static int gfx_ctx_check_resolution(unsigned resolution_id)
{
   (void)resolution_id;
   return 0;
}

static unsigned gfx_ctx_get_resolution_width(unsigned resolution_id)
{
   (void)resolution_id;
   return 0;
}

static unsigned gfx_ctx_get_resolution_height(unsigned resolution_id)
{
   (void)resolution_id;
   return 0;
}

static float gfx_ctx_get_aspect_ratio(void)
{
   return 4.0f / 3.0f;
}

static void gfx_ctx_get_available_resolutions(void)
{}

static void gfx_ctx_set_swap_interval(unsigned interval)
{
   eglSwapInterval(g_egl_dpy, interval);
}

static void gfx_ctx_destroy(void)
{
    eglMakeCurrent(g_egl_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(g_egl_dpy, g_egl_ctx);
    eglDestroySurface(g_egl_dpy, g_egl_surf);
    eglTerminate(g_egl_dpy);
   
    g_egl_dpy = EGL_NO_DISPLAY;
    g_egl_surf = EGL_NO_SURFACE;
    g_egl_ctx = EGL_NO_CONTEXT;
    g_config   = 0;

    _width = 0;
    _height = 0;
}

static bool gfx_ctx_init(void)
{
   const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config;    
    EGLint numConfigs;
    EGLint format;
    EGLint width;
    EGLint height;
    GLfloat ratio;
   
    RARCH_LOG("Initializing context\n");
   
    if ((g_egl_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY)
        goto error;

    EGLint egl_major, egl_minor;
    if (!eglInitialize(g_egl_dpy, &egl_major, &egl_minor))
        goto error;

    RARCH_LOG("[ANDROID/EGL]: EGL version: %d.%d\n", egl_major, egl_minor);

    EGLint num_configs;
    if (!eglChooseConfig(g_egl_dpy, attribs, &g_config, 1, &numConfigs))
        goto error;

    if (!eglGetConfigAttrib(g_egl_dpy, config, EGL_NATIVE_VISUAL_ID, &format))
        goto error;

    ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    if (!(g_egl_surf = eglCreateWindowSurface(g_egl_dpy, config, window, 0)))
        goto error;
   
    if (!(g_egl_ctx = eglCreateContext(g_egl_dpy, config, 0, 0)))
        goto error;
   
    if (!eglMakeCurrent(g_egl_dpy, g_egl_surf, g_egl_surf, g_egl_ctx))
        goto error;

    if (!eglQuerySurface(g_egl_dpy, g_egl_surf, EGL_WIDTH, &width) ||
        !eglQuerySurface(g_egl_dpy, g_egl_surf, EGL_HEIGHT, &height))
        goto error;

    _width = width;
    _height = height;

    return true;

error:
    RARCH_ERR("Returned error %d.\n", eglGetError());
    gfx_ctx_destroy();
    return false;
}

void gfx_ctx_check_window(bool *quit,
      bool *resize, unsigned *width, unsigned *height, unsigned frame_count)
{
   (void)quit;
   (void)resize;
   (void)width;
   (void)height;
   (void)frame_count;

   // process incoming messages
   switch (_msg)
   {
      case MSG_WINDOW_SET:
         gfx_ctx_init();
	 break;
      case MSG_RENDER_LOOP_EXIT:
	 *quit = false;
	 gfx_ctx_destroy();
	 break;
      default:
	 break;
   }
   _msg = MSG_NONE;
}

static void gfx_ctx_swap_buffers(void)
{
   eglSwapBuffers(g_egl_dpy, g_egl_surf);
}

static void gfx_ctx_clear(void)
{}

static void gfx_ctx_set_blend(bool enable)
{}

static void gfx_ctx_set_resize(unsigned width, unsigned height)
{
   (void)width;
   (void)height;
}

static bool gfx_ctx_menu_init(void)
{}

static void gfx_ctx_update_window_title(bool reset)
{
   (void)reset;
}

void gfx_ctx_set_window(JNIEnv *jenv,jobject obj, jobject surface)
{
   window = ANativeWindow_fromSurface(jenv, surface);
}

void gfx_ctx_free_window(JNIEnv *jenv,jobject obj, jobject surface)
{
   ANativeWindow_release(window);
}

static void gfx_ctx_get_video_size(unsigned *width, unsigned *height)
{
   (void)width;
   (void)height;
}


bool gfx_ctx_set_video_mode(
      unsigned width, unsigned height,
      unsigned bits, bool fullscreen)
{
   (void)width;
   (void)height;
   (void)bits;
   (void)fullscreen;
   return true;
}


void gfx_ctx_input_driver(const input_driver_t **input, void **input_data)
{
   *input = NULL;
   *input_data = NULL;
}

void gfx_ctx_set_filtering(unsigned index, bool set_smooth)
{
   (void)index;
   (void)set_smooth;
}

static void gfx_ctx_set_fbo(bool enable)
{
   (void)enable;
}

static void gfx_ctx_apply_fbo_state_changes(unsigned mode)
{
   (void)mode;
}

static void gfx_ctx_set_aspect_ratio(void *data, unsigned aspectratio_index)
{
   (void)data;
   (void)aspectratio_index;
}

static void gfx_ctx_set_overscan(void)
{}

// Enforce void (*)(void) as it's not really legal to cast void* to fn-pointer.
// POSIX allows this, but strict C99 doesn't.
#ifndef __PSL1GHT__
static gfx_ctx_proc_t gfx_ctx_get_proc_address(const char *symbol)
{
   rarch_assert(sizeof(void*) == sizeof(void (*)(void)));
   gfx_ctx_proc_t ret;

   void *sym__ = eglGetProcAddress(symbol);
   memcpy(&ret, &sym__, sizeof(void*));

   return ret;
}
#endif

static bool gfx_ctx_bind_api(enum gfx_ctx_api api)
{
   g_api = api;
   return api == GFX_CTX_OPENGL_ES_API;
}

static bool gfx_ctx_has_focus(void)
{
   return true;
}

const gfx_ctx_driver_t gfx_ctx_android = {
   gfx_ctx_init,
   gfx_ctx_destroy,
   gfx_ctx_bind_api,
   gfx_ctx_set_swap_interval,
   gfx_ctx_set_video_mode,
   gfx_ctx_get_video_size,
   NULL,
   gfx_ctx_update_window_title,
   gfx_ctx_check_window,
#ifdef ANDROID
   gfx_ctx_set_window,
   gfx_ctx_free_window,
#endif
   gfx_ctx_set_resize,
   gfx_ctx_has_focus,
   gfx_ctx_swap_buffers,
   gfx_ctx_input_driver,
   NULL,
   "android",

   // RARCH_CONSOLE stuff.
   gfx_ctx_set_filtering,
   gfx_ctx_get_available_resolutions,
   gfx_ctx_check_resolution,

#ifdef HAVE_CG_MENU
   gfx_ctx_menu_init,
#else
   NULL,
#endif

   gfx_ctx_set_fbo,
   gfx_ctx_apply_fbo_state_changes,
};
