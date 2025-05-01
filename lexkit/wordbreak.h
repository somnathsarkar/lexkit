#include <lexkit/types.h>

typedef enum
{
  UWBRKCLS_CR,                   // Carriage Return
  UWBRKCLS_LF,                   // Line Feed
  UWBRKCLS_Newline,
  UWBRKCLS_Extend,
  UWBRKCLS_ZWJ,                  // Zero Width Joiner
  UWBRKCLS_Regional_Indicator,
  UWBRKCLS_Format,
  UWBRKCLS_Katakana,
  UWBRKCLS_Hebrew_Letter,
  UWBRKCLS_ALetter,
  UWBRKCLS_Single_Quote,
  UWBRKCLS_Double_Quote,
  UWBRKCLS_MidNumLet,
  UWBRKCLS_MidLetter,
  UWBRKCLS_MidNum,
  UWBRKCLS_Numeric,
  UWBRKCLS_ExtendNumLet,
  UWBRKCLS_E_Base,               // Obsolete
  UWBRKCLS_E_Modifier,           // Obsolete
  UWBRKCLS_Glue_After_Zwj,       // Obsolete
  UWBRKCLS_E_BASE_GAZ,           // Obsolete
  UWBRKCLS_WSegSpace,
  UWBRKCLS_Any,                  // All unicode glyphs without an assigned class

  UWBRKCLS_Count
} UWBRKCLS;

typedef struct
{
  int*          range_start;
  int*          range_end;
  UWBRKCLS*     range_wbrkcls;
  int           num_class;
} UnicodeWordBreakClassMapping;

void UnicodeWordBreakClassMappingLoadFromPath(
    const char* str_path,
    UnicodeWordBreakClassMapping* uwbcm);

typedef struct
{
  int*          range_start;
  int*          range_end;
  int           num_range;
} UnicodeExtendedPictographMapping;

typedef enum
{
  WBRKRESK_Invalid    = 0,
  WBRKRESK_Found      = 1,
  WBRKRESK_Complete   = 2,
  WBRKRESK_ParseError = 3,
} WBRKRESK;

typedef struct
{
  WBRKRESK  kind;
  int       glyph;
} WordBreakerResult;

typedef struct
{
  i32       idx; 
  i32       pos;
  i32       advance;
  u32       codepoint;
  UWBRKCLS  wbrkcls;
  bool      extended_pictograph;
} WordBreakGlyph;

typedef struct
{
  const u8*     str;
  u64           len_str;
  int           num_glyph;

  WordBreakGlyph      wbrkcls_cur; 
  WordBreakGlyph      wbrkcls_cur_raw;
  WordBreakGlyph      wbrkcls_next;   
  WordBreakGlyph      wbrkcls_next2;   
  WordBreakGlyph      wbrkcls_prev;     
} WordBreaker;

bool WordBreakerTryCreate(const u8* str, u64 len_str, int num_glyph, UnicodeWordBreakClassMapping* uwbcm, WordBreaker* o_wb);
WordBreakerResult WordBreakerNextBreak(WordBreaker* wb, UnicodeWordBreakClassMapping* uwbcm);
