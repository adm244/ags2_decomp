/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Main system driver for the Windows library.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>
#include <ctype.h>
#include <process.h>

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"
#include <time.h>

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

/* general */
#define ALLEGRO_WND_CLASS "AllegroWindow"
#define ALLEGRO_WND_TITLE "Allegro"
HWND allegro_wnd = NULL;
static HWND user_wnd = NULL;
static WNDPROC user_wnd_proc = NULL;
static HANDLE wnd_thread = NULL;
static int wnd_width = 0;
static int wnd_height = 0;
static int old_style = 0;
static BOOL allow_wm_close = FALSE;

/* background drawing */
DWORD wnd_back_color = 0x0000ffff;
BOOL wnd_paint_back = FALSE;

/* custom window msgs */
static UINT msg_call_proc = 0;
static UINT msg_acquire_mouse = 0;



/* win_set_window:
 *  Selects a user degined window for Allegro
 */
void win_set_window(HWND wnd)
{
   /* todo: add code to remove old window */
   user_wnd = wnd;
}



/* win_get_window:
 *  returns the allegro window handle
 */
HWND win_get_window(void)
{
   return (user_wnd ? user_wnd : allegro_wnd);
}



/* wnd_call_proc:
 *  lets call a procedure from the window thread
 */
int wnd_call_proc(void (*proc) (void))
{
   if (proc)
      return SendMessage(allegro_wnd, msg_call_proc, (DWORD) proc, 0);
   else
      return -1;
}



/* wnd_acquire_mouse:
 *  post msg to window to acquire the mouse device
 */
void wnd_acquire_mouse(void)
{
   PostMessage(allegro_wnd, msg_acquire_mouse, 0, 0);
}



/* directx_wnd_proc:
 *  window proc for the Allegro window class
 */
static LRESULT CALLBACK directx_wnd_proc(HWND wnd, UINT message,
					 WPARAM wparam, LPARAM lparam)
{
   PAINTSTRUCT ps;

   if (message == msg_call_proc)
      return ((int (*)(void))wparam) ();

   if (message == msg_acquire_mouse)
      return mouse_dinput_acquire();

   switch (message) {
      case WM_CREATE:
	 allegro_wnd = wnd;
	 break;

      case WM_SETCURSOR:
	 SetCursor(NULL);
	 return TRUE;

      case WM_DESTROY:
	 allegro_wnd = NULL;
	 PostQuitMessage(0);
	 break;

      case WM_ACTIVATE:
	 if (LOWORD(wparam) == WA_INACTIVE)
	    sys_switch_out();
	 else
	    sys_switch_in();
	 break;

      case WM_ENTERMENULOOP:
      case WM_ENTERSIZEMOVE:
	 sys_switch_out();
	 break;

      case WM_EXITMENULOOP:
      case WM_EXITSIZEMOVE:
	 if (GetActiveWindow() == allegro_wnd && !IsIconic(allegro_wnd))
	    sys_switch_in();
	 break;

      case WM_MOVE:
	 handle_window_size((int)LOWORD(lparam), (int)HIWORD(lparam),
			    wnd_width, wnd_height);
	 break;

      case WM_SIZE:
	 wnd_width = LOWORD(lparam);
	 wnd_height = HIWORD(lparam);
	 break;
      
      case WM_NCPAINT:
      	 if (!wnd_paint_back)
	    return -1;
	 break;

      case WM_PAINT:
         if (!wnd_paint_back)
         {
            BeginPaint(wnd, &ps);
            EndPaint(wnd, &ps);
         }
         break;

      case WM_CLOSE:
         if (!allow_wm_close) return 0;
         break;
   }

   /* pass message to default window proc */
   if (user_wnd_proc)
      return CallWindowProc(user_wnd_proc, wnd, message, wparam, lparam);
   else
      return DefWindowProc(wnd, message, wparam, lparam);
}



/* hook_user_window:
 *  inserts the directx_wnd_proc into the proc chain of current window
 */
static int hook_user_window(void)
{
   user_wnd_proc = (WNDPROC) SetWindowLong(user_wnd, GWL_WNDPROC, (long)directx_wnd_proc);
   if (user_wnd_proc)
      return 0;
   else
      return -1;
}



