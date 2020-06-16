/*! \file */ 

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <math.h>

#include "helpers.h"
#include "defs.h"
#include "instructions.h"
#include "object.h"
#include "sebo.h"


#define VM_SETTING_REGITERS_COUNT 16
static bool VM_SETTING_SILENT = FALSE;


struct Environment
{
  struct Environment * parent;
  struct ModlObject vartable;
};

struct CallFrame
{
  size_t return_address;
  struct Environment * environment;
};

struct RegisterAccessMonitor
{
  bool read_after_rewrite[VM_SETTING_REGITERS_COUNT];
  size_t last_write_points[VM_SETTING_REGITERS_COUNT];
};

struct VMState
{
  byte * code;

  size_t const max_count_call_stack, max_count_stack, max_count_externals;

  size_t ip, csp, sp;
  size_t efc;

  struct CallFrame  * call_stack;
  struct ModlObject registers[VM_SETTING_REGITERS_COUNT];
  struct ModlObject *stack;
  struct ModlObject (* *external_functions) (struct VMState *);

  struct RegisterAccessMonitor ram;
};


/*! 
 *  \brief Gets the current call data
 *  \param self Virtual machine instance
 *  \return Call stack most recent entry
 */
struct CallFrame vm_get_current_call_frame(struct VMState const * self)
{
  if (NULL == self)
  {
    printf("%s\n", "Trying to get current call frame from NULL object");
    exit(EXIT_FAILURE);
  }
  
  return self->call_stack[self->csp];
}

/*!
 *  \brief Get current environment of virtual machine
 *  \param self Virtual machine instance
 *  \returns Current environment
 */
struct Environment * vm_get_current_environment(struct VMState const * self)
{
  return vm_get_current_call_frame(self).environment;
}

/*!
 * \brief Add external function to VM
 * \param self Virutal machine instance
 * \param ef Function pointer, returning ModlObject*
 * \return Identifier of the function
 */
uint64_t vm_add_external_function(struct VMState * self, struct ModlObject (*ef) (struct VMState *))
{
  if (NULL == self)
  {
    printf("%s\n", "Trying to add external function to NULL object");
    exit(EXIT_FAILURE);
  }
  
  if (self->efc + 1 > self->max_count_externals)
  {
    printf(
      "\x1b[31;1m  %s: external_functions_max_count=%lu\x1b[0m\n",
      "  maximum external functions count exceeded",
      self->max_count_externals
    );
    exit(EXIT_FAILURE);
  }

  self->external_functions[self->efc] = ef;
  return self->efc++;
}

/*!
 * \brief Get external function by id
 * \param self Virtual machine instance
 * \param id Identifier
 * \return Function pointer, returning ModlObject*
 */
struct ModlObject (* vm_get_external_function(struct VMState const * const self, uint64_t id)) (struct VMState *)
{
  if (id >= self->efc)
  {
    printf(
      "\x1b[31;1m  %s: id=%lu\x1b[0m\n",
      "  external function with given id does not exist",
      id
    );
    exit(EXIT_FAILURE);
  }

  return self->external_functions[id];
}

/*!
 * \brief Get external function by function pointer
 * \param self Virutal machine instance
 * \param f Function pointer, returning ModlObject*
 * \return External function as ModlObject*
 */
struct ModlObject vm_find_external_function(struct VMState const * const self, struct ModlObject (*f) (struct VMState *))
{
  uint64_t id = 0;
  while (id < self->efc && self->external_functions[id] != f) ++id;

  if (id >= self->efc)
  {
    printf(
      "\x1b[31;1m  %s\x1b[0m\n",
      "  given external function is not registered"
    );
    exit(EXIT_FAILURE);
  }

  return efun_to_modl(id);
}

static struct Sebo *decoded_sebo_table;

/*!
 *  \brief Decode instruction
 */
static struct Instruction decode_instruction(struct VMState * state)
{
  enum ModlOpcode opcode = state->code[state->ip];
  struct Instruction instruction = { .opcode = opcode };
  struct InstructionParametersTemplate template = instruction_parameters_templates[opcode];
  if (TP_ERROR == template.p[0])
  {
    char const * name = instructions_names_table[opcode];
    if (NULL == name) name = "#???#";
    printf("\x1b[31;1mfailed to decode instruction: %s\x1b[0m\n", name);
    exit(EXIT_FAILURE);
  }

  size_t offset = 1;
  for (byte i = 0; i < INSTRUCTION_TEMPLATE_VALUES_COUNT; ++i)
  {
    switch (template.p[i])
    {
      case TP_ERROR: case TP_EMPTY: break;

      case TP_REGAL:
      {
        instruction.a[i].r[0] = state->code[state->ip + offset] & 0xF;
        offset += 1;
      } break;

      case TP_REGSP:
      {
        instruction.a[i].r[0] = (state->code[state->ip + offset] >> 4) & 0xF;
        instruction.a[i].r[1] = (state->code[state->ip + offset] >> 0) & 0xF;
        offset += 1;
      } break;

      case TP_INT64:
      {
        instruction.a[i].i64 = ((int64_t) 0)
          | (((int64_t) state->code[state->ip + offset + 0]) << 8*7)
          | (((int64_t) state->code[state->ip + offset + 1]) << 8*6)
          | (((int64_t) state->code[state->ip + offset + 2]) << 8*5)
          | (((int64_t) state->code[state->ip + offset + 3]) << 8*4)
          | (((int64_t) state->code[state->ip + offset + 4]) << 8*3)
          | (((int64_t) state->code[state->ip + offset + 5]) << 8*2)
          | (((int64_t) state->code[state->ip + offset + 6]) << 8*1)
          | (((int64_t) state->code[state->ip + offset + 7]) << 8*0);
        offset += 8;
      } break;

      case TP_SEBO:
      {
        if (ModlTypeNil != decoded_sebo_table[state->ip + offset].object.type)
        {
          instruction.a[i].object = decoded_sebo_table[state->ip + offset].object;
          offset += decoded_sebo_table[state->ip + offset].byte_length;
          instruction.byte_length = offset;
          return instruction;
        }

        struct Sebo data = modl_decode_sebo(&state->code[state->ip] + offset);
        if (data.object.type != ModlTypeTable)
        {
          // memcpy(&decoded_sebo_table[state->ip + offset], &data, sizeof (struct Sebo));
          decoded_sebo_table[state->ip + offset] = data;
          modl_object_take(data.object);
        }

        instruction.a[i].object = data.object;
        offset += data.byte_length;
      } break;
    }
  }

