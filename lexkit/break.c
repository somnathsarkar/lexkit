#include <lexkit/break.h>
#include <lexkit/lexkit.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define UNICODE_REPLACEMENT_CHARACTER 0xFFFD

const char* g_map_gc_str[] = {
  "Lu",      // GC_Lu
  "Ll",      // GC_Ll
  "Lt",      // GC_Lt
  "Lm",      // GC_Lm
  "Lo",      // GC_Lo
  "Mn",      // GC_Mn
  "Mc",      // GC_Mc
  "Me",      // GC_Me
  "Nd",      // GC_Nd
  "Nl",      // GC_Nl
  "No",      // GC_No
  "Pc",      // GC_Pc
  "Pd",      // GC_Pd
  "Ps",      // GC_Ps
  "Pe",      // GC_Pe
  "Pi",      // GC_Pi
  "Pf",      // GC_Pf
  "Po",      // GC_Po
  "Sm",      // GC_Sm
  "Sc",      // GC_Sc
  "Sk",      // GC_Sk
  "So",      // GC_So
  "Zs",      // GC_Zs
  "Zl",      // GC_Zl
  "Zp",      // GC_Zp
  "Cc",      // GC_Cc
  "Cf",      // GC_Cf
  "Cs",      // GC_Cs
  "Co",      // GC_Co
  "Cn",      // GC_Cn
};

const char* g_map_lbc_str[] = {
  "AI",     // LBC_AI
  "AK",     // LBC_AK
  "AL",     // LBC_AL
  "AP",     // LBC_AP
  "AS",     // LBC_AS
  "BA",     // LBC_BA
  "BB",     // LBC_BB
  "B2",     // LBC_B2
  "BK",     // LBC_BK
  "CB",     // LBC_CB
  "CJ",     // LBC_CJ
  "CL",     // LBC_CL
  "CM",     // LBC_CM
  "CP",     // LBC_CP
  "CR",     // LBC_CR
  "EB",     // LBC_EB
  "EM",     // LBC_EM
  "EX",     // LBC_EX
  "GL",     // LBC_GL
  "H2",     // LBC_H2
  "H3",     // LBC_H3
  "HY",     // LBC_HY
  "ID",     // LBC_ID
  "HL",     // LBC_HL
  "IN",     // LBC_IN
  "IS",     // LBC_IS
  "JL",     // LBC_JL
  "JT",     // LBC_JT
  "JV",     // LBC_JV
  "LF",     // LBC_LF
  "NL",     // LBC_NL
  "NS",     // LBC_NS
  "NU",     // LBC_NU
  "OP",     // LBC_OP
  "PO",     // LBC_PO
  "PR",     // LBC_PR
  "QU",     // LBC_QU
  "RI",     // LBC_RI
  "SA",     // LBC_SA
  "SG",     // LBC_SG
  "SP",     // LBC_SP
  "SY",     // LBC_SY
  "VF",     // LBC_VF
  "VI",     // LBC_VI
  "WJ",     // LBC_WJ
  "XX",     // LBC_XX
  "ZW",     // LBC_ZW
  "ZWJ",    // LBC_ZWJ
};

const char* g_map_wbc_str[] = {
  "CR",                 // WBC_CR
  "LF",                 // WBC_LF
  "Newline",            // WBC_Newline
  "Extend",             // WBC_Extend
  "ZWJ",                // WBC_ZWJ
  "Regional_Indicator", // WBC_Regional_Indicator
  "Format",             // WBC_Format
  "Katakana",           // WBC_Katakana
  "Hebrew_Letter",      // WBC_Hebrew_Letter
  "ALetter",            // WBC_ALetter
  "Single_Quote",       // WBC_Single_Quote
  "Double_Quote",       // WBC_Double_Quote
  "MidNumLet",          // WBC_MidNumLet
  "MidLetter",          // WBC_MidLetter
  "MidNum",             // WBC_MidNum
  "Numeric",            // WBC_Numeric
  "ExtendNumLet",       // WBC_ExtendNumLet
  "WSegSpace",          // WBC_WSegSpace
  "XX",                 // WBC_XX
};

const char* g_map_gbc_str[] = {
  "CR",                 // GBC_CR
  "LF",                 // GBC_LF
  "Control",            // GBC_Control
  "Extend",             // GBC_Extend
  "ZWJ",                // GBC_ZWJ
  "Regional_Indicator", // GBC_Regional_Indicator
  "Prepend",            // GBC_Prepend
  "SpacingMark",        // GBC_SpacingMark
  "L",                  // GBC_L
  "V",                  // GBC_V
  "T",                  // GBC_T
  "LV",                 // GBC_LV
  "LVT",                // GBC_LVT
  "XX",                 // GBC_XX
};

const char* g_map_eaw_str[] = {
  "A",                  // EAW_A
  "F",                  // EAW_F
  "H",                  // EAW_H
  "N",                  // EAW_N
  "Na",                 // EAW_Na
  "W",                  // EAW_W
};

const char* g_map_incb_str[] = {
  "None",               // INCB_None
  "Linker",             // INCB_Linker
  "Consonant",          // INCB_Consonant
  "Extend",             // INCB_Extend
};

const char* g_map_bidic_str[] = {
  "L",									// BIDIC_L
  "R",									// BIDIC_R
  "AL",									// BIDIC_AL
  "EN",									// BIDIC_EN
  "ES",									// BIDIC_ES
  "ET",									// BIDIC_ET
  "AN",									// BIDIC_AN
  "CS",									// BIDIC_CS
  "NSM",								// BIDIC_NSM
  "BN",									// BIDIC_BN
  "B",									// BIDIC_B
  "S",									// BIDIC_S
  "WS",									// BIDIC_WS
  "ON",									// BIDIC_ON
  "LRE",								// BIDIC_LRE
  "LRO",								// BIDIC_LRO
  "RLE",								// BIDIC_RLE
  "RLO",								// BIDIC_RLO
  "PDF",								// BIDIC_PDF
  "LRI",								// BIDIC_LRI
  "RLI",								// BIDIC_RLI
  "FSI",								// BIDIC_FSI
  "PDI",								// BIDIC_PDI
};

