#include <lexkit/wordbreak.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* g_map_str_to_uwbrkcls[] = {
  "CR",                 // UWBRKCLS_CR
  "LF",	                // UWBRKCLS_LF
  "Newline",	          // UWBRKCLS_Newline
  "Extend",	            // UWBRKCLS_Extend
  "ZWJ",	              // UWBRKCLS_ZWJ
  "Regional_Indicator",	// UWBRKCLS_Regional_Indicator
  "Format",	            // UWBRKCLS_Format
  "Katakana",	          // UWBRKCLS_Katakana
  "Hebrew_Letter",	    // UWBRKCLS_Hebrew_Letter
  "ALetter",	          // UWBRKCLS_ALetter
  "Single_Quote",	      // UWBRKCLS_Single_Quote
  "Double_Quote",	      // UWBRKCLS_Double_Quote
  "MidNumLet",	        // UWBRKCLS_MidNumLet
  "MidLetter",	        // UWBRKCLS_MidLetter
  "MidNum",	            // UWBRKCLS_MidNum
  "Numeric",	          // UWBRKCLS_Numeric
  "ExtendNumLet",     	// UWBRKCLS_ExtendNumLet
  "E_Base",	            // UWBRKCLS_E_Base
  "E_Modifier",	        // UWBRKCLS_E_Modifier
  "Glue_After_Zwj",	    // UWBRKCLS_Glue_After_Zwj
  "E_BASE_GAZ",	        // UWBRKCLS_E_BASE_GAZ
  "WSegSpace",	        // UWBRKCLS_WSegSpace
  "Any",	              // UWBRKCLS_Any
};

typedef enum
{
  WBRK_BRK,             // Break oppurtunity
  WBRK_PRB,             // Prohibited break
  WBRK_CDB,             // Conditional break, needs advanced handling
} WBRK;

const WBRK g_wpair_table[24][24] = {
  //CR,       LF,       Newline,  Extend,   ZWJ,      RI,       Format,   Katakana, HL,       AL,       SQ,       DQ,       MNL,      MidLetter,MidNum,   Numeric,  ENL,      E_Base,   E_M,      G_A_ZWJ,  EBG,      WSegSpace,Any,
  { WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // CR
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // LF
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // Newline
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // Extend
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // ZWJ
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_CDB, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // RI
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // Format
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // Katakana
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_PRB, WBRK_CDB, WBRK_CDB, WBRK_CDB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // HL
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_CDB, WBRK_BRK, WBRK_CDB, WBRK_CDB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // AL
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_CDB, WBRK_CDB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_CDB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // SQ
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_CDB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // DQ
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_CDB, WBRK_CDB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_CDB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // MNL
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_CDB, WBRK_CDB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // MidLetter
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_CDB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // MidNum
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_CDB, WBRK_BRK, WBRK_CDB, WBRK_BRK, WBRK_CDB, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // Numeric
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // ENL
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // E_Base
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // E_M
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // G_A_ZWJ
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // EBG
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_BRK},  // WSegSpace
  { WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_PRB, WBRK_PRB, WBRK_BRK, WBRK_PRB, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK, WBRK_BRK},  // Any
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

void UnicodeWordBreakClassMappingLoadFromPath(
    const char* str_path,
    UnicodeWordBreakClassMapping* o_uwbcm)
{
  assert(str_path != NULL);
  assert(o_uwbcm != NULL);

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

  o_uwbcm->range_start   = (int*)malloc(num_class * sizeof(int));
  o_uwbcm->range_end     = (int*)malloc(num_class * sizeof(int));
  o_uwbcm->range_wbrkcls = (UWBRKCLS*)malloc(num_class * sizeof(UWBRKCLS));
  o_uwbcm->num_class     = num_class;

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
    int uwbrkcls_start = i;
    int uwbrkcls_end = i;
    while (buf[i] != '\0' && buf[i] >= 'A' && buf[i] <= 'Z')
    {
      uwbrkcls_end++;
      i++;
    }

    UWBRKCLS wbrkcls = UWBRKCLS_Any;
    for (int i = 0; i < UWBRKCLS_Count; i++) 
    {
      int len_check = (uwbrkcls_end - uwbrkcls_start < 3) ? 
                      (uwbrkcls_end - uwbrkcls_start) :
                      3;
      if (!strncmp(buf + uwbrkcls_start, g_map_str_to_uwbrkcls[i], len_check))
      {
        wbrkcls = (UWBRKCLS) i;
        break;
      }
    }
   
    o_uwbcm->range_start[i_class]    = range_start;
    o_uwbcm->range_end[i_class]      = range_end;
    o_uwbcm->range_wbrkcls[i_class]   = wbrkcls;
    i_class++;
  }
}

static UWBRKCLS UnicodeWordBreakClassMappingLookup(
    UnicodeWordBreakClassMapping* uwbcm,
    u32 codepoint)
{
  for (int i = 0; i < uwbcm->num_class; i++)
  {
    if (codepoint >= uwbcm->range_start[i] && codepoint <= uwbcm->range_end[i])
    {
      return uwbcm->range_wbrkcls[i];
    }
  }

  return UWBRKCLS_Any;
}

