#include <lexkit/linebreak.h>
#include <lexkit/types.h>
#include <lexkit/pairs.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* g_map_str_to_ubrkcls[] = {
  "OP",		//	UBRKCLS_OP
  "CL",		//	UBRKCLS_CL
  "CP",		//	UBRKCLS_CP
  "QU",		//	UBRKCLS_QU
  "GL",		//	UBRKCLS_GL
  "NS",		//	UBRKCLS_NS
  "EX",		//	UBRKCLS_EX
  "SY",		//	UBRKCLS_SY
  "IS",		//	UBRKCLS_IS
  "PR",		//	UBRKCLS_PR
  "PO",		//	UBRKCLS_PO
  "NU",		//	UBRKCLS_NU
  "AL",		//	UBRKCLS_AL
  "HL",		//	UBRKCLS_HL
  "ID",		//	UBRKCLS_ID
  "IN",		//	UBRKCLS_IN
  "HY",		//	UBRKCLS_HY
  "BA",		//	UBRKCLS_BA
  "BB",		//	UBRKCLS_BB
  "B2",		//	UBRKCLS_B2
  "ZW",		//	UBRKCLS_ZW
  "CM",		//	UBRKCLS_CM
  "WJ",		//	UBRKCLS_WJ
  "H2",		//	UBRKCLS_H2
  "H3",		//	UBRKCLS_H3
  "JL",		//	UBRKCLS_JL
  "JV",		//	UBRKCLS_JV
  "JT",		//	UBRKCLS_JT
  "RI",		//	UBRKCLS_RI
  "EB",		//	UBRKCLS_EB
  "EM",		//	UBRKCLS_EM
  "ZWJ",	//	UBRKCLS_ZWJ
  "CB",		//	UBRKCLS_CB

  "AI",		//	UBRKCLS_AI
  "BK",		//	UBRKCLS_BK
  "CJ",		//	UBRKCLS_CJ
  "CR",		//	UBRKCLS_CR
  "LF",		//	UBRKCLS_LF
  "NL",		//	UBRKCLS_NL
  "SA",		//	UBRKCLS_SA
  "SG",		//	UBRKCLS_SG
  "SP",		//	UBRKCLS_SP
  "XX",		//	UBRKCLS_XX
};

static bool IsHexChar(char c)
{
  return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'));
}

static u32 CharToHex(char c)
{
  if (c >= '0' && c <= '9')
  {
    return c - '0';
  }
  return c - 'A' + 0xA;
}

void UnicodeBreakClassMappingLoadFromPath(
    const char* str_path,
    UnicodeBreakClassMapping* o_ubcm)
{
  assert(str_path != NULL);
  assert(o_ubcm != NULL);

  FILE* fil = NULL;
  errno_t error = fopen_s(&fil, str_path, "r");
  assert(error == 0);

  char buf[256];

  // TODO: Faster way to check line count?

  int num_class = 0;

  while (fgets(buf, 256, fil) != NULL)
  {
    if (!IsHexChar(buf[0]))
      continue;
    num_class++;
  }

  o_ubcm->range_start   = (int*)malloc(num_class * sizeof(int));
  o_ubcm->range_end     = (int*)malloc(num_class * sizeof(int));
  o_ubcm->range_brkcls  = (UBRKCLS*)malloc(num_class * sizeof(UBRKCLS));
  o_ubcm->num_class     = num_class;

  rewind(fil);

  int i_class = 0;
  while (fgets(buf, 256, fil) != NULL)
  {
    // TODO: Check line length correctly

    if (!IsHexChar(buf[0]))
      continue;

    int range_start = 0;
    int i = 0;
    while (IsHexChar(buf[i]))
    {
      range_start <<= 4;
      range_start +=  CharToHex(buf[i]);
      i++;
    }
    int range_end = range_start;
    if (buf[i] == '.')
    {
      i += 2;
      range_end = 0;
      while (IsHexChar(buf[i]))
      {
        range_end <<= 4;
        range_end +=  CharToHex(buf[i]);
        i++;
      }
    }
    while (buf[i] != '\0' && buf[i] != ';')
    {
      i++;
    }
    i += 2;
    int ubrkcls_start = i;
    int ubrkcls_end = i;
    while (buf[i] != '\0' && buf[i] >= 'A' && buf[i] <= 'Z')
    {
      ubrkcls_end++;
      i++;
    }

    UBRKCLS brkcls = UBRKCLS_XX;
    for (int i = 0; i < UBRKCLS_Count; i++) 
    {
      int len_check = (ubrkcls_end - ubrkcls_start < 3) ? 
                      (ubrkcls_end - ubrkcls_start) :
                      3;
      if (!strncmp(buf + ubrkcls_start, g_map_str_to_ubrkcls[i], len_check))
      {
        brkcls = (UBRKCLS) i;
        break;
      }
    }
   
    o_ubcm->range_start[i_class]    = range_start;
    o_ubcm->range_end[i_class]      = range_end;
    o_ubcm->range_brkcls[i_class]   = brkcls;
    i_class++;
  }
}

