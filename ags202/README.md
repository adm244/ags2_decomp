# Adventure Game Studio 2.02

September 1999

- DJGPP: 2.0.1 (October 31 1996 by GCC 2.7.2.1) (Walnut Creek CDROM August 1998)
- GCC: 2.8.x(?) (decided to go with 2.8.1)
- Allegro: 3.1 (19990103)
- SeeR: 0.92a (couldn't find this exact version, patched 0.94a to work with 0.92a scripts)
- JGMOD: ?? (1999?) (decided to go with 0.99)
- libamp: 0.2

By CJ:
- CLIB32: 1.1
- Script Editor: 1.2 (run-time component)
- PathFinder: 2.0
- MouseLib32: ?? (1998)
- ChrisRoom: 2.00
- WGT2Allegro: ?? (1998)

## Changes

Functions added: 4
```
offset_over_inv
update_invorder
GetInvName
_sc_strcat
```

Functions modified: 26
```
wloadfont
enable_cursor_mode
disable_cursor_mode
run_event_block_inv
check_controls
draw_interface
draw_screen_background
atexit_handler
load_game_file
play_audio_clip_by_index
add_inventory
run_event_block
ProcessClick
do_movelist_move
newmusic
GetLocationName
script_debug
run_graph_commandlist
save_game
load_game
__actual_invscreen
setup_script_exports
mainloop
main
preparesavegamelist
clibfopen
```
