# Adventure Game Studio 2.04

## Basic information

January 2000

### Common

- CLIB32: 1.1
- Script Editor: 1.2 (run-time component)
- PathFinder: 2.0
- MouseLib32: ?? (1998)
- ChrisRoom: 2.00
- WGT2Allegro: ?? (1998)

### DOS version

Build: 2.04.042\
Date: 8 January 2000 at 20:35:16

- DJGPP: 2.0.1 (October 31 1996 by GCC 2.7.2.1) (Walnut Creek CDROM August 1998)
- GCC: 2.8.x(?) (decided to go with 2.8.1)
- Allegro: 3.1 (19990103)
- SeeR: 0.94a
- JGMOD: ?? (1999?) (decided to go with 0.99)
- libamp: 0.2

### Windows version

Build: 2.04.042\
Date: 11 January 2000 at 11:59:36

- Windows: 95 (4.0)
- Visual C++: 6.0 Build 8168 (Professional)
- Allegro: 3.9.27 WIP (MSVC)
- SeeR: 0.94a

"Rich" header data extracted from Windows PE of `acwin.exe`:

```
         id  ver cnt
000c1c7b 0c 7291   9 [AOb] build 7291 (VS6 OLDNAMES.LIB)
000e1c83 0e 7299  21 [ASM] MASM 6.13 build 7299
000a1fe8 0a 8168 119 [ C ] VS98 (6.0) build 8168
00000000 00    0  11 [---] Unmarked objects (old)
00131f62 13 8034   5 [LNK] 5.12 build 8034 (Likely Libs)
00010000 01    0 128 [---] Unmarked objects
000b1fe8 0b 8168  12 [C++] VS98 (6.0) build 8168
000606b8 06 1720   1 [RES] VS98 (6.0) cvtres build 1720
00041fe8 04 8168   3 [LNK] VS98 (6.0) imp/exp build 8168
```

## Changes

Functions added: 5
```
DisplayAlert
remove_screen_overlay
add_screen_overlay
SaveScreenShot
is_pos_in_sprite
```

Functions modified: 29
```
save_room
load_room
run_text_script
load_new_room
check_controls
update_stuff
draw_screen_background
draw_topbar
atexit_handler*
quit*
draw_button_background
_display_at
DisplaySpeech
DisplayMessage
CaptureScreen
EndCapture
CapturedPrint
run_event_block
check_click_on_object
is_pos_on_character
ProcessClick
GetLocationName
load_game
setup_script_exports
main_game_loop
main
mgetgraphpos
mgraphconfine
minstalled
```

\* - the change is due to a macro redefinition; no source code was modified

## Links

https://github.com/dishather/richprint