  instruction.byte_length = offset;
  return instruction;
}

static void instruction_display(struct VMState * vm, struct Instruction instruction)
{
  printf("\x1b[36m%-8s\x1b[0m", instructions_names_table[instruction.opcode]);
  struct InstructionParametersTemplate template = instruction_parameters_templates[instruction.opcode];

  if (template.p[0] != TP_EMPTY)
    printf("%c", ' ');

  for (byte i = 0; i < INSTRUCTION_TEMPLATE_VALUES_COUNT && template.p[i] > TP_EMPTY; ++i)
  {
    if (i > 0) printf("%s", ", ");
    switch (template.p[i])
    {
      case TP_REGAL:
        printf("\x1b[37;1mR%d\x1b[0m", instruction.a[i].r[0]);
        printf("%c", '{');
        modl_object_display(&vm->registers[instruction.a[i].r[0]]);
        printf("%c", '}');
        break;
      case TP_REGSP:
        printf("\x1b[37;1mR%d\x1b[0m", instruction.a[i].r[0]);
        printf("%c", '{');
        modl_object_display(&vm->registers[instruction.a[i].r[0]]);
        printf("%c", '}');
        printf(", \x1b[37;1mR%d\x1b[0m", instruction.a[i].r[1]);
        printf("%c", '{');
        modl_object_display(&vm->registers[instruction.a[i].r[1]]);
        printf("%c", '}');
        break;
      case TP_INT64:
        printf("[%" PRId64 "]", instruction.a[i].i64);
        break;
      case TP_SEBO:
        modl_object_display(&instruction.a[i].object);
        break;

      case TP_ERROR: case TP_EMPTY: /* unreachable */ break;
    }
  }

  printf("%c", '\n');
}

void instruction_release(struct Instruction instruction)
{
  struct InstructionParametersTemplate template = instruction_parameters_templates[instruction.opcode];
  for (byte i = 0; i < INSTRUCTION_TEMPLATE_VALUES_COUNT; ++i)
  {
    switch (template.p[i])
    {
      case TP_SEBO:
        // printf("%s\n", "  releasing instruction's temporary object");
        modl_object_release_tmp(instruction.a[i].object);
      default: break;
    }
  }
}


struct ModlObject vm_reg_read(struct VMState * state, byte r)
{
  state->ram.read_after_rewrite[r] = TRUE;
  return state->registers[r];
}

void vm_reg_write(struct VMState * state, byte r, struct ModlObject val)
{
  if (not VM_SETTING_SILENT)
  {
    if (not state->ram.read_after_rewrite[r])
    {
      printf(
        "\x1b[33;1m%s\x1b[0m[%04lx]\n",
        "  register value rewritten without previous reads at: ",
        state->ram.last_write_points[r]
      );
    }

    state->ram.read_after_rewrite[r] = FALSE;
    state->ram.last_write_points[r] = state->ip;
  }

  if (modl_object_equals(val, state->registers[r])) return;

  modl_object_release(state->registers[r]);
  state->registers[r] = modl_object_take(val);
}

void vm_reg_write_i(struct VMState * const state, byte r, int64_t val)
{
  // if (not modl_object_is_single(&state->registers[r]))
  return vm_reg_write(state, r, int_to_modl(val));

  // if (not VM_SETTING_SILENT)
  // {
  //   if (not state->ram.read_after_rewrite[r])
  //   {
  //     printf(
  //       "\x1b[33;1m%s\x1b[0m[%04lx]\n",
  //       "  register value rewritten without previous reads at: ",
  //       state->ram.last_write_points[r]
  //     );
  //   }

  //   state->ram.read_after_rewrite[r] = FALSE;
  //   state->ram.last_write_points[r] = state->ip;
  // }
  
  // /* If we are here then it means that the object in register has no more instances */
  // state->registers[r]->type = ModlTypeInteger;
  // state->registers[r]->value.integer = val;
}


struct ModlObject run(struct VMState * state);
struct ModlObject vm_call_function(struct VMState * state, struct ModlObject obj)
{
  if (ModlTypeFunction != obj.type)
  {
    printf("\x1b[31;1m  Object of type %s is not callable\x1b[0m\n", modl_types_names_table[obj.type]);
    exit(EXIT_FAILURE);
  }

  if (state->csp + 1 >= state->max_count_call_stack)
  {
    printf(
      "\x1b[31;1m  %s: call_stack_max_size=%lu\x1b[0m\n",
      "  maximum call stack size exceeded",
      state->max_count_call_stack
    );
    exit(EXIT_FAILURE);
  }

  struct Environment * env = calloc(1, sizeof (struct Environment));
  env->parent = obj.value.ref->value.fun.context;
  env->vartable = modl_table_new();
  state->call_stack[++state->csp] = (struct CallFrame) {
    .return_address = state->ip,
    .environment = env,
  };

  struct ModlObject ret;

  if (obj.value.ref->value.fun.is_external)
  {
    ret = vm_get_external_function(state, obj.value.ref->value.fun.position)(state);
    vm_reg_write(state, REG(0), ret);
  }
  else
  {
    state->ip = obj.value.ref->value.fun.position;
    ret = run(state);
  }

  modl_object_release(env->vartable);
  free(env);
  state->ip = state->call_stack[state->csp].return_address;
  state->csp -= 1;

  modl_object_release_tmp(obj);
  return ret;
}


