#include <orc/orcmips.h>
#include <orc/orcdebug.h>
#include <stdlib.h>

#define ORC_SW_MAX 32767
#define ORC_SW_MIN (-1-ORC_SW_MAX)
#define ORC_SB_MAX 127
#define ORC_SB_MIN (-1-ORC_SB_MAX)

void
mips_rule_load (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = compiler->vars[insn->src_args[0]].ptr_register;
  int dest = compiler->vars[insn->dest_args[0]].alloc;
  /* such that 2^total_shift is the amount to load at a time */
  int total_shift = compiler->insn_shift + ORC_PTR_TO_INT (user);
  int is_aligned = compiler->vars[insn->src_args[0]].is_aligned;
  OrcMipsRegister tmp = orc_compiler_get_temp_reg (compiler);

  if (compiler->vars[insn->src_args[0]].vartype == ORC_VAR_TYPE_CONST) {
    ORC_PROGRAM_ERROR (compiler, "not implemented");
    return;
  }

  ORC_DEBUG ("insn_shift=%d", compiler->insn_shift);
  /* FIXME: Check alignment. We are assuming data is aligned here */
  switch (total_shift) {
  case 0:
    orc_mips_emit_lb (compiler, dest, src, 0);
    break;
  case 1:
    if (is_aligned) {
      orc_mips_emit_lh (compiler, dest, src, 0);
    } else {
      orc_mips_emit_lb (compiler, tmp, src, 0);
      orc_mips_emit_lb (compiler, dest, src, 1);
      orc_mips_emit_append (compiler, dest, tmp, 8);
    }
    break;
  case 2:
    if (is_aligned) {
      orc_mips_emit_lw (compiler, dest, src, 0);
    } else {
      /* note: the code below is little endian specific */
      orc_mips_emit_lwr (compiler, dest, src, 0);
      orc_mips_emit_lwl (compiler, dest, src, 3);
    }
    break;
  default:
    ORC_PROGRAM_ERROR(compiler, "Don't know how to handle that shift");
  }
  compiler->vars[insn->src_args[0]].update_type = 2;
}

void
mips_rule_store (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = compiler->vars[insn->src_args[0]].alloc;
  int dest = compiler->vars[insn->dest_args[0]].ptr_register;
  int total_shift = compiler->insn_shift + ORC_PTR_TO_INT (user);
  int is_aligned = compiler->vars[insn->dest_args[0]].is_aligned;
  OrcMipsRegister tmp = orc_compiler_get_temp_reg (compiler);

  ORC_DEBUG ("insn_shift=%d", compiler->insn_shift);

  /* FIXME: Check alignment. We are assuming data is aligned here */
  switch (total_shift) {
  case 0:
    orc_mips_emit_sb (compiler, src, dest, 0);
    break;
  case 1:
    if (is_aligned) {
      orc_mips_emit_sh (compiler, src, dest, 0);
    } else {
      /* Note: the code below is little endian specific */
      orc_mips_emit_sb (compiler, src, dest, 0);
      orc_mips_emit_srl (compiler, tmp, src, 8);
      orc_mips_emit_sb (compiler, tmp, dest, 1);
    }
    break;
  case 2:
    if (is_aligned) {
      orc_mips_emit_sw (compiler, src, dest, 0);
    } else {
      orc_mips_emit_swr (compiler, src, dest, 0);
      orc_mips_emit_swl (compiler, src, dest, 3);
    }
    break;
  default:
    ORC_PROGRAM_ERROR(compiler, "Don't know how to handle that shift");
  }
  compiler->vars[insn->dest_args[0]].update_type = 2;
}


void
mips_rule_addl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_addu (compiler, dest, src1, src2);
}

void
mips_rule_addw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  switch (compiler->insn_shift) {
  case 0:
    orc_mips_emit_addu (compiler, dest, src1, src2);
    break;
  case 1:
    orc_mips_emit_addu_ph (compiler, dest, src1, src2);
    break;
  default:
    ORC_PROGRAM_ERROR (compiler, "Don't know how to handle that insn_shift");
  }
}

void
mips_rule_addb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  switch (compiler->insn_shift) {
  case 0:
    orc_mips_emit_addu (compiler, dest, src1, src2);
    break;
  case 1:
  case 2:
    orc_mips_emit_addu_qb (compiler, dest, src1, src2);
    break;
  default:
    ORC_PROGRAM_ERROR (compiler, "Don't know how to handle that insn_shift");
  }

}

void
mips_rule_subb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_subu_qb (compiler, dest, src1, src2);

}

void
mips_rule_copyl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_move (compiler, dest, src);
}

