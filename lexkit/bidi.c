#include <lexkit/lexkit.h>
#include <lexkit/alloc.h>
#include <lexkit/bidi.h>
#include <lexkit/break.h>
#include <lexkit/job.h>

#include <stdlib.h>
#include <assert.h>

#define UNICODE_LEFT_POINTING_ANGLE_BRACKET 0x2329
#define UNICODE_LEFT_ANGLE_BRACKET 0x3008

BidiUnit BidiUnitCreate(const u32 codepoint, const LkUnicodeData* ud)
{
  BidiUnit ret = {0};
  ret.bidic = BIDIC_L;
  ret.bidic = LkTwoStepLookup(ud->ts_bidi, codepoint);
  if (ret.bidic == BIDIC_ON)
  {
    for (i32 i = 0; i < ud->bidipb_count; i++)
    {
      if (codepoint == ud->bidipb_key[i])
      {
        ret.bidipbt = ud->bidipbt[i];
        ret.bidipb = (ret.bidipbt == BIDIPBT_Open) ? codepoint : ud->bidipb_value[i];
        if (ret.bidipb == UNICODE_LEFT_POINTING_ANGLE_BRACKET)
          ret.bidipb = UNICODE_LEFT_ANGLE_BRACKET;
        break;
      }
    }
  }
  if (ret.bidic == BIDIC_NSM)
  {
    ret.bidic_orig = ret.bidic;
  }
  return ret;
}

const i32 g_bidi_max_depth = 125; // Fixed by Unicode, guaranteed to never change
static const i32 g_bracket_stack_size = 63; // Unicode constant

typedef enum
{
  DIROVR_Neutral,
  DIROVR_LeftToRight,
  DIROVR_RightToLeft,
} DIROVR;

typedef struct
{
  i32 level;
  DIROVR override;
  bool isolate;
} BidiStatus;

static bool IsX6BidiClass(BIDIC bidic)
{
  return (bidic != BIDIC_B) &&
          (bidic != BIDIC_BN) &&
          (bidic != BIDIC_RLE) &&
          (bidic != BIDIC_LRE) &&
          (bidic != BIDIC_RLO) &&
          (bidic != BIDIC_LRO) &&
          (bidic != BIDIC_PDF) &&
          (bidic != BIDIC_RLI) &&
          (bidic != BIDIC_LRI) &&
          (bidic != BIDIC_FSI) &&
          (bidic != BIDIC_PDI);
}

static LkLevelRun LevelRunFromIndex(
    i32* levels,
    i32 start_i,
    i32 end_i,
    i32 para_level)
{
  LkLevelRun lr = {start_i, end_i, start_i, end_i};
  bool valid_found = false;
  i32 current_level = -1;
  for (i32 i = start_i; i <= end_i; i++)
  {
    if (levels[i] != -1)
    {
      if (!valid_found)
      {
        valid_found = true;
        current_level = levels[i];
        lr.end_i = i;
        lr.valid_start_i = i;
        lr.valid_end_i = i;
        lr.level = current_level;
      }
      else if (levels[i] != current_level)
      {
        break;
      }
      else
      {
        lr.end_i = i;
        lr.valid_end_i = i;
      }
    }
    else
    {
      lr.end_i = i;
    }
  }
  return lr;
}

typedef struct
{
  i32 lrun_start_i;
  i32 lrun_end_i;
  BIDIC sos;
  BIDIC eos;
} IsolatingRunSequence;

static i32 max_i32(i32 a, i32 b)
{
  return (a < b) ? b : a;
}

typedef struct
{
  i32 lrun_start_i;
  i32 lrun_end_i;
  i32 unit_start_i;
  i32 unit_end_i;
} BracketPair;

static int CmpBracketPair(void* ctx, const void* v_bp_a, const void* v_bp_b)
{
  const BracketPair* bp_a = (BracketPair*) v_bp_a;
  const BracketPair* bp_b = (BracketPair*) v_bp_b;

  if (bp_a->unit_start_i < bp_b->unit_start_i) return -1;
  if (bp_a->unit_start_i > bp_b->unit_start_i) return 1;
  return 0;
}

static bool FindN1StrongClass(const BIDIC bidic, BIDIC* o_bidic)
{
  if (bidic == BIDIC_L)
  {
    *o_bidic = bidic;
    return true;
  }
  if (bidic == BIDIC_R || bidic == BIDIC_AN || bidic == BIDIC_EN)
  {
    *o_bidic = BIDIC_R;
    return true;
  }
  *o_bidic = BIDIC_L;
  return false;
}

static bool IsNI(const BIDIC bidic)
{
  return (bidic == BIDIC_B ||
          bidic == BIDIC_S ||
          bidic == BIDIC_WS ||
          bidic == BIDIC_ON ||
          bidic == BIDIC_FSI ||
          bidic == BIDIC_LRI ||
          bidic == BIDIC_RLI ||
          bidic == BIDIC_PDI);
}