typedef enum
{
  UCPRESK_Invalid,
  UCPRESK_Found,
  UCPRESK_Complete,
  UCPRESK_ParseError,
} UCPRESK;

typedef struct
{
  UCPRESK kind;
  u32     codepoint;
  i32     advance;
} UnicodeCodepointResult;


static UnicodeCodepointResult GetCodepointAtPos(
    const u8* str, u64 len_str, i32 pos)
{
  if (pos >= len_str)
  {
    UnicodeCodepointResult res = {0};
    res.kind      = UCPRESK_Complete;
    return res;
  }

  if (str[pos] <= 0x7F)
  {
    UnicodeCodepointResult res = {0};
    res.kind      = UCPRESK_Found;
    res.codepoint = str[0];
    res.advance   = 1;
    return res;
  }
  else if (str[pos] <= 0xDF)
  {
    if (pos + 1 < len_str)
    {
      UnicodeCodepointResult res = {0};
      res.kind      = UCPRESK_Found;
      res.codepoint = ((str[pos] & 0b00011111) << 6) | (str[pos + 1] & 0b00111111);
      res.advance   = 2;
      return res;
    }
  }
  else if (str[pos] <= 0xEF)
  {
    if (pos + 2 < len_str)
    {
      UnicodeCodepointResult res = {0};
      res.kind      = UCPRESK_Found;
      res.codepoint = ((str[pos] & 0b00001111) << 12) |
        ((str[pos + 1] & 0b00111111) << 6) |
        (str[pos + 2] & 0b00111111);
      res.advance   = 3;
      return res;
    }
  }
  else
  {
    if (pos + 3 < len_str)
    {
      UnicodeCodepointResult res = {0};
      res.kind      = UCPRESK_Found;
      res.codepoint = ((str[pos] & 0b00000111) << 18) |
        ((str[pos + 1] & 0b00111111) << 12) |
        ((str[pos + 2] & 0b00111111) << 6) |
        (str[pos + 3] & 0b00111111);
      res.advance   = 4;
      
      if (res.codepoint <= 0x10FFFF)
      {
        return res;
      }
    }
  }

  UnicodeCodepointResult res = {0};
  res.kind = UCPRESK_ParseError;
  return res;
}

bool WordBreakerTryCreate(
    const u8* str, u64 len_str,
    int num_glyph,
    UnicodeWordBreakClassMapping* uwbcm,
    WordBreaker* o_wb)
{
  assert(o_wb != NULL);

  o_wb->str       = str;
  o_wb->len_str   = len_str;
  o_wb->num_glyph = num_glyph;
  
  i32 pos = 0;
  
  if (pos < len_str)
  {
    UnicodeCodepointResult ucr_cur = GetCodepointAtPos(str, len_str, pos);
    if (ucr_cur.kind != UCPRESK_Found) return false;
    o_wb->wbrkcls_cur = UnicodeWordBreakClassMappingLookup(uwbcm, ucr_cur.codepoint);
    o_wb->wbrkcls_cur_raw = o_wb->wbrkcls_cur;
    pos += ucr_cur.advance;
  }

  if (pos < len_str)
  {
    UnicodeCodepointResult ucr_next = GetCodepointAtPos(str, len_str, pos);
    if (ucr_next.kind != UCPRESK_Found) return false;
    o_wb->wbrkcls_next = UnicodeWordBreakClassMappingLookup(uwbcm, ucr_next.codepoint);
    pos += ucr_next.advance;
  }

  if (pos < len_str)
  {
    UnicodeCodepointResult ucr_next2 = GetCodepointAtPos(str, len_str, pos);
    if (ucr_next2.kind != UCPRESK_Found) return false;
    o_wb->wbrkcls_next2 = UnicodeWordBreakClassMappingLookup(uwbcm, ucr_next2.codepoint);
    pos += ucr_next2.advance;
  }

  o_wb->wbrkcls_prev = UWBRKCLS_Any;
  return true;
}

typedef enum
{
  WBRKADVRES_Invalid,
  WBRKADVRES_Found,
  WBRKADVRES_Complete,
  WBRKADVRES_ParseError,
} WBRKADVRES;

WBRKADVRES WordBreakerTryAdvance(WordBreaker* wb, UnicodeWordBreakClassMapping* uwbcm)
{
  UnicodeCodepointResult res = GetCodepointAtPos(wb->str, wb->len_str, wb->pos_next3);
  switch (res.kind)
  {
    case UCPRESK_Invalid:
      return WBRKADVRES_Invalid;

    case UCPRESK_ParseError:
      return WBRKADVRES_ParseError;

    default:
      assert(res.kind == UCPRESK_Found || res.kind == UCPRESK_Complete);
  }

  if (res.kind == UCPRESK_Complete)
  {

  }

  wb->wbrkcls_prev = wb->wbrkcls_
}

// Unicode "macro" correspoding to ALetter | Hebrew_Letter

