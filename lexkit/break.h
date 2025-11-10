#ifndef __LEXKIT_BREAK__
#define __LEXKIT_BREAK__

#include <lexkit/alloc.h>
#include <lexkit/types.h>
#include <lexkit/twostep.h>

// Unicode General Categories

typedef enum
{
  GC_Lu,      // an uppercase letter
  GC_Ll,      // a lowercase letter
  GC_Lt,      // a digraph encoded as a single character, with first part uppercase
  GC_Lm,      // a modifier letter
  GC_Lo,      // other letters, including syllables and ideographs
  GC_Mn,      // a nonspacing combining mark (zero advance width)
  GC_Mc,      // a spacing combining mark (positive advance width)
  GC_Me,      // an enclosing combining mark
  GC_Nd,      // a decimal digit
  GC_Nl,      // a letterlike numeric character
  GC_No,      // a numeric character of other type
  GC_Pc,      // a connecting punctuation mark, like a tie
  GC_Pd,      // a dash or hyphen punctuation mark
  GC_Ps,      // an opening punctuation mark (of a pair)
  GC_Pe,      // a closing punctuation mark (of a pair)
  GC_Pi,      // an initial quotation mark
  GC_Pf,      // a final quotation mark
  GC_Po,      // a punctuation mark of other type
  GC_Sm,      // a symbol of mathematical use
  GC_Sc,      // a currency sign
  GC_Sk,      // a non-letterlike modifier symbol
  GC_So,      // a symbol of other type
  GC_Zs,      // a space character (of various non-zero widths)
  GC_Zl,      // a U+2028 LINE SEPARATOR only
  GC_Zp,      // a U+2029 PARAGRAPH SEPARATOR only
  GC_Cc,      // a C0 or C1 control code
  GC_Cf,      // a format control character
  GC_Cs,      // a surrogate code point
  GC_Co,      // a private-use character
  GC_Cn,      // a reserved unassigned code point or a noncharacter

  GC_Count,
} GC;

// Unicode Line Break Class

typedef enum
{
  LBC_AI,     // Ambiguous (Alphabetic or Ideograph)
  LBC_AK,     // Aksara
  LBC_AL,     // Ordinary Alphabetic and Symbol Characters
  LBC_AP,     // Aksarar Pre-Base
  LBC_AS,     // Aksara Start
  LBC_BA,     // Break After
  LBC_BB,     // Break Before
  LBC_B2,     // Break Oppurtunity Before and After
  LBC_BK,     // Mandatory Break (Non-tailorable)
  LBC_CB,     // Contingent Break Oppurtunity
  LBC_CJ,     // Conditional Japanese Starter
  LBC_CL,     // Close Punctuation
  LBC_CM,     // Combining Mark (Non-tailorable)
  LBC_CP,     // Closing Parenthesis
  LBC_CR,     // Carriage Return (Non-tailorable)
  LBC_EB,     // Emoji Base
  LBC_EM,     // Emoji Modifier
  LBC_EX,     // Exclamation/Interrogation
  LBC_GL,     // Non-breaking ("Glue") (Non-tailorable)
  LBC_H2,     // Hangul LV Symbol
  LBC_H3,     // Hangul LVT Syllable
  LBC_HY,     // Hyphen
  LBC_ID,     // Ideographic
  LBC_HL,     // Hebrew Letter
  LBC_IN,     // Inseparable Character
  LBC_IS,     // Infix Numeric Separator
  LBC_JL,     // Hangul L Jamo
  LBC_JT,     // Hangul T Jamo
  LBC_JV,     // Hangul V Jamo
  LBC_LF,     // Line Feed (Non-tailorable)
  LBC_NL,     // Next Line (Non-tailorable)
  LBC_NS,     // Nonstarters
  LBC_NU,     // Numeric
  LBC_OP,     // Open Punctuation
  LBC_PO,     // Postfix Numeric
  LBC_PR,     // Prefix Numeric
  LBC_QU,     // Quotation
  LBC_RI,     // Regional Indicator
  LBC_SA,     // Complex-Context Dependant (South East Asian)
  LBC_SG,     // Surrogate (Non-tailorable)
  LBC_SP,     // Space (Non-tailorable)
  LBC_SY,     // Symbols Allowing Break After
  LBC_VF,     // Virama Final
  LBC_VI,     // Virama
  LBC_WJ,     // Word Joiner (Non-tailorable)
  LBC_XX,     // Unknown
  LBC_ZW,     // Zero Width Space (Non-tailorable)
  LBC_ZWJ,    // Zero Width Joiner (Non-tailorable)

  LBC_Count,
} LBC;