static UBRKCLS UnicodeBreakClassMappingLookup(
    UnicodeBreakClassMapping* ubcm,
    u32 codepoint)
{
  for (int i = 0; i < ubcm->num_class; i++)
  {
    if (codepoint >= ubcm->range_start[i] && codepoint <= ubcm->range_end[i])
    {
      return ubcm->range_brkcls[i];
    }
  }

  // Invalid unicode code point should have been checked before calling this function

  assert(false);
  return UBRKCLS_XX;
}

static UBRKCLS UnicodeBreakClassMap(UBRKCLS brkcls)
{
  switch(brkcls)
  {
    case UBRKCLS_AI:
      return UBRKCLS_AL;

    case UBRKCLS_SA:
    case UBRKCLS_SG:
    case UBRKCLS_XX:
      return UBRKCLS_AL;

    case UBRKCLS_CJ:
      return UBRKCLS_NS;

    default:
      return brkcls;
  }
}

static UBRKCLS UnicodeBreakClassMapFirst(UBRKCLS brkcls)
{
  switch (brkcls)
  {
    case UBRKCLS_LF:
    case UBRKCLS_NL:
      return UBRKCLS_BK;

    case UBRKCLS_SP:
      return UBRKCLS_WJ;

    default:
      return brkcls;
  }
}

typedef enum
{
  BRKADVRESK_Invalid    = 0,
  BRKADVRESK_Found      = 1,
  BRKADVRESK_Complete   = 2,
  BRKADVRESK_ParseError = 3,
} BRKADVRESK;

// Advance to next codepoint in lb->str
// Modifies lb->pos and lb->brkcls_next

static BRKADVRESK LineBreakerTryAdvance(
    LineBreaker* lb,
    UnicodeBreakClassMapping* ubcm)
{
  const u8* x = &lb->str[lb->pos];
  u32 codepoint = 0;

  if (lb->pos == lb->len_str)
  {
    return BRKADVRESK_Complete;
  }

  if (x[0] <= 0x7F)
  {
    codepoint = x[0];
    lb->pos++;
  }
  else if (x[0] <= 0xDF)
  {
    if (lb->pos + 1 < lb->len_str)
    {
      codepoint = ((x[0] & 0b00011111) << 6) | (x[1] & 0b00111111);
      lb->pos += 2;
    }
    else 
      return BRKADVRESK_ParseError;
  }
  else if (x[0] <= 0xEF)
  {
    if (lb->pos + 2 < lb->len_str)
    {
      codepoint = ((x[0] & 0b00001111) << 12) |
        ((x[1] & 0b00111111) << 6) |
        (x[2] & 0b00111111);
      lb->pos += 3;
    }
    else
      return BRKADVRESK_ParseError;
  }
  else
  {
    if (lb->pos + 3 < lb->len_str)
    {
      codepoint = ((x[0] & 0b00000111) << 18) |
        ((x[1] & 0b00111111) << 12) |
        ((x[2] & 0b00111111) << 6) |
        (x[3] & 0b00111111);
      lb->pos += 4;
      
      if (codepoint > 0x10FFFF)
      {
        return BRKADVRESK_ParseError;
      }
    }
    else
      return BRKADVRESK_ParseError;
  }

  UBRKCLS brkcls = UnicodeBreakClassMappingLookup(ubcm, codepoint);
  lb->brkcls_next = UnicodeBreakClassMap(brkcls);

  return BRKADVRESK_Found;
}

bool LineBreakerTryCreate(
    const u8* str,
    u64 len_str,
    UnicodeBreakClassMapping* ubcm,
    LineBreaker* o_lb)
{
  assert(o_lb != NULL);
  assert(str != NULL);

  o_lb->str         = str;
  o_lb->len_str     = len_str;
  o_lb->pos         = 0;
  o_lb->pos_next    = 0;
  o_lb->brkcls_cur  = UBRKCLS_XX;
  o_lb->brkcls_next = UBRKCLS_XX;
  o_lb->LB8a        = false;
  o_lb->LB21a       = false;
  o_lb->LB30a       = 0;

  BRKADVRESK res = LineBreakerTryAdvance(o_lb, ubcm);
  if (res != BRKADVRESK_Found)
  {
    return false;
  }

  o_lb->brkcls_cur  = UnicodeBreakClassMapFirst(o_lb->brkcls_next);
  o_lb->LB8a        = (o_lb->brkcls_next == UBRKCLS_ZWJ);
  o_lb->pos         = 0;

  return true;
}