static void ResolveIsolatingRunSequence(
    LkArena* arena,
    i32* levels,
    BidiUnit* units,
    LkLevelRun* lruns,
    i32* irun_lrun_idxs,
    IsolatingRunSequence irun)
{
  LkArena* scratch = arena->alt;
  u64 scratch_pos = scratch->pos;

  i32 embedding_level = lruns[irun_lrun_idxs[irun.lrun_start_i]].level;
  BIDIC matching_class = (embedding_level & 1) ?
                          BIDIC_R :
                          BIDIC_L;
  BIDIC opposite_class = (embedding_level & 1) ?
                          BIDIC_L :
                          BIDIC_R;

  // W1 + W2
  BIDIC cls_prev = irun.sos;
  BIDIC cls_prev_str = irun.sos;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_NSM)
      {
        if (cls_prev == BIDIC_LRI ||
            cls_prev == BIDIC_RLI ||
            cls_prev == BIDIC_PDI)
        {
          units[unit_i].bidic = BIDIC_ON;
        }
        else
        {
          units[unit_i].bidic = cls_prev;
        }
      }
      else if (cls == BIDIC_EN)
      {
        if (cls_prev_str == BIDIC_AL)
          units[unit_i].bidic = BIDIC_AN;
      }
      cls_prev = units[unit_i].bidic;
      if (cls_prev == BIDIC_L ||
          cls_prev == BIDIC_R ||
          cls_prev == BIDIC_AL)
      {
        cls_prev_str = cls_prev;
      }
    }
  }

  // W3
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_AL)
          units[unit_i].bidic = BIDIC_R;
    }
  }

  // W4
  cls_prev = irun.sos;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if ((cls == BIDIC_ES || cls == BIDIC_CS) &&
          (cls_prev == BIDIC_EN || cls_prev == BIDIC_AN))
      {
        bool found_next = false;
        BIDIC cls_next = irun.eos;
        for (i32 lrun_j = lrun_i; lrun_j <= irun.lrun_end_i; lrun_j++)
        {
          LkLevelRun lrun_next = lruns[irun_lrun_idxs[lrun_j]];
          for (i32 unit_j = (lrun_j == lrun_i) ? unit_i + 1 : lrun_next.valid_start_i;
                unit_j <= lrun_next.valid_end_i; unit_j++)
          {
            if (levels[unit_j] == -1) continue;
            found_next = true;
            cls_next = units[unit_j].bidic;
            break;
          }
          if (found_next) break;
        }
        if (!found_next) cls_next = irun.eos;
        if (cls_prev == cls_next)
        {
          BIDIC cls_common = cls_prev;
          // ES needs EN to change, CS can change into both EN and AN
          if ((cls == BIDIC_ES && cls_common == BIDIC_EN) || (cls == BIDIC_CS))
          {
            units[unit_i].bidic = cls_common;
          }
        }
      }
      cls_prev = units[unit_i].bidic;
    }
  }

  // W5
  cls_prev = irun.sos;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_ET && cls_prev == BIDIC_EN)
        units[unit_i].bidic = BIDIC_EN;
      cls_prev = units[unit_i].bidic;
    }
  }

  BIDIC cls_next = irun.eos;
  for (i32 lrun_i = irun.lrun_end_i; lrun_i >= irun.lrun_start_i; lrun_i--)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_end_i; unit_i >= lrun.valid_start_i; unit_i--)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_ET && cls_next == BIDIC_EN)
        units[unit_i].bidic = BIDIC_EN;
      cls_next = units[unit_i].bidic;
    }
  }

  // W6
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_ET ||
          cls == BIDIC_ES ||
          cls == BIDIC_CS)
          units[unit_i].bidic = BIDIC_ON;
    }
  }

  // W7
  cls_prev_str = irun.sos;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_EN && cls_prev_str == BIDIC_L)
      {
        units[unit_i].bidic = BIDIC_L;
      }
      BIDIC cls_prev = units[unit_i].bidic;
      if (cls_prev == BIDIC_L ||
          cls_prev == BIDIC_R ||
          cls_prev == BIDIC_AL)
      {
        cls_prev_str = cls_prev;
      }
    }
  }

  // N0
  // Identify bracket pairs

  i32 max_bracket_pairs = 0;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    max_bracket_pairs += lrun.valid_end_i - lrun.valid_start_i + 1;
  }
  max_bracket_pairs /= 2;

  typedef struct
  {
    i32 lrun_i;
    i32 unit_i;
  } BracketStackItem;

  BracketPair* bp = APushArrayNZ(scratch, BracketPair, max_bracket_pairs);
  i32 bracket_count = 0;
  BracketStackItem* bracket_stack = APushArrayNZ(scratch, BracketStackItem, g_bracket_stack_size);
  i32 bracket_sp = 0;
  bool stack_overflow = false;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i && !stack_overflow; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (units[unit_i].bidic == BIDIC_ON)
      {
        if (units[unit_i].bidipbt == BIDIPBT_Open)
        {
          if (bracket_sp >= g_bracket_stack_size)
          {
            stack_overflow = true;
            break;
          } 
          else
          {
            bracket_stack[bracket_sp++] = (BracketStackItem) {lrun_i, unit_i};
          }
        }
        else if (units[unit_i].bidipbt == BIDIPBT_Close)
        {
          for (i32 bracket_i = bracket_sp - 1; bracket_i >= 0; bracket_i--)
          {
            if (units[bracket_stack[bracket_i].unit_i].bidipb == units[unit_i].bidipb)
            {
              bp[bracket_count++] = (BracketPair) {
                                        bracket_stack[bracket_i].lrun_i,
                                        lrun_i,
                                        bracket_stack[bracket_i].unit_i,
                                        unit_i };
              bracket_sp = bracket_i;
              break;
            }
          }
        }
      }
    }
  }
  qsort_s(bp, bracket_count, sizeof(BracketPair), CmpBracketPair, NULL);

  // Whether a class was assigned to the matching bracket pair
  bool* assigned_class = APushArrayNZ(scratch, bool, bracket_count);

  for (i32 bracket_i = 0; bracket_i < bracket_count; bracket_i++)
  {
    // 1. Find strong class inside brackets

    bool found_strong_class = false;
    bool found_matching_strong_class = false;

    for (i32 lrun_i = bp[bracket_i].lrun_start_i; lrun_i <= bp[bracket_i].lrun_end_i; lrun_i++)
    {
      LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
      i32 unit_start_i = (lrun_i == bp[bracket_i].lrun_start_i) ? 
                                    bp[bracket_i].unit_start_i :
                                    lrun.valid_start_i;
      i32 unit_end_i = (lrun_i == bp[bracket_i].lrun_end_i) ?
                                  bp[bracket_i].unit_end_i :
                                  lrun.valid_end_i;

      for (i32 unit_i = unit_start_i; unit_i <= unit_end_i; unit_i++)
      {
        if (levels[unit_i] == -1) continue; 
        BIDIC strong_class;
        if (FindN1StrongClass(units[unit_i].bidic, &strong_class))
        {
          found_strong_class = true;
          found_matching_strong_class |= (strong_class == matching_class);
        }
      }
    }

    assigned_class[bracket_i] = (found_strong_class || found_matching_strong_class);

    if (found_matching_strong_class)
    {
      // 2. If matching strong class exists switch brackets to that type 

      units[bp[bracket_i].unit_start_i].bidic = matching_class;
      units[bp[bracket_i].unit_end_i].bidic = matching_class;
    }
    else if (found_strong_class)
    {
      // 3. Else, if opposite strong class exists, switch to the first strong class that
      //  precedes the opening bracket (or sos)

      BIDIC class_search_result = irun.sos;
      bool class_search_found = false; 

      for (i32 lrun_i = bp[bracket_i].lrun_start_i; lrun_i >= irun.lrun_start_i; lrun_i--)
      {
        LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
        i32 unit_start_i = (lrun_i == bp[bracket_i].lrun_start_i) ? 
                                      bp[bracket_i].unit_start_i :
                                      lrun.valid_end_i;

        for (i32 unit_i = unit_start_i; unit_i >= lrun.valid_start_i; unit_i--)
        {
          if (levels[unit_i] == -1) continue; 
          BIDIC strong_class;
          if (FindN1StrongClass(units[unit_i].bidic, &strong_class))
          {
            class_search_found = true;
            class_search_result = strong_class;
            break;
          }
        }
        
        if (class_search_found) break;
      }

      BIDIC resolved_class = (class_search_result == opposite_class) ? opposite_class : matching_class;
      units[bp[bracket_i].unit_start_i].bidic = resolved_class;
      units[bp[bracket_i].unit_end_i].bidic = resolved_class;
    }

    // 4. Else, don't set bracket pair
  }

  // Set all following (original, before W1) NSMs to match any changed brackets

  bool adjacent_changed_bracket = false;
  BIDIC adjacent_changed_class = BIDIC_L;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;

      // TODO: This could be faster, full linear search isn't required

      bool is_changed_bracket = false;
      for (i32 bracket_i = 0; bracket_i < bracket_count; bracket_i++)
      {
        if (assigned_class[bracket_i] && (bp[bracket_i].unit_start_i == unit_i ||
              bp[bracket_i].unit_end_i == unit_i))
        {
          adjacent_changed_bracket = true;
          adjacent_changed_class = units[unit_i].bidic;
          is_changed_bracket = true;
          break;
        }
      }
      if (is_changed_bracket)
        continue;

      if (units[unit_i].bidic_orig == BIDIC_NSM && adjacent_changed_bracket)
      {
        units[unit_i].bidic = adjacent_changed_class;
      }
      else
      {
        adjacent_changed_bracket = false;
      }
    }
  }

  // N1

  bool in_strong_class_segment = true;
  BIDIC strong_class_segment = irun.sos;
  i32 strong_class_segment_i = lruns[irun_lrun_idxs[irun.lrun_start_i]].valid_start_i;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;

      BIDIC bidic_strong = BIDIC_L;
      if (FindN1StrongClass(units[unit_i].bidic, &bidic_strong))
      {
        if (in_strong_class_segment && bidic_strong == strong_class_segment)
        {
          for (i32 segment_i = strong_class_segment_i; segment_i <= unit_i; segment_i++)
          {
            if (levels[segment_i] != embedding_level) continue;
            if (IsNI(units[segment_i].bidic))
            {
              units[segment_i].bidic = strong_class_segment;
            }
          }
          in_strong_class_segment = true;
          strong_class_segment = bidic_strong;
          strong_class_segment_i = unit_i;
        }
        else
        {
          in_strong_class_segment = true;
          strong_class_segment = bidic_strong;
          strong_class_segment_i = unit_i;
        }
      }
      else if (!IsNI(units[unit_i].bidic))
      {
        in_strong_class_segment = false;
      }
      else if (lrun_i == irun.lrun_end_i && unit_i == lrun.valid_end_i &&
                in_strong_class_segment && strong_class_segment == irun.eos)
      {
        // EOS case for last strong segment

        for (i32 segment_i = strong_class_segment_i; segment_i <= unit_i; segment_i++)
        {
          if (levels[segment_i] != embedding_level) continue;
          if (IsNI(units[segment_i].bidic))
          {
            units[segment_i].bidic = strong_class_segment;
          }
        }
      }
    }
  }

  // N2

  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (IsNI(units[unit_i].bidic))
        units[unit_i].bidic = matching_class;
    }
  }

  // I1 + I2

  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LkLevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1) continue;

      if (levels[unit_i] % 2 == 0)
      {
        // I1

        if (units[unit_i].bidic == BIDIC_R)
        {
          levels[unit_i] += 1;
        }
        else if (units[unit_i].bidic == BIDIC_AN ||
                  units[unit_i].bidic == BIDIC_EN)
        {
          levels[unit_i] += 2;
        }
      }
      else
      {
        // I2

        if (units[unit_i].bidic == BIDIC_L ||
            units[unit_i].bidic == BIDIC_AN ||
            units[unit_i].bidic == BIDIC_EN)
        {
          levels[unit_i] += 1;
        }
      }
    }
  }

  lkArenaRestore(scratch, scratch_pos);
}
  