typedef enum
{
  LBCX_AI,     // Ambiguous (Alphabetic or Ideograph)
  LBCX_AK,     // Aksara
  LBCX_AL,     // Ordinary Alphabetic and Symbol Characters
  LBCX_AP,     // Aksarar Pre-Base
  LBCX_AS,     // Aksara Start
  LBCX_BA,     // Break After
  LBCX_BB,     // Break Before
  LBCX_B2,     // Break Oppurtunity Before and After
  LBCX_BK,     // Mandatory Break (Non-tailorable)
  LBCX_CB,     // Contingent Break Oppurtunity
  LBCX_CJ,     // Conditional Japanese Starter
  LBCX_CL,     // Close Punctuation
  LBCX_CM,     // Combining Mark (Non-tailorable)
  LBCX_CP,     // Closing Parenthesis
  LBCX_CR,     // Carriage Return (Non-tailorable)
  LBCX_EB,     // Emoji Base
  LBCX_EM,     // Emoji Modifier
  LBCX_EX,     // Exclamation/Interrogation
  LBCX_GL,     // Non-breaking ("Glue") (Non-tailorable)
  LBCX_H2,     // Hangul LV Symbol
  LBCX_H3,     // Hangul LVT Syllable
  LBCX_HY,     // Hyphen
  LBCX_ID,     // Ideographic
  LBCX_HL,     // Hebrew Letter
  LBCX_IN,     // Inseparable Character
  LBCX_IS,     // Infix Numeric Separator
  LBCX_JL,     // Hangul L Jamo
  LBCX_JT,     // Hangul T Jamo
  LBCX_JV,     // Hangul V Jamo
  LBCX_LF,     // Line Feed (Non-tailorable)
  LBCX_NL,     // Next Line (Non-tailorable)
  LBCX_NS,     // Nonstarters
  LBCX_NU,     // Numeric
  LBCX_OP,     // Open Punctuation
  LBCX_PO,     // Postfix Numeric
  LBCX_PR,     // Prefix Numeric
  LBCX_QU,     // Quotation
  LBCX_RI,     // Regional Indicator
  LBCX_SA,     // Complex-Context Dependant (South East Asian)
  LBCX_SG,     // Surrogate (Non-tailorable)
  LBCX_SP,     // Space (Non-tailorable)
  LBCX_SY,     // Symbols Allowing Break After
  LBCX_VF,     // Virama Final
  LBCX_VI,     // Virama
  LBCX_WJ,     // Word Joiner (Non-tailorable)
  LBCX_XX,     // Unknown
  LBCX_ZW,     // Zero Width Space (Non-tailorable)
  LBCX_ZWJ,    // Zero Width Joiner (Non-tailorable)

  LBCX_ZW_SP,  // ZW followed by one or more SP
  LBCX_OP_SP,  // OP followed by one or more SP
  LBCX_15a,    // LB15a [\p{Pi}&QU]
  LBCX_15a_SP, // ... followed by one or more SP
  LBCX_CL_CP_SP,// CL/CP followed by one ore more SP
  LBCX_B2_SP,  // B2 followed by one or more SP
  LBCX_19a,
  LBCX_20a,
  LBCX_21a,
  LBCX_NU_SY,
  LBCX_NU_IS,
  LBCX_NU_CL,
  LBCX_NU_CP,
  LBCX_28a,
  LBCX_RI_RI,

  LBCX_Count,
} LBCX;


// Unicode Word Break Class