const char* g_map_bidipbt_str[] = {
  "n",                  // BIDIPBT_None
  "o",                  // BIDIPBT_Open
  "c",                  // BIDIPBT_Close
};

static bool IsHexChar(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

#define S_MAX_LINE 256

static i32 CountImportantLines(FILE* fp)
{
  char buf[S_MAX_LINE];
  i32 ans = 0;
  while (fgets(buf, S_MAX_LINE, fp))   
  {
    if (IsHexChar(buf[0]))
      ans++;
  }
  return ans;
}

static i32 CountImportantLinesInCB(FILE* fp)
{
  char buf[S_MAX_LINE];
  i32 ans = 0;
  while (fgets(buf, S_MAX_LINE, fp))   
  {
    if (IsHexChar(buf[0]) && strstr(buf, "InCB") != NULL)
      ans++;
  }
  return ans;
}

static i32 CountImportantLinesEmoji(FILE* fp)
{
  char buf[S_MAX_LINE];
  i32 ans = 0;
  while (fgets(buf, S_MAX_LINE, fp))   
  {
    if (IsHexChar(buf[0]) && strstr(buf, "Extended_Pictographic") != NULL)
      ans++;
  }
  return ans;
}

bool lkTryLoadUnicodeDataFromSpec(
    LkArena* arena,
    const char* str_path_lb,
    const char* str_path_wb,
    const char* str_path_gb,
    const char* str_path_gc,
    const char* str_path_eaw,
    const char* str_path_incb,
    const char* str_path_ep,
    const char* str_path_bidi,
    const char* str_path_bidipb,
    LkUnicodeData* o_ud)
{
  assert(o_ud != NULL);
  
  o_ud->ts_lb = LkTwoStepCreate(arena, str_path_lb, g_map_lbc_str, LBC_Count, LBC_XX, UNIFMT_A);
  o_ud->ts_wb = LkTwoStepCreate(arena, str_path_wb, g_map_wbc_str, WBC_Count, WBC_XX, UNIFMT_A);
  o_ud->ts_gb = LkTwoStepCreate(arena, str_path_gb, g_map_gbc_str, GBC_Count, GBC_XX, UNIFMT_A);
  o_ud->ts_gc = LkTwoStepCreate(arena, str_path_gc, g_map_gc_str, GC_Count, GC_Cc, UNIFMT_B);
  o_ud->ts_eaw = LkTwoStepCreate(arena, str_path_eaw, g_map_eaw_str, EAW_Count, EAW_Na, UNIFMT_A);
  o_ud->ts_incb = LkTwoStepCreate(arena, str_path_incb, g_map_incb_str, INCB_Count, INCB_None, UNIFMT_C);
  o_ud->ts_ep = LkTwoStepCreate(arena, str_path_ep, NULL, 2, 0 /* Extended_Pictographic */, UNIFMT_D);
  o_ud->ts_bidi = LkTwoStepCreate(arena, str_path_bidi, g_map_bidic_str, BIDIC_Count, BIDIC_L, UNIFMT_A);

  // TODO: Clean this up, not suitable for TwoStep table, but could be something else (hashmap?)
  
  FILE* fp = NULL;
  errno_t err = fopen_s(&fp, str_path_bidipb, "r");
  char buf[S_MAX_LINE];
  char buf_cls[S_MAX_LINE];
  if (err)
    return false;
  o_ud->bidipb_count = CountImportantLines(fp);
  rewind(fp);
  o_ud->bidipb_key = APushArray(arena, u32, o_ud->bidipb_count);
  o_ud->bidipb_value = APushArray(arena, u32, o_ud->bidipb_count);
  o_ud->bidipbt = APushArray(arena, BIDIPBT, o_ud->bidipb_count);
  i32 i_range = 0;
  while (fgets(buf, S_MAX_LINE, fp))
  {
    if (!IsHexChar(buf[0]))
      continue;

    u32 key = 0;
    u32 val = 0;

    int three_parse = sscanf_s(buf, "%x ; %x ; %s", &key, &val, buf_cls, S_MAX_LINE);
    if (three_parse < 3) return false;
    o_ud->bidipb_key[i_range]     = key;
    o_ud->bidipb_value[i_range]   = val;
    o_ud->bidipbt[i_range]        = BIDIPBT_None;
    for (int i_cls = 0; i_cls < BIDIPBT_Count; i_cls++)
    {
      if (strncmp(g_map_bidipbt_str[i_cls], buf_cls, strnlen_s(g_map_bidipbt_str[i_cls], S_MAX_LINE)) == 0)
      {
        o_ud->bidipbt[i_range] = (BIDIPBT) i_cls;
        break;
      }
    }
    i_range++;
  }
  return true;
}

Glyph GetGlyphAtIndex(const u32* codepoints, i32 len_codepoints, i32 idx, LkUnicodeData* ud)
{
Glyph glyph                 = {0};
  if (idx < 0 || idx >= len_codepoints)
  {
    Glyph glyph = {0};
    return glyph;
  }

  // Get codepoint
  
  u32 codepoint = codepoints[idx];
  LBC lbc   = LBC_XX;
  WBC wbc   = WBC_XX;
  GBC gbc   = GBC_XX;
  EAW eaw   = EAW_Na;
  GC  gc    = GC_Cn;
  INCB incb = INCB_None;
  bool extended_pictographic = false;

  lbc = LkTwoStepLookup(ud->ts_lb, codepoint);
  wbc = LkTwoStepLookup(ud->ts_wb, codepoint);
  gbc = LkTwoStepLookup(ud->ts_gb, codepoint);
  gc = LkTwoStepLookup(ud->ts_gc, codepoint);
  eaw = LkTwoStepLookup(ud->ts_eaw, codepoint);
  incb = LkTwoStepLookup(ud->ts_incb, codepoint);
  extended_pictographic = (bool)LkTwoStepLookup(ud->ts_ep, codepoint);

  // LB1: Assign a line breaking class to each code point of the input.
  //  Resolve AI, CB, CJ, SA, SG, and XX into other line breaking classes depending on criteria outside the scope of this algorithm.

  if (lbc == LBC_AI || lbc == LBC_SG || lbc == LBC_XX)
    lbc = LBC_AL;
  else if (lbc == LBC_SA && (gc == GC_Mn || gc == GC_Mc))
    lbc = LBC_CM;
  else if (lbc == LBC_SA)
    lbc = LBC_AL;
  else if (lbc == LBC_CJ)
    lbc = LBC_NS;

  glyph.idx                   = idx;
  glyph.codepoint             = codepoint;
  glyph.lbc                   = lbc;
  glyph.wbc                   = wbc;
  glyph.gbc                   = gbc;
  glyph.gc                    = gc;
  glyph.eaw                   = eaw;
  glyph.incb                  = incb;
  glyph.extended_pictographic = extended_pictographic;
  return glyph;
}

void BreakerCreate(const u32* codepoints, i32 len_codepoints, Breaker* o_brk)
{
  o_brk->codepoints = codepoints;
  o_brk->len_codepoints = len_codepoints;
  o_brk->idx = -1;
}

static void BreakerGetNextGlyphLineBreak(Breaker* brk, LkUnicodeData* ud, bool* io_next, i32* io_idx_next, Glyph* o_g)
{
  if (*io_next)
    return;

  while ((*io_idx_next) + 1 < brk->len_codepoints)
  {
    (*io_idx_next)++;
    (*o_g) = GetGlyphAtIndex(brk->codepoints, brk->len_codepoints, *io_idx_next, ud);
    if (o_g->lbc != LBC_ZWJ &&
        o_g->lbc != LBC_CM)
    {
      *io_next = true;
      break;
    }
  }
}

static LBRK BreakerComputeLbrk(Breaker* brk, LkUnicodeData* ud)
{
  // LB3

  if (brk->idx == brk->len_codepoints - 1)
  {
    return LBRK_MAN;
  }

  // LB4

  if (brk->lbcx == LBCX_BK)
  {
    return LBRK_MAN;
  }

  Glyph g = GetGlyphAtIndex(brk->codepoints, brk->len_codepoints, brk->idx + 1, ud);

  // LB5

  if (brk->lbcx == LBCX_CR && g.lbc == LBC_LF)
    return LBRK_PRO;

  if (brk->lbcx == LBCX_CR ||
      brk->lbcx == LBCX_LF ||
      brk->lbcx == LBCX_NL)
    return LBRK_MAN;

  // LB6

  if (g.lbc == LBC_BK ||
      g.lbc == LBC_CR ||
      g.lbc == LBC_LF ||
      g.lbc == LBC_NL)
    return LBRK_PRO;

  // LB7

  if (g.lbc == LBC_SP ||
      g.lbc == LBC_ZW)
    return LBRK_PRO;

  // LB8

  if (brk->lbcx == LBCX_ZW ||
      brk->lbcx == LBCX_ZW_SP)
    return LBRK_OPT;

  // LB8a

  if (brk->lbcx == LBCX_ZWJ)
    return LBRK_PRO;

  // LB9

  if (g.lbc == LBC_ZWJ ||
      g.lbc == LBC_CM)
    return LBRK_PRO;

  // LB11

  if (g.lbc == LBC_WJ ||
      brk->lbcx_adj == LBCX_WJ)
    return LBRK_PRO;

  // LB12

  if (brk->lbcx_adj == LBCX_GL)
    return LBRK_PRO;

  // LB12a

  if (brk->lbcx_adj != LBCX_SP &&
      brk->lbcx_adj != LBCX_BA &&
      brk->lbcx_adj != LBCX_HY &&
      brk->lbcx_adj == LBCX_GL)
    return LBRK_PRO;

  // LB13

  if (g.lbc == LBC_CL ||
      g.lbc == LBC_CP ||
      g.lbc == LBC_EX ||
      g.lbc == LBC_SY)
    return LBRK_PRO;

  // LB14

  if (brk->lbcx_adj == LBCX_OP ||
      brk->lbcx_adj == LBCX_OP_SP)
    return LBRK_PRO;

  // LB15a

  if (brk->lbcx_adj == LBCX_15a ||
      brk->lbcx_adj == LBCX_15a_SP)
    return LBRK_PRO;

  bool next2 = false;
  bool next3 = false;
  i32 idx_next2 = brk->idx + 1;
  i32 idx_next3 = -1;
  Glyph g_next2 = {0};
  Glyph g_next3 = {0};

  // LB15b

  if (g.gc == GC_Pf &&
      g.lbc == LBC_QU)
  {
    BreakerGetNextGlyphLineBreak(brk, ud, &next2, &idx_next2, &g_next2);
    if (!next2 || // eot
        g_next2.lbc == LBC_SP ||
        g_next2.lbc == LBC_GL ||
        g_next2.lbc == LBC_WJ ||
        g_next2.lbc == LBC_CL ||
        g_next2.lbc == LBC_QU ||
        g_next2.lbc == LBC_CP ||
        g_next2.lbc == LBC_EX ||
        g_next2.lbc == LBC_IS ||
        g_next2.lbc == LBC_SY ||
        g_next2.lbc == LBC_BK ||
        g_next2.lbc == LBC_CR ||
        g_next2.lbc == LBC_LF ||
        g_next2.lbc == LBC_NL ||
        g_next2.lbc == LBC_ZW)
      return LBRK_PRO;
  }

  // LB15c

  if (brk->lbcx_adj == LBCX_SP)
  {
    if (g.lbc == LBC_IS)
    {
      BreakerGetNextGlyphLineBreak(brk, ud, &next2, &idx_next2, &g_next2);
      if (!next2 && g_next2.lbc == LBC_NU)
        return LBRK_OPT;
    }
  }

  // LB15d

  if (g.lbc == LBC_IS)
    return LBRK_PRO;

  // LB16

  if ((brk->lbcx_adj == LBCX_CL ||
      brk->lbcx_adj == LBCX_CP ||
      brk->lbcx_adj == LBCX_CL_CP_SP) &&
      g.lbc == LBC_NS)
    return LBRK_PRO;

  // LB17

  if ((brk->lbcx_adj == LBCX_B2 ||
      brk->lbcx_adj == LBCX_B2_SP) &&
      g.lbc == LBC_B2)
    return LBRK_PRO;

  // LB18

  if (brk->lbcx_adj == LBCX_SP)
    return LBRK_OPT;

  // LB19

  if (g.lbc == LBC_QU &&
      g.gc != GC_Pi)
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_QU &&
      brk->gc_adj != GC_Pf)
    return LBRK_PRO;

  // LB19a

  if (brk->eaw_adj != EAW_F &&
      brk->eaw_adj != EAW_W &&
      brk->eaw_adj != EAW_H &&
      g.lbc == LBC_QU)
    return LBRK_PRO;

  if (g.lbc == LBC_QU &&
      (!next2 ||                // eot
       (g_next2.eaw != EAW_F &&
        g_next2.eaw != EAW_W &&
        g_next2.eaw != EAW_H)))
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_QU &&
      (g.eaw != EAW_F &&
       g.eaw != EAW_W &&
       g.eaw != EAW_H))
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_19a)
    return LBRK_PRO;

  // LB20

  if (g.lbc == LBC_CB)
    return LBRK_OPT;

  if (brk->lbcx_adj == LBCX_CB)
    return LBRK_OPT;

  // LB20a

  if (brk->lbcx_adj == LBCX_20a &&
      g.lbc == LBC_AL)
    return LBRK_PRO;

  // LB21

  if (g.lbc == LBC_BA ||
      g.lbc == LBC_HY ||
      g.lbc == LBC_NS ||
      brk->lbcx_adj == LBCX_BB)
    return LBRK_PRO;

  // LB21a

  if (brk->lbcx_adj == LBCX_21a &&
      g.lbc != LBC_HL)
    return LBRK_PRO;

  // LB21b

  if (brk->lbcx_adj == LBCX_SY &&
      g.lbc == LBC_HL)
    return LBRK_PRO;

  // LB22

  if (g.lbc == LBC_IN)
    return LBRK_PRO;

  // LB23

  if ((brk->lbcx_adj == LBCX_AL ||
        brk->lbcx_adj == LBCX_HL) &&
      g.lbc == LBC_NU)
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_NU &&
      (g.lbc == LBC_AL ||
       g.lbc == LBC_HL))
    return LBRK_PRO;

  // LB23a

  if (brk->lbcx_adj == LBCX_PR &&
      (g.lbc == LBC_ID ||
       g.lbc == LBC_EB ||
       g.lbc == LBC_EM))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_ID ||
        brk->lbcx_adj == LBCX_EB ||
        brk->lbcx_adj == LBCX_EM) &&
      g.lbc == LBC_PO)
    return LBRK_PRO;

  // LB24

  if ((brk->lbcx_adj == LBCX_PR ||
        brk->lbcx_adj == LBCX_PO) &&
      (g.lbc == LBC_AL ||
       g.lbc == LBC_HL))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_AL ||
        brk->lbcx_adj == LBCX_HL) &&
      (g.lbc == LBC_PR ||
       g.lbc == LBC_PO))
    return LBRK_PRO;

  // LB25

  if ((brk->lbcx_adj == LBCX_NU_CL ||
        brk->lbcx_adj == LBCX_NU_CP) &&
      (g.lbc == LBC_PO ||
       g.lbc == LBC_PR))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_NU ||
        brk->lbcx_adj == LBCX_NU_SY ||
        brk->lbcx_adj == LBCX_NU_IS) &&
      (g.lbc == LBC_PO ||
       g.lbc == LBC_PR))
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_PO &&
      g.lbc == LBC_OP)
  {
    BreakerGetNextGlyphLineBreak(brk, ud, &next2, &idx_next2, &g_next2);
    if (g_next2.lbc == LBC_NU)
      return LBRK_PRO;
    else if (g_next2.lbc == LBC_IS)
    {
      if (!next3)
        idx_next3 = idx_next2;
      BreakerGetNextGlyphLineBreak(brk, ud, &next3, &idx_next3, &g_next3);
      if (g_next3.lbc == LBC_NU)
        return LBRK_PRO;
    }
  }

  if ((brk->lbcx_adj == LBCX_PO ||
        brk->lbcx_adj == LBCX_PR ||
        brk->lbcx_adj == LBCX_HY ||
        brk->lbcx_adj == LBCX_IS) &&
      g.lbc == LBC_NU)
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_PR &&
      g.lbc == LBC_OP)
  {
    BreakerGetNextGlyphLineBreak(brk, ud, &next2, &idx_next2, &g_next2);
    if (g_next2.lbc == LBC_NU)
      return LBRK_PRO;
    else if (g_next2.lbc == LBC_IS)
    {
      if (!next3)
        idx_next3 = idx_next2;
      BreakerGetNextGlyphLineBreak(brk, ud, &next3, &idx_next3, &g_next3);
      if (g_next3.lbc == LBC_NU)
        return LBRK_PRO;
    }
  }

  if ((brk->lbcx_adj == LBCX_NU ||
      brk->lbcx_adj == LBCX_NU_SY ||
      brk->lbcx_adj == LBCX_NU_IS) &&
      g.lbc == LBC_NU)
    return LBRK_PRO;

  // LB26

  if (brk->lbcx_adj == LBCX_JL &&
      (g.lbc == LBC_JL ||
       g.lbc == LBC_JV ||
       g.lbc == LBC_H2 ||
       g.lbc == LBC_H3))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_JV ||
        brk->lbcx_adj == LBCX_H2) &&
      (g.lbc == LBC_JV ||
       g.lbc == LBC_JT))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_JT ||
        brk->lbcx_adj == LBCX_H3) &&
      g.lbc == LBC_JT)
    return LBRK_PRO;

  // LB27

  if ((brk->lbcx_adj == LBCX_JL ||
        brk->lbcx_adj == LBCX_JV ||
        brk->lbcx_adj == LBCX_JT ||
        brk->lbcx_adj == LBCX_H2 ||
        brk->lbcx_adj == LBCX_H3) &&
      g.lbc == LBC_PO)
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_PR &&
      (g.lbc == LBC_JL ||
       g.lbc == LBC_JV ||
       g.lbc == LBC_JT ||
       g.lbc == LBC_H2 ||
       g.lbc == LBC_H3))
    return LBRK_PRO;

  // LB28

  if ((brk->lbcx_adj == LBCX_AL ||
        brk->lbcx_adj == LBCX_HL) &&
      (g.lbc == LBC_AL ||
       g.lbc == LBC_HL))
    return LBRK_PRO;

  // LB28a

  if (brk->lbcx_adj == LBCX_AP &&
      (g.lbc == LBC_AK ||
       g.codepoint == 0x25CC ||
       g.lbc == LBC_AS))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_AK ||
        brk->codepoints[brk->idx] == 0x25CC ||
        brk->lbcx_adj == LBCX_AS) &&
      (g.lbc == LBC_VF ||
       g.lbc == LBC_VI))
    return LBRK_PRO;

  if (brk->lbcx_adj == LBCX_28a &&
      (g.lbc == LBC_AK ||
       g.codepoint == 0x25CC))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_AK ||
        brk->codepoints[brk->idx] == 0x25CC ||
        brk->lbcx_adj == LBCX_AS) &&
      (g.lbc == LBC_AK ||
       g.codepoint == 0x25CC ||
       g.lbc == LBC_AS))
  {
    BreakerGetNextGlyphLineBreak(brk, ud, &next2, &idx_next2, &g_next2);
    if (next2 && g_next2.lbc == LBC_VF)
      return LBRK_PRO;
  }

  // LB29

  if (brk->lbcx_adj == LBCX_IS &&
      (g.lbc == LBC_AL ||
       g.lbc == LBC_HL))
    return LBRK_PRO;

  // LB30

  if ((brk->lbcx_adj == LBCX_AL ||
        brk->lbcx_adj == LBCX_HL ||
        brk->lbcx_adj == LBCX_NU) &&
      (g.lbc = LBC_OP &&
       g.eaw != EAW_F &&
       g.eaw != EAW_W &&
       g.eaw != EAW_H))
    return LBRK_PRO;

  if ((brk->lbcx_adj == LBCX_CP &&
        brk->eaw_adj != EAW_F &&
        brk->eaw_adj != EAW_W &&
        brk->eaw_adj != EAW_H) &&
      (g.lbc == LBC_AL ||
       g.lbc == LBC_HL ||
       g.lbc == LBC_NU))
    return LBRK_PRO;

  // LB30a

  if (brk->lbcx_adj == LBCX_RI &&
      g.lbc == LBC_RI)
    return LBRK_PRO;

  // LB30b

  if (brk->lbcx_adj == LBCX_EB &&
      g.lbc == LBC_EM)
    return LBRK_PRO;

  if (brk->ep_adj && brk->gc_adj == GC_Cn &&
      g.lbc == LBC_EM)
    return LBRK_PRO;

  // LB31

  return LBRK_OPT;
}

