# Adventure Game Studio 2.03

This is the first version to include Windows binaries.\
Switched to SeeR 0.94a. Windows build is using Allegro 3.9.27.

## Basic information

November 1999

### Common

- CLIB32: 1.1
- Script Editor: 1.2 (run-time component)
- PathFinder: 2.0
- MouseLib32: ?? (1998)
- ChrisRoom: 2.00
- WGT2Allegro: ?? (1998)

### DOS version

Build: 2.03.039\
Date: 22 November 1999 at 11:23:35

- DJGPP: 2.0.1 (October 31 1996 by GCC 2.7.2.1) (Walnut Creek CDROM August 1998)
- GCC: 2.8.x(?) (decided to go with 2.8.1)
- Allegro: 3.1 (19990103)
- SeeR: 0.94a
- JGMOD: ?? (1999?) (decided to go with 0.99)
- libamp: 0.2

### Windows version

Build: 2.03.039\
Date: 24 November 1999 at 21:56:48

- Windows: 95 (4.0)
- Visual C++: 6.0 Build 8168
- Allegro: 3.9.27 WIP (MSVC)
- SeeR: 0.94a
- JGMOD: ??
- libamp: ??

"Rich" header data extracted from Windows PE of `acwin.exe`:

```
         id  ver cnt
000c1c7b 0c 7291   9 [AOb] VS6 OLDNAMES.LIB
000e1c83 0e 7299  21 [ASM] MASM 6.13 build 7299
000a1fe8 0a 8168 119 [ C ] VS6 build 8168
00000000 00    0  10 [---] Unmarked objects (old)
00131f62 13 8034   5 [LNK] 5.12 build 8034 (Likely Libs)
00010000 01    1 126 [---] Unmarked objects
000b1fe8 0b 8168  11 [C++] VS6 build 8168
00041fe8 04 8168   3 [LNK] VS6 build 8168
```

## Changes

Functions added: 4
```
__sc_displayspeech
move_object
NewRoomEx
MoveObject
```

Functions modified: 26
```
allocall
load_room
prepare_text_script
post_script_cleanup
fix_bitmap_size
save_room_data_segment
load_new_room
run_animation_stage
update_stuff
printtext
draw_interface
atexit_handler
quit
load_game_file
break_up_text_into_lines
Display
play_flc_file
walk_character
do_movelist_move
SetPlayerCharacter
MoveCharacterToHotspot
run_graph_commandlist
setup_script_exports
mainloop
main_game_loop
main
```

## Links

https://github.com/dishather/richprint