typedef enum
{
  WBC_CR,
  WBC_LF,
  WBC_Newline,
  WBC_Extend,
  WBC_ZWJ,
  WBC_Regional_Indicator,
  WBC_Format,
  WBC_Katakana,
  WBC_Hebrew_Letter,
  WBC_ALetter,
  WBC_Single_Quote,
  WBC_Double_Quote,
  WBC_MidNumLet,
  WBC_MidLetter,
  WBC_MidNum,
  WBC_Numeric,
  WBC_ExtendNumLet,
  WBC_WSegSpace,
  WBC_XX,

  WBC_Count,
} WBC;

typedef enum
{
  WBCX_CR,
  WBCX_LF,
  WBCX_Newline,
  WBCX_Extend,
  WBCX_ZWJ,
  WBCX_Regional_Indicator,
  WBCX_Format,
  WBCX_Katakana,
  WBCX_Hebrew_Letter,
  WBCX_ALetter,
  WBCX_Single_Quote,
  WBCX_Double_Quote,
  WBCX_MidNumLet,
  WBCX_MidLetter,
  WBCX_MidNum,
  WBCX_Numeric,
  WBCX_ExtendNumLet,
  WBCX_WSegSpace,
  WBCX_XX,

  WBCX_7,
  WBCX_7c,
  WBCX_11,
  WBCX_RI_RI,

  WBCX_Count,
} WBCX;

// Unicode Grapheme Cluster Break Classes

typedef enum
{
  GBC_CR,
  GBC_LF,
  GBC_Control,
  GBC_Extend,
  GBC_ZWJ,
  GBC_Regional_Indicator,
  GBC_Prepend,
  GBC_SpacingMark,
  GBC_L,
  GBC_V,
  GBC_T,
  GBC_LV,
  GBC_LVT,
  GBC_XX,

  GBC_Count,
} GBC;

typedef enum
{
  GBCX_CR,
  GBCX_LF,
  GBCX_Control,
  GBCX_Extend,
  GBCX_ZWJ,
  GBCX_Regional_Indicator,
  GBCX_Prepend,
  GBCX_SpacingMark,
  GBCX_L,
  GBCX_V,
  GBCX_T,
  GBCX_LV,
  GBCX_LVT,
  GBCX_XX,

  GBCX_InCB_Consonant_Extend,
  GBCX_InCB_Consonant_Linker,
  GBCX_InCB_Consonant_Linker_Extend,
  GBCX_EP_Extend,
  GBCX_EP_Extend_ZWJ,
  GBCX_RI_RI,

  GBCX_Count,
} GBCX;

// Unicode East Asian Width

typedef enum
{
  EAW_A,    // Ambiguous
  EAW_F,    // Fullwidth
  EAW_H,    // Halfwidth
  EAW_N,    // Neutral
  EAW_Na,   // Narrow
  EAW_W,    // Wide

  EAW_Count,
} EAW;

// Unicode Indic Conjunct Break

typedef enum
{
  INCB_None,
  INCB_Linker,
  INCB_Consonant,
  INCB_Extend,

  INCB_Count,
} INCB;

// Unicode Derived Bidirectional Class

typedef enum
{
  BIDIC_L,
  BIDIC_R,
  BIDIC_AL,
  BIDIC_EN,
  BIDIC_ES,
  BIDIC_ET,
  BIDIC_AN,
  BIDIC_CS,
  BIDIC_NSM,
  BIDIC_BN,
  BIDIC_B,
  BIDIC_S,
  BIDIC_WS,
  BIDIC_ON,
  BIDIC_LRE,
  BIDIC_LRO,
  BIDIC_RLE,
  BIDIC_RLO,
  BIDIC_PDF,
  BIDIC_LRI,
  BIDIC_RLI,
  BIDIC_FSI,
  BIDIC_PDI,

  BIDIC_Count,
} BIDIC;

// Unicode Bidi_Paired_Bracket_Type property value

typedef enum
{
  BIDIPBT_None,
  BIDIPBT_Open,
  BIDIPBT_Close,

  BIDIPBT_Count,
} BIDIPBT;

