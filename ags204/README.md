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
Date: ???

- Windows: 95 (4.0)
- Visual C++: 6.0 Build 8168 (Professional)
- Allegro: 3.9.27 WIP (MSVC)
- SeeR: 0.94a

"Rich" header data extracted from Windows PE of `acwin.exe`:

```
???
```

## Changes

Functions added: 4
```
remove_screen_overlay
add_screen_overlay
SaveScreenShot
is_pos_in_sprite
```

Functions modified: 24
```
save_room
load_room
run_text_script
load_new_room
check_controls
update_stuff
draw_screen_background
draw_topbar
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
```

## Links

https://github.com/dishather/richprint
