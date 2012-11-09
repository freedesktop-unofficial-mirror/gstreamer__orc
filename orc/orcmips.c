#include <orc/orcmips.h>
#include <orc/orcdebug.h>

#define MIPS_IMMEDIATE_INSTRUCTION(opcode,rs,rt,immediate) \
    ((opcode & 0x3f) << 26 \
     |(rs-ORC_GP_REG_BASE) << 21 \
     |(rt-ORC_GP_REG_BASE) << 16 \
     |(immediate & 0xffff))

#define MIPS_BINARY_INSTRUCTION(opcode,rs,rt,rd,sa,function) \
    ((opcode & 0x3f) << 26 \
     | (rs-ORC_GP_REG_BASE) << 21 \
     | (rt-ORC_GP_REG_BASE) << 16 \
     | (rd-ORC_GP_REG_BASE) << 11 \
     | (sa & 0x1f) << 6 \
     | (function & 0x3f))

const char *
orc_mips_reg_name (int reg)
{
  static const char *regs[] = {
    "$0", "$at",
    "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3","$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3","$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9",
    "$k0", "$k1",
    "$gp", "$sp", "$fp", "$ra"
  };

  if (reg < ORC_GP_REG_BASE || reg > ORC_GP_REG_BASE + 32)
    return "ERROR";

  return regs[reg-32];
}

void
orc_mips_emit (OrcCompiler *compiler, orc_uint32 insn)
{
  ORC_WRITE_UINT32_LE (compiler->codeptr, insn);
  compiler->codeptr+=4;
}

void
orc_mips_emit_label (OrcCompiler *compiler, unsigned int label)
{
  ORC_ASSERT (label < ORC_N_LABELS);
  ORC_ASM_CODE(compiler,".L%s%d:\n", compiler->program->name, label);
  compiler->labels[label] = compiler->codeptr;
}

void
orc_mips_add_fixup (OrcCompiler *compiler, int label, int type)
{
  ORC_ASSERT (compiler->n_fixups < ORC_N_FIXUPS);

  compiler->fixups[compiler->n_fixups].ptr = compiler->codeptr;
  compiler->fixups[compiler->n_fixups].label = label;
  compiler->fixups[compiler->n_fixups].type = type;
  compiler->n_fixups++;
}

void
orc_mips_do_fixups (OrcCompiler *compiler)
{
  int i;
  for(i=0;i<compiler->n_fixups;i++){
    /* Type 0 of fixup is a branch label that could not be resolved at first
     * pass. We compute the offset, which should be the 16 least significant
     * bits of the instruction. */
    unsigned char *label = compiler->labels[compiler->fixups[i].label];
    unsigned char *ptr = compiler->fixups[i].ptr;
    orc_uint32 code;
    int offset;
    ORC_ASSERT (compiler->fixups[i].type == 0);
    offset = (label - (ptr + 4)) >> 2;
    code = ORC_READ_UINT32_LE (ptr);
    code |= offset & 0xffff;
    ORC_WRITE_UINT32_LE (ptr, code);
  }
}

void
orc_mips_emit_align (OrcCompiler *compiler, int align_shift)
{
  int diff;

  diff = (compiler->code - compiler->codeptr)&((1<<align_shift) - 1);
  while (diff) {
    orc_mips_emit_nop (compiler);
    diff-=4;
  }
}

void
orc_mips_emit_nop (OrcCompiler *compiler)
{
  ORC_ASM_CODE(compiler,"  nop\n");
  /* We emit "or $at, $at, $0" aka "move $at, $at" for nop, because that's what
   * gnu as does. */
  orc_mips_emit (compiler,
      MIPS_BINARY_INSTRUCTION(0,ORC_MIPS_AT, ORC_MIPS_ZERO, ORC_MIPS_AT,
                              0, 045));
}

void
orc_mips_emit_sw (OrcCompiler *compiler, OrcMipsRegister reg,
                  OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  sw      %s, %d(%s)\n",
                orc_mips_reg_name (reg),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(053, base, reg, offset));
}

void
orc_mips_emit_swr (OrcCompiler *compiler, OrcMipsRegister reg,
                   OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  swr     %s, %d(%s)\n",
                orc_mips_reg_name (reg),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(056, base, reg, offset));
}

void
orc_mips_emit_swl (OrcCompiler *compiler, OrcMipsRegister reg,
                   OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  swl     %s, %d(%s)\n",
                orc_mips_reg_name (reg),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(052, base, reg, offset));
}

void
orc_mips_emit_sh (OrcCompiler *compiler, OrcMipsRegister reg,
                  OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  sh      %s, %d(%s)\n",
                orc_mips_reg_name (reg),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(051, base, reg, offset));
}

void
orc_mips_emit_sb (OrcCompiler *compiler, OrcMipsRegister reg,
                  OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  sb      %s, %d(%s)\n",
                orc_mips_reg_name (reg),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(050, base, reg, offset));
}

