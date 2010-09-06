
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <orc/orc.h>
#include <orc/orcprogram.h>
#include <orc/orcdebug.h>

static const char *c_get_type_name (int size);
static void c_get_name_int (char *name, OrcCompiler *p, OrcInstruction *insn, int var);

void orc_c_init (void);

void
orc_compiler_c_init (OrcCompiler *compiler)
{
  int i;

  for(i=ORC_GP_REG_BASE;i<ORC_GP_REG_BASE+32;i++){
    compiler->valid_regs[i] = 1;
  }
  compiler->loop_shift = 0;
}

const char *
orc_target_c_get_typedefs (void)
{
  return
    "#ifndef _ORC_INTEGER_TYPEDEFS_\n"
    "#define _ORC_INTEGER_TYPEDEFS_\n"
    "#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L\n"
    "#include <stdint.h>\n"
    "typedef int8_t orc_int8;\n"
    "typedef int16_t orc_int16;\n"
    "typedef int32_t orc_int32;\n"
    "typedef int64_t orc_int64;\n"
    "typedef uint8_t orc_uint8;\n"
    "typedef uint16_t orc_uint16;\n"
    "typedef uint32_t orc_uint32;\n"
    "typedef uint64_t orc_uint64;\n"
    "#elif defined(_MSC_VER)\n"
    "typedef signed __int8 orc_int8;\n"
    "typedef signed __int16 orc_int16;\n"
    "typedef signed __int32 orc_int32;\n"
    "typedef signed __int64 orc_int64;\n"
    "typedef unsigned __int8 orc_uint8;\n"
    "typedef unsigned __int16 orc_uint16;\n"
    "typedef unsigned __int32 orc_uint32;\n"
    "typedef unsigned __int64 orc_uint64;\n"
    "#else\n"
    "#include <limits.h>\n"
    "typedef signed char orc_int8;\n"
    "typedef short orc_int16;\n"
    "typedef int orc_int32;\n"
    "typedef unsigned char orc_uint8;\n"
    "typedef unsigned short orc_uint16;\n"
    "typedef unsigned int orc_uint32;\n"
    "#if INT_MAX == LONG_MAX\n"
    "typedef long long orc_int64;\n"
    "typedef unsigned long long orc_uint64;\n"
    "#else\n"
    "typedef long orc_int64;\n"
    "typedef unsigned long orc_uint64;\n"
    "#endif\n"
    "#endif\n"
    "typedef union { orc_int16 i; orc_int8 x2[2]; } orc_union16;\n"
    "typedef union { orc_int32 i; float f; orc_int16 x2[2]; orc_int8 x4[4]; } orc_union32;\n"
    "typedef union { orc_int64 i; double f; orc_int32 x2[2]; orc_int16 x4[4]; } orc_union64;\n"
    "#endif\n";
}

const char *
orc_target_c_get_asm_preamble (void)
{
  return "\n"
    "/* begin Orc C target preamble */\n"
    "#define ORC_CLAMP(x,a,b) ((x)<(a) ? (a) : ((x)>(b) ? (b) : (x)))\n"
    "#define ORC_ABS(a) ((a)<0 ? -(a) : (a))\n"
    "#define ORC_MIN(a,b) ((a)<(b) ? (a) : (b))\n"
    "#define ORC_MAX(a,b) ((a)>(b) ? (a) : (b))\n"
    "#define ORC_SB_MAX 127\n"
    "#define ORC_SB_MIN (-1-ORC_SB_MAX)\n"
    "#define ORC_UB_MAX 255\n"
    "#define ORC_UB_MIN 0\n"
    "#define ORC_SW_MAX 32767\n"
    "#define ORC_SW_MIN (-1-ORC_SW_MAX)\n"
    "#define ORC_UW_MAX 65535\n"
    "#define ORC_UW_MIN 0\n"
    "#define ORC_SL_MAX 2147483647\n"
    "#define ORC_SL_MIN (-1-ORC_SL_MAX)\n"
    "#define ORC_UL_MAX 4294967295U\n"
    "#define ORC_UL_MIN 0\n"
    "#define ORC_CLAMP_SB(x) ORC_CLAMP(x,ORC_SB_MIN,ORC_SB_MAX)\n"
    "#define ORC_CLAMP_UB(x) ORC_CLAMP(x,ORC_UB_MIN,ORC_UB_MAX)\n"
    "#define ORC_CLAMP_SW(x) ORC_CLAMP(x,ORC_SW_MIN,ORC_SW_MAX)\n"
    "#define ORC_CLAMP_UW(x) ORC_CLAMP(x,ORC_UW_MIN,ORC_UW_MAX)\n"
    "#define ORC_CLAMP_SL(x) ORC_CLAMP(x,ORC_SL_MIN,ORC_SL_MAX)\n"
    "#define ORC_CLAMP_UL(x) ORC_CLAMP(x,ORC_UL_MIN,ORC_UL_MAX)\n"
    "#define ORC_SWAP_W(x) ((((x)&0xff)<<8) | (((x)&0xff00)>>8))\n"
    "#define ORC_SWAP_L(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))\n"
    "#define ORC_SWAP_Q(x) ((((x)&0xffULL)<<56) | (((x)&0xff00ULL)<<40) | (((x)&0xff0000ULL)<<24) | (((x)&0xff000000ULL)<<8) | (((x)&0xff00000000ULL)>>8) | (((x)&0xff0000000000ULL)>>24) | (((x)&0xff000000000000ULL)>>40) | (((x)&0xff00000000000000ULL)>>56))\n"
    "#define ORC_PTR_OFFSET(ptr,offset) ((void *)(((unsigned char *)(ptr)) + (offset)))\n"
    "#define ORC_DENORMAL(x) ((x) & ((((x)&0x7f800000) == 0) ? 0xff800000 : 0xffffffff))\n"
    "#define ORC_ISNAN(x) ((((x)&0x7f800000) == 0x7f800000) && (((x)&0x007fffff) != 0))\n"
    "#define ORC_DENORMAL_DOUBLE(x) ((x) & ((((x)&0x7ff0000000000000ULL) == 0) ? 0xfff0000000000000ULL : 0xffffffffffffffffULL))\n"
    "#define ORC_ISNAN_DOUBLE(x) ((((x)&0x7ff0000000000000ULL) == 0x7ff0000000000000ULL) && (((x)&0x000fffffffffffffULL) != 0))\n"
    "#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L\n"
    "#define ORC_RESTRICT restrict\n"
    "#elif defined(__GNUC__) && __GNUC__ >= 4\n"
    "#define ORC_RESTRICT __restrict__\n"
    "#else\n"
    "#define ORC_RESTRICT\n"
    "#endif\n"
    "/* end Orc C target preamble */\n\n";
}