static struct ModlObject modl_std_concat_strings(struct VMState * vm);
struct ModlObject run(struct VMState * state)
{
  while (TRUE)
  {
    if (not VM_SETTING_SILENT)
    {
      // printf("%s", "Registers: [ ");
      // for (size_t i = 0; i < VM_SETTING_REGITERS_COUNT; ++i)
      // {
      //   modl_object_display(state->registers[i]);
      //   printf("%c", ' ');
      // }
      // printf("%c\n", ']');
      //
      // printf("Stack(%lu): [ ", state->sp);
      // for (size_t i = 0; i < state->sp; ++i)
      // {
      //   modl_object_display(state->stack[i]);
      //   printf("%c", ' ');
      // }
      // printf("%c\n", ']');
      //
      // printf("%s", "Environment:");
      // struct Environment * env = state->call_stack[state->csp].environment;
      // while (NULL != env)
      // {
      //   printf("%c", ' ');
      //   modl_object_display(env->vartable);
      //   env = env->parent;
      // }
      // printf("%c", '\n');
    }

    if (not VM_SETTING_SILENT) printf("[%04lx] ", state->ip);
    struct Instruction instruction = decode_instruction(state);
    if (not VM_SETTING_SILENT) instruction_display(state, instruction);

    switch (instruction.opcode)
    {
      case OP_NOP: break;

      case OP_RET:
      {
        // modl_object_display(vm_get_current_call_frame(state).environment->vartable);
        return vm_reg_read(state, REG(0));
      } break;

      case OP_MOV:
      {
        vm_reg_write(state, instruction.a[0].r[0], vm_reg_read(state, instruction.a[0].r[1]));
      } break;

      case OP_LOADC:
      {
        vm_reg_write(state, instruction.a[0].r[0], instruction.a[1].object);
      } break;

      case OP_TBLGETR:
      {
        byte const reg_tbl = instruction.a[0].r[0];
        byte const reg_name = instruction.a[0].r[1];

        struct ModlObject obj_tbl = vm_reg_read(state, reg_tbl);
        struct ModlObject obj_name = vm_reg_read(state, reg_name);

        vm_reg_write(
          state, reg_tbl,
          modl_table_get_v(&obj_tbl, obj_name)
        );
      } break;

      case OP_CALLR:
      {
        struct ModlObject obj = vm_reg_read(state, instruction.a[0].r[0]);
        vm_call_function(state, obj);
      } break;

      case OP_LOADFUN:
      {
        struct Environment * env = state->call_stack[state->csp].environment;
        vm_reg_write(state, instruction.a[0].r[0], ifun_to_modl(env, state->ip + instruction.a[1].i64));
      } break;

      case OP_JMP: state->ip += instruction.a[0].i64 - instruction.byte_length; break;

      case OP_ROL:
      case OP_ROR:
      case OP_IDIV:
      case OP_ADD:
      case OP_SUB:
      case OP_MUL:
      case OP_DIV:
      case OP_MOD:
      case OP_AND:
      case OP_OR:
      case OP_XOR:
      case OP_NAND:
      case OP_NOR:
      case OP_NXOR:
      case OP_CMPLT:
      case OP_CMPNLT:
      case OP_CMPGT:
      case OP_CMPNGT:
      case OP_CMPLE:
      case OP_CMPNLE:
      case OP_CMPGE:
      case OP_CMPNGE:
      {
        byte const reg_dst = instruction.a[0].r[0];
        byte const reg_src = instruction.a[0].r[1];

        struct ModlObject obj_l = modl_object_disown(vm_reg_read(state, reg_dst));
        state->registers[reg_dst] = modl_nil();
        struct ModlObject obj_r = vm_reg_read(state, reg_src);

        if (modl_object_type_is(obj_l, ModlTypeFloating))
        {
          obj_r = modl_maybe_cast(obj_r, ModlTypeFloating);
        }
        if (modl_object_type_is(obj_r, ModlTypeFloating))
        {
          obj_l = modl_maybe_cast(obj_l, ModlTypeFloating);
        }

        if (obj_l.type != obj_r.type)
        {
          printf(
            "\x1b[31;1m  Values are required to have the same type: %s <> %s\x1b[0m\n",
            modl_types_names_table[obj_l.type],
            modl_types_names_table[obj_r.type]
          );
          exit(EXIT_FAILURE);
        }

        if (ModlTypeString == obj_l.type && instruction.opcode == OP_ADD)
        {
          state->stack[state->sp++] = obj_r;
          state->stack[state->sp++] = obj_l;
          vm_call_function(state, vm_find_external_function(state, modl_std_concat_strings));
          break;
        }

        if (ModlTypeInteger != obj_l.type && ModlTypeFloating != obj_l.type)
        {
          printf(
            "\x1b[31;1m  This operation requires operands of integer or floating types: %s <> %s/%s\x1b[0m\n",
            modl_types_names_table[obj_l.type],
            modl_types_names_table[ModlTypeInteger],
            modl_types_names_table[ModlTypeFloating]
          );
          exit(EXIT_FAILURE);
        }

        switch (ModlTypeInteger == obj_l.type)
        {
          case TRUE: switch (instruction.opcode)
          {
            case OP_ROL: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) << modl_to_int(obj_r)); break;
            case OP_ROR: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) >> modl_to_int(obj_r)); break;
            case OP_IDIV: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) / modl_to_int(obj_r)); break;
            case OP_ADD: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) + modl_to_int(obj_r)); break;
            case OP_SUB: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) - modl_to_int(obj_r)); break;
            case OP_MUL: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) * modl_to_int(obj_r)); break;
            case OP_DIV: vm_reg_write(state, reg_dst, double_to_modl((double)modl_to_int(obj_l) / (double)modl_to_int(obj_r))); break;
            case OP_MOD: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) % modl_to_int(obj_r)); break;
            case OP_AND: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) & modl_to_int(obj_r)); break;
            case OP_OR: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) | modl_to_int(obj_r)); break;
            case OP_XOR: vm_reg_write_i(state, reg_dst, modl_to_int(obj_l) ^ modl_to_int(obj_r)); break;
            case OP_NAND: vm_reg_write_i(state, reg_dst, ~(modl_to_int(obj_l) & modl_to_int(obj_r))); break;
            case OP_NOR: vm_reg_write_i(state, reg_dst, ~(modl_to_int(obj_l) | modl_to_int(obj_r))); break;
            case OP_NXOR: vm_reg_write_i(state, reg_dst, ~(modl_to_int(obj_l) ^ modl_to_int(obj_r))); break;
            case OP_CMPLT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) < modl_to_int(obj_r))); break;
            case OP_CMPNLT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) < modl_to_int(obj_r)))); break;
            case OP_CMPGT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) > modl_to_int(obj_r))); break;
            case OP_CMPNGT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) > modl_to_int(obj_r)))); break;
            case OP_CMPLE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) <= modl_to_int(obj_r))); break;
            case OP_CMPNLE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) <= modl_to_int(obj_r)))); break;
            case OP_CMPGE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) >= modl_to_int(obj_r))); break;
            case OP_CMPNGE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) >= modl_to_int(obj_r)))); break;
            default: break;
          } break;

          case FALSE: switch (instruction.opcode)
          {
            case OP_ADD: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) + modl_to_double(obj_r))); break;
            case OP_SUB: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) - modl_to_double(obj_r))); break;
            case OP_MUL: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) * modl_to_double(obj_r))); break;
            case OP_DIV: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) / modl_to_double(obj_r))); break;
            case OP_MOD: vm_reg_write(state, reg_dst, double_to_modl(fmod(modl_to_double(obj_l), modl_to_double(obj_r)))); break;
            // case OP_AND: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) & modl_to_double(obj_r))); break;
            // case OP_OR: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) | modl_to_double(obj_r))); break;
            // case OP_XOR: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) ^ modl_to_double(obj_r))); break;
            // case OP_NAND: vm_reg_write(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) & modl_to_double(obj_r)))); break;
            // case OP_NOR: vm_reg_write(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) | modl_to_double(obj_r)))); break;
            // case OP_NXOR: vm_reg_write(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) ^ modl_to_double(obj_r)))); break;
            case OP_CMPLT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) < modl_to_double(obj_r))); break;
            case OP_CMPNLT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) < modl_to_double(obj_r)))); break;
            case OP_CMPGT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) > modl_to_double(obj_r))); break;
            case OP_CMPNGT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) > modl_to_double(obj_r)))); break;
            case OP_CMPLE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) <= modl_to_double(obj_r))); break;
            case OP_CMPNLE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) <= modl_to_double(obj_r)))); break;
            case OP_CMPGE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) >= modl_to_double(obj_r))); break;
            case OP_CMPNGE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) >= modl_to_double(obj_r)))); break;
            default:
            {
              printf("%s\n", "\x1b[31;1m  this operation is not supported on floats\x1b[0m");
              exit(EXIT_FAILURE);
            } break;
          } break;
        }
      } break;

      case OP_CMPEQ:
      case OP_CMPNEQ:
      {
        vm_reg_write(state, instruction.a[0].r[0], bool_to_modl(modl_object_equals(
          vm_reg_read(state, instruction.a[0].r[0]),
          vm_reg_read(state, instruction.a[0].r[1])
        ) == (instruction.opcode == OP_CMPEQ)));
      } break;


      case OP_JCF:
      case OP_JCT:
      {
        if (modl_to_bool(vm_reg_read(state, instruction.a[0].r[0])) == (instruction.opcode == OP_JCT))
          state->ip += instruction.a[1].i64 - instruction.byte_length;
      } break;

      case OP_POP:
      {
        if (0 == state->sp)
        {
          printf("\x1b[31;1m  Cannot pop from empty stack\x1b[0m\n");
          exit(EXIT_FAILURE);
        }

        state->sp -= 1;
        vm_reg_write(state, instruction.a[0].r[0], modl_object_disown(state->stack[state->sp]));
      } break;

      case OP_PUSH:
      {
        if (state->sp + 1 > state->max_count_stack)
        {
          printf(
            "\x1b[31;1m%s: stack_max_size=%lu\x1b[0m\n",
            "  maximum stack size exceeded",
            state->max_count_stack
          );
          exit(EXIT_FAILURE);
        }

        state->stack[state->sp++] = modl_object_take(vm_reg_read(state, instruction.a[0].r[0]));
      } break;

      case OP_TBLPUSH:
      {
        // TODO: Use normal objects instead of pointers!!!!
        struct ModlObject tmp = vm_reg_read(state, instruction.a[0].r[0]);
        modl_table_push_v(
          &tmp,
          vm_reg_read(state, instruction.a[0].r[1])
        );
      } break;

      case OP_TBLSETR:
      {
        struct ModlObject tmp = vm_reg_read(state, instruction.a[0].r[0]);
        modl_table_insert_kv(
          &tmp,
          vm_reg_read(state, instruction.a[0].r[1]),
          vm_reg_read(state, instruction.a[1].r[0])
        );
      } break;

      case OP_ENVGETC:
      {
        struct Environment * env = state->call_stack[state->csp].environment;
        // modl_object_display(state->call_stack[state->csp].environment->vartable);
        // printf("%c", '\n');
        
        while (NULL != env)
        {
          if (modl_table_has_k(&env->vartable, instruction.a[1].object))
          {
            vm_reg_write(
              state, instruction.a[0].r[0],
              modl_table_get_v(
                &env->vartable,
                instruction.a[1].object
              )
            );
            goto CASE_OP_ENVGETC_END;
          }
          env = env->parent;
        }
        vm_reg_write(state, instruction.a[0].r[0], modl_nil());
        CASE_OP_ENVGETC_END: break;
      } break;

      case OP_ENVSETC:
      {
        modl_table_insert_kv(
          &state->call_stack[state->csp].environment->vartable,
          instruction.a[1].object,
          vm_reg_read(state, instruction.a[0].r[0])
        );
        // modl_object_display(state->call_stack[state->csp].environment->vartable);
        // printf("%c", '\n');
      } break;

      case OP_ENVUPKC:
      {
        byte const reg_val = instruction.a[0].r[0];
        byte const reg_arg = instruction.a[0].r[1];

        struct ModlObject obj_vals = vm_reg_read(state, reg_val);
        struct ModlObject obj_args = vm_reg_read(state, reg_arg);

        struct ModlObject index = int_to_modl(0);
        while (modl_table_has_k(&obj_vals, index) && modl_table_has_k(&obj_args, index))
        {
          modl_table_insert_kv(
            &vm_get_current_environment(state)->vartable,
            modl_table_get_v(&obj_args, index),
            modl_table_get_v(&obj_vals, index)
          );
          index.value.integer += 1;
        }

        modl_object_release_tmp(index);
      } break;

      case OP_ENVPUSH:
      {

      } break;

      case OP_NOT:
      {
        struct ModlObject obj = vm_reg_read(state, instruction.a[0].r[0]);
        vm_reg_write(state, instruction.a[0].r[0], bool_to_modl(!modl_to_bool(obj)));
      } break;

      case OP_INV:
      {
        struct ModlObject obj = vm_reg_read(state, instruction.a[0].r[0]);
        vm_reg_write_i(state, instruction.a[0].r[0], ~modl_to_int(obj));
      } break;

      case OP_LEN:
      {
        struct ModlObject obj = vm_reg_read(state, instruction.a[0].r[0]);
        int64_t length = 0;
        // Need to throw an error if type can not be taken length of
        if (obj.type == ModlTypeTable)
        {
          struct ModlObject key = int_to_modl(0);
          while (modl_table_has_k(&obj, key))
          {
            ++length;
            ++key.value.integer;
          }
        }
        else if (obj.type == ModlTypeString)
        {
          length = strlen(obj.value.ref->value.string);
        }
        else
        {
          printf(
            "\x1b[31;1m  Length of object of type %s cannot be taken\x1b[0m\n",
            modl_types_names_table[obj.type]
          );
          exit(EXIT_FAILURE);
        }

        vm_reg_write_i(state, instruction.a[0].r[0], length);
      } break;

      default:
      {
        printf("\x1b[31;1m  Instruction implementation not found\x1b[0m\n");
        exit(EXIT_FAILURE);
      } break;
    }

    state->ip += instruction.byte_length;
    instruction_release(instruction);
  }
}