static void BreakerGetNextGlyphWordBreak(Breaker* brk, LkUnicodeData* ud, bool* io_next, i32* io_idx_next, Glyph* o_g)
{
  if (*io_next)
    return;

  while ((*io_idx_next) + 1 < brk->len_codepoints)
  {
    (*io_idx_next)++;
    (*o_g) = GetGlyphAtIndex(brk->codepoints, brk->len_codepoints, *io_idx_next, ud);
    if (o_g->wbc != WBC_CR &&
        o_g->wbc != WBC_LF &&
        o_g->wbc != WBC_Newline)
    {
      *io_next = true;
      break;
    }
  }
}

static bool IsAHLetter(WBC wbc)
{
  return (wbc == WBC_ALetter || wbc == WBC_Hebrew_Letter);
}

static bool IsAHLetterX(WBCX wbcx)
{
  return (wbcx == WBCX_ALetter || wbcx == WBCX_Hebrew_Letter);
}

WBRK BreakerComputeWbrk(Breaker* brk, LkUnicodeData* ud)
{
  // WB2

  if (brk->idx == brk->len_codepoints - 1)
  {
    return WBRK_BRK;
  }

  Glyph g = GetGlyphAtIndex(brk->codepoints, brk->len_codepoints, brk->idx + 1, ud);

  // WB3

  if (brk->wbcx == WBCX_CR &&
      g.wbc == WBC_LF)
  {
    return WBRK_PRO;
  }

  // WB3a

  if (brk->wbcx == WBCX_Newline ||
      brk->wbcx == WBCX_CR ||
      brk->wbcx == WBCX_LF)
  {
    return WBRK_BRK;
  }

  // WB3b

  if (g.wbc == WBC_Newline ||
      g.wbc == WBC_CR ||
      g.wbc == WBC_LF)
  {
    return WBRK_BRK;
  }

  // WB3c

  if (brk->wbcx == WBCX_ZWJ &&
      g.extended_pictographic)
  {
    return WBRK_PRO;
  }

  // WB3d

  if (brk->wbcx == WBCX_WSegSpace &&
      g.wbc == WBC_WSegSpace)
  {
    return WBRK_PRO;
  }

  // WB4

  if (g.wbc == WBC_Extend ||
      g.wbc == WBC_Format ||
      g.wbc == WBC_ZWJ)
  {
    return WBRK_PRO;
  }

  // WB5

  if (IsAHLetterX(brk->wbcx_adj) &&
      IsAHLetter(g.wbc))
  {
    return WBRK_PRO;
  }

  bool next2 = false;
  i32 idx_next2 = brk->idx + 1;
  Glyph g_next2 = {0};

  // WB6

  if (IsAHLetterX(brk->wbcx_adj) &&
      (g.wbc == WBC_MidLetter ||
       g.wbc == WBC_MidNumLet ||
       g.wbc == WBC_Single_Quote)) 
  {
    BreakerGetNextGlyphWordBreak(brk, ud, &next2, &idx_next2, &g_next2);
    if (next2 && IsAHLetter(g_next2.wbc))
    {
      return WBRK_PRO;
    }
  }

  // WB7

  if (brk->wbcx_adj == WBCX_7 &&
      IsAHLetter(g.wbc))
  {
    return WBRK_PRO;
  }

  // WB7a

  if (brk->wbcx_adj == WBCX_Hebrew_Letter &&
      g.wbc == WBC_Single_Quote)
  {
    return WBRK_PRO;
  }

  // WB7b

  if (brk->wbcx_adj == WBCX_Hebrew_Letter &&
      g.wbc == WBC_Double_Quote)
  {
    BreakerGetNextGlyphWordBreak(brk, ud, &next2, &idx_next2, &g_next2);
    if (next2 && g_next2.wbc == WBC_Hebrew_Letter)
    {
      return WBRK_PRO;
    }
  }

  // WB7c

  if (brk->wbcx_adj == WBCX_7c &&
      g.wbc == WBC_Hebrew_Letter)
  {
    return WBRK_PRO;
  }

  // WB8

  if (brk->wbcx_adj == WBCX_Numeric &&
      g.wbc == WBC_Numeric)
  {
    return WBRK_PRO;
  }

  // WB9

  if (IsAHLetterX(brk->wbcx_adj) &&
      g.wbc == WBC_Numeric)
  {
    return WBRK_PRO;
  }

  // WB10

  if (brk->wbcx_adj == WBCX_Numeric &&
      IsAHLetter(g.wbc))
  {
    return WBRK_PRO;
  }

  // WB11

  if (brk->wbcx_adj == WBCX_11 &&
      g.wbc == WBC_Numeric)
  {
    return WBRK_PRO;
  }

  // WB12

  if (brk->wbcx_adj == WBCX_Numeric &&
      (g.wbc == WBC_MidNum ||
       g.wbc == WBC_MidNumLet ||
       g.wbc == WBC_Single_Quote))
  {
    BreakerGetNextGlyphWordBreak(brk, ud, &next2, &idx_next2, &g_next2);
    if (next2 && g_next2.wbc == WBC_Numeric)
    {
      return WBRK_PRO;
    }
  }

  // WB13

  if (brk->wbcx_adj == WBCX_Katakana ||
      g.wbc == WBC_Katakana)
  {
    return WBRK_PRO;
  }

  // WB13a

  if ((IsAHLetterX(brk->wbcx_adj) ||
      brk->wbcx_adj == WBCX_Numeric ||
      brk->wbcx_adj == WBCX_Katakana ||
      brk->wbcx_adj == WBCX_ExtendNumLet) &&
      g.wbc == WBC_ExtendNumLet)
  {
    return WBRK_PRO;
  }

  // WB13b

  if (brk->wbcx_adj == WBCX_ExtendNumLet &&
      (IsAHLetter(g.wbc) ||
       g.wbc == WBC_Numeric ||
       g.wbc == WBC_Katakana))
  {
    return WBRK_PRO;
  }

  // WB15 + 16

  if (brk->wbcx_adj == WBCX_Regional_Indicator &&
      g.wbc == WBC_Regional_Indicator)
  {
    return WBRK_PRO;
  }

  // WB999

  return WBRK_BRK;
}