unsigned int
orc_compiler_c_get_default_flags (void)
{
  return 0;
}

static const char *varnames[] = {
  "d1", "d2", "d3", "d4",
  "s1", "s2", "s3", "s4",
  "s5", "s6", "s7", "s8",
  "a1", "a2", "a3", "d4",
  "c1", "c2", "c3", "c4",
  "c5", "c6", "c7", "c8",
  "p1", "p2", "p3", "p4",
  "p5", "p6", "p7", "p8",
  "t1", "t2", "t3", "t4",
  "t5", "t6", "t7", "t8",
  "t9", "t10", "t11", "t12",
  "t13", "t14", "t15", "t16",
};

static void
get_varname (char *s, OrcCompiler *compiler, int var)
{
  if (compiler->target_flags & ORC_TARGET_C_NOEXEC) {
    if (var < 48) {
      strcpy (s, varnames[var]);
    } else {
      sprintf(s, "t%d", var-32);
    }
  } else if (compiler->target_flags & ORC_TARGET_C_OPCODE) {
    if (var < ORC_VAR_S1) {
      sprintf(s, "ex->dest_ptrs[%d]", var-ORC_VAR_D1);
    } else {
      sprintf(s, "ex->src_ptrs[%d]", var-ORC_VAR_S1);
    }
  } else {
    sprintf(s, "ex->arrays[%d]", var);
  }
}

static void
get_varname_stride (char *s, OrcCompiler *compiler, int var)
{
  if (compiler->target_flags & ORC_TARGET_C_NOEXEC) {
    sprintf(s, "%s_stride", varnames[var]);
  } else {
    sprintf(s, "ex->params[%d]", var);
  }
}