static void LevelRunSplit(
    LkArena* arena,
    i32* levels, 
    i32 para_start_i,
    i32 para_end_i,
    i32 para_level,
    i32 *o_level_run_count,
    LkLevelRun** o_level_runs)
{
  assert(*o_level_runs == NULL);
  i32 level_run_count = 0;
  i32 focus_i = para_start_i;
  while (focus_i <= para_end_i)
  {
    LkLevelRun lr = LevelRunFromIndex(levels, focus_i, para_end_i, para_level);
    focus_i = lr.end_i + 1;
    level_run_count++;
  }
  *o_level_runs = APushArrayNZ(arena, LkLevelRun, level_run_count);
  i32 level_run_i = 0;
  focus_i = para_start_i;
  while (focus_i <= para_end_i)
  {
    LkLevelRun lr = LevelRunFromIndex(levels, focus_i, para_end_i, para_level);
    (*o_level_runs)[level_run_i++] = lr;
    focus_i = lr.end_i + 1;
  }
  *o_level_run_count = level_run_count;
}

static void lkSplitBidiRunsParagraph(
    LkArena* arena,
    BidiUnit* units,
    i32 len_units,
    i32 para_start_i,
    i32 para_end_i,
    i32 para_level,
    i32* io_level,
    i32* io_matching_isolate)
{
  LkArena* scratch = arena->alt;
  u64 scratch_pos = scratch->pos;

  // X1

  BidiStatus *stack = APushArrayNZ(scratch, BidiStatus, g_bidi_max_depth + 2);
  i32 sp = 0;
  stack[sp++] = (BidiStatus){para_level, DIROVR_Neutral, false};
  i32 overflow_isolate_count = 0;
  i32 overflow_embedding_count = 0;
  i32 valid_isolate_count = 0;
  i32* isolate_stack = APushArrayNZ(scratch, i32, para_end_i - para_start_i + 1);
  i32 isolate_sp = 0;

  for (i32 i = para_start_i; i <= para_end_i; i++)
  {
    io_level[i] = -1;
    io_matching_isolate[i] = -1;
    const BidiUnit unit = units[i];
    switch (unit.bidic)
    {
      // X2
      case BIDIC_RLE:
        {
          assert(sp > 0);
          BidiStatus curr_status = stack[sp - 1];
          i32 next_level = curr_status.level + 1;
          next_level = (next_level & 1) ? next_level : (next_level + 1);
          if (next_level <= g_bidi_max_depth &&
              overflow_embedding_count == 0 &&
              overflow_isolate_count == 0)
          {
            assert(sp < g_bidi_max_depth + 2);
            stack[sp++] = (BidiStatus){next_level, DIROVR_Neutral, false};
          }
          else if (overflow_isolate_count == 0)
          {
            overflow_embedding_count++;
          }
        }
        break;
        
      // X3
      case BIDIC_LRE:
        {
          assert(sp > 0);
          BidiStatus curr_status = stack[sp - 1];
          i32 next_level = curr_status.level + 1;
          next_level = (next_level & 1) ? (next_level + 1) : next_level;
          if (next_level <= g_bidi_max_depth &&
              overflow_embedding_count == 0 &&
              overflow_isolate_count == 0)
          {
            assert(sp < g_bidi_max_depth + 2);
            stack[sp++] = (BidiStatus){next_level, DIROVR_Neutral, false};
          }
          else if (overflow_isolate_count == 0)
          {
            overflow_embedding_count++;
          }
        }
        break;

      // X4
      case BIDIC_RLO:
        {
          assert(sp > 0);
          BidiStatus curr_status = stack[sp - 1];
          i32 next_level = curr_status.level + 1;
          next_level = (next_level & 1) ? next_level : (next_level + 1);
          if (next_level <= g_bidi_max_depth &&
              overflow_embedding_count == 0 &&
              overflow_isolate_count == 0)
          {
            assert(sp < g_bidi_max_depth + 2);
            stack[sp++] = (BidiStatus){next_level, DIROVR_RightToLeft, false};
          }
          else if (overflow_isolate_count == 0)
          {
            overflow_embedding_count++;
          }
        }
        break;

      // X5
      case BIDIC_LRO:
        {
          assert(sp > 0);
          BidiStatus curr_status = stack[sp - 1];
          i32 next_level = curr_status.level + 1;
          next_level = (next_level & 1) ? (next_level + 1) : next_level;
          if (next_level <= g_bidi_max_depth &&
              overflow_embedding_count == 0 &&
              overflow_isolate_count == 0)
          {
            assert(sp < g_bidi_max_depth + 2);
            stack[sp++] = (BidiStatus){next_level, DIROVR_LeftToRight, false};
          }
          else if (overflow_isolate_count == 0)
          {
            overflow_embedding_count++;
          }
        }
        break;

      // X5a
      case BIDIC_RLI:
        {
          assert(sp > 0);
          io_level[i] = stack[sp - 1].level;
          BidiStatus curr_status = stack[sp - 1];
          i32 next_level = curr_status.level + 1;
          next_level = (next_level & 1) ? next_level : (next_level + 1);
          isolate_stack[isolate_sp++] = i;
          if (curr_status.override == DIROVR_LeftToRight)
          {
            units[i].bidic = BIDIC_L;
          }
          else if (curr_status.override == DIROVR_RightToLeft)
          {
            units[i].bidic = BIDIC_R;
          }
          if (next_level <= g_bidi_max_depth &&
              overflow_embedding_count == 0 &&
              overflow_isolate_count == 0)
          {
            valid_isolate_count++;
            assert(sp < g_bidi_max_depth + 2);
            stack[sp++] = (BidiStatus){next_level, DIROVR_Neutral, true};
          }
          else
          {
            overflow_isolate_count++;
          }
        }
        break;

      // X5b
      case BIDIC_LRI:
        {
          assert(sp > 0);
          io_level[i] = stack[sp - 1].level;
          BidiStatus curr_status = stack[sp - 1];
          i32 next_level = curr_status.level + 1;
          next_level = (next_level & 1) ? (next_level + 1) : next_level;
          isolate_stack[isolate_sp++] = i;
          if (curr_status.override == DIROVR_LeftToRight)
          {
            units[i].bidic = BIDIC_L;
          }
          else if (curr_status.override == DIROVR_RightToLeft)
          {
            units[i].bidic = BIDIC_R;
          }
          if (next_level <= g_bidi_max_depth &&
              overflow_embedding_count == 0 &&
              overflow_isolate_count == 0)
          {
            valid_isolate_count++;
            assert(sp < g_bidi_max_depth + 2);
            stack[sp++] = (BidiStatus){next_level, DIROVR_Neutral, true};
          }
          else
          {
            overflow_isolate_count++;
          }
        }
        break;

      // X5c
      case BIDIC_FSI:
        {
          assert(sp > 0);
          io_level[i] = stack[sp - 1].level;
          i32 pdi_match = 0;
          i32 first_isolate_level = 0;
          isolate_stack[isolate_sp++] = i;
          for (int j = i + 1; j < len_units; j++)
          {
            if (units[j].bidic == BIDIC_L)
            {
              if (pdi_match == 0)
              {
                first_isolate_level = 0;
                break;
              }
            }
            else if (units[j].bidic == BIDIC_AL || units[j].bidic == BIDIC_R)
            {
              if (pdi_match == 0)
              {
                first_isolate_level = 1;
                break;
              }
            }
            else if (units[j].bidic == BIDIC_LRI ||
                      units[j].bidic == BIDIC_RLI ||
                      units[j].bidic == BIDIC_FSI)
            {
              pdi_match++;
            }
            else if (units[j].bidic == BIDIC_PDI)
            {
              if (pdi_match == 0)
                break;
              pdi_match--;
            }
          }

          BidiStatus curr_status = stack[sp - 1];
          i32 next_level = curr_status.level + 1;
          bool next_level_condition = (first_isolate_level == 0) ?
                                        (next_level % 2 == 0) :
                                        (next_level % 2 == 1);
          next_level = (next_level_condition) ? next_level : (next_level + 1);
          if (curr_status.override == DIROVR_LeftToRight)
          {
            units[i].bidic = BIDIC_L;
          }
          else if (curr_status.override == DIROVR_RightToLeft)
          {
            units[i].bidic = BIDIC_R;
          }
          if (next_level <= g_bidi_max_depth &&
              overflow_embedding_count == 0 &&
              overflow_isolate_count == 0)
          {
            valid_isolate_count++;
            assert(sp < g_bidi_max_depth + 2);
            stack[sp++] = (BidiStatus){next_level, DIROVR_Neutral, true};
          }
          else
          {
            overflow_isolate_count++;
          }
        }
        break;

      // X6 implemented as default for this switch block

      // X6a
      case BIDIC_PDI:
        {
          assert(sp > 0);
          if (isolate_sp > 0)
          {
            i32 initiator_i = isolate_stack[--isolate_sp];
            io_matching_isolate[i] = initiator_i;
            io_matching_isolate[initiator_i] = i;
          }
          if (overflow_isolate_count > 0)
          {
            overflow_isolate_count--;
          }
          else if (valid_isolate_count == 0)
          {
            // PDI with no matching isolate initiator, do nothing
          }
          else {
            overflow_embedding_count = 0;
            while (sp > 0 && stack[sp - 1].isolate == false)
            {
              sp--;
            }
            assert(sp > 0);
            sp--;
            assert(sp > 0);
            valid_isolate_count--;
          }
          io_level[i] = stack[sp - 1].level;
          if (stack[sp - 1].override == DIROVR_LeftToRight)
          {
            units[i].bidic = BIDIC_L;
          }
          else if (stack[sp - 1].override == DIROVR_RightToLeft)
          {
            units[i].bidic = BIDIC_R;
          }
        }
        break;

      // X7
      case BIDIC_PDF:
        {
          assert(sp > 0);
          if (overflow_isolate_count > 0)
          {
            // Do nothing
          }
          else if (overflow_embedding_count > 0)
          {
            overflow_embedding_count--;
          }
          else if (stack[sp - 1].isolate == false && sp > 1)
          {
            sp--;
          }
          else
          {
            // Do nothing. PDF doesn't match embedding initiator.
          }
        }
        break;

      // X8: All explicit directional embeddings, overrides and isolates 
      //  are completely terminated at the end of each paragraph.

      // X9: Remove all RLE, LRE, RLO, LRO, PDF, and BN characters.
      //  Ignore them past this point

      // X6
      default:
        {
          // This condition excludes all above case blocks + X9 characters
          //  + other BIDIC classes

          if (IsX6BidiClass(unit.bidic))
          {
            io_level[i] = stack[sp - 1].level;
            if (stack[sp - 1].override == DIROVR_LeftToRight)
            {
              units[i].bidic = BIDIC_L;
            }
            else if (stack[sp - 1].override == DIROVR_RightToLeft)
            {
              units[i].bidic = BIDIC_R;
            }
          }
        }
        break;
    }
  }

  while (isolate_sp > 0)
    io_matching_isolate[isolate_stack[--isolate_sp]] = -2;

  // X10
  i32 lrun_count = -1;
  LkLevelRun *lruns = NULL;
  LevelRunSplit(scratch, io_level, para_start_i, para_end_i, para_level, &lrun_count, &lruns);

  // Build isolating run sequences
  bool* lrun_used = APushArray(scratch, bool, lrun_count);
  IsolatingRunSequence* iruns = APushArrayNZ(scratch, IsolatingRunSequence, lrun_count);
  i32* irun_lrun_idxs = APushArrayNZ(scratch, i32, lrun_count);
  i32 irun_count = 0;

  i32 irun_i = 0;
  for (i32 lrun_i = 0; lrun_i < lrun_count; lrun_i++)
  {
    if (lrun_used[lrun_i])
      continue;
    i32 focus = lrun_i;
    iruns[irun_count].lrun_start_i = irun_i;
    while (true)
    {
      irun_lrun_idxs[irun_i] = focus;
      iruns[irun_count].lrun_end_i = irun_i;
      lrun_used[focus] = true;
      irun_i++;
      i32 matching_isolate = io_matching_isolate[lruns[focus].valid_end_i];
      if (matching_isolate <= lruns[focus].valid_end_i)
        break;
      i32 found_match_lrun_i = -1;
      for (i32 lrun_j = focus + 1; lrun_j < lrun_count; lrun_j++)
      {
        if (lruns[lrun_j].start_i > matching_isolate)
          break;
        if (lruns[lrun_j].valid_start_i == matching_isolate)
        {
          found_match_lrun_i = lrun_j;
          break;
        }
      }
      if (found_match_lrun_i == -1)
        break;
      focus = found_match_lrun_i;
    }
    irun_count++;
  }

  // Fill in sos/eos
  for (i32 irun_i = 0; irun_i < irun_count; irun_i++)
  {
    LkLevelRun lrun_first = lruns[irun_lrun_idxs[iruns[irun_i].lrun_start_i]];
    LkLevelRun lrun_last = lruns[irun_lrun_idxs[iruns[irun_i].lrun_end_i]];

    {
      // SOS
      i32 level_a = para_level;
      for (i32 i = lrun_first.start_i - 1; i >= para_start_i; i--)
      {
        if (io_level[i] == -1) continue;
        level_a = io_level[i];
        break;
      }
      i32 level_b = io_level[lrun_first.valid_start_i];
      i32 level_ab = (level_a < level_b) ? level_b : level_a;
      if (level_ab & 1)
        iruns[irun_i].sos = BIDIC_R;
      else
        iruns[irun_i].sos = BIDIC_L;
    }

    {
      // EOS
      i32 level_a = para_level;
      if (io_matching_isolate[lrun_last.valid_end_i] != -2)
      {
        for (i32 i = lrun_last.end_i + 1; i <= para_end_i; i++)
        {
          if (io_level[i] == -1) continue;
          level_a = io_level[i];
          break;
        }
      }
      i32 level_b = io_level[lrun_last.valid_end_i];
      i32 level_ab = (level_a < level_b) ? level_b : level_a;
      if (level_ab & 1)
        iruns[irun_i].eos = BIDIC_R;
      else
        iruns[irun_i].eos = BIDIC_L;
    }
  }

  for (i32 i = 0; i < irun_count; i++)
    ResolveIsolatingRunSequence(scratch, io_level, units, lruns, irun_lrun_idxs, iruns[i]);

  for (i32 i = para_start_i; i <= para_end_i; i++)
  {
    if (io_level[i] == -1)
      io_level[i] = para_level;
  }

  lkArenaRestore(scratch, scratch_pos);
}