GBRK BreakerComputeGbrk(Breaker* brk, LkUnicodeData* ud)
{
  // GB2

  if (brk->idx == brk->len_codepoints - 1)
  {
    return GBRK_BRK;
  }

  Glyph g = GetGlyphAtIndex(brk->codepoints, brk->len_codepoints, brk->idx + 1, ud);

  // GB3

  if (brk->gbcx == GBCX_CR &&
      g.gbc == GBC_LF)
  {
    return GBRK_PRO;
  }

  // GB4

  if (brk->gbcx == GBCX_Control ||
      brk->gbcx == GBCX_CR ||
      brk->gbcx == GBCX_LF)
  {
    return GBRK_BRK;
  }

  // GB5

  if (g.gbc == GBC_Control ||
      g.gbc == GBC_CR ||
      g.gbc == GBC_LF)
  {
    return GBRK_BRK;
  }

  // GB6

  if (brk->gbcx == GBCX_L &&
      (g.gbc == GBC_L ||
       g.gbc == GBC_V ||
       g.gbc == GBC_LV ||
       g.gbc == GBC_LVT))
  {
    return GBRK_PRO;
  }

  // GB7

  if ((brk->gbcx == GBCX_LV ||
      brk->gbcx == GBCX_V) &&
      (g.gbc == GBC_V ||
       g.gbc == GBC_T))
  {
    return GBRK_PRO;
  }

  // GB8

  if ((brk->gbcx == GBCX_LVT ||
        brk->gbcx == GBCX_T) &&
      g.gbc == GBC_T)
  {
    return GBRK_PRO;
  }

  // GB9

  if (g.gbc == GBC_Extend ||
      g.gbc == GBC_ZWJ)
  {
    return GBRK_PRO;
  }

  // GB9a

  if (g.gbc == GBC_SpacingMark)
  {
    return GBRK_PRO;
  }

  // GB9b

  if (brk->gbcx == GBCX_Prepend)
  {
    return GBRK_PRO;
  }

  // GB9c

  if ((brk->gbcx == GBCX_InCB_Consonant_Linker ||
        brk->gbcx == GBCX_InCB_Consonant_Linker_Extend) &&
      g.incb == INCB_Consonant)
  {
    return GBRK_PRO;
  }

  // GB11

  if (brk->gbcx == GBCX_EP_Extend_ZWJ &&
      g.extended_pictographic == true)
  {
    return GBRK_PRO;
  }

  // GB12 + 13

  if (brk->gbcx == GBCX_RI_RI &&
      g.gbc == GBC_Regional_Indicator)
  {
    return GBRK_PRO;
  }

  // GB999

  return GBRK_BRK;
}