void
orc_compiler_c_assemble (OrcCompiler *compiler)
{
  int i;
  int j;
  OrcInstruction *insn;
  OrcStaticOpcode *opcode;
  OrcRule *rule;
  int prefix = 0;

  if (!(compiler->target_flags & ORC_TARGET_C_BARE)) {
    ORC_ASM_CODE(compiler,"void\n");
    ORC_ASM_CODE(compiler,"%s (OrcExecutor *ex)\n", compiler->program->name);
    ORC_ASM_CODE(compiler,"{\n");
  }

  ORC_ASM_CODE(compiler,"%*s  int i;\n", prefix, "");
  if (compiler->program->is_2d) {
    ORC_ASM_CODE(compiler,"  int j;\n");
  }
  if (compiler->program->constant_n == 0) {
    if (!(compiler->target_flags & ORC_TARGET_C_NOEXEC) &&
        !(compiler->target_flags & ORC_TARGET_C_OPCODE)) {
      ORC_ASM_CODE(compiler,"  int n = ex->n;\n");
    }
  } else {
    ORC_ASM_CODE(compiler,"  int n = %d;\n", compiler->program->constant_n);
  }
  if (compiler->program->is_2d) {
    if (compiler->program->constant_m == 0) {
      if (!(compiler->target_flags & ORC_TARGET_C_NOEXEC)) {
        ORC_ASM_CODE(compiler,"  int m = ex->params[ORC_VAR_A1];\n");
      }
    } else {
      ORC_ASM_CODE(compiler,"  int m = %d;\n", compiler->program->constant_m);
    }
  }

  for(i=0;i<ORC_N_COMPILER_VARIABLES;i++){
    OrcVariable *var = compiler->vars + i;
    if (var->name == NULL) continue;
    switch (var->vartype) {
      case ORC_VAR_TYPE_CONST:
        break;
      case ORC_VAR_TYPE_TEMP:
        if (!(var->last_use == -1 && var->first_use == 0)) {
          ORC_ASM_CODE(compiler,"  %s var%d;\n", c_get_type_name(var->size), i);
        }
        break;
      case ORC_VAR_TYPE_SRC:
        ORC_ASM_CODE(compiler,"  const %s * ORC_RESTRICT ptr%d;\n",
            c_get_type_name (var->size),
            i);
        break;
      case ORC_VAR_TYPE_DEST:
        ORC_ASM_CODE(compiler,"  %s * ORC_RESTRICT ptr%d;\n",
            c_get_type_name (var->size),
            i);
        break;
      case ORC_VAR_TYPE_ACCUMULATOR:
        if (var->size >= 2) {
          ORC_ASM_CODE(compiler,"  %s var%d =  { 0 };\n",
              c_get_type_name (var->size),
              i);
        } else {
          ORC_ASM_CODE(compiler,"  %s var%d = 0;\n",
              c_get_type_name (var->size),
              i);
        }
        break;
      case ORC_VAR_TYPE_PARAM:
        break;
      default:
        ORC_COMPILER_ERROR(compiler, "bad vartype");
        break;
    }
  }

  ORC_ASM_CODE(compiler,"\n");
  if (compiler->program->is_2d) {
    ORC_ASM_CODE(compiler,"  for (j = 0; j < m; j++) {\n");
    prefix = 2;

    for(i=0;i<ORC_N_COMPILER_VARIABLES;i++){
      OrcVariable *var = compiler->vars + i;
      if (var->name == NULL) continue;
      switch (var->vartype) {
        case ORC_VAR_TYPE_SRC:
          {
            char s1[40], s2[40];
            get_varname(s1, compiler, i);
            get_varname_stride(s2, compiler, i);
            ORC_ASM_CODE(compiler,
                "    ptr%d = ORC_PTR_OFFSET(%s, %s * j);\n",
                i, s1, s2);
          }
          break;
        case ORC_VAR_TYPE_DEST:
          {
            char s1[40], s2[40];
            get_varname(s1, compiler, i),
            get_varname_stride(s2, compiler, i),
            ORC_ASM_CODE(compiler,
                "    ptr%d = ORC_PTR_OFFSET(%s, %s * j);\n",
                i, s1, s2);
          }
          break;
        default:
          break;
      }
    }
  } else {
    for(i=0;i<ORC_N_COMPILER_VARIABLES;i++){
      OrcVariable *var = compiler->vars + i;
      char s[40];
      if (var->name == NULL) continue;
      get_varname(s, compiler, i);
      switch (var->vartype) {
        case ORC_VAR_TYPE_SRC:
          ORC_ASM_CODE(compiler,"  ptr%d = (%s *)%s;\n", i,
              c_get_type_name (var->size), s);
          break;
        case ORC_VAR_TYPE_DEST:
          ORC_ASM_CODE(compiler,"  ptr%d = (%s *)%s;\n", i,
              c_get_type_name (var->size), s);
          break;
        default:
          break;
      }
    }
  }

  ORC_ASM_CODE(compiler,"\n");
  for(j=0;j<compiler->n_insns;j++){
    insn = compiler->insns + j;
    opcode = insn->opcode;

    if (!(insn->flags & ORC_INSN_FLAG_INVARIANT)) continue;

    ORC_ASM_CODE(compiler,"%*s    /* %d: %s */\n", prefix, "",
        j, insn->opcode->name);

    rule = insn->rule;
    if (!rule) {
      ORC_COMPILER_ERROR(compiler, "No rule for: %s on target %s", opcode->name,
          compiler->target->name);
      compiler->error = TRUE;
    }
    ORC_ASM_CODE(compiler,"%*s", prefix, "");
    if (insn->flags & (ORC_INSTRUCTION_FLAG_X2|ORC_INSTRUCTION_FLAG_X4)) {
      int n;
      if (insn->flags & ORC_INSTRUCTION_FLAG_X2) {
        n = 2;
      } else {
        n = 4;
      }
      for(i=0;i<n;i++){
        compiler->unroll_index = i;
        ORC_ASM_CODE(compiler,"%*s", prefix, "");
        rule->emit (compiler, rule->emit_user, insn);
      }
    } else {
      ORC_ASM_CODE(compiler,"%*s", prefix, "");
      rule->emit (compiler, rule->emit_user, insn);
    }
  }

  ORC_ASM_CODE(compiler,"\n");
  ORC_ASM_CODE(compiler,"%*s  for (i = 0; i < n; i++) {\n", prefix, "");

  /* Emit instructions */
  for(j=0;j<compiler->n_insns;j++){
    insn = compiler->insns + j;
    opcode = insn->opcode;

    if (insn->flags & ORC_INSN_FLAG_INVARIANT) continue;

    ORC_ASM_CODE(compiler,"%*s    /* %d: %s */\n", prefix, "",
        j, insn->opcode->name);

    rule = insn->rule;
    if (!rule) {
      ORC_COMPILER_ERROR(compiler, "No rule for: %s on target %s", opcode->name,
          compiler->target->name);
      compiler->error = TRUE;
    }

    if (insn->flags & (ORC_INSTRUCTION_FLAG_X2|ORC_INSTRUCTION_FLAG_X4)) {
      int n;
      if (insn->flags & ORC_INSTRUCTION_FLAG_X2) {
        n = 2;
      } else {
        n = 4;
      }
      for(i=0;i<n;i++){
        compiler->unroll_index = i;
        ORC_ASM_CODE(compiler,"%*s", prefix, "");
        rule->emit (compiler, rule->emit_user, insn);
      }
    } else {
      ORC_ASM_CODE(compiler,"%*s", prefix, "");
      rule->emit (compiler, rule->emit_user, insn);
    }
  }
  ORC_ASM_CODE(compiler,"%*s  }\n", prefix, "");
  if (compiler->program->is_2d) {
    ORC_ASM_CODE(compiler,"  }\n");
  }

  for(i=0;i<ORC_N_COMPILER_VARIABLES;i++){
    char varname[40];
    OrcVariable *var = compiler->vars + i;
    if (var->name == NULL) continue;
    switch (var->vartype) {
      case ORC_VAR_TYPE_ACCUMULATOR:
        c_get_name_int (varname, compiler, NULL, i);
        if (var->size == 2) {
          if (compiler->target_flags & ORC_TARGET_C_NOEXEC) {
            ORC_ASM_CODE(compiler,"  *%s = (%s & 0xffff);\n",
                varnames[i], varname);
          } else if (compiler->target_flags & ORC_TARGET_C_OPCODE) {
            ORC_ASM_CODE(compiler,"  ((orc_union32 *)ex->dest_ptrs[%d])->i = "
                "(%s + ((orc_union32 *)ex->dest_ptrs[%d])->i) & 0xffff;\n",
                i - ORC_VAR_A1, varname, i - ORC_VAR_A1);
          } else {
            ORC_ASM_CODE(compiler,"  ex->accumulators[%d] = (%s & 0xffff);\n",
                i - ORC_VAR_A1, varname);
          }
        } else {
          if (compiler->target_flags & ORC_TARGET_C_NOEXEC) {
            ORC_ASM_CODE(compiler,"  *%s = %s;\n",
                varnames[i], varname);
          } else if (compiler->target_flags & ORC_TARGET_C_OPCODE) {
            ORC_ASM_CODE(compiler,"  ((orc_union32 *)ex->dest_ptrs[%d])->i += %s;\n",
                i - ORC_VAR_A1, varname);
          } else {
            ORC_ASM_CODE(compiler,"  ex->accumulators[%d] = %s;\n",
                i - ORC_VAR_A1, varname);
          }
        }
        break;
      default:
        break;
    }
  }

  if (!(compiler->target_flags & ORC_TARGET_C_BARE)) {
    ORC_ASM_CODE(compiler,"}\n");
    ORC_ASM_CODE(compiler,"\n");
  }
}


/* rules */