typedef struct
{
  int i;
  int sz;
  const LkUnicodeData* ud;
  const u32* codepoints;
  BidiUnit* o_units;
} ComputeBidiUnitData;

static void ComputeBidiUnitJob(int worker_id, void* data)
{
  ComputeBidiUnitData* cdata = data;
  for (int i = cdata->i; i < cdata->i + cdata->sz; i++)
  {
    cdata->o_units[i] = BidiUnitCreate(cdata->codepoints[i], cdata->ud);
  }
}

void lkComputeBidiUnits(
    LkContext* ctx,
    LkArena* arena,
    const u32* codepoints,
    i32 len_codepoints,
    BidiUnit** o_units)
{
  assert(o_units != NULL && *o_units == NULL);

  *o_units = APushArrayNZ(arena, BidiUnit, len_codepoints);
  LkArena* scratch = arena->alt;
  u64 scratch_pos = scratch->pos;
  i32 work_chunk = len_codepoints / ((ctx->queue->num_workers + 1) * 4);
  if (work_chunk < 16384) work_chunk = 16384;
  i32 num_chunks = (len_codepoints / work_chunk) + (len_codepoints % work_chunk > 0);
  ComputeBidiUnitData* job_data = APushArrayNZ(scratch, ComputeBidiUnitData, num_chunks);
  int chunk_i = 0;
  for (i32 i = 0; i < len_codepoints; i += work_chunk)
  {
    job_data[chunk_i].i = i;
    job_data[chunk_i].sz = (i + work_chunk > len_codepoints) ? (len_codepoints - i) : work_chunk;
    job_data[chunk_i].ud = ctx->ud;
    job_data[chunk_i].codepoints = codepoints;
    job_data[chunk_i].o_units = *o_units;
    LkJob job;
    job.func = ComputeBidiUnitJob;
    job.data = &job_data[chunk_i];
    chunk_i++;
    lkJobQueuePush(ctx->queue, job);
  }
  lkJobQueueWait(ctx->queue);
  lkArenaRestore(scratch, scratch_pos);
}

