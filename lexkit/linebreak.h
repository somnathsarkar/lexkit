#include <stdbool.h>

#include <lexkit/types.h>

// Unicode line break classes

typedef enum
{
  UBRKCLS_OP = 0,   // Opening punctuation
  UBRKCLS_CL = 1,   // Closing punctuation
  UBRKCLS_CP = 2,   // Closing parenthesis
  UBRKCLS_QU = 3,   // Ambiguous quotation
  UBRKCLS_GL = 4,   // Glue
  UBRKCLS_NS = 5,   // Non-starters
  UBRKCLS_EX = 6,   // Exclamation/Interrogation
  UBRKCLS_SY = 7,   // Symbols allowing break after
  UBRKCLS_IS = 8,   // Infix separator
  UBRKCLS_PR = 9,   // Prefix
  UBRKCLS_PO = 10,  // Postfix
  UBRKCLS_NU = 11,  // Numeric
  UBRKCLS_AL = 12,  // Alphabetic
  UBRKCLS_HL = 13,  // Hebrew Letter
  UBRKCLS_ID = 14,  // Ideographic
  UBRKCLS_IN = 15,  // Inseparable characters
  UBRKCLS_HY = 16,  // Hyphen
  UBRKCLS_BA = 17,  // Break after
  UBRKCLS_BB = 18,  // Break before
  UBRKCLS_B2 = 19,  // Break on either side (but not pair)
  UBRKCLS_ZW = 20,  // Zero-width space
  UBRKCLS_CM = 21,  // Combining marks
  UBRKCLS_WJ = 22,  // Word joiner
  UBRKCLS_H2 = 23,  // Hangul LV
  UBRKCLS_H3 = 24,  // Hangul LVT
  UBRKCLS_JL = 25,  // Hangul L Jamo
  UBRKCLS_JV = 26,  // Hangul V Jamo
  UBRKCLS_JT = 27,  // Hangul T Jamo
  UBRKCLS_RI = 28,  // Regional Indicator
  UBRKCLS_EB = 29,  // Emoji Base
  UBRKCLS_EM = 30,  // Emoji Modifier
  UBRKCLS_ZWJ = 31, // Zero Width Joiner
  UBRKCLS_CB = 32,  // Contingent break

  // The following break classes are not handled by the pair table
  UBRKCLS_AI = 33,  // Ambiguous (Alphabetic or Ideograph)
  UBRKCLS_BK = 34,  // Break (mandatory)
  UBRKCLS_CJ = 35,  // Conditional Japanese Starter
  UBRKCLS_CR = 36,  // Carriage return
  UBRKCLS_LF = 37,  // Line feed
  UBRKCLS_NL = 38,  // Next line
  UBRKCLS_SA = 39,  // South-East Asian
  UBRKCLS_SG = 40,  // Surrogates
  UBRKCLS_SP = 41,  // Space
  UBRKCLS_XX = 42,  // Unknown

  UBRKCLS_Count,
} UBRKCLS;

typedef struct
{
  int*      range_start;
  int*      range_end;
  UBRKCLS*  range_brkcls;
  int       num_class;
} UnicodeBreakClassMapping;

void UnicodeBreakClassMappingLoadFromPath(
    const char* str_path,
    UnicodeBreakClassMapping* o_ubcm);

typedef enum
{
  BRKRESK_Invalid     = 0,
  BRKRESK_Found       = 1,
  BRKRESK_Complete    = 2,
  BRKRESK_ParseError  = 3,
} BRKRESK;

typedef struct
{
  BRKRESK kind;
  bool    force_break;
  int     pos;
} LineBreakerResult;

typedef struct
{
  const u8* str;
  u64       len_str;

  int pos;
  int pos_next;
  
  UBRKCLS brkcls_cur;
  UBRKCLS brkcls_next;

  bool LB8a;
  bool LB21a;
  int  LB30a;
} LineBreaker;

bool LineBreakerTryCreate(const u8* str, u64 len_str, UnicodeBreakClassMapping* ubcm, LineBreaker* o_lb);
LineBreakerResult LineBreakerNextBreak(LineBreaker* lb, UnicodeBreakClassMapping* ubcm);