struct BytecodeCompiler
{
  byte* data;
  size_t offset;
};


size_t emit(struct BytecodeCompiler * compiler, byte value)
{
  size_t addr = compiler->offset;
  compiler->data[compiler->offset++] = value;
  return addr;
}

size_t emit_modl(struct BytecodeCompiler * compiler, struct ModlObject * object)
{
  struct Sebo repr = modl_encode_sebo(*object);
  if (0 == repr.byte_length)
  {
    printf("%s\n", "Cannot encode NULL object");
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < repr.byte_length; ++i)
    emit(compiler, repr.data[i]);

  return repr.byte_length;
}


/* LEAK-FREE */
static struct ModlObject modl_std_print(struct VMState * vm)
{
  modl_object_display(&vm->stack[--vm->sp]);
  printf("%c", '\n');
  modl_object_release(vm->stack[vm->sp]);
  return modl_nil();
}

/* LEAK-FREE */
static struct ModlObject modl_std_concat_strings(struct VMState * vm)
{
  struct ModlObject a = vm->stack[--vm->sp];
  struct ModlObject b = vm->stack[--vm->sp];

  if (ModlTypeString != a.type || ModlTypeString != b.type)
  {
    perror("std_concat_strings expected integer parameters");
    exit(EXIT_FAILURE);
  }