void lkSplitParagraphs(
    LkContext* ctx,
    LkArena* arena,
    const u32* codepoints,
    i32 len_codepoints,
    const BidiUnit* units,
    i32* o_paragraph_count,
    LkParagraph** o_paragraphs)
{
  assert(*o_paragraph_count == -1);
  assert(*o_paragraphs == NULL);

  LkArena* scratch = arena->alt;
  u64 scratch_pos = scratch->pos;

  i32 para_count = 0;
  for (i32 i = 0; i < len_codepoints; i++)
  {
    if (units[i].bidic == BIDIC_B)
      para_count++;
  }
  if (len_codepoints > 0 && units[len_codepoints - 1].bidic != BIDIC_B)
    para_count++;

  *o_paragraph_count = 0;
  *o_paragraphs = APushArrayNZ(arena, LkParagraph, para_count);

  i32 para_start_i = 0;
  bool para_level_found = false;
  i32 para_level = 0;
  i32 isolate_count = 0;
  for (i32 i = 0; i < len_codepoints; i++)
  {
    const BidiUnit unit = units[i];
    if (unit.bidic == BIDIC_LRI || unit.bidic == BIDIC_RLI || unit.bidic == BIDIC_FSI)
    {
      isolate_count++;
    }
    else if (unit.bidic == BIDIC_PDI)
    {
      isolate_count = (isolate_count > 0) ? (isolate_count - 1) : isolate_count;
    }
    else if (unit.bidic == BIDIC_AL || unit.bidic == BIDIC_R)
    {
      if (isolate_count == 0 && !para_level_found)
      {
        para_level_found = true;
        para_level = 1;
      }
    }
    else if (unit.bidic == BIDIC_L)
    {
      if (isolate_count == 0 && !para_level_found)
      {
        para_level_found = true;
        para_level = 0;
      }
    }
    else if (unit.bidic == BIDIC_B)
    {
      (*o_paragraphs)[(*o_paragraph_count)++] = (LkParagraph) { para_start_i, i, para_level };
      para_start_i = i + 1;
      para_level_found = false;
      para_level = 0;
      isolate_count = 0;
    }
  }
  if (len_codepoints > 0 && para_start_i < len_codepoints)
  {
    (*o_paragraphs)[(*o_paragraph_count)++] = (LkParagraph) { para_start_i, len_codepoints - 1, para_level };
  }

  lkArenaRestore(scratch, scratch_pos);
}

