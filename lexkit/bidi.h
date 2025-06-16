#ifndef __LEXKIT_BIDI__
#define __LEXKIT_BIDI__

#include <lexkit/types.h>
#include <lexkit/break.h>

typedef struct
{
  i32 start_i;
  i32 end_i;
  i32 valid_start_i;
  i32 valid_end_i;
  i32 level;
} LevelRun;

typedef struct
{
  i32 para_start_i;
  i32 para_end_i;
  i32 para_level;
} Paragraph;

typedef struct
{
  BIDIC bidic;
  u32   bidipb;
  BIDIPBT bidipbt;
  BIDIC bidic_orig;
} BidiUnit;

BidiUnit BidiUnitCreate(const u32 codepoint, LkUnicodeData* ud);

extern const i32 g_bidi_max_depth; // Fixed by Unicode, guaranteed to never change

void lkSplitParagraphs(
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    i32* o_paragraph_count,
    Paragraph** o_paragraphs);

void lkSplitBidiRuns(
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    i32 paragraph_count,
    Paragraph* paragraphs,
    i32** o_levels,
    i32* o_level_run_count,
    LevelRun** o_level_runs);

#endif // __LEXKIT_BIDI__