BreakerResult BreakerAdvance(Breaker* brk, LkUnicodeData* ud)
{
  BreakerResult res = {0};
  
  if (brk->idx + 1 >= brk->len_codepoints)
  {
    res.glyph_idx = brk->idx;
    res.done = true;
    res.lbrk = LBRK_MAN;
    res.wbrk = WBRK_BRK;
    return res;
  }

  brk->idx++;
  if (brk->idx == 0)
  {
    Glyph g = GetGlyphAtIndex(brk->codepoints, brk->len_codepoints, 0, ud);
    brk->lbcx = (LBCX) g.lbc;
    brk->lbcx_adj = brk->lbcx;
    brk->wbcx = (WBCX) g.wbc;
    brk->wbcx_adj = brk->wbcx;
    brk->gbcx = (GBCX) g.gbc;
    brk->gc_adj = g.gc;
    brk->eaw_adj = g.eaw;
    brk->incb = g.incb;
    brk->ep = g.extended_pictographic;
    brk->ep_adj = g.extended_pictographic;
    if (brk->lbcx_adj == LBCX_CM ||
        brk->lbcx_adj == LBCX_ZWJ)
    {
      brk->lbcx_adj = LBCX_AL;
      brk->gc_adj = GC_Lu;
      brk->eaw_adj = EAW_Na;
      brk->ep_adj = false;
    }

    if (g.lbc == LBC_QU && g.gc == GC_Pi)
    {
      brk->lbcx = LBCX_15a;
      brk->lbcx_adj = LBCX_15a;
    }
    
    if (g.lbc == LBC_QU)
    {
      brk->lbcx = LBCX_19a;
      brk->lbcx_adj = LBCX_19a;
    }

    if (g.lbc == LBC_HY && g.codepoint == 0x2010)
    {
      brk->lbcx = LBCX_20a;
      brk->lbcx_adj = LBCX_20a;
    }
  }
  else
  {
    Glyph g = GetGlyphAtIndex(brk->codepoints, brk->len_codepoints, brk->idx, ud);
    LBCX lbcx_new = brk->lbcx;
    LBCX s = brk->lbcx_adj;
    LBC e = g.lbc;
    if ((s == LBCX_ZW ||
          s == LBCX_ZW_SP) &&
        e == LBC_SP)
    {
      lbcx_new = LBCX_ZW_SP;
    }
    else if ((s == LBCX_OP ||
          s == LBCX_OP_SP) &&
        e == LBC_SP)
    {
      lbcx_new = LBCX_OP_SP;
    }
    else if ((s == LBCX_BK ||
              s == LBCX_CR ||
              s == LBCX_LF ||
              s == LBCX_NL ||
              s == LBCX_OP ||
              s == LBCX_QU ||
              s == LBCX_GL ||
              s == LBCX_SP ||
              s == LBCX_ZW) &&
            (e == LBC_QU &&
             g.gc == GC_Pi))
    {
      lbcx_new = LBCX_15a;
    }
    else if (s == LBCX_15a && e == LBC_SP)
    {
      lbcx_new = LBCX_15a_SP;
    }
    else if ((s == LBCX_CL ||
              s == LBCX_CP ||
              s == LBCX_CL_CP_SP) &&
              e == LBC_SP)
    {
      lbcx_new = LBCX_CL_CP_SP;
    }
    else if ((s == LBCX_B2 ||
              s == LBCX_B2_SP) &&
              e == LBC_SP)
    {
      lbcx_new = LBCX_B2_SP;
    }
    else if ((brk->eaw_adj != EAW_F &&
              brk->eaw_adj != EAW_W &&
              brk->eaw_adj != EAW_H) &&
              e == LBC_QU)
    {
      lbcx_new = LBCX_19a;
    }
    else if ((s == LBCX_BK ||
              s == LBCX_CR ||
              s == LBCX_LF ||
              s == LBCX_NL ||
              s == LBCX_SP ||
              s == LBCX_ZW ||
              s == LBCX_CB ||
              s == LBCX_GL) &&
            (e == LBC_HY ||
             brk->codepoints[brk->idx] == 0x2010))
    {
      lbcx_new = LBCX_20a;
    }
    else if (s == LBCX_HL &&
            (e == LBC_HY ||
            (e == LBC_BA &&
             g.eaw != EAW_F &&
             g.eaw != EAW_W &&
             g.eaw != EAW_H)))
    {
      lbcx_new = LBCX_21a;
    }
    else if ((s == LBCX_NU ||
              s == LBCX_NU_SY ||
              s == LBCX_NU_IS) &&
              e == LBC_SY)
    {
      lbcx_new = LBCX_NU_SY;
    }
    else if ((s == LBCX_NU ||
              s == LBCX_NU_SY ||
              s == LBCX_NU_IS) &&
              e == LBC_IS)
    {
      lbcx_new = LBCX_NU_IS;
    }
    else if ((s == LBCX_NU ||
              s == LBCX_NU_SY ||
              s == LBCX_NU_IS) &&
              e == LBC_CL)
    {
      lbcx_new = LBCX_NU_CL;
    }
    else if ((s == LBCX_NU ||
              s == LBCX_NU_SY ||
              s == LBCX_NU_IS) &&
              e == LBC_CP)
    {
      lbcx_new = LBCX_NU_CP;
    }
    else if ((s == LBCX_AK ||
              brk->codepoints[brk->idx - 1] == 0x25CC ||
              s == LBCX_AS) &&
              e == LBC_VI)
    {
      lbcx_new = LBCX_28a;
    }
    else if (s == LBCX_RI && e == LBC_RI)
    {
      lbcx_new = LBCX_RI_RI;
    }
    else
    {
      lbcx_new = (LBCX) e;
    }

    if ((e == LBC_CM ||
          e == LBC_ZWJ) &&
        (s != LBCX_BK &&
         s != LBCX_CR &&
         s != LBCX_LF &&
         s != LBCX_NL &&
         s != LBCX_SP &&
         s != LBCX_ZW))
    {
      assert(lbcx_new == LBCX_ZWJ || lbcx_new == LBCX_CM);
      brk->lbcx = lbcx_new;
      brk->lbcx_adj = brk->lbcx_adj;  // unchanged
      brk->gc_adj = brk->gc_adj;      // unchanged 
      brk->eaw_adj = brk->eaw_adj;    // unchanged
      brk->ep_adj = brk->ep_adj;      // unchanged
    }
    else if ((e == LBC_CM ||
              e == LBC_ZWJ) &&
            (s == LBCX_BK ||
             s == LBCX_CR ||
             s == LBCX_LF ||
             s == LBCX_NL ||
             s == LBCX_SP ||
             s == LBCX_ZW))
    {
      assert(lbcx_new == LBCX_ZWJ || lbcx_new == LBCX_CM);
      brk->lbcx = lbcx_new;
      brk->lbcx_adj = LBCX_AL;
      brk->gc_adj = GC_Lu;
      brk->eaw_adj = EAW_Na;
      brk->ep_adj = false;
    }
    else
    {
      brk->lbcx = lbcx_new;
      brk->lbcx_adj = lbcx_new;
      brk->gc_adj = g.gc;
      brk->eaw_adj = g.eaw;
      brk->ep_adj = g.extended_pictographic;
    }

    WBCX wbcx_new = WBCX_XX;

    if ((brk->wbcx_adj == WBCX_ALetter ||
        brk->wbcx_adj == WBCX_Hebrew_Letter) &&
        (g.wbc == WBC_MidLetter || 
         g.wbc == WBC_MidNumLet ||
         g.wbc == WBC_Single_Quote))
    {
      wbcx_new = WBCX_7;
    }
    else if (brk->wbcx_adj == WBCX_Hebrew_Letter &&
            g.wbc == WBC_Double_Quote)
    {
      wbcx_new = WBCX_7c;
    }
    else if (brk->wbcx_adj == WBCX_Numeric &&
        (g.wbc == WBC_MidNum ||
         g.wbc == WBC_MidNumLet ||
         g.wbc == WBC_Single_Quote))
    {
      wbcx_new = WBCX_11;
    }
    else if (brk->wbcx_adj == WBCX_Regional_Indicator &&
            g.wbc == WBC_Regional_Indicator)
    {
      wbcx_new = WBCX_RI_RI;
    }
    else 
    {
      wbcx_new = (WBCX) g.wbc;
    }

    if ((g.wbc == WBC_Extend ||
        g.wbc == WBC_Format ||
        g.wbc == WBC_ZWJ) &&
        (brk->wbcx_adj != WBCX_CR &&
         brk->wbcx_adj != WBCX_LF &&
         brk->wbcx_adj != WBCX_Newline))
    {
      brk->wbcx = wbcx_new;
      brk->wbcx_adj = brk->wbcx_adj; // unchanged
    }
    else
    {
      brk->wbcx = wbcx_new;
      brk->wbcx_adj = wbcx_new;
    }

    // Grapheme Breaking Properties

    if ((brk->incb == INCB_Consonant ||
          brk->gbcx == GBCX_InCB_Consonant_Extend) &&
        g.incb == INCB_Extend)
    {
      brk->gbcx = GBCX_InCB_Consonant_Extend;
    }
    else if ((brk->incb == INCB_Consonant ||
              brk->gbcx == GBCX_InCB_Consonant_Extend ||
              brk->gbcx == GBCX_InCB_Consonant_Linker ||
              brk->gbcx == GBCX_InCB_Consonant_Linker_Extend) &&
              g.incb == INCB_Linker)
    {
      brk->gbcx = GBCX_InCB_Consonant_Linker;
    }
    else if ((brk->gbcx == GBCX_InCB_Consonant_Linker ||
              brk->gbcx == GBCX_InCB_Consonant_Linker_Extend) &&
              g.incb == INCB_Extend)
    {
      brk->gbcx = GBCX_InCB_Consonant_Linker_Extend;
    }
    else if ((brk->ep ||
              brk->gbcx == GBCX_EP_Extend) &&
              g.incb == INCB_Extend)
    {
      brk->gbcx = GBCX_EP_Extend;
    }
    else if ((brk->ep ||
              brk->gbcx == GBCX_EP_Extend) &&
              g.gbc == GBC_ZWJ)
    {
      brk->gbcx = GBCX_EP_Extend_ZWJ;
    }
    else if (brk->gbcx == GBCX_Regional_Indicator &&
            g.gbc == GBC_Regional_Indicator)
    {
      brk->gbcx = GBCX_RI_RI;
    }
    else
    {
      brk->gbcx = (GBCX) g.gbc;
    }

    brk->ep = g.extended_pictographic;
    brk->incb = g.incb;
  }


  LBRK lbrk = BreakerComputeLbrk(brk, ud);
  WBRK wbrk = BreakerComputeWbrk(brk, ud);
  GBRK gbrk = BreakerComputeGbrk(brk, ud);

  res.glyph_idx = brk->idx;
  res.done = false; 
  res.lbrk = lbrk;
  res.wbrk = wbrk;
  res.gbrk = gbrk;

  return res;
}

BreakerResult* lkGetBreaks(LkArena* arena, const struct LkText* text, LkUnicodeData* ud)
{
  BreakerResult* breaks = APushArray(arena, BreakerResult, text->codepoint_count);
  Breaker brk = {0};
  BreakerCreate(text->codepoints, text->codepoint_count, &brk);
  for (i32 i = 0; i < text->codepoint_count; i++)
  {
    breaks[i] = BreakerAdvance(&brk, ud);
  }
  return breaks;
}