void
mips_rule_copyw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_move (compiler, dest, src);
}

void
mips_rule_copyb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_move (compiler, dest, src);
}

void
mips_rule_mul (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_mul (compiler, dest, src1, src2);
}

void
mips_rule_mullw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_mul_ph (compiler, dest, src1, src2);
}


void
mips_rule_shrs (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int shift = ORC_SRC_VAL (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_sra (compiler, dest, src, shift);
}

void
mips_rule_convssslw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int dest = ORC_DEST_ARG (compiler, insn, 0);
  OrcMipsRegister tmp0 = orc_compiler_get_temp_reg (compiler);
  OrcMipsRegister tmp1 = orc_compiler_get_temp_reg (compiler);

  if (dest != src)
    orc_mips_emit_move (compiler, dest, src);
  orc_mips_emit_ori (compiler, tmp0, ORC_MIPS_ZERO, ORC_SW_MAX);
  orc_mips_emit_slt (compiler, tmp1, tmp0, src);
  orc_mips_emit_movn (compiler, dest, tmp0, tmp1);
  orc_mips_emit_lui (compiler, tmp0, (ORC_SW_MIN >> 16) & 0xffff);
  orc_mips_emit_ori (compiler, tmp0, tmp0, ORC_SW_MAX & 0xffff);
  /* this still works if src == dest since in that case, its value is either
   * the original src or ORC_SW_MAX, which works as well here */
  orc_mips_emit_slt (compiler, tmp1, src, tmp0);
  orc_mips_emit_movn (compiler, dest, tmp0, tmp1);
}

void
mips_rule_convssswb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int dest = ORC_DEST_ARG (compiler, insn, 0);
  OrcMipsRegister tmp = orc_compiler_get_temp_reg (compiler);

  orc_mips_emit_repl_ph (compiler, tmp, ORC_SB_MAX);
  orc_mips_emit_cmp_lt_ph (compiler, tmp, src);
  orc_mips_emit_pick_ph (compiler, dest, tmp, src);
  orc_mips_emit_repl_ph (compiler, tmp, ORC_SB_MIN);
  orc_mips_emit_cmp_lt_ph (compiler, src, tmp);
  orc_mips_emit_pick_ph (compiler, dest, tmp, src);
}

void
mips_rule_convsbw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  /* left shift 8 bits, then right shift signed 8 bits, so that the sign bit
   * gets replicated in the upper 8 bits */
  orc_mips_emit_shll_ph (compiler, dest, src, 8);
  orc_mips_emit_shra_ph (compiler, dest, dest, 8);
}

void
mips_rule_mergewl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  /* FIXME: use replv.ph if src1 == src2 */
  if (dest != src1)
    orc_mips_emit_move (compiler, dest, src1);
  orc_mips_emit_append (compiler, dest, src2, 16);
}

void
mips_rule_mergebw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_shll_ph (compiler, dest, src1, 8);
  orc_mips_emit_and (compiler, dest, dest, src2);
}

void
mips_rule_addssw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_addq_s_ph (compiler, dest, src1, src2);
}

void
mips_rule_subssw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_subq_s_ph (compiler, dest, src1, src2);
}

void
mips_rule_shrsw (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (compiler, insn, 0);
  int src2 = ORC_SRC_ARG (compiler, insn, 1);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_shra_ph (compiler, dest, src1, src2);
}