static void
c_get_name_int (char *name, OrcCompiler *p, OrcInstruction *insn, int var)
{
  if (p->vars[var].vartype == ORC_VAR_TYPE_PARAM) {
    if (p->target_flags & ORC_TARGET_C_NOEXEC) {
      sprintf(name,"%s", varnames[var]);
    } else if (p->target_flags & ORC_TARGET_C_OPCODE) {
      switch (p->vars[var].param_type) {
        case ORC_PARAM_TYPE_INT:
          sprintf(name,"((orc_union32 *)(ex->src_ptrs[%d]))->i",
              var - ORC_VAR_P1 + p->program->n_src_vars);
          break;
        case ORC_PARAM_TYPE_FLOAT:
          sprintf(name,"((orc_union32 *)(ex->src_ptrs[%d]))->i",
              var - ORC_VAR_P1 + p->program->n_src_vars);
          break;
        case ORC_PARAM_TYPE_INT64:
          sprintf(name,"((orc_union64 *)(ex->src_ptrs[%d]))->i",
              var - ORC_VAR_P1 + p->program->n_src_vars);
          break;
        case ORC_PARAM_TYPE_DOUBLE:
          sprintf(name,"((orc_union64 *)(ex->src_ptrs[%d]))->i",
              var - ORC_VAR_P1 + p->program->n_src_vars);
          break;
        default:
          ORC_ASSERT(0);
      }
    } else {
      switch (p->vars[var].param_type) {
        case ORC_PARAM_TYPE_INT:
          sprintf(name,"ex->params[%d]", var);
          break;
        case ORC_PARAM_TYPE_FLOAT:
          sprintf(name,"((orc_union32 *)(ex->params+%d))->i", var);
          break;
        case ORC_PARAM_TYPE_INT64:
          /* FIXME */
          sprintf(name,"((orc_union32 *)(ex->params+%d))->i", var);
          break;
        case ORC_PARAM_TYPE_DOUBLE:
          /* FIXME */
          sprintf(name,"((orc_union32 *)(ex->params+%d))->i", var);
          break;
        default:
          ORC_ASSERT(0);
      }
    }
  } else if (p->vars[var].vartype == ORC_VAR_TYPE_CONST) {
    if (p->vars[var].value.i == 0x80000000) {
      sprintf(name,"0x80000000");
    } else {
      if (p->vars[var].value.i == (int)p->vars[var].value.i) {
        sprintf(name, "%d", (int)p->vars[var].value.i);
      } else {
        ORC_ASSERT(0);
      }
    }
  } else {
    if (insn && (insn->flags & ORC_INSTRUCTION_FLAG_X2)) {
      sprintf(name, "var%d.x2[%d]", var, p->unroll_index);
    } else if (insn && (insn->flags & ORC_INSTRUCTION_FLAG_X4)) {
      sprintf(name, "var%d.x4[%d]", var, p->unroll_index);
    } else {
      if (p->vars[var].size >= 2) {
        sprintf(name, "var%d.i", var);
      } else {
        sprintf(name, "var%d", var);
      }
    }
  }
}

static void
c_get_name_float (char *name, OrcCompiler *p, OrcInstruction *insn, int var)
{
  if (insn && (insn->flags & ORC_INSTRUCTION_FLAG_X2)) {
    sprintf(name, "var%d[%d].x2f", var, p->unroll_index);
  } else if (insn && (insn->flags & ORC_INSTRUCTION_FLAG_X4)) {
    sprintf(name, "var%d[%d].x4f", var, p->unroll_index);
  } else {
    switch (p->vars[var].vartype) {
      case ORC_VAR_TYPE_CONST:
      case ORC_VAR_TYPE_TEMP:
      case ORC_VAR_TYPE_ACCUMULATOR:
      case ORC_VAR_TYPE_SRC:
      case ORC_VAR_TYPE_DEST:
        sprintf(name, "var%d.f", var);
        break;
      case ORC_VAR_TYPE_PARAM:
        sprintf(name, "var%d", var);
        break;
      default:
        ORC_COMPILER_ERROR(p, "bad vartype");
        sprintf(name, "ERROR");
        break;
    }
  }
}

static const char *
c_get_type_name (int size)
{
  switch (size) {
    case 1:
      return "orc_int8";
    case 2:
      return "orc_union16";
    case 4:
      return "orc_union32";
    case 8:
      return "orc_union64";
    default:
      return "ERROR";
  }
}


#define UNARY(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
 \
  ORC_ASM_CODE(p,"    %s = " op ";\n", dest, src1); \
}

#define BINARY(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40], src2[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
  c_get_name_int (src2, p, insn, insn->src_args[1]); \
 \
  ORC_ASM_CODE(p,"    %s = " op ";\n", dest, src1, src2); \
}

#define UNARYF(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
 \
  ORC_ASM_CODE(p, "    {\n"); \
  ORC_ASM_CODE(p,"       orc_union32 _src1;\n"); \
  ORC_ASM_CODE(p,"       orc_union32 _dest1;\n"); \
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL(%s);\n", src1); \
  ORC_ASM_CODE(p,"       _dest1.f = " op ";\n", "_src1.f"); \
  ORC_ASM_CODE(p,"       %s = ORC_DENORMAL(_dest1.i);\n", dest); \
  ORC_ASM_CODE(p, "    }\n"); \
}

#define BINARYF(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40], src2[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
  c_get_name_int (src2, p, insn, insn->src_args[1]); \
 \
  ORC_ASM_CODE(p, "    {\n"); \
  ORC_ASM_CODE(p,"       orc_union32 _src1;\n"); \
  ORC_ASM_CODE(p,"       orc_union32 _src2;\n"); \
  ORC_ASM_CODE(p,"       orc_union32 _dest1;\n"); \
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL(%s);\n", src1); \
  ORC_ASM_CODE(p,"       _src2.i = ORC_DENORMAL(%s);\n", src2); \
  ORC_ASM_CODE(p,"       _dest1.f = " op ";\n", "_src1.f", "_src2.f"); \
  ORC_ASM_CODE(p,"       %s = ORC_DENORMAL(_dest1.i);\n", dest); \
  ORC_ASM_CODE(p, "    }\n"); \
}

#define BINARYFL(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40], src2[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
  c_get_name_int (src2, p, insn, insn->src_args[1]); \
 \
  ORC_ASM_CODE(p, "    {\n"); \
  ORC_ASM_CODE(p,"       orc_union32 _src1;\n"); \
  ORC_ASM_CODE(p,"       orc_union32 _src2;\n"); \
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL(%s);\n", src1); \
  ORC_ASM_CODE(p,"       _src2.i = ORC_DENORMAL(%s);\n", src2); \
  ORC_ASM_CODE(p,"       %s = " op ";\n", dest, "_src1.f", "_src2.f"); \
  ORC_ASM_CODE(p, "    }\n"); \
}

#define UNARYD(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
 \
  ORC_ASM_CODE(p, "    {\n"); \
  ORC_ASM_CODE(p,"       orc_union64 _src1;\n"); \
  ORC_ASM_CODE(p,"       orc_union64 _dest1;\n"); \
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL_DOUBLE(%s);\n", src1); \
  ORC_ASM_CODE(p,"       _dest1.f = " op ";\n", "_src1.f"); \
  ORC_ASM_CODE(p,"       %s = ORC_DENORMAL_DOUBLE(_dest1.i);\n", dest); \
  ORC_ASM_CODE(p, "    }\n"); \
}

