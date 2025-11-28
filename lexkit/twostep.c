#include <lexkit/twostep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define S_MAX_LINE 256
#define BLOCK_SIZE 256

static bool IsHexChar(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

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

static i32 CountImportantLinesC(FILE* fp)
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

static i32 CountImportantLinesD(FILE* fp)
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

typedef struct
{
  u32 start;
  u32 end;
  i32 cls;
} LkTwoStepSection;

int CompareTwoStepSection(void* ctx, const void* a, const void* b)
{
  const LkTwoStepSection* ta = (const LkTwoStepSection*)a;
  const LkTwoStepSection* tb = (const LkTwoStepSection*)b;

  if (ta->start != tb->start)
    return ta->start - tb->start;
  return ta->end - tb->end;
}

static LkTwoStepSection* LkTwoStepSectionParseA(LkArena* arena, const char* filepath, const char* map_enum_str[], u64 enum_max, i32 enum_default, int* o_section_count)
{
  assert(o_section_count != NULL && *o_section_count == 0);
  char buf[S_MAX_LINE];
  char buf_cls[S_MAX_LINE];
  FILE* fp = NULL;
  errno_t err = fopen_s(&fp, filepath, "r");
  if (err)
    return NULL;
  int section_count = CountImportantLines(fp);
  LkTwoStepSection* sections = APushArray(arena, LkTwoStepSection, section_count);
  rewind(fp);
  i32 range_i = 0;
  while (fgets(buf, S_MAX_LINE, fp))
  {
    if (!IsHexChar(buf[0]))
      continue;

    u32 range_start = 0;
    u32 range_end = 0;

    int three_parse = sscanf_s(buf, "%x..%x ; %s", &range_start, &range_end, buf_cls, S_MAX_LINE);
    if (three_parse < 3)
    {
      int two_parse = sscanf_s(buf, "%x ; %s", &range_start, buf_cls, S_MAX_LINE);
      if (two_parse < 2)
        return NULL;

      sections[range_i].start = range_start;
      sections[range_i].end = range_start;
    }
    else
    {
      sections[range_i].start = range_start;
      sections[range_i].end = range_end;
    }

    sections[range_i].cls = enum_default;
    for (i32 cls_i = 0; cls_i < enum_max; cls_i++)
    {
      if (strncmp(map_enum_str[cls_i], buf_cls, strnlen_s(map_enum_str[cls_i], S_MAX_LINE)) == 0)
      {
        sections[range_i].cls = cls_i;
        break;
      }
    }
    range_i++;
  }
  fclose(fp);
  *o_section_count = section_count;
  return sections;
}

static LkTwoStepSection* LkTwoStepSectionParseB(LkArena* arena, const char* filepath, const char* map_enum_str[], u64 enum_max, i32 enum_default, int *o_section_count)
{
  assert(o_section_count != NULL && *o_section_count == 0);
  char buf[S_MAX_LINE];
  char buf_tmp[S_MAX_LINE];
  char buf_cls[S_MAX_LINE];
  FILE* fp = NULL;
  errno_t err = fopen_s(&fp, filepath, "r");
  if (err)
    return NULL;
  int section_count = CountImportantLines(fp);
  LkTwoStepSection* sections = APushArray(arena, LkTwoStepSection, section_count);
  rewind(fp);
  i32 range_i = 0;
  while (fgets(buf, S_MAX_LINE, fp))
  {
    u32 cp = 0;
    i32 three_parse = sscanf_s(buf, "%x;%[^;];%[^;]", &cp, buf_tmp, S_MAX_LINE, buf_cls, S_MAX_LINE);
    if (three_parse < 3)
      return false;
    sections[range_i].start = cp;
    sections[range_i].end = cp;
    sections[range_i].cls = enum_default;
    for (int i_cls = 0; i_cls < enum_max; i_cls++)
    {
      if(strncmp(map_enum_str[i_cls], buf_cls, strnlen_s(map_enum_str[i_cls], S_MAX_LINE)) == 0)
      {
        sections[range_i].cls = i_cls;
        break;
      }
    }
    range_i++;
  }
  fclose(fp);
  *o_section_count = section_count;
  return sections;
}

static LkTwoStepSection* LkTwoStepSectionParseC(LkArena* arena, const char* filepath, const char* map_enum_str[], u64 enum_max, i32 enum_default, int *o_section_count)
{
  assert(o_section_count != NULL && *o_section_count == 0);
  char buf[S_MAX_LINE];
  char buf_tmp[S_MAX_LINE];
  char buf_cls[S_MAX_LINE];
  FILE* fp = NULL;
  errno_t err = fopen_s(&fp, filepath, "r");
  if (err)
    return NULL;
  int section_count = CountImportantLinesC(fp);
  LkTwoStepSection* sections = APushArray(arena, LkTwoStepSection, section_count);
  rewind(fp);
  int range_i = 0;
  while (fgets(buf, S_MAX_LINE, fp))
  {
    if (!IsHexChar(buf[0]) || strstr(buf, "InCB") == NULL)
      continue;

    u32 range_start = 0;
    u32 range_end = 0;

    int four_parse = sscanf_s(buf, "%x..%x ; %[^;] ; %s", &range_start, &range_end, buf_tmp, S_MAX_LINE, buf_cls, S_MAX_LINE);
    if (four_parse < 4)
    {
      int three_parse = sscanf_s(buf, "%x ; %[^;] ; %s", &range_start, buf_tmp, S_MAX_LINE, &buf_cls, S_MAX_LINE);
      if (three_parse < 3)
        return false;
      sections[range_i].start = range_start;
      sections[range_i].end   = range_start;
    }
    else
    {
      sections[range_i].start = range_start;
      sections[range_i].end   = range_end;
    }
    sections[range_i].cls = enum_default;
    for (int i_cls = 0; i_cls < enum_max; i_cls++)
    {
      if (strncmp(map_enum_str[i_cls], buf_cls, strnlen_s(map_enum_str[i_cls], S_MAX_LINE)) == 0)
      {
        sections[range_i].cls = i_cls;
        break;
      }
    }
    range_i++;
  }
  fclose(fp);
  *o_section_count = section_count;
  return sections;
}

static LkTwoStepSection* LkTwoStepSectionParseD(LkArena* arena, const char* filepath, const char* map_enum_str[], u64 enum_max, i32 enum_default, int *o_section_count)
{
  // Extended_Pictographic is boolean flag, so always set cls to 1

  assert(o_section_count != NULL && *o_section_count == 0);
  char buf[S_MAX_LINE];
  char buf_cls[S_MAX_LINE];
  FILE* fp = NULL;
  errno_t err = fopen_s(&fp, filepath, "r");
  if (err)
    return false;
  int section_count = CountImportantLinesD(fp);
  LkTwoStepSection* sections = APushArray(arena, LkTwoStepSection, section_count);
  rewind(fp);
  int range_i = 0;
  while (fgets(buf, S_MAX_LINE, fp))
  {
    if (!IsHexChar(buf[0]) || strstr(buf, "Extended_Pictographic") == NULL)
      continue;

    u32 range_start = 0;
    u32 range_end = 0;

    int two_parse = sscanf_s(buf, "%x..%x", &range_start, &range_end, buf_cls, S_MAX_LINE);
    if (two_parse < 2)
    {
      int one_parse = sscanf_s(buf, "%x", &range_start, &buf_cls, S_MAX_LINE);
      if (one_parse < 1)
        return false;
      sections[range_i].start = range_start;
      sections[range_i].end   = range_start;
    }
    else
    {
      sections[range_i].start = range_start;
      sections[range_i].end   = range_end;
    }
    sections[range_i].cls = 1;
    range_i++;
  }
  fclose(fp);
  *o_section_count = section_count;
  return sections;
}

LkTwoStep* LkTwoStepCreate(LkArena* arena, const char* filepath, const char* map_enum_str[], u64 enum_max, i32 enum_default, UNIFMT unifmt)
{
  u64 scratch_pos = lkArenaGetPos(arena->alt);
  LkTwoStepSection* sections = NULL;
  int section_count = 0;
  switch (unifmt)
  {
    case UNIFMT_A:
    {
      sections = LkTwoStepSectionParseA(arena->alt, filepath, map_enum_str, enum_max, enum_default, &section_count);
      break;
    }

    case UNIFMT_B:
    {
      sections = LkTwoStepSectionParseB(arena->alt, filepath, map_enum_str, enum_max, enum_default, &section_count);
      break;
    }

    case UNIFMT_C:
    {
      sections = LkTwoStepSectionParseC(arena->alt, filepath, map_enum_str, enum_max, enum_default, &section_count);
      break;
    }

    case UNIFMT_D:
    {
      sections = LkTwoStepSectionParseD(arena->alt, filepath, map_enum_str, enum_max, enum_default, &section_count);
      break;
    }

    default:
      assert(false);
  }
  qsort_s(sections, section_count, sizeof(LkTwoStepSection), CompareTwoStepSection, NULL);
  u32 max_codepoint = sections[section_count - 1].end + 1;
  u32 block_count = (max_codepoint + BLOCK_SIZE - 1) / BLOCK_SIZE;
  i32* block_list = APushArray(arena->alt, i32, block_count * BLOCK_SIZE);
  i32* reduced_block_list = APushArray(arena->alt, i32, block_count * BLOCK_SIZE);
  i32 reduced_block_count = 0;
  for (int i = 0; i < block_count * BLOCK_SIZE; i++)
  {
    block_list[i] = enum_default;
  }
  for (int i = 0; i < section_count; i++)
  {
    for(int j = sections[i].start; j <= sections[i].end; j++)
    {
      block_list[j] = sections[i].cls;
    }
  }

  LkTwoStep* ts = APush(arena, LkTwoStep);
  ts->block0 = APushArray(arena, i32, block_count);
  ts->block0_len = block_count;
  for (int i = 0; i < block_count; i++)
  {
    int found_block = -1;
    for (int j = 0; j < reduced_block_count; j++)
    {
      bool failed = false;
      for (int k = 0; k < BLOCK_SIZE; k++)
      {
        int p = i * BLOCK_SIZE + k;
        int q = j * BLOCK_SIZE + k;
        if (block_list[p] != reduced_block_list[q])
        {
          failed = true;
          break;
        }
      }
      if (!failed)
        found_block = j;
    }
    if (found_block == -1)
    {
      for (int k = 0; k < BLOCK_SIZE; k++)
      {
        int p = i * BLOCK_SIZE + k;
        int q = reduced_block_count * BLOCK_SIZE + k;
        reduced_block_list[q] = block_list[p];
      }
      found_block = reduced_block_count;
      reduced_block_count++;
    }
    ts->block0[i] = found_block;
  }
  ts->block1 = APushArray(arena, i32, reduced_block_count * BLOCK_SIZE);
  ts->block1_len = reduced_block_count * BLOCK_SIZE;
  for (int i = 0; i < reduced_block_count * BLOCK_SIZE; i++)
    ts->block1[i] = reduced_block_list[i];
  lkArenaRestore(arena->alt, scratch_pos);
  return ts;
}

i32 LkTwoStepLookup(LkTwoStep* ts, u32 ch)
{
  u32 block0_i = ch / BLOCK_SIZE;
  assert(block0_i < ts->block0_len);
  u32 block1_i = ts->block0[block0_i] * BLOCK_SIZE + (ch % BLOCK_SIZE);
  assert(block1_i < ts->block1_len);
  return ts->block1[block1_i];
}