/* restore_window_style:
 */
void restore_window_style(void)
{
   SetWindowLong(allegro_wnd, GWL_STYLE, old_style);
}



/* create_directx_window:
 *  create the Allegro window
 */
static HWND create_directx_window(void)
{
   HWND wnd;
   WNDCLASS wnd_class;
   int err;

   /* setup the window class */
   wnd_class.style = CS_HREDRAW | CS_VREDRAW;
   wnd_class.lpfnWndProc = directx_wnd_proc;
   wnd_class.cbClsExtra = 0;
   wnd_class.cbWndExtra = 0;
   wnd_class.hInstance = allegro_inst;
   wnd_class.hIcon = LoadIcon(allegro_inst, IDI_APPLICATION);
   wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
   wnd_class.hbrBackground = NULL;
   wnd_class.lpszMenuName = NULL;
   wnd_class.lpszClassName = ALLEGRO_WND_CLASS;

   RegisterClass(&wnd_class);

   /* create the window now */
   wnd = CreateWindowEx(
			  WS_EX_TOPMOST,
			  ALLEGRO_WND_CLASS,
			  ALLEGRO_WND_TITLE,
			  WS_OVERLAPPED | WS_MINIMIZEBOX,
			  -1000, -1000, 0, 0,
			  NULL, NULL,
			  allegro_inst,
			  NULL);

   if (wnd == NULL) {
      err = GetLastError();
      TRACE("CreateWindowEx = %s (%x)\n", win_err_str(err), err);
      return NULL;
   }

   ShowWindow(wnd, SW_SHOWNORMAL);
   UpdateWindow(wnd);

   return wnd;
}



/* wnd_thread_proc:
 *  thread that handles the messages of the directx window
 */
static void wnd_thread_proc(HANDLE setup_event)
{
   MSG msg;

   win_init_thread();

   /* setup window */
   allegro_wnd = create_directx_window();
   if (allegro_wnd == NULL)
      goto Error;

   /* now the thread it running successfully, let's acknowledge */
   SetEvent(setup_event);

   /* message loop */
   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

 Error:
   win_exit_thread();

   TRACE("window thread exits\n");
}



/* init_directx_window:
 *  If the user has called win_set_window, the user window will be hooked to
 *  receive messages from Allegro. Otherwise a thread is created that creates
 *  a new window.
 */
int init_directx_window(void)
{
   HANDLE events[2];
   long result;

   /* setup globals */
   msg_call_proc = RegisterWindowMessage("Allegro call proc");
   msg_acquire_mouse = RegisterWindowMessage("Allegro mouse acquire proc");
   allow_wm_close = FALSE;
   
   /* prepare window for allegro */
   if (user_wnd) {
      /* hook the user window */
      if (hook_user_window() != 0)
	 return -1;
      allegro_wnd = user_wnd;
   }
   else {
      /* create window thread */
      events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);	/* acknowledges that thread is up */
      events[1] = (HANDLE) _beginthread(wnd_thread_proc, 0, events[0]);
      result = WaitForMultipleObjects(2, events, FALSE, INFINITE);

      CloseHandle(events[0]);

      switch (result) {
	 case WAIT_OBJECT_0:	/* window was created successfully */
	    wnd_thread = events[1];
	    break;

	 default:		/* thread failed to create window */
	    return -1;
      }      

      /* this should never happen because the thread would also stop */
      if (allegro_wnd == NULL)
	 return -1;
   }

   /* save window style */
   old_style = GetWindowLong(allegro_wnd, GWL_STYLE);

   return 0;
}



/* exit_directx_window:
 *  if a user window was hooked, the old window proc is set. Otherwise
 *  the created window is destroy.
 */
void exit_directx_window(void)
{
   if (user_wnd_proc) {
      /* restore old window proc */
      SetWindowLong(user_wnd, GWL_WNDPROC, (long)user_wnd_proc);
      user_wnd_proc = NULL;
   }
   else {
      /* close window */
      allow_wm_close = TRUE;
      PostMessage(allegro_wnd, WM_CLOSE, 0, 0);      

      /* wait until the window thread ends */
      WaitForSingleObject(wnd_thread, INFINITE);
      wnd_thread = NULL;
   }
}