#define BINARYD(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40], src2[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
  c_get_name_int (src2, p, insn, insn->src_args[1]); \
 \
  ORC_ASM_CODE(p, "    {\n"); \
  ORC_ASM_CODE(p,"       orc_union64 _src1;\n"); \
  ORC_ASM_CODE(p,"       orc_union64 _src2;\n"); \
  ORC_ASM_CODE(p,"       orc_union64 _dest1;\n"); \
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL_DOUBLE(%s);\n", src1); \
  ORC_ASM_CODE(p,"       _src2.i = ORC_DENORMAL_DOUBLE(%s);\n", src2); \
  ORC_ASM_CODE(p,"       _dest1.f = " op ";\n", "_src1.f", "_src2.f"); \
  ORC_ASM_CODE(p,"       %s = ORC_DENORMAL_DOUBLE(_dest1.i);\n", dest); \
  ORC_ASM_CODE(p, "    }\n"); \
}

#define BINARYDQ(name,op) \
static void \
c_rule_ ## name (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  char dest[40], src1[40], src2[40]; \
\
  c_get_name_int (dest, p, insn, insn->dest_args[0]); \
  c_get_name_int (src1, p, insn, insn->src_args[0]); \
  c_get_name_int (src2, p, insn, insn->src_args[1]); \
 \
  ORC_ASM_CODE(p, "    {\n"); \
  ORC_ASM_CODE(p,"       orc_union64 _src1;\n"); \
  ORC_ASM_CODE(p,"       orc_union64 _src2;\n"); \
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL_DOUBLE(%s);\n", src1); \
  ORC_ASM_CODE(p,"       _src2.i = ORC_DENORMAL_DOUBLE(%s);\n", src2); \
  ORC_ASM_CODE(p,"       %s = " op ";\n", dest, "_src1.f", "_src2.f"); \
  ORC_ASM_CODE(p, "    }\n"); \
}

#define BINARY_SB(a,b) BINARY(a,b)
#define BINARY_UB(a,b) BINARY(a,b)
#define BINARY_SW(a,b) BINARY(a,b)
#define BINARY_UW(a,b) BINARY(a,b)
#define BINARY_SL(a,b) BINARY(a,b)
#define BINARY_UL(a,b) BINARY(a,b)
#define BINARY_SQ(a,b) BINARY(a,b)
#define BINARY_UQ(a,b) BINARY(a,b)
#define UNARY_SB(a,b) UNARY(a,b)
#define UNARY_UB(a,b) UNARY(a,b)
#define UNARY_SW(a,b) UNARY(a,b)
#define UNARY_UW(a,b) UNARY(a,b)
#define UNARY_SL(a,b) UNARY(a,b)
#define UNARY_UL(a,b) UNARY(a,b)
#define UNARY_SQ(a,b) UNARY(a,b)
#define UNARY_UQ(a,b) UNARY(a,b)
#define BINARY_BW(a,b) BINARY(a,b)
#define BINARY_WL(a,b) BINARY(a,b)
#define BINARY_LW(a,b) BINARY(a,b)
#define BINARY_WB(a,b) BINARY(a,b)
#define UNARY_BW(a,b) UNARY(a,b)
#define UNARY_WL(a,b) UNARY(a,b)
#define UNARY_LQ(a,b) UNARY(a,b)
#define UNARY_QL(a,b) UNARY(a,b)
#define UNARY_LW(a,b) UNARY(a,b)
#define UNARY_WB(a,b) UNARY(a,b)

#define BINARY_F(a,b) BINARYF(a,b)
#define BINARY_FL(a,b) BINARYFL(a,b)
#define UNARY_F(a,b) UNARYF(a,b)
#define UNARY_FL(a,b) UNARYFL(a,b)
#define UNARY_LF(a,b) UNARYLF(a,b)

#define BINARY_D(a,b) BINARYD(a,b)
#define BINARY_DQ(a,b) BINARYDQ(a,b)
#define UNARY_D(a,b) UNARYD(a,b)
#define UNARY_DL(a,b) UNARYFL(a,b)
#define UNARY_LD(a,b) UNARYLF(a,b)
#define UNARY_DF(a,b) UNARYF(a,b)
#define UNARY_FD(a,b) UNARYF(a,b)

#include "opcodes.h"

#undef BINARY_SB
#undef BINARY_UB
#undef BINARY_SW
#undef BINARY_UW
#undef BINARY_SL
#undef BINARY_UL
#undef BINARY_SQ
#undef BINARY_UQ
#undef BINARY_F
#undef BINARY_D
#undef UNARY_SB
#undef UNARY_UB
#undef UNARY_SW
#undef UNARY_UW
#undef UNARY_SL
#undef UNARY_UL
#undef UNARY_SQ
#undef UNARY_UQ
#undef UNARY_F
#undef UNARY_D
#undef BINARY_BW
#undef BINARY_WL
#undef BINARY_LW
#undef BINARY_WB
#undef UNARY_BW
#undef UNARY_WL
#undef UNARY_LQ
#undef UNARY_QL
#undef UNARY_LW
#undef UNARY_WB
#undef UNARY_FL
#undef UNARY_DL
#undef UNARY_LF
#undef UNARY_LD
#undef BINARY_FL
#undef BINARY_DQ
#undef UNARY_FD
#undef UNARY_DF


static void
c_rule_loadpX (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40];
  int size = ORC_PTR_TO_INT(user);

  c_get_name_int (dest, p, insn, insn->dest_args[0]);

  if (p->vars[insn->src_args[0]].vartype == ORC_VAR_TYPE_PARAM) {
    if (p->target_flags & ORC_TARGET_C_NOEXEC) {
      ORC_ASM_CODE(p,"    %s = %s;\n", dest, varnames[insn->src_args[0]]);
    } else if (p->target_flags & ORC_TARGET_C_OPCODE) {
      if (size == 8) {
        ORC_ASM_CODE(p,"    %s = ((orc_union64 *)(ex->src_ptrs[%d]))->i;\n",
            dest, insn->src_args[0] - ORC_VAR_P1 + p->program->n_src_vars);
      } else {
        ORC_ASM_CODE(p,"    %s = ((orc_union32 *)(ex->src_ptrs[%d]))->i;\n",
            dest, insn->src_args[0] - ORC_VAR_P1 + p->program->n_src_vars);
      }
    } else {
      if (size == 8) {
        ORC_ASM_CODE(p,"    %s = (ex->params[%d] & 0xffffffff) | (((orc_uint64)(ex->params[%d + (ORC_VAR_T1 - ORC_VAR_P1)]) << 32);\n",
            dest, insn->src_args[0], insn->src_args[0]);
      } else {
        ORC_ASM_CODE(p,"    %s = ex->params[%d];\n", dest,
            insn->src_args[0]);
      }
    }
  } else if (p->vars[insn->src_args[0]].vartype == ORC_VAR_TYPE_CONST) {
    ORC_ASM_CODE(p,"    %s = 0x%08x; /* %d or %gf */\n", dest,
        (unsigned int)p->vars[insn->src_args[0]].value.i,
        (int)p->vars[insn->src_args[0]].value.i,
        p->vars[insn->src_args[0]].value.f);
  } else {
    ORC_COMPILER_ERROR(p, "expected param or constant");
  }
}