void
mips_rule_loadupib (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  OrcMipsRegister tmp0 = orc_compiler_get_temp_reg (compiler);
  OrcMipsRegister tmp1 = orc_compiler_get_temp_reg (compiler);
  OrcMipsRegister tmp2 = orc_compiler_get_temp_reg (compiler);

  if (compiler->vars[insn->src_args[0]].vartype == ORC_VAR_TYPE_CONST) {
    ORC_PROGRAM_ERROR (compiler, "not implemented");
    return;
  }
  switch (compiler->insn_shift) {
  case 0:
    orc_mips_emit_andi (compiler, tmp0, src->ptr_offset, 1);
    /* We only do the first lb if offset is even */
    orc_mips_emit_conditional_branch_with_offset (compiler,
                                                  ORC_MIPS_BEQ,
                                                  tmp0,
                                                  ORC_MIPS_ZERO,
                                                  16);
    orc_mips_emit_lb (compiler, dest->alloc, src->ptr_register, 0);

    orc_mips_emit_lb (compiler, tmp0, src->ptr_register, 1);
    orc_mips_emit_adduh_r_qb (compiler, dest->alloc, dest->alloc, tmp0);
    /* In the case where there is no insn_shift, src->ptr_register needs to be
     * incremented only when ptr_offset is odd, _emit_loop() doesn't update it
     * in that case, and therefore we do it here */
    orc_mips_emit_addiu (compiler, src->ptr_register, src->ptr_register, 1);

    orc_mips_emit_addiu (compiler, src->ptr_offset, src->ptr_offset, 1);
    break;
  case 2:
    /*
       lb       t3, 0(src)      # a
       lb       t4, 1(src)      # b
       lb       dest, 2(src)    # c
       andi     t5, ptr_offset, 1 # i&1 NEW
       replv.qb t3, t3          # aaaa
       replv.qb t4, t4          # bbbb
       replv.qb dest, dest      # cccc NEW
       packrl.ph t3, t3, t4     # aabb
       packrl.ph dest, t4, dest # bbcc NEW
       move     t4, t3          # aabb
       append   t4, dest, 8     # abbc
       # if t5
       # t3 <- dest
       movn     t3, dest, t5    # NEW

       adduh_r.qb dest, t3, t4  # a(a,b)b(b,c) | (a,b)b(b,c)c

     */
    orc_mips_emit_lb (compiler, tmp0, src->ptr_register, 0);
    orc_mips_emit_lb (compiler, tmp1, src->ptr_register, 1);
    orc_mips_emit_lb (compiler, dest->alloc, src->ptr_register, 2);
    orc_mips_emit_andi (compiler, tmp2, src->ptr_offset, 1);
    orc_mips_emit_replv_qb (compiler, tmp0, tmp0);
    orc_mips_emit_replv_qb (compiler, tmp1, tmp1);
    orc_mips_emit_replv_qb (compiler, dest->alloc, dest->alloc);
    orc_mips_emit_packrl_ph (compiler, tmp0, tmp0, tmp1);
    orc_mips_emit_packrl_ph (compiler, dest->alloc, tmp1, dest->alloc);
    orc_mips_emit_move (compiler, tmp1, tmp0);
    orc_mips_emit_append (compiler, tmp1, dest->alloc, 8);
    orc_mips_emit_movn (compiler, tmp0, dest->alloc, tmp2);
    orc_mips_emit_adduh_r_qb (compiler, dest->alloc, tmp0, tmp1);
    /* FIXME: should we remove that as we only use ptr_offset for parity? */
    orc_mips_emit_addiu (compiler, src->ptr_offset, src->ptr_offset, 4);
    break;
  default:
    ORC_PROGRAM_ERROR (compiler, "unimplemented");
  }
  src->update_type = 1;
}

void
mips_rule_loadupdb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  OrcMipsRegister tmp = orc_compiler_get_temp_reg (compiler);

  if (compiler->vars[insn->src_args[0]].vartype == ORC_VAR_TYPE_CONST) {
    ORC_PROGRAM_ERROR (compiler, "not implemented");
    return;
  }

  switch (compiler->insn_shift) {
  case 0:
    orc_mips_emit_andi (compiler, tmp, src->ptr_offset, 1);
    orc_mips_emit_conditional_branch_with_offset (compiler,
                                                  ORC_MIPS_BEQ,
                                                  tmp,
                                                  ORC_MIPS_ZERO,
                                                  8);
    /* this is always done: in branch delay slot*/
    orc_mips_emit_lb (compiler, dest->alloc, src->ptr_register, 0);
    /* In the case where there is no insn_shift, src->ptr_register needs to be
     * incremented only when ptr_offset is odd, _emit_loop() doesn't update it
     * in that case, and therefore we do it here */
    orc_mips_emit_addiu (compiler, src->ptr_register, src->ptr_register, 1);
    orc_mips_emit_append (compiler, dest->alloc, dest->alloc, 8);

    orc_mips_emit_addiu (compiler, src->ptr_offset, src->ptr_offset, 1);
    break;
  case 2:
    orc_mips_emit_lb (compiler, tmp, src->ptr_register, 0);
    orc_mips_emit_lb (compiler, dest->alloc, src->ptr_register, 1);
    orc_mips_emit_replv_qb (compiler, tmp, tmp);
    orc_mips_emit_replv_qb (compiler, dest->alloc, dest->alloc);
    orc_mips_emit_packrl_ph (compiler, dest->alloc, tmp, dest->alloc);
    /* FIXME: should we remove that as we only use ptr_offset for parity? */
    orc_mips_emit_addiu (compiler, src->ptr_offset, src->ptr_offset, 4);
    break;
  default:
    ORC_PROGRAM_ERROR (compiler, "unimplemented");
  }
  src->update_type = 1;
}