  char res[strlen(a.value.ref->value.string) + strlen(b.value.ref->value.string) + 1];
  strcpy(res, a.value.ref->value.string);
  strcat(res, b.value.ref->value.string);

  modl_object_release(a);
  modl_object_release(b);

  return str_to_modl(res);
}

/* LEAK-FREE */
static struct ModlObject modl_std_to_string(struct VMState * vm)
{
  struct ModlObject a = vm->stack[--vm->sp];

  switch (a.type)
  {
    case ModlTypeNil: return str_to_modl("nil");
    
    case ModlTypeBoolean:
    {
      bool val = modl_to_bool(a);
      modl_object_release(a);
      return str_to_modl(val ? "true" : "false");
    }

    case ModlTypeInteger:
    {
      char res[21];
      sprintf(res, "%ld", modl_to_int(a));
      modl_object_release(a);
      return str_to_modl(res);
    }

    case ModlTypeFloating:
    {
      char res[64];
      sprintf(res, "%.17g", modl_to_double(a));
      modl_object_release(a);
      return str_to_modl(res);
    }

    case ModlTypeString:
    {
      return modl_object_disown(a);
    }

    default: modl_object_release(a); return modl_nil();
  }
}

static struct ModlObject modl_std_string_substring(struct VMState * vm)
{
  struct ModlObject str = vm->stack[--vm->sp];
  struct ModlObject from = vm->stack[--vm->sp];
  struct ModlObject length = vm->stack[--vm->sp];
  char * csubstr = malloc((length.value.integer + 1) * sizeof (char));
  
  memcpy(csubstr, str.value.ref->value.string + from.value.integer, length.value.integer);
  csubstr[length.value.integer] = '\0';

  modl_object_release(from);
  modl_object_release(length);
  modl_object_release(str);
  return transfer_str_to_modl(csubstr);
}

static struct ModlObject modl_std_string_to_array(struct VMState * vm)
{
  struct ModlObject str = vm->stack[--vm->sp];
  struct ModlObject arr = modl_table();
  char const * c = str.value.ref->value.string;
  char csc[2] = {0, 0};