#if 0
static void
c_rule_loadpX (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40];
  char src[40];
  OrcVariable *var;

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  var = &p->vars[insn->src_args[0]];

  ORC_ASM_CODE(p,"    %s = %s;\n", dest, src);
}
#endif

static void
c_rule_loadX (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    var%d = ptr%d[offset + i];\n", insn->dest_args[0],
        insn->src_args[0]);
  } else {
    ORC_ASM_CODE(p,"    var%d = ptr%d[i];\n", insn->dest_args[0],
        insn->src_args[0]);
  }
}

static void
c_rule_loadoffX (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char src[40];

  c_get_name_int (src, p, insn, insn->src_args[1]);

  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    var%d = ptr%d[offset + i + %s];\n", insn->dest_args[0],
        insn->src_args[0], src);
  } else {
    ORC_ASM_CODE(p,"    var%d = ptr%d[i + %s];\n", insn->dest_args[0],
        insn->src_args[0], src);
  }
}

static void
c_rule_loadupdb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    var%d = ptr%d[(offset + i)>>1];\n", insn->dest_args[0],
        insn->src_args[0]);
  } else {
    ORC_ASM_CODE(p,"    var%d = ptr%d[i>>1];\n", insn->dest_args[0],
        insn->src_args[0]);
  }
}

static void
c_rule_loadupib (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    var%d = ((offset + i)&1) ? ((orc_uint8)ptr%d[(offset + i)>>1] + (orc_uint8)ptr%d[((offset + i)>>1)+1] + 1)>>1 : ptr%d[(offset + i)>>1];\n",
        insn->dest_args[0], insn->src_args[0], insn->src_args[0],
        insn->src_args[0]);
  } else {
    ORC_ASM_CODE(p,"    var%d = (i&1) ? ((orc_uint8)ptr%d[i>>1] + (orc_uint8)ptr%d[(i>>1)+1] + 1)>>1 : ptr%d[i>>1];\n",
        insn->dest_args[0], insn->src_args[0], insn->src_args[0],
        insn->src_args[0]);
  }
}

static void
c_rule_ldresnearX (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char src1[40];
  char src2[40];

  c_get_name_int (src1, p, insn, insn->src_args[1]);
  c_get_name_int (src2, p, insn, insn->src_args[2]);

  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    var%d = ptr%d[(%s + (offset + i)*%s)>>16];\n",
        insn->dest_args[0], insn->src_args[0], src1, src2);
  } else {
    ORC_ASM_CODE(p,"    var%d = ptr%d[(%s + i*%s)>>16];\n",
        insn->dest_args[0], insn->src_args[0], src1, src2);
  }
}

static void
c_rule_ldreslinb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char src1[40];
  char src2[40];

  c_get_name_int (src1, p, insn, insn->src_args[1]);
  c_get_name_int (src2, p, insn, insn->src_args[2]);

  ORC_ASM_CODE(p,"    {\n");
  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    int tmp = %s + (offset + i) * %s;\n", src1, src2);
  } else {
    ORC_ASM_CODE(p,"    int tmp = %s + i * %s;\n", src1, src2);
  }
  ORC_ASM_CODE(p,"    var%d = ((orc_uint8)ptr%d[tmp>>16] * (256-((tmp>>8)&0xff)) + (orc_uint8)ptr%d[(tmp>>16)+1] * ((tmp>>8)&0xff))>>8;\n",
      insn->dest_args[0], insn->src_args[0], insn->src_args[0]);
  ORC_ASM_CODE(p,"    }\n");
}

static void
c_rule_ldreslinl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int i;
  char src1[40];
  char src2[40];

  c_get_name_int (src1, p, insn, insn->src_args[1]);
  c_get_name_int (src2, p, insn, insn->src_args[2]);


  ORC_ASM_CODE(p,"    {\n");
  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    int tmp = %s + (offset + i) * %s;\n", src1, src2);
  } else {
    ORC_ASM_CODE(p,"    int tmp = %s + i * %s;\n", src1, src2);
  }
  ORC_ASM_CODE(p,"    orc_union32 a = ptr%d[tmp>>16];\n", insn->src_args[0]);
  ORC_ASM_CODE(p,"    orc_union32 b = ptr%d[(tmp>>16)+1];\n", insn->src_args[0]);
  for (i=0;i<4;i++){
    ORC_ASM_CODE(p,"    var%d.x4[%d] = ((orc_uint8)a.x4[%d] * (256-((tmp>>8)&0xff)) + (orc_uint8)b.x4[%d] * ((tmp>>8)&0xff))>>8;\n",
        insn->dest_args[0], i, i, i);
  }
  ORC_ASM_CODE(p,"    }\n");
}

static void
c_rule_storeX (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->target_flags & ORC_TARGET_C_OPCODE &&
      !(insn->flags & ORC_INSN_FLAG_ADDED)) {
    ORC_ASM_CODE(p,"    ptr%d[offset + i] = var%d;\n", insn->dest_args[0],
        insn->src_args[0]);
  } else {
    ORC_ASM_CODE(p,"    ptr%d[i] = var%d;\n", insn->dest_args[0],
        insn->src_args[0]);
  }
}

static void
c_rule_accw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"    %s = %s + %s;\n", dest, dest, src1);
}

static void
c_rule_accl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"    %s = %s + %s;\n", dest, dest, src1);
}

static void
c_rule_accsadubl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40], src2[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);
  c_get_name_int (src2, p, insn, insn->src_args[1]);

  ORC_ASM_CODE(p,
      "    %s = %s + ORC_ABS((orc_int32)(orc_uint8)%s - (orc_int32)(orc_uint8)%s);\n",
      dest, dest, src1, src2);
}

static void
c_rule_splitql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest1[40], dest2[40], src[40];

  c_get_name_int (dest1, p, insn, insn->dest_args[0]);
  c_get_name_int (dest2, p, insn, insn->dest_args[1]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"    %s = (%s >> 32) & 0xffffffff;\n", dest1, src);
  ORC_ASM_CODE(p,"    %s = %s & 0xffffffff;\n", dest2, src);
}