void
orc_mips_emit_lw (OrcCompiler *compiler, OrcMipsRegister dest,
                  OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  lw      %s, %d(%s)\n",
                orc_mips_reg_name (dest),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(043, base, dest, offset));
}

void
orc_mips_emit_lwr (OrcCompiler *compiler, OrcMipsRegister dest,
                   OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  lwr     %s, %d(%s)\n",
                orc_mips_reg_name (dest),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(046, base, dest, offset));
}

void
orc_mips_emit_lwl (OrcCompiler *compiler, OrcMipsRegister dest,
                   OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  lwl     %s, %d(%s)\n",
                orc_mips_reg_name (dest),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(042, base, dest, offset));
}

void
orc_mips_emit_lh (OrcCompiler *compiler, OrcMipsRegister dest,
                  OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  lh      %s, %d(%s)\n",
                orc_mips_reg_name (dest),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(041, base, dest, offset));
}

void
orc_mips_emit_lb (OrcCompiler *compiler, OrcMipsRegister dest,
                  OrcMipsRegister base, unsigned int offset)
{
  ORC_ASM_CODE (compiler, "  lb      %s, %d(%s)\n",
                orc_mips_reg_name (dest),
                offset, orc_mips_reg_name (base));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(040, base, dest, offset));
}

void
orc_mips_emit_jr (OrcCompiler *compiler, OrcMipsRegister address_reg)
{
  ORC_ASM_CODE (compiler, "  jr      %s\n", orc_mips_reg_name (address_reg));
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(0, address_reg, ORC_MIPS_ZERO, 010));
}

void
orc_mips_emit_conditional_branch (OrcCompiler *compiler,
                                  int condition,
                                  OrcMipsRegister rs,
                                  OrcMipsRegister rt,
                                  unsigned int label)
{
  int offset;
  char *opcode_name[] = { NULL, NULL, NULL, NULL,
    "beq ",
    "bne ",
    "blez",
    "bgtz"
  };
  switch (condition) {
  case ORC_MIPS_BEQ:
  case ORC_MIPS_BNE:
    ORC_ASM_CODE (compiler, "  %s    %s, %s, .L%s%d\n", opcode_name[condition],
                  orc_mips_reg_name (rs), orc_mips_reg_name (rt),
                  compiler->program->name, label);
    break;
  case ORC_MIPS_BLEZ:
  case ORC_MIPS_BGTZ:
    ORC_ASSERT (rt == ORC_MIPS_ZERO);
    ORC_ASM_CODE (compiler, "  %s    %s, .L%s%d\n", opcode_name[condition],
                  orc_mips_reg_name (rs),
                  compiler->program->name, label);
    break;
  default:
    ORC_PROGRAM_ERROR (compiler, "unknown branch type: 0x%x", condition);
  }
  if (compiler->labels[label]) {
    offset = (compiler->labels[label] - (compiler->codeptr+4)) >> 2;
  } else {
    orc_mips_add_fixup (compiler, label, 0);
    offset = 0;
  }
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(condition, rs, rt, offset));
}

void
orc_mips_emit_conditional_branch_with_offset (OrcCompiler *compiler,
                                              int condition,
                                              OrcMipsRegister rs,
                                              OrcMipsRegister rt,
                                              int offset)
{
  char *opcode_name[] = { NULL, NULL, NULL, NULL,
    "beq ",
    "bne ",
    "blez",
    "bgtz"
  };
  switch (condition) {
  case ORC_MIPS_BEQ:
  case ORC_MIPS_BNE:
    ORC_ASM_CODE (compiler, "  %s    %s, %s, %d\n", opcode_name[condition],
                  orc_mips_reg_name (rs), orc_mips_reg_name (rt), offset);
    break;
  case ORC_MIPS_BLEZ:
  case ORC_MIPS_BGTZ:
    ORC_ASSERT (rt == ORC_MIPS_ZERO);
    ORC_ASM_CODE (compiler, "  %s    %s, %d\n", opcode_name[condition],
                  orc_mips_reg_name (rs), offset);
    break;
  default:
    ORC_PROGRAM_ERROR (compiler, "unknown branch type: 0x%x", condition);
  }

  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(condition, rs, rt, offset>>2));
}

void
orc_mips_emit_addiu (OrcCompiler *compiler,
                     OrcMipsRegister dest, OrcMipsRegister source, int value)
{
  ORC_ASM_CODE (compiler, "  addiu   %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), value);
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(011, source, dest, value));
}

void
orc_mips_emit_addi (OrcCompiler *compiler,
                     OrcMipsRegister dest, OrcMipsRegister source, int value)
{
  ORC_ASM_CODE (compiler, "  addi    %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), value);
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(010, source, dest, value));
}

