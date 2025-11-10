#ifndef __LEXKIT_BIDI__
#define __LEXKIT_BIDI__

#include <lexkit/alloc.h>
#include <lexkit/types.h>
#include <lexkit/break.h>

struct LkLevelRun
{
  i32 start_i;
  i32 end_i;
  i32 valid_start_i;
  i32 valid_end_i;
  i32 level;
};

typedef struct LkLevelRun LkLevelRun;

struct LkParagraph
{
  i32 para_start_i;
  i32 para_end_i;
  i32 para_level;
  struct LkParagraph* next;
};

typedef struct LkParagraph LkParagraph;

typedef struct
{
  BIDIC bidic;
  u32   bidipb;
  BIDIPBT bidipbt;
  BIDIC bidic_orig;
} BidiUnit;

BidiUnit BidiUnitCreate(const u32 codepoint, LkUnicodeData* ud);
BidiUnit BidiUnitCreateTwoStep(const u32 codepoint, LkUnicodeData* ud, LkUnicodeDataTwoStep* udts);

extern const i32 g_bidi_max_depth; // Fixed by Unicode, guaranteed to never change

void lkSplitParagraphs(
    LkArena* arena,
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    i32* o_paragraph_count,
    LkParagraph** o_paragraphs);

void lkSplitParagraphsTwoStep(
    LkArena* arena,
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    LkUnicodeDataTwoStep* udts,
    i32* o_paragraph_count,
    LkParagraph** o_paragraphs);

void lkSplitBidiRuns(
    LkArena* arena,
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    i32 paragraph_count,
    LkParagraph* paragraphs,
    i32** o_levels,
    i32* o_level_run_count,
    LkLevelRun** o_level_runs);

void lkSplitBidiRunsTwoStep(
    LkArena* arena,
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    LkUnicodeDataTwoStep* udts,
    i32 paragraph_count,
    LkParagraph* paragraphs,
    i32** o_levels,
    i32* o_level_run_count,
    LkLevelRun** o_level_runs);

#endif // __LEXKIT_BIDI__
