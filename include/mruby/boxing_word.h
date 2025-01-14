/**
** @file mruby/boxing_word.h - word boxing mrb_value definition
**
** See Copyright Notice in mruby.h
*/

#ifndef MRUBY_BOXING_WORD_H
#define MRUBY_BOXING_WORD_H

#if defined(MRB_INT16)
# error MRB_INT16 is too small for MRB_WORD_BOXING.
#endif

#if defined(MRB_INT64) && !defined(MRB_64BIT)
#error MRB_INT64 cannot be used with MRB_WORD_BOXING in 32-bit mode.
#endif

#ifndef MRB_WITHOUT_FLOAT
struct RFloat {
  MRB_OBJECT_HEADER;
  mrb_float f;
};
#endif

struct RCptr {
  MRB_OBJECT_HEADER;
  void *p;
};

enum mrb_special_consts {
  MRB_Qnil    =  0,
  MRB_Qfalse  =  4,
  MRB_Qtrue   = 12,
  MRB_Qundef  = 20,
};

#define MRB_FIXNUM_SHIFT    1
#define MRB_SYMBOL_SHIFT    2
#define MRB_FIXNUM_FLAG     (1 << (MRB_FIXNUM_SHIFT - 1))
#define MRB_SYMBOL_FLAG     (1 << (MRB_SYMBOL_SHIFT - 1))
#define MRB_FIXNUM_MASK     ((1 << MRB_FIXNUM_SHIFT) - 1)
#define MRB_SYMBOL_MASK     ((1 << MRB_SYMBOL_SHIFT) - 1)
#define MRB_IMMEDIATE_MASK  0x07

#ifdef MRB_64BIT
#define MRB_SYMBOL_BITSIZE  (sizeof(mrb_sym) * CHAR_BIT)
#define MRB_SYMBOL_MAX      UINT32_MAX
#else
#define MRB_SYMBOL_BITSIZE  (sizeof(mrb_sym) * CHAR_BIT - MRB_SYMBOL_SHIFT)
#define MRB_SYMBOL_MAX      (UINT32_MAX >> MRB_SYMBOL_SHIFT)
#endif

#define BOXWORD_SHIFT_VALUE(o,n,t) \
  ((((t)(o).w)) >> MRB_##n##_SHIFT)
#define BOXWORD_SET_SHIFT_VALUE(o,n,v) \
  ((o).w = (((unsigned long)(v)) << MRB_##n##_SHIFT) | MRB_##n##_FLAG)
#define BOXWORD_SHIFT_VALUE_P(o,n) \
  (((o).w & MRB_##n##_MASK) == MRB_##n##_FLAG)

/*
 * mrb_value representation:
 *
 *   nil   : ...0000 0000 (all bits are zero)
 *   false : ...0000 0100
 *   true  : ...0000 1100
 *   undef : ...0001 0100
 *   fixnum: ...IIII III1
 *   symbol: ...SSSS SS10 (high-order 32-bit are symbol value in 64-bit mode)
 *   object: ...PPPP P000
 */
typedef union mrb_value {
  union {
    void *p;
#ifdef MRB_64BIT
    /* use struct to avoid bit shift. */
    struct {
      MRB_ENDIAN_LOHI(
        mrb_sym sym;
        ,uint32_t sym_flag;
      )
    };
#endif
    struct RBasic *bp;
#ifndef MRB_WITHOUT_FLOAT
    struct RFloat *fp;
#endif
    struct RCptr *vp;
  } value;
  unsigned long w;
} mrb_value;

MRB_API mrb_value mrb_word_boxing_cptr_value(struct mrb_state*, void*);
#ifndef MRB_WITHOUT_FLOAT
MRB_API mrb_value mrb_word_boxing_float_value(struct mrb_state*, mrb_float);
MRB_API mrb_value mrb_word_boxing_float_pool(struct mrb_state*, mrb_float);
#endif

#ifndef MRB_WITHOUT_FLOAT
#define mrb_float_pool(mrb,f) mrb_word_boxing_float_pool(mrb,f)
#endif

#define mrb_ptr(o)     (o).value.p
#define mrb_cptr(o)    (o).value.vp->p
#ifndef MRB_WITHOUT_FLOAT
#define mrb_float(o)   (o).value.fp->f
#endif
#define mrb_fixnum(o)  BOXWORD_SHIFT_VALUE(o, FIXNUM, mrb_int)
#ifdef MRB_64BIT
#define mrb_symbol(o)  (o).value.sym
#else
#define mrb_symbol(o)  BOXWORD_SHIFT_VALUE(o, SYMBOL, mrb_sym)
#endif
#define mrb_bool(o)    (((o).w & ~(unsigned long)MRB_Qfalse) != 0)

#define mrb_immediate_p(o) ((o).w & MRB_IMMEDIATE_MASK || (o).w == MRB_Qnil)
#define mrb_fixnum_p(o) BOXWORD_SHIFT_VALUE_P(o, FIXNUM)
#ifdef MRB_64BIT
#define mrb_symbol_p(o) ((o).value.sym_flag == MRB_SYMBOL_FLAG)
#else
#define mrb_symbol_p(o) BOXWORD_SHIFT_VALUE_P(o, SYMBOL)
#endif
#define mrb_undef_p(o) ((o).w == MRB_Qundef)
#define mrb_nil_p(o)  ((o).w == MRB_Qnil)
#define mrb_false_p(o) ((o).w == MRB_Qfalse)
#define mrb_true_p(o)  ((o).w == MRB_Qtrue)

#ifndef MRB_WITHOUT_FLOAT
#define SET_FLOAT_VALUE(mrb,r,v) ((r) = mrb_word_boxing_float_value(mrb, v))
#endif
#define SET_CPTR_VALUE(mrb,r,v) ((r) = mrb_word_boxing_cptr_value(mrb, v))
#define SET_UNDEF_VALUE(r) ((r).w = MRB_Qundef)
#define SET_NIL_VALUE(r) ((r).w = MRB_Qnil)
#define SET_FALSE_VALUE(r) ((r).w = MRB_Qfalse)
#define SET_TRUE_VALUE(r) ((r).w = MRB_Qtrue)
#define SET_BOOL_VALUE(r,b) ((b) ? SET_TRUE_VALUE(r) : SET_FALSE_VALUE(r))
#define SET_INT_VALUE(r,n) BOXWORD_SET_SHIFT_VALUE(r, FIXNUM, n)
#ifdef MRB_64BIT
#define SET_SYM_VALUE(r,v) ((r).value.sym = v, (r).value.sym_flag = MRB_SYMBOL_FLAG)
#else
#define SET_SYM_VALUE(r,n) BOXWORD_SET_SHIFT_VALUE(r, SYMBOL, n)
#endif
#define SET_OBJ_VALUE(r,v) ((r).value.p = v)

MRB_INLINE enum mrb_vtype
mrb_type(mrb_value o)
{
  return !mrb_bool(o)    ? MRB_TT_FALSE :
         mrb_true_p(o)   ? MRB_TT_TRUE :
         mrb_fixnum_p(o) ? MRB_TT_FIXNUM :
         mrb_symbol_p(o) ? MRB_TT_SYMBOL :
         mrb_undef_p(o)  ? MRB_TT_UNDEF :
         o.value.bp->tt;
}

#endif  /* MRUBY_BOXING_WORD_H */
