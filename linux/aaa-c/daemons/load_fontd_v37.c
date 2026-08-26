/**********************************************************************
 * LPC Decompiled Source (v37 - Readability Enhanced)
 * File: load_fontd.o
 * Source: /daemons/load_fontd.c
 * Inherits: (none)
 * Functions: 2 names, 2 code segments found
 * Strings: 4, Variables: 1
 *********************************************************************/

/* ================================================================
   Global/Object Variables
   ================================================================ */
   mapping font_files;

/* ================================================================
   FORWARD DECLARATIONS
   ================================================================ */

private void destruct();
private void create();

/* ================================================================
   GLOBAL VARIABLE INITIALIZATIONS (from init code segment)
   ================================================================ */

   font_files = ([ "gbsn00lp.ttf" : 0, "gkai00mp.ttf" : 0 ]);

/* ================================================================
   FUNCTION DEFINITIONS
   ================================================================ */

// --------------------------------------------------------------------
// @brief  destructor: cleanup before object removal
// @code  nc=22, nargs=0, insts=22
// --------------------------------------------------------------------
private void destruct() {
    /* ---- Enhanced Pseudocode ---- */
    int fob;

    loop (fob in keys(font_files)) {
        if ((font_files[fob] == 1)) {
            verigen_remove_font(fob);
        }
    }  // end loop (verified)

}

// --------------------------------------------------------------------
// @brief  constructor: initialize object state
// @code  nc=33, nargs=0, insts=33
// --------------------------------------------------------------------
private void create() {
    /* ---- Enhanced Pseudocode ---- */
    int file;
    mixed buf;

    loop (file in keys(font_files)) {
        buf = read_etc_file(file, "b");
        if ((!verigen_load_font(file, buf))) {
            error("Load font file %s failed.\n", file);
        }
        font_files[file] = 1;
    }  // end loop (verified)

}

/* ================================================================
   DECOMPILATION SUMMARY
   ================================================================
   Total function names:        2
   Code segments found:         2
   Successfully decoded:        2
   Unresolved/minimal:          0
   Functions without code:      0
   ================================================================ */