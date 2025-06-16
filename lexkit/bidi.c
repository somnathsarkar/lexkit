#include <lexkit/bidi.h>
#include <lexkit/break.h>

#include <stdlib.h>
#include <assert.h>

BidiUnit BidiUnitCreate(const u32 codepoint, LkUnicodeData* ud)
{
  BidiUnit ret = {0};
  ret.bidic = BIDIC_L;
  for (i32 i = 0; i < ud->bidi_range_count; i++)
  {
    if (codepoint >= ud->bidi_range_start[i] && codepoint <= ud->bidi_range_end[i])
    {
      ret.bidic = ud->bidi_range_cls[i];
      break;
    }
  }
  for (i32 i = 0; i < ud->bidipb_count; i++)
  {
    if (codepoint == ud->bidipb_key[i])
    {
      ret.bidipb = ud->bidipb_value[i];
      ret.bidipbt = ud->bidipbt[i];
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

static LevelRun LevelRunFromIndex(
    i32* levels,
    i32 start_i,
    i32 end_i,
    i32 para_level)
{
  LevelRun lr = {start_i, end_i, start_i, end_i};
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

static int CmpBracketPair(void * v_bp_a, const void* v_bp_b, const void* ctx)
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
    i32* levels,
    BidiUnit* units,
    LevelRun* lruns,
    i32* irun_lrun_idxs,
    IsolatingRunSequence irun)
{
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if ((cls == BIDIC_ES || cls == BIDIC_CS) &&
          (cls_prev == BIDIC_EN || cls == BIDIC_AN))
      {
        bool found_next = false;
        BIDIC cls_next = irun.eos;
        for (i32 lrun_j = lrun_i; lrun_j <= irun.lrun_end_i; lrun_j++)
        {
          for (i32 unit_j = max_i32(unit_i + 1, lruns[lrun_j].valid_start_i);
                unit_j <= lruns[lrun_j].valid_end_i; unit_j++)
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_end_i; unit_i >= lrun.valid_start_i; unit_i--)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_ET && cls_prev == BIDIC_EN)
        units[unit_i].bidic = BIDIC_EN;
      cls_next = units[unit_i].bidic;
    }
  }

  // W6
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (levels[unit_i] == -1)
        continue;
      BIDIC cls = units[unit_i].bidic;
      if (cls == BIDIC_EN && cls_prev_str == BIDIC_L)
      {
        cls = BIDIC_L;
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    max_bracket_pairs += lrun.valid_end_i - lrun.valid_start_i + 1;
  }
  max_bracket_pairs /= 2;

  typedef struct
  {
    i32 lrun_i;
    i32 unit_i;
  } BracketStackItem;

  BracketPair* bp = (BracketPair*)calloc(max_bracket_pairs, sizeof(BracketPair));
  i32 bracket_count = 0;
  BracketStackItem* bracket_stack = (BracketStackItem*)calloc(g_bracket_stack_size, sizeof(BracketStackItem));
  i32 bracket_sp = 0;
  bool stack_overflow = false;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
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
          for (i32 bracket_i = bracket_sp - 1; bracket_i >= 0; bracket_i++)
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
  bool* assigned_class = (bool*)calloc(bracket_count, sizeof(bool));    

  for (i32 bracket_i = 0; bracket_i < bracket_count; bracket_i++)
  {
    // 1. Find strong class inside brackets

    bool found_strong_class = false;
    bool found_matching_strong_class = false;

    for (i32 lrun_i = bp[bracket_i].lrun_start_i; lrun_i <= bp[bracket_i].lrun_end_i; lrun_i++)
    {
      LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
      i32 unit_start_i = (lrun_i == bp[bracket_i].lrun_start_i) ? 
                                    bp[bracket_i].unit_start_i :
                                    lrun.valid_start_i;
      i32 unit_end_i = (lrun_i == bp[bracket_i].lrun_end_i) ?
                                  bp[bracket_i].unit_end_i :
                                  lrun.valid_end_i;

      for (i32 unit_i = unit_start_i; unit_i <= unit_end_i; unit_i++)
      {
        if (levels[unit_i] == -1) continue; 
        if (units[unit_i].bidic == BIDIC_L ||
            units[unit_i].bidic == BIDIC_R)
        {
          found_strong_class = true;
          found_matching_strong_class |= (units[unit_i].bidic == matching_class);
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
        LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
        i32 unit_start_i = (lrun_i == bp[bracket_i].lrun_start_i) ? 
                                      bp[bracket_i].unit_start_i :
                                      lrun.valid_end_i;

        for (i32 unit_i = unit_start_i; unit_i >= lrun.valid_start_i; unit_i--)
        {
          if (levels[unit_i] == -1) continue; 
          if (units[unit_i].bidic == BIDIC_L ||
              units[unit_i].bidic == BIDIC_R)
          {
            class_search_found = true;
            class_search_result = units[unit_i].bidic;
            break;
          }
        }
        
        if (class_search_found) break;
      }
    }

    // 4. Else, don't set bracket pair
  }

  // Set all adjacent (original, before W1) NSMs to match any changed brackets
  // Forward direction

  bool adjacent_changed_bracket = false;
  BIDIC adjacent_changed_class = BIDIC_L;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      // TODO: This could be faster, full linear search isn't required

      for (i32 bracket_i = 0; bracket_i < bracket_count; bracket_i++)
      {
        if (assigned_class[bracket_i] && (bp[bracket_i].unit_start_i == unit_i ||
              bp[bracket_i].unit_end_i == unit_i))
        {
          adjacent_changed_bracket = true;
          adjacent_changed_class = units[unit_i].bidic;
          break;
        }
      }

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

  // Reverse direction

  adjacent_changed_bracket = false;
  adjacent_changed_class = BIDIC_L;
  for (i32 lrun_i = irun.lrun_end_i; lrun_i >= irun.lrun_start_i; lrun_i--)
  {
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_end_i; unit_i >= lrun.valid_end_i; unit_i--)
    {
      // TODO: This could be faster, full linear search isn't required

      for (i32 bracket_i = 0; bracket_i < bracket_count; bracket_i++)
      {
        if (assigned_class[bracket_i] && (bp[bracket_i].unit_start_i == unit_i ||
              bp[bracket_i].unit_end_i == unit_i))
        {
          adjacent_changed_bracket = true;
          adjacent_changed_class = units[unit_i].bidic;
          break;
        }
      }

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
  i32 strong_class_segment_i = lruns[irun.lrun_start_i].valid_start_i;
  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
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
            if (levels[segment_i] == -1) continue;
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
          if (levels[segment_i] == -1) continue;
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
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
    for (i32 unit_i = lrun.valid_start_i; unit_i <= lrun.valid_end_i; unit_i++)
    {
      if (IsNI(units[unit_i].bidic))
        units[unit_i].bidic = matching_class;
    }
  }

  // I1 + I2

  for (i32 lrun_i = irun.lrun_start_i; lrun_i <= irun.lrun_end_i; lrun_i++)
  {
    LevelRun lrun = lruns[irun_lrun_idxs[lrun_i]];
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
}
  
static void LevelRunSplit(
    i32* levels, 
    i32 para_start_i,
    i32 para_end_i,
    i32 para_level,
    i32 *o_level_run_count,
    LevelRun** o_level_runs)
{
  assert(*o_level_runs == NULL);
  i32 level_run_count = 0;
  i32 focus_i = para_start_i;
  while (focus_i <= para_end_i)
  {
    LevelRun lr = LevelRunFromIndex(levels, focus_i, para_end_i, para_level);
    focus_i = lr.end_i + 1;
    level_run_count++;
  }
  *o_level_runs = (LevelRun*)calloc(level_run_count, sizeof(LevelRun));
  i32 level_run_i = 0;
  focus_i = para_start_i;
  while (focus_i <= para_end_i)
  {
    LevelRun lr = LevelRunFromIndex(levels, focus_i, para_end_i, para_level);
    (*o_level_runs)[level_run_i++] = lr;
    focus_i = lr.end_i + 1;
  }
  *o_level_run_count = level_run_count;
}

static void lkSplitBidiRunsParagraph(
    BidiUnit* units,
    i32 len_units,
    LkUnicodeData* ud,
    i32 para_start_i,
    i32 para_end_i,
    i32 para_level,
    i32* io_level,
    i32* io_matching_isolate)
{

  // X1

  BidiStatus *stack = (BidiStatus*)calloc(g_bidi_max_depth + 2, sizeof(BidiStatus));
  i32 sp = 0;
  stack[sp++] = (BidiStatus){para_level, DIROVR_Neutral, false};
  i32 overflow_isolate_count = 0;
  i32 overflow_embedding_count = 0;
  i32 valid_isolate_count = 0;
  i32 last_isolate_initiator = -1;

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
          if (next_level <= g_bidi_max_depth)
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
          if (next_level <= g_bidi_max_depth)
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
          if (next_level <= g_bidi_max_depth)
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
          if (next_level <= g_bidi_max_depth)
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
          if (last_isolate_initiator != -1)
          {
            io_matching_isolate[last_isolate_initiator] = i;
          }
          last_isolate_initiator = i;
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
          if (last_isolate_initiator != -1)
          {
            io_matching_isolate[last_isolate_initiator] = i;
          }
          last_isolate_initiator = i;
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
          if (last_isolate_initiator != -1)
          {
            io_matching_isolate[last_isolate_initiator] = i;
          }
          last_isolate_initiator = i;
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
          if (last_isolate_initiator != -1)
          {
            i32 new_last_initiator = io_matching_isolate[last_isolate_initiator];
            io_matching_isolate[i] = last_isolate_initiator;
            io_matching_isolate[last_isolate_initiator] = i;
            last_isolate_initiator = new_last_initiator;
          }
          last_isolate_initiator = i;
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

  // X10
  i32 lrun_count = -1;
  LevelRun *lruns = NULL;
  LevelRunSplit(io_level, para_start_i, para_end_i, para_level, &lrun_count, &lruns);

  // Build isolating run sequences
  bool* lrun_used = (bool*)calloc(lrun_count, sizeof(bool));
  IsolatingRunSequence* iruns = (IsolatingRunSequence*)calloc(lrun_count, sizeof(IsolatingRunSequence));
  i32* irun_lrun_idxs = (i32*)calloc(lrun_count, sizeof(i32));
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
      BIDIC last_valid_class = units[lruns[focus].valid_end_i].bidic;
      if (last_valid_class == BIDIC_LRI ||
          last_valid_class == BIDIC_RLI ||
          last_valid_class == BIDIC_FSI ||
          io_matching_isolate[lruns[focus].valid_end_i] != -1)
      {
        i32 matching_isolate = io_matching_isolate[lruns[focus].valid_end_i];
        i32 found_match_lrun_i = -1;
        for (i32 lrun_j = lrun_i + 1; lrun_j < lrun_count; lrun_j++)
        {
          if (lruns[lrun_j].start_i > matching_isolate)
            break;
          if (units[lruns[lrun_j].valid_start_i].bidic == BIDIC_PDI &&
              lruns[lrun_j].valid_start_i == matching_isolate)
          {
            found_match_lrun_i = lrun_j;
            break;
          }
        }
        if (found_match_lrun_i != -1 &&
            lruns[found_match_lrun_i].level != lruns[lrun_i].level)
        {
          focus = found_match_lrun_i;
        }
        else
          break;
      }
      else
        break;
    }
    irun_count++;
  }

  // Fill in sos/eos
  for (i32 irun_i = 0; irun_i < irun_count; irun_i++)
  {
    {
      // SOS
      i32 level_a = (irun_i > 0) ?
                      io_level[lruns[iruns[irun_i - 1].lrun_end_i].valid_end_i] :
                      para_level;
      i32 level_b = io_level[lruns[iruns[irun_i].lrun_start_i].valid_start_i];
      i32 level_ab = (level_a < level_b) ? level_b : level_a;
      if (level_ab & 1)
        iruns[irun_i].sos = BIDIC_R;
      else
        iruns[irun_i].sos = BIDIC_L;
    }

    {
      // EOS
      i32 level_a = (irun_i + 1 < irun_count) ?
                      io_level[lruns[iruns[irun_i + 1].lrun_start_i].valid_start_i] :
                      para_level;
      i32 level_b = io_level[lruns[iruns[irun_i].lrun_end_i].valid_end_i];
      i32 level_ab = (level_a < level_b) ? level_b : level_a;
      if (level_ab & 1)
        iruns[irun_i].eos = BIDIC_R;
      else
        iruns[irun_i].eos = BIDIC_L;
    }
  }

  for (i32 i = 0; i < irun_count; i++)
    ResolveIsolatingRunSequence(io_level, units, lruns, irun_lrun_idxs, iruns[i]);

  for (i32 i = para_start_i; i <= para_end_i; i++)
  {
    if (io_level[i] == -1)
      io_level[i] = para_level;
  }
}

void lkSplitParagraphs(
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    i32* o_paragraph_count,
    Paragraph** o_paragraphs)
{
  assert(*o_paragraph_count == -1);
  assert(*o_paragraphs == NULL);

  // TODO: Need reallocable list

  *o_paragraph_count = 0;
  *o_paragraphs = (Paragraph*)calloc(10, sizeof(Paragraph));

  i32 para_start_i = 0;
  bool para_level_found = false;
  i32 para_level = 0;
  i32 isolate_count = 0;
  BidiUnit *units = (BidiUnit*)calloc(len_codepoints, sizeof(BidiUnit));
  for (i32 i = 0; i < len_codepoints; i++)
    units[i] = BidiUnitCreate(codepoints[i], ud);
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
      Paragraph para = { para_start_i, i, para_level };
      (*o_paragraphs)[*o_paragraph_count] = para;
      (*o_paragraph_count)++;
      para_start_i = i + 1;
      para_level_found = false;
      para_level = 0;
      isolate_count = 0;
    }
  }
  if (len_codepoints > 0 && para_start_i < len_codepoints)
  {
    Paragraph para = { para_start_i, len_codepoints - 1, para_level };
    (*o_paragraphs)[*o_paragraph_count] = para;
    (*o_paragraph_count)++;
  }
}