void
orc_mips_emit_add (OrcCompiler *compiler,
                   OrcMipsRegister dest,
                   OrcMipsRegister source1, OrcMipsRegister source2)
{
  ORC_ASM_CODE (compiler, "  add     %s, %s, %s\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source1),
                orc_mips_reg_name (source2));
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(0, source1, source2, dest, 0, 040));
}

void
orc_mips_emit_addu (OrcCompiler *compiler,
                    OrcMipsRegister dest,
                    OrcMipsRegister source1, OrcMipsRegister source2)
{
  ORC_ASM_CODE (compiler, "  addu    %s, %s, %s\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source1),
                orc_mips_reg_name (source2));
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(0, source1, source2, dest, 0, 041));
}

void
orc_mips_emit_addu_qb (OrcCompiler *compiler,
                    OrcMipsRegister dest,
                    OrcMipsRegister source1, OrcMipsRegister source2)
{
  ORC_ASM_CODE (compiler, "  addu.qb %s, %s, %s\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source1),
                orc_mips_reg_name (source2));
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(037, source1, source2, dest, 0, 020));
}

void
orc_mips_emit_addu_ph (OrcCompiler *compiler,
                    OrcMipsRegister dest,
                    OrcMipsRegister source1, OrcMipsRegister source2)
{
  ORC_ASM_CODE (compiler, "  addu.ph %s, %s, %s\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source1),
                orc_mips_reg_name (source2));
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(037, source1, source2, dest, 010, 020));
}

void
orc_mips_emit_ori (OrcCompiler *compiler,
                     OrcMipsRegister dest, OrcMipsRegister source, int value)
{
  ORC_ASM_CODE (compiler, "  ori     %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), value);
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(015, source, dest, value));
}

void
orc_mips_emit_or (OrcCompiler *compiler,
                   OrcMipsRegister dest,
                   OrcMipsRegister source1, OrcMipsRegister source2)
{
  ORC_ASM_CODE (compiler, "  or      %s, %s, %s\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source1),
                orc_mips_reg_name (source2));
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(0, source1, source2, dest, 0, 045));
}


void
orc_mips_emit_move (OrcCompiler *compiler,
                    OrcMipsRegister dest, OrcMipsRegister source)
{
  orc_mips_emit_add (compiler, dest, source, ORC_MIPS_ZERO);
}

void
orc_mips_emit_sub (OrcCompiler *compiler,
                   OrcMipsRegister dest,
                   OrcMipsRegister source1, OrcMipsRegister source2)
{
  ORC_ASM_CODE (compiler, "  sub     %s, %s, %s\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source1),
                orc_mips_reg_name (source2));
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(0, source1, source2, dest, 0, 042));
}

void
orc_mips_emit_srl (OrcCompiler *compiler,
                     OrcMipsRegister dest, OrcMipsRegister source, int value)
{
  ORC_ASM_CODE (compiler, "  srl     %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), value);
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(0, ORC_MIPS_ZERO, source, dest, value, 02));
}

void
orc_mips_emit_sll (OrcCompiler *compiler,
                     OrcMipsRegister dest, OrcMipsRegister source, int value)
{
  ORC_ASM_CODE (compiler, "  sll     %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), value);
  orc_mips_emit (compiler, MIPS_BINARY_INSTRUCTION(0, ORC_MIPS_ZERO, source, dest, value, 0));
}

void
orc_mips_emit_andi (OrcCompiler *compiler,
                     OrcMipsRegister dest, OrcMipsRegister source, int value)
{
  ORC_ASM_CODE (compiler, "  andi    %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), value);
  orc_mips_emit (compiler, MIPS_IMMEDIATE_INSTRUCTION(014, source, dest, value));
}


void
orc_mips_emit_prepend (OrcCompiler *compiler, OrcMipsRegister dest,
                       OrcMipsRegister source, int shift_amount)
{
  ORC_ASM_CODE (compiler, "  prepend %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), shift_amount);
  orc_mips_emit (compiler, (037 << 26
                            | (source-ORC_GP_REG_BASE) << 21
                            | (dest-ORC_GP_REG_BASE) << 16
                            | shift_amount << 11
                            | 01 << 6 /* prepend */
                            | 061 /* append */));
}

void
orc_mips_emit_append (OrcCompiler *compiler, OrcMipsRegister dest,
                       OrcMipsRegister source, int shift_amount)
{
  ORC_ASM_CODE (compiler, "  append  %s, %s, %d\n",
                orc_mips_reg_name (dest),
                orc_mips_reg_name (source), shift_amount);
  orc_mips_emit (compiler, (037 << 26
                            | (source-ORC_GP_REG_BASE) << 21
                            | (dest-ORC_GP_REG_BASE) << 16
                            | shift_amount << 11
                            | 0 << 6 /* append */
                            | 061 /* append */));
}