  while ((*csc = *c++))
    modl_table_push_v(&arr, int_to_modl(*csc));

  modl_object_release(str);
  return arr;
}

static struct ModlObject modl_std_string_from_array(struct VMState * vm)
{
  struct ModlObject arr = vm->stack[--vm->sp];

  char * cstr = malloc(arr.value.ref->value.table.size + 1);
  char * cc = cstr;
  
  struct ModlObject key = int_to_modl(0);
  while (modl_table_has_k(&arr, key))
  {
    *cc++ = (char) modl_to_int(modl_table_get_v(&arr, key));
    key.value.integer += 1;
  }
  *cc = '\0';

  modl_object_release(arr);
  return transfer_str_to_modl(cstr);
}

/*
static struct ModlObject * modl_std_table_keys(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];
  struct ModlObject * ret = modl_table();

  {
    struct ModlTableNode * node = table->value.table.integer_nodes;
    while (NULL != node->key)
    {
      modl_table_push_v(ret, (struct ModlObject *) node->key);
      node = node->next;
    }
  }

  {
    struct ModlTableNode * node = table->value.table.string_nodes;
    while (NULL != node->key)
    {
      modl_table_push_v(ret, (struct ModlObject *) node->key);
      node = node->next;
    }
  }

  modl_object_release(table);
  return ret;
}

static struct ModlObject * modl_std_table_size(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];
  int64_t count = 0;

  {
    struct ModlTableNode const * node = table->value.table.integer_nodes;
    while (NULL != node->key)
    {
      count += 1;
      node = node->next;
    }
  }

  {
    struct ModlTableNode const * node = table->value.table.string_nodes;
    while (NULL != node->key)
    {
      count += 1;
      node = node->next;
    }
  }

  modl_object_release(table);
  return int_to_modl(count);
}

static struct ModlObject * modl_std_table_empty(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];

  {
    struct ModlTableNode * node = table->value.table.integer_nodes;
    if (NULL != node->key)
    {
      modl_object_release(table);
      return bool_to_modl(FALSE);
    }
  }

  {
    struct ModlTableNode * node = table->value.table.string_nodes;
    if (NULL != node->key)
    {
      modl_object_release(table);
      return bool_to_modl(FALSE);
    }
  }

  modl_object_release(table);
  return bool_to_modl(TRUE);
}

static struct ModlObject * modl_std_table_pop(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];

  if (NULL == table->value.table.last_consecutive_integer_node)
  {
    modl_object_release(table);
    return modl_nil();
  }

  struct ModlTableNode * ret_node = table->value.table.last_consecutive_integer_node;

  struct ModlTableNode * pred = NULL;
  {
    struct ModlTableNode * node = table->value.table.integer_nodes;
    while (NULL != node->key && node != ret_node)
    {
      if (node->next == ret_node)
      {
        pred = node;
        break;
      }
      node = node->next;
    }
  }

  if (NULL == pred)
  {
    table->value.table.integer_nodes = ret_node->next;
  }
  else
  {
    pred->next = ret_node->next;
  }

  if (ret_node->key->value.integer == 0)
  {
    table->value.table.last_consecutive_integer_node = NULL;
  }
  else
  {
    table->value.table.last_consecutive_integer_node = pred;
  }

  modl_object_release((struct ModlObject *) ret_node->key);
  modl_object_release(table);
  return modl_object_disown(ret_node->value);
}

static struct ModlObject * modl_std_table_zip(struct VMState * vm)
{
  struct ModlObject * kt = vm->stack[--vm->sp];
  struct ModlObject * vt = vm->stack[--vm->sp];

  struct ModlObject * rt = modl_table_new();

  if (NULL != kt->value.table.last_consecutive_integer_node)
  if (NULL != vt->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode * nodeK = kt->value.table.integer_nodes;
    struct ModlTableNode * nodeV = vt->value.table.integer_nodes;
    while (0 != nodeK->key->value.integer)
      nodeK = nodeK->next;
    while (0 != nodeV->key->value.integer)
      nodeV = nodeV->next;

    while (nodeK != kt->value.table.last_consecutive_integer_node->next
        && nodeV != vt->value.table.last_consecutive_integer_node->next)
    {
      modl_table_insert_kv(rt, nodeK->value, nodeV->value);
      nodeK = nodeK->next;
      nodeV = nodeV->next;
    }
  }

  modl_object_release(kt);
  modl_object_release(vt);
  return modl_object_disown(rt);
}

static struct ModlObject * modl_std_array_foreach(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * func = vm->stack[--vm->sp];

  if (NULL != array->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      vm->stack[vm->sp++] = modl_object_take(node->value);
      vm_call_function(vm, func);
      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(func);
  return modl_nil();
}

static struct ModlObject * modl_std_array_map(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * func = vm->stack[--vm->sp];

  struct ModlObject * ret = modl_table_new();

  if (NULL != array->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      vm->stack[vm->sp++] = modl_object_take(node->value);
      vm_call_function(vm, func);
      modl_table_push_v(ret, vm_reg_read(vm, REG(0)));
      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(func);
  return modl_object_disown(ret);
}

static struct ModlObject * modl_std_array_concat(struct VMState * vm)
{
  struct ModlObject * a = vm->stack[--vm->sp];
  struct ModlObject * b = vm->stack[--vm->sp];

  struct ModlObject * rt = modl_table_new();

  if (NULL != a->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode const * node = a->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != a->value.table.last_consecutive_integer_node->next)
    {
      modl_table_push_v(rt, node->value);
      node = node->next;
    }
  }

  if (NULL != b->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode const * node = b->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != b->value.table.last_consecutive_integer_node->next)
    {
      modl_table_push_v(rt, node->value);
      node = node->next;
    }
  }

  modl_object_release(a);
  modl_object_release(b);
  return modl_object_disown(rt);
}

static struct ModlObject * modl_std_array_contains(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * obj = vm->stack[--vm->sp];

  if (NULL == array->value.table.last_consecutive_integer_node)
  {
    modl_object_release(array);
    modl_object_release(obj);
    return bool_to_modl(FALSE);
  }

  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      if (modl_object_equals(obj, node->value))
      {
        modl_object_release(array);
        modl_object_release(obj);
        return bool_to_modl(TRUE);
      }

      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(obj);
  return bool_to_modl(FALSE);
}

static struct ModlObject * modl_std_array_map_flat(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * func = vm->stack[--vm->sp];

  struct ModlObject * ret = modl_table_new();

  if (NULL == array->value.table.last_consecutive_integer_node)
  {
    modl_object_release(array);
    modl_object_release(func);
    return modl_object_disown(ret);
  }

  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      vm->stack[vm->sp++] = modl_object_take(node->value);
      vm_call_function(vm, func);

      vm->stack[vm->sp++] = modl_object_take(vm_reg_read(vm, REG(0)));
      vm->stack[vm->sp++] = ret;
      vm_call_function(vm, vm_find_external_function(vm, modl_std_array_concat));
      ret = modl_object_take(vm_reg_read(vm, REG(0)));
      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(func);
  return modl_object_disown(ret);
}
*/


