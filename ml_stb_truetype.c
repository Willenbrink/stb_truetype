#include <assert.h>
#include <stdio.h>
#include <caml/mlvalues.h>
#include <caml/fail.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/bigarray.h>
#include <caml/custom.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

CAMLprim value ml_stbtt_GetFontOffsetForIndex(value ba, value vindex)
{
  CAMLparam2(ba, vindex);
  unsigned char *data = Caml_ba_data_val(ba);
  int index = Int_val(vindex);
  int result = stbtt_GetFontOffsetForIndex(data, index);
  CAMLreturn(Val_int(result));
}

static struct custom_operations fontinfo_custom_ops = {
    identifier: "stbtt_fontinfo",
    finalize:    custom_finalize_default,
    compare:     custom_compare_default,
    hash:        custom_hash_default,
    serialize:   custom_serialize_default,
    deserialize: custom_deserialize_default
};

#define Fontinfo_val Data_custom_val

CAMLprim value ml_stbtt_InitFont(value ba, value voffset)
{
  CAMLparam2(ba, voffset);
  CAMLlocal2(ret, fontinfo);

  unsigned char *data = Caml_ba_data_val(ba);
  int index = Int_val(voffset);

  fontinfo = caml_alloc_custom(&fontinfo_custom_ops, sizeof(stbtt_fontinfo), 0, 1);
  int result = stbtt_InitFont(Fontinfo_val(fontinfo), data, index);

  if (result == 0)
    ret = Val_unit;
  else
  {
    ret = caml_alloc(1, 0);
    Store_field(ret, 0, fontinfo);
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_FindGlyphIndex(value fontinfo, value codepoint)
{
  CAMLparam2(fontinfo, codepoint);
  CAMLlocal1(ret);

  int index = stbtt_FindGlyphIndex(Fontinfo_val(fontinfo), Int_val(codepoint));
  CAMLreturn(Val_int(index));
}

CAMLprim value ml_stbtt_ScaleForPixelHeight(value fontinfo, value height)
{
  CAMLparam2(fontinfo, height);
  CAMLlocal1(ret);

  float scale = stbtt_ScaleForPixelHeight(Fontinfo_val(fontinfo), Double_val(height));
  ret = caml_copy_double(scale);

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_ScaleForMappingEmToPixels(value fontinfo, value height)
{
  CAMLparam2(fontinfo, height);
  CAMLlocal1(ret);

  float scale = stbtt_ScaleForMappingEmToPixels(Fontinfo_val(fontinfo), Double_val(height));
  ret = caml_copy_double(scale);

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetFontVMetrics(value fontinfo)
{
  CAMLparam1(fontinfo);
  CAMLlocal1(ret);

  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(Fontinfo_val(fontinfo), &ascent, &descent, &line_gap);

  ret = caml_alloc(3, 0);
  Store_field(ret, 0, Val_int(ascent));
  Store_field(ret, 1, Val_int(descent));
  Store_field(ret, 2, Val_int(line_gap));

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetGlyphHMetrics(value fontinfo, value glyph)
{
  CAMLparam2(fontinfo, glyph);
  CAMLlocal1(ret);

  int adv, lsb;
  stbtt_GetGlyphHMetrics(Fontinfo_val(fontinfo), Int_val(glyph), &adv, &lsb);

  ret = caml_alloc(2, 0);
  Store_field(ret, 0, Val_int(adv));
  Store_field(ret, 1, Val_int(lsb));

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetGlyphKernAdvance(value fontinfo, value glyph1, value glyph2)
{
  CAMLparam3(fontinfo, glyph1, glyph2);
  CAMLlocal1(ret);

  int adv = stbtt_GetGlyphKernAdvance(Fontinfo_val(fontinfo), Int_val(glyph1), Int_val(glyph2));

  ret = Val_int(adv);

  CAMLreturn(ret);
}

static value box(int x0, int y0, int x1, int y1)
{
  CAMLparam0();
  CAMLlocal1(ret);

  ret = caml_alloc(4, 0);
  Store_field(ret, 0, x0);
  Store_field(ret, 1, y0);
  Store_field(ret, 2, x1);
  Store_field(ret, 3, y1);

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_GetFontBoundingBox(value fontinfo)
{
  CAMLparam1(fontinfo);

  int x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(Fontinfo_val(fontinfo), &x0, &y0, &x1, &y1);

  CAMLreturn(box(x0, y0, x1, y1));
}

CAMLprim value ml_stbtt_GetGlyphBox(value fontinfo, value glyph)
{
  CAMLparam2(fontinfo, glyph);

  int x0, y0, x1, y1;
  stbtt_GetGlyphBox(Fontinfo_val(fontinfo), Int_val(glyph), &x0, &y0, &x1, &y1);

  CAMLreturn(box(x0, y0, x1, y1));
}

// Bitmap packer
#define Pack_context_val Data_custom_val

static void pack_context_finalize(value v)
{
  CAMLparam1(v);
  stbtt_PackEnd(Pack_context_val(v));
  CAMLreturn0;
}

static struct custom_operations pack_context_custom_ops = {
    identifier: "stbtt_pack_context",
    finalize:    pack_context_finalize,
    compare:     custom_compare_default,
    hash:        custom_hash_default,
    serialize:   custom_serialize_default,
    deserialize: custom_deserialize_default
};

CAMLprim value ml_stbtt_PackBegin(value buffer, value w, value h, value s, value p)
{
  CAMLparam5(buffer, w, h, s, p);
  CAMLlocal2(ret, pack_context);

  unsigned char *data = Caml_ba_data_val(buffer);
  int width = Int_val(w), height = Int_val(h), stride = Int_val(s),
      padding = Int_val(p);

  pack_context = caml_alloc_custom(&pack_context_custom_ops, sizeof(stbtt_pack_context), 0, 1);
  int result = stbtt_PackBegin(Pack_context_val(pack_context), data, width, height, stride, padding, NULL);

  if (result == 0)
    ret = Val_unit;
  else
  {
    ret = caml_alloc(1, 0);
    Store_field(ret, 0, pack_context);
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_PackSetOversampling(value ctx, value h, value v)
{
  CAMLparam3(ctx, h, v);
  stbtt_PackSetOversampling(Pack_context_val(ctx), Int_val(h), Int_val(v));
  CAMLreturn(Val_unit);
}

typedef struct {
  int count;
  stbtt_packedchar chars[1];
} ml_stbtt_packed_chars;

static struct custom_operations packed_chars_custom_ops = {
    identifier: "ml_stbtt_packed_chars",
    finalize:    custom_finalize_default,
    compare:     custom_compare_default,
    hash:        custom_hash_default,
    serialize:   custom_serialize_default,
    deserialize: custom_deserialize_default
};

CAMLprim value ml_stbtt_packed_chars_count(value packed_chars)
{
  CAMLparam1(packed_chars);
  ml_stbtt_packed_chars *data = Data_custom_val(packed_chars);
  CAMLreturn(Val_int(data->count));
}

CAMLprim value ml_stbtt_packed_chars_box(value packed_chars, value index)
{
  CAMLparam2(packed_chars, index);
  CAMLlocal1(ret);

  ml_stbtt_packed_chars *data = Data_custom_val(packed_chars);
  unsigned int idx = Int_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_box");
  else
  {
    stbtt_packedchar *pack = &data->chars[idx];
    ret = box(pack->x0, pack->y0, pack->x1, pack->y1);
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_packed_chars_metrics(value packed_chars, value index)
{
  CAMLparam2(packed_chars, index);
  CAMLlocal1(ret);

  ml_stbtt_packed_chars *data = Data_custom_val(packed_chars);
  unsigned int idx = Int_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_metrics");
  else
  {
    stbtt_packedchar *pack = &data->chars[idx];

    ret = caml_alloc(5 * Double_wosize, Double_array_tag);
    Store_double_field(ret, 0, pack->xoff);
    Store_double_field(ret, 1, pack->yoff);
    Store_double_field(ret, 2, pack->xadvance);
    Store_double_field(ret, 3, pack->xoff2);
    Store_double_field(ret, 4, pack->yoff2);
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_packed_chars_quad(value packed_chars, value index, value bw, value bh, value sx, value sy, value int_align)
{
  CAMLparam5(packed_chars, index, bw, bh, sx);
  CAMLxparam1(sy);
  CAMLlocal3(sx2, quad, ret);

  ml_stbtt_packed_chars *data = Data_custom_val(packed_chars);
  stbtt_aligned_quad q;
  unsigned int idx = Int_val(index);

  if (idx >= data->count)
    caml_invalid_argument("Stb_truetype.packed_chars_quad");
  else
  {
    float xpos = Double_val(sx), ypos = Double_val(sy);
    stbtt_GetPackedQuad(&data->chars[0], Int_val(bw), Int_val(bh), idx, &xpos, &ypos, &q, Int_val(int_align));

    quad = caml_alloc(8 * Double_wosize, Double_array_tag);
    Store_double_field(quad, 0, q.x0);
    Store_double_field(quad, 1, q.y0);
    Store_double_field(quad, 2, q.s0);
    Store_double_field(quad, 3, q.t0);
    Store_double_field(quad, 4, q.x1);
    Store_double_field(quad, 5, q.y1);
    Store_double_field(quad, 6, q.s1);
    Store_double_field(quad, 7, q.t1);

    sx2 = caml_copy_double(xpos);

    ret = caml_alloc(2, 0);
    Store_field(ret, 0, sx2);
    Store_field(ret, 1, quad);
  }

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_packed_chars_quad_bc(value *argv, int argn)
{
  return ml_stbtt_packed_chars_quad(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
}

static float font_range_font_size(value font_range)
{
  value sz = Field(font_range, 0);
  if (Tag_val(sz) == 0)
    return Double_val(Field(sz, 0));
  else
    return STBTT_POINT_SIZE(Double_val(Field(sz, 0)));
}

static value packed_chars_alloc(stbtt_pack_range* range)
{
  CAMLparam0();
  CAMLlocal1(ret);
  int count = range->num_chars_in_range;
  int size = sizeof(ml_stbtt_packed_chars) + sizeof(stbtt_packedchar) * (count - 1);

  ret = caml_alloc_custom(&packed_chars_custom_ops, size, 0, 1);
  ml_stbtt_packed_chars *data = Data_custom_val(ret);
  data->count = count;
  range->chardata_for_range = &data->chars[0];

  CAMLreturn(ret);
}

CAMLprim value ml_stbtt_pack_font_ranges(value pack_context, value font_info, value font_ranges)
{
  CAMLparam3(pack_context, font_info, font_ranges);
  CAMLlocal3(font_range, packed_ranges, ret);

  int num_ranges = Wosize_val(font_ranges), i;
  stbtt_pack_range *ranges = alloca(sizeof (stbtt_pack_range) * num_ranges);

  packed_ranges = caml_alloc(num_ranges, 0);
  for (i = 0; i < num_ranges; ++i)
  {
    font_range = Field(font_ranges, i);
    // Validate font_range input?
    ranges[i].font_size = font_range_font_size(font_range);
    ranges[i].first_unicode_char_in_range = Int_val(Field(font_range, 1));
    ranges[i].num_chars_in_range = Int_val(Field(font_range, 2));
    ranges[i].chardata_for_range = NULL;

    Store_field(packed_ranges, i, packed_chars_alloc(&ranges[i]));
  }

  int result = stbtt_PackFontInfoRanges(Pack_context_val(pack_context), Fontinfo_val(font_info), ranges, num_ranges);

  ret = caml_alloc(2, 0);
  Store_field(ret, 0, Val_int(result != 0));
  Store_field(ret, 1, packed_ranges);

  CAMLreturn(ret);
}