// Handle breaks that are not handled by the pair table,
// Currently this only tells us not to break after whitespace
// Returns true if break is handled here so we can skip more
// expensive pair table lookup. Only modifies o_should_break
// if true is returned.

static bool LineBreakerTrySimpleBreak(LineBreaker* lb, bool *o_should_break)
{
  switch (lb->brkcls_next)
  {
    case UBRKCLS_SP:
      *o_should_break = false;
      return true;

    case UBRKCLS_BK:
    case UBRKCLS_LF:
    case UBRKCLS_NL:
      lb->brkcls_cur = UBRKCLS_BK;
      *o_should_break = false;
      return true;

    case UBRKCLS_CR:
      lb->brkcls_cur = UBRKCLS_CR;
      *o_should_break = false;
      return true;

    default:
      return false;
  }
}

static bool LineBreakerPairTableBreak(LineBreaker* lb, UBRKCLS brkcls_last)
{
  bool should_break = false;

  switch (g_pair_table[lb->brkcls_cur][lb->brkcls_next])
  {
    case BRK_DI: // Direct break
      should_break = true;
      break;

    case BRK_IN: // Indirect break
      should_break = (brkcls_last == UBRKCLS_SP);
      break;

    case BRK_CI: // Indirect break for combining marks
      should_break = (brkcls_last == UBRKCLS_SP);
      if (!should_break)
      {
        should_break = false;
        return should_break;
      }
      break;

    case BRK_CP: // Prohibitied for combining marks
      if (brkcls_last != UBRKCLS_SP)
        return should_break;
      break;

    case BRK_PR: // Prohibited break
      break;
  }

  if (lb->LB8a)
    should_break = false;

  if (lb->LB21a && (lb->brkcls_cur == UBRKCLS_HY || lb->brkcls_cur == UBRKCLS_BA))
  {
    should_break = false;
    lb->LB21a = false;
  }
  else
  {
    lb->LB21a = (lb->brkcls_cur == UBRKCLS_HL);
  }

  if (lb->brkcls_cur == UBRKCLS_RI)
  {
    lb->LB30a++;
    if (lb->LB30a == 2 && lb->LB30a == UBRKCLS_RI)
    {
      should_break = true;
      lb->LB30a = 0;
    }
  }
  else
  {
    lb->LB30a = 0;
  }

  lb->brkcls_cur = lb->brkcls_next;
  return true;
}

LineBreakerResult LineBreakerNextBreak(
    LineBreaker* lb,
    UnicodeBreakClassMapping* ubcm)
{
  assert(lb != NULL);

  LineBreakerResult res = {0};
  i32 pos_last = lb->pos;
  while (lb->pos < lb->len_str)
  {
    pos_last              = lb->pos;
    UBRKCLS brkcls_last   = lb->brkcls_next;
    BRKADVRESK brkadvres  = LineBreakerTryAdvance(lb, ubcm);
    switch (brkadvres)
    {
      case BRKADVRESK_Invalid: 
        res.kind = BRKRESK_Invalid;
        break;

      case BRKADVRESK_Complete:
        res.kind = BRKRESK_Complete;
        break;

      case BRKADVRESK_ParseError:
        res.kind = BRKRESK_ParseError;
        break;
      
      default:
        assert(brkadvres == BRKADVRESK_Found);
        break;
    }

    if (brkadvres != BRKADVRESK_Found)
    {
      res.pos = lb->pos;
      res.force_break = false;
      return res;
    }

    if ((lb->brkcls_cur == UBRKCLS_BK) ||
        ((lb->brkcls_cur == UBRKCLS_CR) && (lb->brkcls_next != UBRKCLS_LF)))
    {
      lb->brkcls_cur = UnicodeBreakClassMapFirst(
          UnicodeBreakClassMap(lb->brkcls_next)
      );
      res.pos = pos_last;
      res.force_break = true;
      return res;
    }

    bool should_break = false;
    bool skip_pair_table_break = LineBreakerTrySimpleBreak(lb, &should_break);
    
    if (!skip_pair_table_break)
    {
      should_break = LineBreakerPairTableBreak(lb, brkcls_last);
    }

    lb->LB8a = (lb->brkcls_next == UBRKCLS_ZWJ);
    
    if (should_break)
    {
      res.kind = BRKRESK_Found;
      res.force_break = false;
      res.pos = pos_last;
      return res;
    }
  }

  if (pos_last < lb->len_str)
  {
    res.kind = BRKRESK_Found;
    res.force_break = false;
    res.pos = lb->len_str;
    return res;
  }

  res.kind = BRKRESK_Invalid;
  return res;
}