static bool IsPlainLtrParagraph(
    const BidiUnit* units,
    i32 para_start_i,
    i32 para_end_i,
    i32 para_level)
{
  const u32 plain_mask =
    (1u << BIDIC_L)  | (1u << BIDIC_EN)  | (1u << BIDIC_ES) | (1u << BIDIC_ET) |
    (1u << BIDIC_CS) | (1u << BIDIC_NSM) | (1u << BIDIC_B)  | (1u << BIDIC_S)  |
    (1u << BIDIC_WS) | (1u << BIDIC_ON);
  if (para_level != 0)
    return false;
  u32 seen = 0;
  for (i32 i = para_start_i; i <= para_end_i; i++)
    seen |= 1u << units[i].bidic;
  return (seen & ~plain_mask) == 0;
}

typedef struct
{
  LkContext* ctx;
  BidiUnit* units;
  i32 len_codepoints;
  const LkParagraph* paragraphs;
  i32 para_begin;
  i32 para_end;
  i32* levels;
  i32* matching_isolate;
  bool* para_is_plain;
  i32 lrun_count;
  i32 lrun_offset;
  LkLevelRun* o_level_runs;
} SplitBidiRunsData;

// Resolves levels for every paragraph in the chunk and stores results in local
//  paragraphs block.
static void SplitBidiRunsResolveJob(int worker_id, void* data)
{
  SplitBidiRunsData* cdata = data;
  LkArena* scratch = cdata->ctx->worker_scratch[worker_id + 1];
  u64 scratch_pos = lkArenaGetPos(scratch);
  i32 lrun_count = 0;
  for (i32 para_i = cdata->para_begin; para_i < cdata->para_end; para_i++)
  {
    const LkParagraph* para_focus = &cdata->paragraphs[para_i];

    // Fast path if the entire path is LTR, entire paragraph is single LTR run at level 0
    cdata->para_is_plain[para_i] = IsPlainLtrParagraph(cdata->units, para_focus->para_start_i, para_focus->para_end_i, para_focus->para_level);
    if (cdata->para_is_plain[para_i])
    {
      for (i32 i = para_focus->para_start_i; i <= para_focus->para_end_i; i++)
      {
        cdata->levels[i] = 0;
        if (cdata->units[i].bidic != BIDIC_B || para_focus->para_start_i == para_focus->para_end_i)
          cdata->units[i].bidic = BIDIC_L;
      }
      lrun_count++;
      continue;
    }

    lkSplitBidiRunsParagraph(
        scratch,
        cdata->units,
        cdata->len_codepoints,
        para_focus->para_start_i,
        para_focus->para_end_i,
        para_focus->para_level,
        cdata->levels,
        cdata->matching_isolate);

    i32 focus_i = para_focus->para_start_i;
    while (focus_i <= para_focus->para_end_i)
    {
      LkLevelRun lr = LevelRunFromIndex(cdata->levels, focus_i, para_focus->para_end_i, para_focus->para_level);
      focus_i = lr.end_i + 1;
      lrun_count++;
    }
  }
  cdata->lrun_count = lrun_count;
  lkArenaRestore(scratch, scratch_pos);
}