static void
c_rule_splitlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest1[40], dest2[40], src[40];

  c_get_name_int (dest1, p, insn, insn->dest_args[0]);
  c_get_name_int (dest2, p, insn, insn->dest_args[1]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"    %s = (%s >> 16) & 0xffff;\n", dest1, src);
  ORC_ASM_CODE(p,"    %s = %s & 0xffff;\n", dest2, src);
}

static void
c_rule_splitwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest1[40], dest2[40], src[40];

  c_get_name_int (dest1, p, insn, insn->dest_args[0]);
  c_get_name_int (dest2, p, insn, insn->dest_args[1]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"    %s = (%s >> 8) & 0xff;\n", dest1, src);
  ORC_ASM_CODE(p,"    %s = %s & 0xff;\n", dest2, src);
}

static void
c_rule_splatbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"    %s = ((%s&0xff) << 8) | (%s&0xff);\n", dest, src, src);
}

static void
c_rule_splatbl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,
      "    %s = ((%s&0xff) << 24) | ((%s&0xff)<<16) | ((%s&0xff) << 8) | (%s&0xff);\n",
      dest, src, src, src, src);
}

static void
c_rule_splatw3q (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,
      "    %s = ((((orc_uint64)%s)>>48) << 48) | "
      "((((orc_uint64)%s)>>48)<<32) | "
      "((((orc_uint64)%s)>>48) << 16) | "
      "((((orc_uint64)%s)>>48));\n",
      dest, src, src, src, src);
}

static void
c_rule_div255w (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,
      "    %s = ((uint16_t)(((orc_uint16)(%s+128)) + (((orc_uint16)(%s+128))>>8)))>>8;\n",
      dest, src, src);
}

static void
c_rule_divluw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40], src2[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);
  c_get_name_int (src2, p, insn, insn->src_args[1]);

  ORC_ASM_CODE(p,
      "    %s = ((%s&0xff) == 0) ? 255 : ORC_CLAMP_UB(((uint16_t)%s)/((uint16_t)%s&0xff));\n",
      dest, src2, src1, src2);
}

static void
c_rule_convlf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40];

  c_get_name_float (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"     %s = %s;\n", dest, src1);
}

static void
c_rule_convld (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40];

  c_get_name_float (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p,"     %s = %s;\n", dest, src1);
}

static void
c_rule_convfd (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40];

  c_get_name_float (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p, "    {\n");
  ORC_ASM_CODE(p,"       orc_union32 _src1;\n");
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL(%s);\n", src1);
  ORC_ASM_CODE(p,"       %s = _src1.f;\n", dest);
  ORC_ASM_CODE(p, "    }\n");
}

static void
c_rule_convdf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p, "    {\n");
  ORC_ASM_CODE(p,"       orc_union64 _src1;\n");
  ORC_ASM_CODE(p,"       orc_union32 _dest;\n");
  ORC_ASM_CODE(p,"       _src1.i = ORC_DENORMAL_DOUBLE(%s);\n", src1);
  ORC_ASM_CODE(p,"       _dest.f = _src1.f;\n");
  ORC_ASM_CODE(p,"       %s = ORC_DENORMAL(_dest.i);\n", dest);
  ORC_ASM_CODE(p, "    }\n");
}

static void
c_rule_convfl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src[40], src_i[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_float (src, p, insn, insn->src_args[0]);
  c_get_name_int (src_i, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p, "    {\n");
  ORC_ASM_CODE(p,"       int tmp;\n");
  ORC_ASM_CODE(p,"       tmp = (int)%s;\n", src);
  ORC_ASM_CODE(p,"       if (tmp == 0x80000000 && !(%s&0x80000000)) tmp = 0x7fffffff;\n", src_i);
  ORC_ASM_CODE(p,"       %s = tmp;\n", dest);
  ORC_ASM_CODE(p, "    }\n");
}

static void
c_rule_convdl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src[40], src_i[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_float (src, p, insn, insn->src_args[0]);
  c_get_name_int (src_i, p, insn, insn->src_args[0]);

  ORC_ASM_CODE(p, "    {\n");
  ORC_ASM_CODE(p,"       int tmp;\n");
  ORC_ASM_CODE(p,"       tmp = %s;\n", src);
  ORC_ASM_CODE(p,"       if (tmp == 0x80000000 && !(%s&0x8000000000000000ULL)) tmp = 0x7fffffff;\n", src_i);
  ORC_ASM_CODE(p,"       %s = tmp;\n", dest);
  ORC_ASM_CODE(p, "    }\n");
}

static void
c_rule_minf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40], src2[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);
  c_get_name_int (src2, p, insn, insn->src_args[1]);

  ORC_ASM_CODE(p,"    {\n");
  ORC_ASM_CODE(p,"      orc_union32 _src1;\n");
  ORC_ASM_CODE(p,"      orc_union32 _src2;\n");
  ORC_ASM_CODE(p,"      _src1.i = ORC_DENORMAL(%s);\n", src1);
  ORC_ASM_CODE(p,"      _src2.i = ORC_DENORMAL(%s);\n", src2);
  ORC_ASM_CODE(p,"      if (ORC_ISNAN(_src1.i)) %s = _src1.i;\n", dest);
  ORC_ASM_CODE(p,"      else if (ORC_ISNAN(_src2.i)) %s = _src2.i;\n", dest);
  ORC_ASM_CODE(p,"      else %s = (_src1.f < _src2.f) ? _src1.i : _src2.i;\n", dest);
  ORC_ASM_CODE(p,"    }\n");
}

static void
c_rule_maxf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40], src2[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);
  c_get_name_int (src2, p, insn, insn->src_args[1]);

  ORC_ASM_CODE(p,"    {\n");
  ORC_ASM_CODE(p,"      orc_union32 _src1;\n");
  ORC_ASM_CODE(p,"      orc_union32 _src2;\n");
  ORC_ASM_CODE(p,"      _src1.i = ORC_DENORMAL(%s);\n", src1);
  ORC_ASM_CODE(p,"      _src2.i = ORC_DENORMAL(%s);\n", src2);
  ORC_ASM_CODE(p,"      if (ORC_ISNAN(_src1.i)) %s = _src1.i;\n", dest);
  ORC_ASM_CODE(p,"      else if (ORC_ISNAN(_src2.i)) %s = _src2.i;\n", dest);
  ORC_ASM_CODE(p,"      else %s = (_src1.f > _src2.f) ? _src1.i : _src2.i;\n", dest);
  ORC_ASM_CODE(p,"    }\n");
}