void lkSplitBidiRuns(
    const u32* codepoints,
    i32 len_codepoints,
    LkUnicodeData* ud,
    i32 paragraph_count,
    Paragraph* paragraphs,
    i32** o_levels,
    i32* o_level_run_count,
    LevelRun** o_level_runs)
{
  assert(*o_level_run_count == -1);
  assert(*o_level_runs == NULL);
  assert(o_levels != NULL && *o_levels == NULL);

  // BB: Fixed size list, need memory rework

  *o_level_run_count = 0;
  *o_level_runs = (LevelRun*)calloc(512, sizeof(LevelRun));

  BidiUnit *units = (BidiUnit*)calloc(len_codepoints, sizeof(BidiUnit));
  for (i32 i = 0; i < len_codepoints; i++)
    units[i] = BidiUnitCreate(codepoints[i], ud);
  *o_levels = (i32*)calloc(len_codepoints, sizeof(i32));
  i32* matching_isolate = (i32*)calloc(len_codepoints, sizeof(i32));
  for (i32 para_i = 0; para_i < paragraph_count; para_i++)
  {
      lkSplitBidiRunsParagraph(
          units,
          len_codepoints,
          ud,
          paragraphs[para_i].para_start_i,
          paragraphs[para_i].para_end_i,
          paragraphs[para_i].para_level,
          *o_levels,
          matching_isolate);

      // BB: Revisit after memory rework

      i32 para_level_run_count = -1;
      LevelRun* para_level_runs = NULL;

      LevelRunSplit(
          *o_levels,
          paragraphs[para_i].para_start_i,
          paragraphs[para_i].para_end_i,
          paragraphs[para_i].para_level,
          &para_level_run_count,
          &para_level_runs);

      for (i32 lrun_i = 0; lrun_i < para_level_run_count; lrun_i++)
      {
        (*o_level_runs)[*o_level_run_count + lrun_i] = para_level_runs[lrun_i];
      }
      *o_level_run_count += para_level_run_count;
  }
}