// Copies local paragraphs block into shared paragraphs array.
static void SplitBidiRunsWriteJob(int worker_id, void* data)
{
  SplitBidiRunsData* cdata = data;
  i32 lrun_i = cdata->lrun_offset;
  for (i32 para_i = cdata->para_begin; para_i < cdata->para_end; para_i++)
  {
    const LkParagraph* para_focus = &cdata->paragraphs[para_i];
    if (cdata->para_is_plain[para_i])
    {
      cdata->o_level_runs[lrun_i++] = (LkLevelRun) {
        para_focus->para_start_i, para_focus->para_end_i,
        para_focus->para_start_i, para_focus->para_end_i, 0 };
      continue;
    }
    i32 focus_i = para_focus->para_start_i;
    while (focus_i <= para_focus->para_end_i)
    {
      LkLevelRun lr = LevelRunFromIndex(cdata->levels, focus_i, para_focus->para_end_i, para_focus->para_level);
      cdata->o_level_runs[lrun_i++] = lr;
      focus_i = lr.end_i + 1;
    }
  }
  assert(lrun_i == cdata->lrun_offset + cdata->lrun_count);
}

void lkSplitBidiRuns(
    LkContext* ctx,
    LkArena* arena,
    const u32* codepoints,
    i32 len_codepoints,
    BidiUnit* units,
    i32 paragraph_count,
    LkParagraph* paragraphs,
    i32** o_levels,
    i32* o_level_run_count,
    LkLevelRun** o_level_runs)
{
  assert(*o_level_run_count == -1);
  assert(*o_level_runs == NULL);
  assert(o_levels != NULL && *o_levels == NULL);

  LkArena* scratch = arena->alt;
  u64 scratch_pos = scratch->pos;

  *o_levels = APushArrayNZ(arena, i32, len_codepoints);
  i32* matching_isolate = APushArrayNZ(scratch, i32, len_codepoints);
  bool* para_is_plain = APushArrayNZ(scratch, bool, paragraph_count);

  // Split set of paragraphs into chunks of roughly equal codepoint count.
  i32 work_chunk = len_codepoints / ((ctx->queue->num_workers + 1) * 4);
  if (work_chunk < 16384) work_chunk = 16384;
  i32 max_chunks = len_codepoints / work_chunk + 1;
  SplitBidiRunsData* job_data = APushArrayNZ(scratch, SplitBidiRunsData, max_chunks);
  i32 chunk_count = 0;
  i32 para_i = 0;
  while (para_i < paragraph_count)
  {
    SplitBidiRunsData* cdata = &job_data[chunk_count++];
    i32 chunk_codepoints = 0;
    cdata->para_begin = para_i;
    while (para_i < paragraph_count && chunk_codepoints < work_chunk)
    {
      chunk_codepoints += paragraphs[para_i].para_end_i - paragraphs[para_i].para_start_i + 1;
      para_i++;
    }
    cdata->para_end = para_i;
    cdata->ctx = ctx;
    cdata->units = units;
    cdata->len_codepoints = len_codepoints;
    cdata->paragraphs = paragraphs;
    cdata->levels = *o_levels;
    cdata->matching_isolate = matching_isolate;
    cdata->para_is_plain = para_is_plain;
    cdata->lrun_count = 0;
    cdata->lrun_offset = 0;
    cdata->o_level_runs = NULL;
  }

  for (i32 chunk_i = 0; chunk_i < chunk_count; chunk_i++)
  {
    LkJob job;
    job.func = SplitBidiRunsResolveJob;
    job.data = &job_data[chunk_i];
    lkJobQueuePush(ctx->queue, job);
  }
  lkJobQueueWait(ctx->queue);

  i32 lrun_total = 0;
  for (i32 chunk_i = 0; chunk_i < chunk_count; chunk_i++)
  {
    job_data[chunk_i].lrun_offset = lrun_total;
    lrun_total += job_data[chunk_i].lrun_count;
  }
  *o_level_runs = APushArrayNZ(arena, LkLevelRun, lrun_total);
  *o_level_run_count = lrun_total;

  for (i32 chunk_i = 0; chunk_i < chunk_count; chunk_i++)
  {
    job_data[chunk_i].o_level_runs = *o_level_runs;
    LkJob job;
    job.func = SplitBidiRunsWriteJob;
    job.data = &job_data[chunk_i];
    lkJobQueuePush(ctx->queue, job);
  }
  lkJobQueueWait(ctx->queue);

  lkArenaRestore(scratch, scratch_pos);
}
