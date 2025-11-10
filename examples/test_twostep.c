#include <lexkit/lexkit.h>
#include <lexkit/twostep.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main() {
  LkArena* arena = lkArenaCreate(Megabytes(64));
  LkUnicodeData ud = {0};
  bool ud_success = lkTryLoadUnicodeDataFromSpec(
      arena,
      "C:/Code/lexkit/lexkit/LineBreakProperty.txt",
      "C:/Code/lexkit/lexkit/WordBreakProperty.txt",
      "C:/Code/lexkit/lexkit/GraphemeBreakProperty.txt",
      "C:/Code/lexkit/lexkit/UnicodeData.txt",
      "C:/Code/lexkit/lexkit/EastAsianWidth.txt",
      "C:/Code/lexkit/lexkit/DerivedCoreProperties.txt",
      "C:/Code/lexkit/lexkit/emoji-data.txt",
      "C:/Code/lexkit/lexkit/DerivedBidiClass.txt",
      "C:/Code/lexkit/lexkit/BidiBrackets.txt",
      &ud);
  LkTwoStep* ts = LkTwoStepCreate(arena, "C:/Code/lexkit/lexkit/LineBreakProperty.txt", g_map_lbc_str, LBC_Count, LBC_XX, UNIFMT_A);
  for (i32 i = 0; i < ud.lb_range_count; i++)
  {
    for (i32 j = ud.lb_range_start[i]; j <= ud.lb_range_end[i]; j++)
    {
      i32 tsl = LkTwoStepLookup(ts, j);
      i32 udl = ud.lb_range_cls[i];
      assert(tsl == udl);
    }
  }
  LkTwoStep* ts_gc = LkTwoStepCreate(arena, "C:/Code/lexkit/lexkit/UnicodeData.txt", g_map_gc_str, GC_Count, GC_Cc, UNIFMT_B);
  for (i32 i = 0; i < ud.gc_count; i++)
  {
    i32 tsl = LkTwoStepLookup(ts_gc, ud.gc_codepoint[i]);
    i32 udl = ud.gc_cls[i];
    assert(tsl == udl);
  }
  
  LkTwoStep* ts_incb = LkTwoStepCreate(arena, "C:/Code/lexkit/lexkit/DerivedCoreProperties.txt", g_map_incb_str, INCB_Count, INCB_None, UNIFMT_C);
  for (i32 i = 0; i < ud.incb_range_count; i++)
  {
    for (i32 j = ud.incb_range_start[i]; j <= ud.incb_range_end[i]; j++)
    {
      i32 tsl = LkTwoStepLookup(ts_incb, j);
      i32 udl = ud.incb_range_cls[i];
      assert(tsl == udl);
    }
  }
  
  LkTwoStep* ts_ep = LkTwoStepCreate(arena, "C:/Code/lexkit/lexkit/emoji-data.txt", NULL, 2, 0, UNIFMT_D);
  for (i32 i = 0; i < ud.ep_range_count; i++)
  {
    for (i32 j = ud.ep_range_start[i]; j <= ud.ep_range_end[i]; j++)
    {
      i32 tsl = LkTwoStepLookup(ts_ep, j);
      assert(tsl == 1);
    }
  }
  return 0;
}