static bool IsAHLetter(UWBRKCLS wbrkcls)
{
  return (wbrkcls == UWBRKCLS_ALetter) || (wbrkcls == UWBRKCLS_Hebrew_Letter);
}

// Unicode "macro" correspoding to MidNumLet | Single_Quote

static bool IsMidNumLetQ(UWBRKCLS wbrkcls)
{
  return (wbrkcls == UWBRKCLS_MidNumLet) || (wbrkcls == UWBRKCLS_Single_Quote);
}

WordBreakerResult WordBreakerNextBreak(
    WordBreaker* wb,
    UnicodeWordBreakClassMapping* uwbcm)
{
  assert(wb != NULL);
  assert(uwbcm != NULL);
  
  if (wb->glyph >= wb->num_glyph)
  {
    WordBreakerResult res = {0};
    res.kind = WBRKRESK_Complete;
    res.glyph = wb->num_glyph;
    return res;
  }

  while (wb->glyph < wb->num_glyph)
  {
    bool has_prev   = (wb->glyph > 0);
    bool has_next   = (wb->glyph + 1 < wb->num_glyph);
    bool has_next2  = (wb->glyph + 2 < wb->num_glyph);

    // WB2: Word break at end of text

    if (!has_next)
    {
      WordBreakerResult res = {0};
      res.kind = WBRKRESK_Found;
      res.glyph = wb->glyph;
      return res;
    }

    // WB3c: Do not break within emoji zwj sequences.

    if (wb->wbrkcls_cur_raw == UWBRKCLS_ZWJ)
    {
      if (wb->wbrkcls_next == UWBRKCLS_Extended_Pictograph)
      {
        WordBreakerResult res = {0};
        res.kind = WBRKRESK_Found;
        res.glyph = wb->glyph;
        return res;
      }
    }

    WBRK wbrk_pair = g_wpair_table[wb->wbrkcls_cur][wb->wbrkcls_next];

    if (wbrk_pair == WBRK_BRK)
    {
      WordBreakerResult res = {0};
      res.kind = WBRKRESK_Found;
      res.glyph = wb->glyph;
      break;
    }

    if (wbrk_pair == WBRK_PRB)
    {
      continue;
    }

    // Conditional Breaks that need extra checking
    assert(wbrk_pair == WBRK_CDB);

    // WB6,7,7b,7c: Do not break letters across certain punctuation, such as within "e.g." or "example.com".
    // WB6

    if (has_next2)
    {
      if (IsAHLetter(wb->wbrkcls_cur) &&
          (wb->wbrkcls_next == UWBRKCLS_MidLetter || IsMidNumLetQ(wb->wbrkcls_next)) &&
          IsAHLetter(wb->wbrkcls_next2))
      { 
        continue;
      }
    }

    // WB7

    if (has_prev)
    {

      if (IsAHLetter(wb->wbrkcls_prev) &&
          (wb->wbrkcls_cur == UWBRKCLS_MidLetter || IsMidNumLetQ(wb->wbrkcls_cur)) &&
          IsAHLetter(wb->wbrkcls_next))
      { 
        continue;
      }
    }

    // WB7b

    if (has_next2)
    {
      if (wb->wbrkcls_cur == UWBRKCLS_Hebrew_Letter &&
          wb->wbrkcls_next == UWBRKCLS_Double_Quote &&
          wb->wbrkcls_next2 == UWBRKCLS_Hebrew_Letter)
      {
        continue;
      }
    }

    // WB7c

    if (has_prev)
    {
      if (wb->wbrkcls_prev == UWBRKCLS_Hebrew_Letter &&
          wb->wbrkcls_cur == UWBRKCLS_Double_Quote &&
          wb->wbrkcls_next == UWBRKCLS_Hebrew_Letter)
      {
        continue;
      }
    }

    // WB11,12: Do not break within sequences, such as “3.2” or “3,456.789”.
    // WB11

    if (has_prev)
    {
      if (wb->wbrkcls_prev == UWBRKCLS_Numeric &&
          (wb->wbrkcls_cur == UWBRKCLS_MidNum || IsMidNumLetQ(wb->wbrkcls_cur)) &&
          wb->wbrkcls_next == UWBRKCLS_Numeric)
      {
        continue;
      }
    }

    // WB12

    if (has_next2)
    {
      if (wb->wbrkcls_cur == UWBRKCLS_Numeric &&
          (wb->wbrkcls_next == UWBRKCLS_MidNum || IsMidNumLetQ(wb->wbrkcls_next)) &&
          wb->wbrkcls_next2 == UWBRKCLS_Numeric)
      {
        continue;
      }
    }

    // WB15,16: Do not break within emoji flag sequences. That is, do not break between regional indicator
    //  (RI) symbols if there is an odd number of RI characters before the break point.

    if (wb->wbrkcls_cur == UWBRKCLS_Regional_Indicator &&
        wb->wbrkcls_next == UWBRKCLS_Regional_Indicator &&
        wb->WB1516 % 2 == 1)
    {
      continue;
    }
  }

  WBRKADVRES advres = WordBreakerTryAdvance(wb, uwbcm);
}