typedef struct
{
  i32 idx;
  u32 codepoint;

  LBC   lbc;
  WBC   wbc;
  GBC   gbc;
  GC    gc;
  EAW   eaw;
  INCB  incb;
  bool  extended_pictographic;
} Glyph;

typedef struct
{
  u32*  lb_range_start;
  u32*  lb_range_end;
  LBC*  lb_range_cls; 
  i32   lb_range_count;

  u32*  wb_range_start;
  u32*  wb_range_end;
  WBC*  wb_range_cls;
  i32   wb_range_count;

  u32*  gb_range_start;
  u32*  gb_range_end;
  GBC*  gb_range_cls;
  i32   gb_range_count;

  u32*  gc_codepoint;
  GC*   gc_cls;
  i32   gc_count;

  u32*  eaw_range_start;
  u32*  eaw_range_end;
  EAW*  eaw_range_cls;
  i32   eaw_range_count;

  u32*  incb_range_start;
  u32*  incb_range_end;
  INCB* incb_range_cls;
  i32   incb_range_count;

  u32*  ep_range_start;
  u32*  ep_range_end;
  i32   ep_range_count;

  u32*  bidi_range_start;
  u32*  bidi_range_end;
  BIDIC* bidi_range_cls;
  i32   bidi_range_count;

  u32*  bidipb_key;
  u32*  bidipb_value;
  BIDIPBT* bidipbt;
  u32   bidipb_count;
} LkUnicodeData;

typedef struct
{
  LkTwoStep* ts_lb;
  LkTwoStep* ts_wb;
  LkTwoStep* ts_gb;
  LkTwoStep* ts_gc;
  LkTwoStep* ts_eaw;
  LkTwoStep* ts_bidi;
} LkUnicodeDataTwoStep;

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
    LkUnicodeData* o_ud);

bool lkTryLoadUnicodeDataFromSpecTwoStep(
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
    LkUnicodeData* o_ud,
    LkUnicodeDataTwoStep* o_udts);

Glyph GetGlyphAtIndex(const u32* codepoints, i32 len_codepoints, i32 idx, LkUnicodeData* ud);
Glyph GetGlyphAtIndexTwoStep(const u32* codepoints, i32 len_codepoints, i32 idx, LkUnicodeData* ud, LkUnicodeDataTwoStep* udts);

typedef struct
{
  const u32*  codepoints;
  i32         len_codepoints;
  i32         idx;

  LBCX        lbcx;
  LBCX        lbcx_adj;
  WBCX        wbcx;
  WBCX        wbcx_adj;
  GBCX        gbcx;
  GC          gc_adj;
  EAW         eaw_adj;
  INCB        incb;
  bool        ep;
  bool        ep_adj;
} Breaker;

typedef enum
{
  LBRK_PRO,   // Prohibited break
  LBRK_OPT,   // Break allowed
  LBRK_MAN,   // Mandatory break
} LBRK;

typedef enum
{
  WBRK_BRK,   // Break allowed
  WBRK_PRO,   // Prohibited break
} WBRK;

typedef enum
{
  GBRK_BRK,   // Break allowed
  GBRK_PRO,   // Prohibited break
} GBRK;

typedef struct
{
  i32   glyph_idx;  // TODO: This should be codepoint_idx
  bool  done;
  LBRK  lbrk;
  WBRK  wbrk;
  GBRK  gbrk;
} BreakerResult;

extern const char* g_map_lbc_str[];
extern const char* g_map_wbc_str[];
extern const char* g_map_gbc_str[];
extern const char* g_map_gc_str[];
extern const char* g_map_eaw_str[];
extern const char* g_map_bidic_str[];

struct LkText;

void BreakerCreate(const u32* codepoints, i32 len_codepoints, Breaker* o_brk);
BreakerResult BreakerAdvance(Breaker* brk, LkUnicodeData* ud, LkUnicodeDataTwoStep* udts);
BreakerResult* lkGetBreaks(LkArena* arena, const struct LkText* text, LkUnicodeData* ud, LkUnicodeDataTwoStep* udts);

#endif