static void
c_rule_mind (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40], src2[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);
  c_get_name_int (src2, p, insn, insn->src_args[1]);

  ORC_ASM_CODE(p,"    {\n");
  ORC_ASM_CODE(p,"      orc_union64 _src1;\n");
  ORC_ASM_CODE(p,"      orc_union64 _src2;\n");
  ORC_ASM_CODE(p,"      _src1.i = ORC_DENORMAL_DOUBLE(%s);\n", src1);
  ORC_ASM_CODE(p,"      _src2.i = ORC_DENORMAL_DOUBLE(%s);\n", src2);
  ORC_ASM_CODE(p,"      if (ORC_ISNAN_DOUBLE(_src1.i)) %s = _src1.i;\n", dest);
  ORC_ASM_CODE(p,"      else if (ORC_ISNAN_DOUBLE(_src2.i)) %s = _src2.i;\n", dest);
  ORC_ASM_CODE(p,"      else %s = (_src1.f < _src2.f) ? _src1.i : _src2.i;\n", dest);
  ORC_ASM_CODE(p,"    }\n");
}

static void
c_rule_maxd (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  char dest[40], src1[40], src2[40];

  c_get_name_int (dest, p, insn, insn->dest_args[0]);
  c_get_name_int (src1, p, insn, insn->src_args[0]);
  c_get_name_int (src2, p, insn, insn->src_args[1]);

  ORC_ASM_CODE(p,"    {\n");
  ORC_ASM_CODE(p,"      orc_union64 _src1;\n");
  ORC_ASM_CODE(p,"      orc_union64 _src2;\n");
  ORC_ASM_CODE(p,"      _src1.i = ORC_DENORMAL_DOUBLE(%s);\n", src1);
  ORC_ASM_CODE(p,"      _src2.i = ORC_DENORMAL_DOUBLE(%s);\n", src2);
  ORC_ASM_CODE(p,"      if (ORC_ISNAN_DOUBLE(_src1.i)) %s = _src1.i;\n", dest);
  ORC_ASM_CODE(p,"      else if (ORC_ISNAN_DOUBLE(_src2.i)) %s = _src2.i;\n", dest);
  ORC_ASM_CODE(p,"      else %s = (_src1.f > _src2.f) ? _src1.i : _src2.i;\n", dest);
  ORC_ASM_CODE(p,"    }\n");
}

static OrcTarget c_target = {
  "c",
  FALSE,
  ORC_GP_REG_BASE,
  orc_compiler_c_get_default_flags,
  orc_compiler_c_init,
  orc_compiler_c_assemble,
  { { 0 } },
  0,
  orc_target_c_get_asm_preamble,
};


void
orc_c_init (void)
{
  OrcRuleSet *rule_set;

  orc_target_register (&c_target);

  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), &c_target, 0);

#define BINARY_SB(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_UB(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_SW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_UW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_SL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_UL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_SQ(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_UQ(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_F(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_D(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_SB(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_UB(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_SW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_UW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_SL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_UL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_SQ(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_UQ(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_F(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_D(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_BW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_WL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_LW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_WB(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_BW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_WL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_LQ(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_QL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_LW(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_WB(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);

#define UNARY_FL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_DL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_FL(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define BINARY_DQ(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_LF(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_LD(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_DF(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);
#define UNARY_FD(a,b) orc_rule_register (rule_set, #a , c_rule_ ## a, NULL);

#include "opcodes.h"

  orc_rule_register (rule_set, "loadpb", c_rule_loadpX, (void *)1);
  orc_rule_register (rule_set, "loadpw", c_rule_loadpX, (void *)2);
  orc_rule_register (rule_set, "loadpl", c_rule_loadpX, (void *)4);
  orc_rule_register (rule_set, "loadpq", c_rule_loadpX, (void *)8);
  orc_rule_register (rule_set, "loadb", c_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadw", c_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadl", c_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadq", c_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadoffb", c_rule_loadoffX, NULL);
  orc_rule_register (rule_set, "loadoffw", c_rule_loadoffX, NULL);
  orc_rule_register (rule_set, "loadoffl", c_rule_loadoffX, NULL);
  orc_rule_register (rule_set, "loadupdb", c_rule_loadupdb, NULL);
  orc_rule_register (rule_set, "loadupib", c_rule_loadupib, NULL);
  orc_rule_register (rule_set, "ldresnearb", c_rule_ldresnearX, NULL);
  orc_rule_register (rule_set, "ldresnearl", c_rule_ldresnearX, NULL);
  orc_rule_register (rule_set, "ldreslinb", c_rule_ldreslinb, NULL);
  orc_rule_register (rule_set, "ldreslinl", c_rule_ldreslinl, NULL);
  orc_rule_register (rule_set, "storeb", c_rule_storeX, NULL);
  orc_rule_register (rule_set, "storew", c_rule_storeX, NULL);
  orc_rule_register (rule_set, "storel", c_rule_storeX, NULL);
  orc_rule_register (rule_set, "storeq", c_rule_storeX, NULL);

  orc_rule_register (rule_set, "accw", c_rule_accw, NULL);
  orc_rule_register (rule_set, "accl", c_rule_accl, NULL);
  orc_rule_register (rule_set, "accsadubl", c_rule_accsadubl, NULL);
  orc_rule_register (rule_set, "splitql", c_rule_splitql, NULL);
  orc_rule_register (rule_set, "splitlw", c_rule_splitlw, NULL);
  orc_rule_register (rule_set, "splitwb", c_rule_splitwb, NULL);
  orc_rule_register (rule_set, "splatbw", c_rule_splatbw, NULL);
  orc_rule_register (rule_set, "splatbl", c_rule_splatbl, NULL);
  orc_rule_register (rule_set, "splatw3q", c_rule_splatw3q, NULL);
  orc_rule_register (rule_set, "div255w", c_rule_div255w, NULL);
  orc_rule_register (rule_set, "divluw", c_rule_divluw, NULL);
  orc_rule_register (rule_set, "convlf", c_rule_convlf, NULL);
  orc_rule_register (rule_set, "convld", c_rule_convld, NULL);
  orc_rule_register (rule_set, "convfl", c_rule_convfl, NULL);
  orc_rule_register (rule_set, "convdl", c_rule_convdl, NULL);
  orc_rule_register (rule_set, "convfd", c_rule_convfd, NULL);
  orc_rule_register (rule_set, "convdf", c_rule_convdf, NULL);
  orc_rule_register (rule_set, "minf", c_rule_minf, NULL);
  orc_rule_register (rule_set, "maxf", c_rule_maxf, NULL);
  orc_rule_register (rule_set, "mind", c_rule_mind, NULL);
  orc_rule_register (rule_set, "maxd", c_rule_maxd, NULL);
}