#define INPUT_SIZE 1024*1024*1
int main(int argc, char *argv[])
{
  int opt, long_index;

  byte input[INPUT_SIZE] = { [0 ... (INPUT_SIZE - 1)] = OP_RET };
  size_t input_length = 0;
  size_t max_count_call_stack = 64;
  size_t max_count_stack = 128;
  size_t max_count_externals = 128;
  double time_spent = 0.0;

  clock_t begin = clock();

  static struct option long_options[] = {
      {"stack_size",      required_argument, 0,  's' },
      {"call_stack_size", required_argument, 0,  'c' },
      {"silent",          no_argument,       0,  'l' },
      {0,                 0,                 0,  0   }
  };

  while((opt = getopt_long(argc, argv, ":i:f:s:cl", long_options, &long_index)) != -1)
  {
    switch(opt)
    {
      case 'i':
      {
        while (*optarg) switch(*optarg)
        {
          case '\0': break;
          case '\\': switch (*++optarg)
            {
                case 'a': input[input_length++] = '\a'; ++optarg; break;
                case 'b': input[input_length++] = '\b'; ++optarg; break;
                case 'f': input[input_length++] = '\f'; ++optarg; break;
                case 'n': input[input_length++] = '\n'; ++optarg; break;
                case 'r': input[input_length++] = '\r'; ++optarg; break;
                case 't': input[input_length++] = '\t'; ++optarg; break;
                case 's': input[input_length++] =  ' '; ++optarg; break;
                case '\\': input[input_length++] = '\\'; ++optarg; break;

                case '0': input[input_length++] = '\0'; ++optarg; break;

                case 'x': input[input_length++] = ((unsigned char)(decode_arbitrary_char(optarg[1]) << 4)
                                                    | decode_arbitrary_char(optarg[2])); optarg += 3; break;

                default: input[input_length++] = *optarg++; break;
            } break;
          default: input[input_length++] = *optarg++; break;
        }
      } break;

      case 'f':
      {
        FILE *fp;
        printf("Opening `%s`\n", optarg);
        fp = fopen(optarg, "rb");
        if (!fp) return (perror(optarg), EXIT_FAILURE);
        fseek(fp, 0, SEEK_END);
        input_length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        printf("File length: %ld\n", input_length);
        if (fread(input, 1, input_length, fp) <= 0)
        {
          printf("cannot read from file!\n");
          exit(EXIT_FAILURE);
        }
        fclose(fp);
        input[input_length] = '\0';

        print_buf("file input", input, input_length + 1);
      } break;

      case 's':
      {
        max_count_stack = atoi(optarg);
      } break;

      case 'c':
      {
        max_count_call_stack = atoi(optarg);
      } break;

      case 'l':
      {
        VM_SETTING_SILENT = TRUE;
      } break;

      case ':':
      {
        printf("option needs a value\n");
        return 1;
      } break;

      case '?':
      {
        printf("unknown option: %c\n", optopt);
        return 1;
      } break;
    }
  }

  // if (VM_RELEASE_QUEUE_SIZE > 0)
  //   modl_object_release_queue = (struct ModlObject **) calloc(VM_RELEASE_QUEUE_SIZE, sizeof (struct ModlObject *));

  struct Environment base_environment = { NULL, modl_table_new() };
  struct VMState vm =
  {
    .code = NULL,

    .max_count_call_stack = max_count_call_stack,
    .max_count_stack = max_count_stack,
    .max_count_externals = 16,

    .ip = 0, .csp = 0, .sp = 0, .efc = 0,
    .call_stack = (struct CallFrame *) malloc(max_count_call_stack * sizeof (struct CallFrame)),
    .registers = { },
    .stack = (struct ModlObject *) (malloc(max_count_stack * sizeof (struct ModlObject))),
    .external_functions = (struct ModlObject (**)(struct VMState*))(malloc(max_count_externals*sizeof(struct ModlObject (**)(struct VMState*)))),

    .ram = {
      .read_after_rewrite = { [0 ... VM_SETTING_REGITERS_COUNT-1] = TRUE },
      .last_write_points = { 0, },
    },
  };
  vm.call_stack[0] = (struct CallFrame) { 0, &base_environment };
  for (size_t i = 0; i < VM_SETTING_REGITERS_COUNT; ++i)
    vm.registers[i] = modl_nil();


  uint64_t std_print_id = vm_add_external_function(&vm, modl_std_print);
  uint64_t std_concat_id = vm_add_external_function(&vm, modl_std_concat_strings);
  uint64_t std_to_string_id = vm_add_external_function(&vm, modl_std_to_string);
  modl_table_insert_kv(&base_environment.vartable, str_to_modl("print"), efun_to_modl(std_print_id));
  modl_table_insert_kv(&base_environment.vartable, str_to_modl("concat"), efun_to_modl(std_concat_id));
  modl_table_insert_kv(&base_environment.vartable, str_to_modl("toString"), efun_to_modl(std_to_string_id));


  // {
  //   struct ModlObject * table_efuns = modl_table();

  //   uint64_t std_table_keys_id = vm_add_external_function(&vm, modl_std_table_keys);
  //   modl_table_insert_kv(table_efuns, str_to_modl("keys"), efun_to_modl(std_table_keys_id));

  //   uint64_t std_table_empty_id = vm_add_external_function(&vm, modl_std_table_empty);
  //   modl_table_insert_kv(table_efuns, str_to_modl("empty"), efun_to_modl(std_table_empty_id));

  //   uint64_t std_table_pop_id = vm_add_external_function(&vm, modl_std_table_pop);
  //   modl_table_insert_kv(table_efuns, str_to_modl("pop"), efun_to_modl(std_table_pop_id));

  //   uint64_t std_table_size_id = vm_add_external_function(&vm, modl_std_table_size);
  //   modl_table_insert_kv(table_efuns, str_to_modl("size"), efun_to_modl(std_table_size_id));

  //   uint64_t std_table_zip_id = vm_add_external_function(&vm, modl_std_table_zip);
  //   modl_table_insert_kv(table_efuns, str_to_modl("zip"), efun_to_modl(std_table_zip_id));

  //   modl_table_insert_kv(base_environment.vartable, str_to_modl("Table"), table_efuns);
  // }

  // {
  //   struct ModlObject * table_efuns = modl_table();

  //   uint64_t std_array_foreach_id = vm_add_external_function(&vm, modl_std_array_foreach);
  //   modl_table_insert_kv(table_efuns, str_to_modl("forEach"), efun_to_modl(std_array_foreach_id));

  //   uint64_t std_array_map_id = vm_add_external_function(&vm, modl_std_array_map);
  //   modl_table_insert_kv(table_efuns, str_to_modl("map"), efun_to_modl(std_array_map_id));

  //   uint64_t std_array_map_flat_id = vm_add_external_function(&vm, modl_std_array_map_flat);
  //   modl_table_insert_kv(table_efuns, str_to_modl("mapFlat"), efun_to_modl(std_array_map_flat_id));

  //   uint64_t std_array_concat_id = vm_add_external_function(&vm, modl_std_array_concat);
  //   modl_table_insert_kv(table_efuns, str_to_modl("concat"), efun_to_modl(std_array_concat_id));

  //   uint64_t std_array_contains_id = vm_add_external_function(&vm, modl_std_array_contains);
  //   modl_table_insert_kv(table_efuns, str_to_modl("contains"), efun_to_modl(std_array_contains_id));

  //   modl_table_insert_kv(base_environment.vartable, str_to_modl("Array"), table_efuns);
  // }

  {
    struct ModlObject string_efuns = modl_table();

    uint64_t std_string_substring_id = vm_add_external_function(&vm, modl_std_string_substring);
    modl_table_insert_kv(&string_efuns, str_to_modl("substring"), efun_to_modl(std_string_substring_id));

    uint64_t std_string_to_array_id = vm_add_external_function(&vm, modl_std_string_to_array);
    modl_table_insert_kv(&string_efuns, str_to_modl("toArray"), efun_to_modl(std_string_to_array_id));

    uint64_t std_string_from_array_id = vm_add_external_function(&vm, modl_std_string_from_array);
    modl_table_insert_kv(&string_efuns, str_to_modl("fromArray"), efun_to_modl(std_string_from_array_id));

    modl_table_insert_kv(&base_environment.vartable, str_to_modl("String"), string_efuns);
  }


  // struct BytecodeCompiler bcc = { (enum ModlOpcode*) calloc(16, sizeof(byte)), 0 };
  // emit(&bcc, OP_LOADC);
  // emit(&bcc, REG(0));
  // emit_modl(&bcc, int_to_modl(42));
  //
  // emit(&bcc, OP_PUSH);
  // emit(&bcc, REG(0));
  //
  // emit(&bcc, OP_LOADC);
  // emit(&bcc, REG(0));
  // emit_modl(&bcc, int_to_modl(400));
  //
  // emit(&bcc, OP_LOADC);
  // emit(&bcc, REG(1));
  // emit_modl(&bcc, int_to_modl(-123));
  //
  // emit(&bcc, OP_ADD);
  // emit(&bcc, REGS(REG(0), REG(1)));
  //
  // emit(&bcc, OP_POP);
  // emit(&bcc, REG(1));
  //
  // emit(&bcc, OP_SUB);
  // emit(&bcc, REGS(REG(0), REG(1)));
  //
  // emit(&bcc, OP_RET);

  // printf("INSTRUCTION SIZE: %ld\n", sizeof(struct Instruction));
  // printf("ModlObject size: %ld\n", sizeof (struct ModlObject));

  if (not VM_SETTING_SILENT)
  {
    printf("\x1b[34;1m%s\x1b[0m\n",   "-----=====      RUN       =====-----");
  }

  vm.code = input;
  decoded_sebo_table = (struct Sebo *) calloc(input_length, sizeof (struct Sebo));
  struct ModlObject result = run(&vm);

  if (not VM_SETTING_SILENT)
    printf("\n\x1b[34;1m%s\x1b[0m\n", "-----=====     RESULT     =====-----");

  modl_object_display(&result);
  printf("%c", '\n');

  for (byte i = 0; i < VM_SETTING_REGITERS_COUNT; ++i)
    modl_object_release(vm.registers[i]);

  modl_object_release(base_environment.vartable);

  for (size_t i = 0; i < vm.sp; ++i)
    modl_object_release(vm.stack[i]);

  free(vm.call_stack);
  free(vm.stack);
  free(vm.external_functions);

  clock_t end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("\ntime: %fs\n", time_spent);

  return EXIT_SUCCESS;
}