void
mips_rule_loadp (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int size = ORC_PTR_TO_INT (user);

  if (src->vartype == ORC_VAR_TYPE_CONST) {
    if (size == 1 || size == 2) {
      orc_mips_emit_ori (compiler, dest->alloc, ORC_MIPS_ZERO, src->value.i);
    } else if (size == 4) {
      orc_int16 high_bits;
      high_bits = ((src->value.i >> 16) & 0xffff);
      if (high_bits) {
        orc_mips_emit_lui (compiler, dest->alloc, high_bits);
        orc_mips_emit_ori (compiler, dest->alloc, dest->alloc, src->value.i & 0xffff);
      } else {
        orc_mips_emit_ori (compiler, dest->alloc, ORC_MIPS_ZERO, src->value.i & 0xffff);
      }
    } else {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
    }
  } else {
    if (size == 1) {
      orc_mips_emit_lb (compiler, dest->alloc, compiler->exec_reg,
                        ORC_MIPS_EXECUTOR_OFFSET_PARAMS(insn->src_args[0]));
    } else if (size == 2) {
      orc_mips_emit_lh (compiler, dest->alloc, compiler->exec_reg,
                        ORC_MIPS_EXECUTOR_OFFSET_PARAMS(insn->src_args[0]));
    } else if (size == 4) {
      orc_mips_emit_lw (compiler, dest->alloc, compiler->exec_reg,
                        ORC_MIPS_EXECUTOR_OFFSET_PARAMS(insn->src_args[0]));
    } else {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
    }
  }
}

void
mips_rule_swapl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (compiler, insn, 0);
  int dest = ORC_DEST_ARG (compiler, insn, 0);

  orc_mips_emit_wsbh (compiler, dest, src);
  orc_mips_emit_packrl_ph (compiler, dest, dest, dest);
}


void
orc_compiler_orc_mips_register_rules (OrcTarget *target)
{
  OrcRuleSet *rule_set;

  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target, 0);

  orc_rule_register (rule_set, "loadl", mips_rule_load, (void *) 2);
  orc_rule_register (rule_set, "loadw", mips_rule_load, (void *) 1);
  orc_rule_register (rule_set, "loadb", mips_rule_load, (void *) 0);
  orc_rule_register (rule_set, "loadpl", mips_rule_loadp, (void *) 4);
  orc_rule_register (rule_set, "loadpw", mips_rule_loadp, (void *) 2);
  orc_rule_register (rule_set, "loadpb", mips_rule_loadp, (void *) 1);
  orc_rule_register (rule_set, "storel", mips_rule_store, (void *)2);
  orc_rule_register (rule_set, "storew", mips_rule_store, (void *)1);
  orc_rule_register (rule_set, "storeb", mips_rule_store, (void *)0);
  orc_rule_register (rule_set, "addl", mips_rule_addl, NULL);
  orc_rule_register (rule_set, "addw", mips_rule_addw, NULL);
  orc_rule_register (rule_set, "addb", mips_rule_addb, NULL);
  orc_rule_register (rule_set, "subb", mips_rule_subb, NULL);
  orc_rule_register (rule_set, "copyl", mips_rule_copyl, NULL);
  orc_rule_register (rule_set, "copyw", mips_rule_copyw, NULL);
  orc_rule_register (rule_set, "copyb", mips_rule_copyb, NULL);
  orc_rule_register (rule_set, "mulswl", mips_rule_mul, NULL);
  orc_rule_register (rule_set, "mullw", mips_rule_mullw, NULL);
  orc_rule_register (rule_set, "shrsl", mips_rule_shrs, NULL);
  orc_rule_register (rule_set, "convssslw", mips_rule_convssslw, NULL);
  orc_rule_register (rule_set, "convssswb", mips_rule_convssswb, NULL);
  orc_rule_register (rule_set, "convsbw", mips_rule_convsbw, NULL);
  orc_rule_register (rule_set, "mergewl", mips_rule_mergewl, NULL);
  orc_rule_register (rule_set, "mergebw", mips_rule_mergebw, NULL);
  orc_rule_register (rule_set, "addssw", mips_rule_addssw, NULL);
  orc_rule_register (rule_set, "subssw", mips_rule_subssw, NULL);
  orc_rule_register (rule_set, "loadupib", mips_rule_loadupib, NULL);
  orc_rule_register (rule_set, "loadupdb", mips_rule_loadupdb, NULL);
  orc_rule_register (rule_set, "shrsw", mips_rule_shrsw, NULL);
  orc_rule_register (rule_set, "swapl", mips_rule_swapl, NULL);
}