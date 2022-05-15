// 
// Created by MIPT student Nikita Dzer on 04.05.2022.
// 

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/compiler_IR_bincode.h"
#include "../include/executer.h"



typedef enum CompilationResult
{
    COMPILATION_PLUG    = 0,
    COMPILATION_SUCCESS = 1,
    COMPILATION_FAILURE = 2
} CompilationResult;

typedef struct Transition
{
    const Intermediate *restrict reference;
    unsigned char *restrict operand_address;
} Transition;


static Bincode       *restrict bincode                     = NULL;
static unsigned char *restrict executable_free             = NULL;
static Transition    *restrict unresolved_transitions_free = NULL;


static inline void write_execution_prologue(void);
static inline void write_execution_data(void);
static CompilationResult compile_intermediates(IR *const restrict IR);
static void              resolve_transitions(const Transition *const restrict unresolved_transitions);


/*!
 * @brief  Compiles Intermediate Representation to x86_64 instructions
 * @param  IR               List of intermediate instructions
 * @param  executable_size  Pointer to variable that will store executable buffer size
 * @return Pointer to struct with compiled binary instructions and data buffer
 */
const Bincode* compile_IR_bincode(IR *const restrict IR, size_t *const restrict executable_size)
{
    bincode = allocate_bincode(IR->size);
    if (bincode == NULL)
        return NULL;
    
    executable_free = bincode->executable;
    
    Transition *const restrict unresolved_transitions = calloc(IR->size, sizeof(Transition)); // can be optimized
    if (unresolved_transitions == NULL)
    {
        free_bincode(bincode);
        return NULL;
    }
    
    unresolved_transitions_free = unresolved_transitions;
    
    write_execution_prologue();
    write_execution_data();
    
    CompilationResult compilation_result = compile_intermediates(IR);
    if (compilation_result == COMPILATION_FAILURE)
    {
        free(unresolved_transitions);
        free_bincode(bincode);
        
        return NULL;
    }
    
    resolve_transitions(unresolved_transitions);
    free(unresolved_transitions);
    
    *executable_size = executable_free - bincode->executable;
    
    return bincode;
}



#define EXECUTABLE_PUSH(type, data)                 \
    do {                                            \
        *(type *)(executable_free) = (type)(data);  \
        executable_free += sizeof(type);            \
    } while (0)
    
#define ADD_TRANSITION(REFERENCE)                                                                           \
    do {                                                                                                    \
        if ((REFERENCE)->is_compiled)                                                                       \
            EXECUTABLE_PUSH(int32_t, (REFERENCE)->argument2.address - (executable_free + sizeof(int32_t))); \
        else                                                                                                \
        {                                                                                                   \
            *unresolved_transitions_free++ = (Transition){ .reference       = (REFERENCE),                  \
                                                           .operand_address = executable_free };            \
            executable_free += sizeof(int32_t);                                                             \
        }                                                                                                   \
    } while (0)

#define ARGUMENT(arg_type, data)                                                \
    (arg_type) == TYPE_REGISTRY ?                                               \
            (IntermediateArgument){ .type = (arg_type), .registry  = (data) } : \
    (arg_type) == TYPE_INTEGER ?                                                \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
    (arg_type) == TYPE_MEM_REGISTRY ?                                           \
            (IntermediateArgument){ .type = (arg_type), .registry  = (data) } : \
    (arg_type) == TYPE_MEM_RELATIVE ?                                           \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
    (arg_type) == TYPE_MEM_OFFSET ?                                             \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
            (IntermediateArgument){ .type = (arg_type), .reference = NULL   }



static const unsigned char REGISTRY_INDIRECT  = 0b00000000;
static const unsigned char REGISTRY_DIRECT    = 0b11000000;
static const unsigned char SIB                = 0b00000100;

static const unsigned char REX_B              = 0b00000001; // extension of the ModRM r/m, SIB base or opcode reg
static const unsigned char REX_R              = 0b00000100; // extension of the ModRM reg
static const unsigned char REX_W              = 0b00001000; // 64-bit operand size
static const unsigned char REX_IDENTIFIER     = 0b01000000;

static const unsigned char SIB_RSP            = 0b00100100; // instruction addressing with RSP
static const unsigned char SIB_DISP32         = 0b00100101; // instruction addressing with imm32

static const unsigned long long N_DARK_REGISTRIES    = 4;
static const unsigned long long DARK_REGISTRIES      = PROCESSOR_RAM_SIZE;

static const unsigned long long INITIAL_RSP_STORAGE  = DARK_REGISTRIES      + sizeof(unsigned long long) * N_DARK_REGISTRIES;
static const unsigned long long PRINTF_LLD_SPECIFIER = INITIAL_RSP_STORAGE  + sizeof(unsigned long long);
static const unsigned long long PRINTF_LG_SPECIFIER  = PRINTF_LLD_SPECIFIER + sizeof(unsigned long long);
static const unsigned long long SCANF_LLD_SPECIFIER  = PRINTF_LG_SPECIFIER  + sizeof(unsigned long long);
static const unsigned long long SCANF_LG_SPECIFIER   = SCANF_LLD_SPECIFIER  + sizeof(unsigned long long);


static inline unsigned long long get_dark_registry(const unsigned char registry_index)
{
    return (unsigned long long)(bincode->data + DARK_REGISTRIES + sizeof(unsigned long long) * registry_index);
}

static inline bool is_new_rq(const IntermediateRegistry registry)
{
    return registry >= R8;
}

static inline BincodeRegistry get_rq(const IntermediateRegistry registry)
{
    switch (registry)
    {
        case RAX:
        case RBX:
        case RCX:
        case RDX:
        case RDI:
        case RSI:
        case RBP:
        case RSP: return (BincodeRegistry)registry;
        
        case R8:
        case R9:
        case R10:
        case R11:
        case R12:
        case R13:
        case R14:
        case R15: return (BincodeRegistry)(registry - R8);
        
        case NO_REGISTRY: return BINCODE_NO_REGISTRY;
    }
}

static inline unsigned char get_modrm(const unsigned char addressing_mode,
                                      const IntermediateRegistry registry,
                                      const unsigned char registry_or_memory)
{
    return addressing_mode | (get_rq(registry) << 3) | registry_or_memory;
}

static inline unsigned char get_mod(const IntermediateArgumentType type)
{
    return type == TYPE_REGISTRY
           ? REGISTRY_DIRECT
           : REGISTRY_INDIRECT;
}

static inline unsigned char set_ext(const IntermediateRegistry registry, const unsigned char extension)
{
    return is_new_rq(registry) ? extension : 0;
}

static inline void convert_relative(IntermediateArgument *const restrict argument)
{
    if (argument->type == TYPE_MEM_RELATIVE)
    {
        argument->type = TYPE_MEM_OFFSET;
        argument->iconstant = (unsigned long long)(bincode->data + argument->iconstant);
    }
}


static inline void imul(const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    const IntermediateRegistry registry1 = argument1.registry;
    
    if (argument2.type == TYPE_INTEGER)
    {
        const int32_t imm32 = argument2.iconstant;
        
        EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry1, REX_B));
        EXECUTABLE_PUSH(int8_t,  0x69);
        EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_DIRECT, registry1, get_rq(registry1)));
        EXECUTABLE_PUSH(int32_t, imm32);
    }
    else if (argument2.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long offset = argument2.iconstant;
        
        EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R));
        EXECUTABLE_PUSH(int8_t,  0x0F);
        EXECUTABLE_PUSH(int8_t,  0xAF);
        EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry1, SIB));
        EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
        EXECUTABLE_PUSH(int32_t, offset);
    }
    else // if (argument2.type == TYPE_REGISTRY || argument2.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry2 = argument2.registry;
        const unsigned char mod = get_mod(argument2.type);
        
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
        EXECUTABLE_PUSH(int8_t, 0x0F);
        EXECUTABLE_PUSH(int8_t, 0xAF);
        EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry1, get_rq(registry2)));
    
        if (registry2 == RSP && mod == REGISTRY_INDIRECT)
            EXECUTABLE_PUSH(int8_t, SIB_RSP);
    }
}

static inline void push(const IntermediateArgument argument)
{
    if (argument.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
        
        EXECUTABLE_PUSH(int8_t, 0x50 + get_rq(registry));
    }
    else if (argument.type == TYPE_INTEGER)
    {
        const long long iconstant = argument.iconstant;
        
        EXECUTABLE_PUSH(int8_t, 0x68);
        EXECUTABLE_PUSH(int32_t, iconstant);
    }
    else if (argument.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
    
        EXECUTABLE_PUSH(int8_t, 0xFF);
        EXECUTABLE_PUSH(int8_t, get_modrm(REGISTRY_INDIRECT, 6, get_rq(registry)));
        //  ------------------------------------------------ ^ --------------------
        //  ------------------------------ instruction-opcode extension -----------
    }
    else // if (argument.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long offset = argument.iconstant;
        
        EXECUTABLE_PUSH(int8_t, 0xFF);
        EXECUTABLE_PUSH(int8_t, get_modrm(REGISTRY_INDIRECT, 6, SIB));
        //  ------------------------------------------------ ^ -------
        //  -------------------------- instruction-opcode extension --
        EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
        EXECUTABLE_PUSH(int32_t, offset);
    }
}

static inline void pop (const IntermediateArgument argument)
{
    if (argument.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
        
        EXECUTABLE_PUSH(int8_t, 0x58 + get_rq(registry));
    }
    else if (argument.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
        
        EXECUTABLE_PUSH(int8_t, 0x8F);
        EXECUTABLE_PUSH(int8_t, get_modrm(REGISTRY_INDIRECT, 0, get_rq(registry)));
        //  ------------------------------------------------ ^ --------------------
        //  ------------------------------ instruction-opcode extension -----------
    }
    else // if (argument.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long offset = argument.iconstant;
    
        EXECUTABLE_PUSH(int8_t, 0x8F);
        EXECUTABLE_PUSH(int8_t, get_modrm(REGISTRY_INDIRECT, 0, SIB));
        //  ------------------------------------------------ ^ -------
        //  -------------------------- instruction-opcode extension --
        EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
        EXECUTABLE_PUSH(int32_t, offset);
    }
}

static inline void mov (const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    if (argument1.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        
        if (argument2.type == TYPE_INTEGER)
        {
            EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R));
            EXECUTABLE_PUSH(int8_t,  0xB8 + get_rq(registry1));
            EXECUTABLE_PUSH(int64_t, argument2.iconstant);
        }
        else if (argument2.type == TYPE_REGISTRY || argument2.type == TYPE_MEM_REGISTRY)
        {
            const IntermediateRegistry registry2 = argument2.registry;
            const unsigned char mod = get_mod(argument2.type);
    
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x8B);
            EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry1, get_rq(registry2)));
            
            if (registry2 == RSP && mod == REGISTRY_INDIRECT)
                EXECUTABLE_PUSH(int8_t, SIB_RSP);
        }
        else // if (argument2.type == TYPE_MEM_OFFSET)
        {
            const unsigned long long offset = argument2.iconstant;
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R));
            
            if (registry1 == RAX)
            {
                EXECUTABLE_PUSH(int8_t,  0xA1);
                EXECUTABLE_PUSH(int64_t, offset);
            }
            else
            {
                EXECUTABLE_PUSH(int8_t,  0x8B);
                EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry1, SIB));
                EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
                EXECUTABLE_PUSH(int32_t, offset);
            }
            
        }
    }
    else if (argument1.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        const IntermediateRegistry registry2 = argument2.registry;
        const unsigned char mod = get_mod(argument1.type);
        
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry2, REX_R) | set_ext(registry1, REX_B));
        EXECUTABLE_PUSH(int8_t, 0x89);
        EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry2, get_rq(registry1)));
    
        if (registry1 == RSP && mod == REGISTRY_INDIRECT)
            EXECUTABLE_PUSH(int8_t, SIB_RSP);
    }
    else // if (argument1.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long   offset  = argument1.iconstant;
        const IntermediateRegistry registry = argument2.registry;

        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry, REX_R));
        
        if (registry == RAX)
        {
            EXECUTABLE_PUSH(int8_t,  0xA3);
            EXECUTABLE_PUSH(int64_t, offset);
        }
        else
        {
            EXECUTABLE_PUSH(int8_t,  0x89);
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry, SIB));
            EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
            EXECUTABLE_PUSH(int32_t, offset); // CAN BE UNSAFE
        }
    }
}

static inline void xchg(const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    if (argument2.type == TYPE_MEM_OFFSET)
    {
        EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(argument1.registry, REX_R));
        EXECUTABLE_PUSH(int8_t,  0x87);
        EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, argument1.registry, SIB));
        EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
        EXECUTABLE_PUSH(int32_t, argument2.iconstant);
    }
    else
    {
        EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(argument1.registry, REX_R) | set_ext(argument2.registry, REX_B));
        EXECUTABLE_PUSH(int8_t,  0x87);
        
        if (argument2.type == TYPE_REGISTRY)
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_DIRECT,   argument1.registry, get_rq(argument2.registry)));
        else // if (argument2.type == TYPE_MEM_REGISTRY)
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, argument1.registry, get_rq(argument2.registry)));
    }
}

static inline void add (const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    /*
    if (argument1.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        
        if (argument2.type == TYPE_INTEGER)
        {
            EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R));
            EXECUTABLE_PUSH(int8_t,  0xB8 + get_rq(registry1));
            EXECUTABLE_PUSH(int64_t, argument2.iconstant);
        }
        else if (argument2.type == TYPE_REGISTRY || argument2.type == TYPE_MEM_REGISTRY)
        {
            const IntermediateRegistry registry2 = argument2.registry;
            const unsigned char mod = get_mod(argument2.type);
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x8B);
            EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry1, get_rq(registry2)));
            
            if (registry2 == RSP && mod == REGISTRY_INDIRECT)
                EXECUTABLE_PUSH(int8_t, SIB_RSP);
        }
        else // if (argument2.type == ARG_TYPE_OFFSET)
        {
            const unsigned long long offset = argument2.iconstant;
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R));
            
            if (registry1 == RAX)
            {
                EXECUTABLE_PUSH(int8_t,  0xA1);
                EXECUTABLE_PUSH(int64_t, offset);
            }
            else
            {
                EXECUTABLE_PUSH(int8_t,  0x8B);
                EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry1, SIB));
                EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
                EXECUTABLE_PUSH(int32_t, offset);
            }
            
        }
    }
    else if (argument1.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        const IntermediateRegistry registry2 = argument2.registry;
        const unsigned char mod = get_mod(argument1.type);
        
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry2, REX_R) | set_ext(registry1, REX_B));
        EXECUTABLE_PUSH(int8_t, 0x89);
        EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry2, get_rq(registry1)));
        
        if (registry1 == RSP && mod == REGISTRY_INDIRECT)
            EXECUTABLE_PUSH(int8_t, SIB_RSP);
    }
    else // if (argument1.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long   offset  = argument1.iconstant;
        const IntermediateRegistry registry = argument2.registry;
        
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry, REX_R));
        
        if (registry == RAX)
        {
            EXECUTABLE_PUSH(int8_t,  0xA3);
            EXECUTABLE_PUSH(int64_t, offset);
        }
        else
        {
            EXECUTABLE_PUSH(int8_t,  0x89);
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry, SIB));
            EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
            EXECUTABLE_PUSH(int32_t, offset); // CAN BE UNSAFE
        }
    }
    */
    
    if (argument2.type == TYPE_INTEGER)
    {
        if (argument1.type == TYPE_MEM_OFFSET)
        {
            const unsigned long long offset = argument1.iconstant;
            
            EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W);
            EXECUTABLE_PUSH(int8_t,  0x81);
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, 0, SIB));
            //  ------------------------------------------------- ^ -------
            //  ------------------------ instruction-opcode extension -----
            EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
            EXECUTABLE_PUSH(int32_t, offset);
        }
        else // if (argument1.type → {TYPE_REGISTRY, TYPE_MEM_REGISTRY})
        {
            const IntermediateRegistry registry = argument1.registry;
            const unsigned char mod = get_mod(argument1.type);
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x81);
            EXECUTABLE_PUSH(int8_t, get_modrm(mod, 0, get_rq(registry)));
            //  ---------------------------------- ^ --------------------
            //  ------------------ instruction-opcode extension ---------
    
            if (registry == RSP && mod == REGISTRY_INDIRECT)
                EXECUTABLE_PUSH(int8_t, SIB_RSP);
        }
        
        EXECUTABLE_PUSH(int32_t, argument2.iconstant);
    }
    else if (argument1.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        
        if (argument2.type == TYPE_MEM_OFFSET)
        {
            const unsigned long long offset = argument1.iconstant;
    
            EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R));
            EXECUTABLE_PUSH(int8_t,  0x03);
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry1, SIB));
            EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
            EXECUTABLE_PUSH(int32_t, offset);
        }
        else // if (argument2.type → {TYPE_REGISTRY, TYPE_MEM_REGISTRY})
        {
            const IntermediateRegistry registry2 = argument2.registry;
            const unsigned char mod = get_mod(argument2.type);
    
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x03);
            EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry1, get_rq(registry2)));
    
            if (registry2 == RSP && mod == REGISTRY_INDIRECT)
                EXECUTABLE_PUSH(int8_t, SIB_RSP);
        }
    }
    else if (argument1.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        const IntermediateRegistry registry2 = argument2.registry;
        const unsigned char mod = get_mod(argument1.type);
    
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry2, REX_R) | set_ext(registry1, REX_B));
        EXECUTABLE_PUSH(int8_t, 0x03);
        EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry2, get_rq(registry1)));
    
        if (registry1 == RSP && mod == REGISTRY_INDIRECT)
            EXECUTABLE_PUSH(int8_t, SIB_RSP);
        
    }
    else // if (argument1.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long offset = argument1.iconstant;
        const IntermediateRegistry registry = argument2.registry;
        
        EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry, REX_R));
        EXECUTABLE_PUSH(int8_t,  0x03);
        EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry, SIB));
        EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
        EXECUTABLE_PUSH(int32_t, offset);
    }
}

static inline void sub (const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    if (argument2.type == TYPE_INTEGER)
    {
        if (argument1.type == TYPE_MEM_OFFSET)
        {
            const unsigned long long offset = argument1.iconstant;
            
            EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W);
            EXECUTABLE_PUSH(int8_t,  0x81);
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, 5, SIB));
            //  ------------------------------------------------- ^ -------
            //  ------------------------ instruction-opcode extension -----
            EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
            EXECUTABLE_PUSH(int32_t, offset);
        }
        else // if (argument1.type → {TYPE_REGISTRY, TYPE_MEM_REGISTRY})
        {
            const IntermediateRegistry registry = argument1.registry;
            const unsigned char mod = get_mod(argument1.type);
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x81);
            EXECUTABLE_PUSH(int8_t, get_modrm(mod, 5, get_rq(registry)));
            //  ---------------------------------- ^ --------------------
            //  ------------------ instruction-opcode extension ---------
            
            if (registry == RSP && mod == REGISTRY_INDIRECT)
                EXECUTABLE_PUSH(int8_t, SIB_RSP);
        }
        
        EXECUTABLE_PUSH(int32_t, argument2.iconstant);
    }
    else if (argument1.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        
        if (argument2.type == TYPE_MEM_OFFSET)
        {
            const unsigned long long offset = argument1.iconstant;
            
            EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R));
            EXECUTABLE_PUSH(int8_t,  0x2B);
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry1, SIB));
            EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
            EXECUTABLE_PUSH(int32_t, offset);
        }
        else // if (argument2.type → {TYPE_REGISTRY, TYPE_MEM_REGISTRY})
        {
            const IntermediateRegistry registry2 = argument2.registry;
            const unsigned char mod = get_mod(argument2.type);
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x2B);
            EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry1, get_rq(registry2)));
            
            if (registry2 == RSP && mod == REGISTRY_INDIRECT)
                EXECUTABLE_PUSH(int8_t, SIB_RSP);
        }
    }
    else if (argument1.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        const IntermediateRegistry registry2 = argument2.registry;
        const unsigned char mod = get_mod(argument1.type);
        
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry2, REX_R) | set_ext(registry1, REX_B));
        EXECUTABLE_PUSH(int8_t, 0x29);
        EXECUTABLE_PUSH(int8_t, get_modrm(mod, registry2, get_rq(registry1)));
        
        if (registry1 == RSP && mod == REGISTRY_INDIRECT)
            EXECUTABLE_PUSH(int8_t, SIB_RSP);
        
    }
    else // if (argument1.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long offset = argument1.iconstant;
        const IntermediateRegistry registry = argument2.registry;
        
        EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry, REX_R));
        EXECUTABLE_PUSH(int8_t,  0x29);
        EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_INDIRECT, registry, SIB));
        EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
        EXECUTABLE_PUSH(int32_t, offset);
    }
}

static inline void neg (const IntermediateArgument argument)
{
    if (argument.type == TYPE_REGISTRY || argument.type == TYPE_MEM_REGISTRY)
    {
        const IntermediateRegistry registry = argument.registry;
        const unsigned char mod = get_mod(argument.type);
        
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry, REX_B));
        EXECUTABLE_PUSH(int8_t, 0xF7);
        EXECUTABLE_PUSH(int8_t, get_modrm(mod, 3, get_rq(registry)));
        //  ---------------------------------- ^ --------------------
        //  ------------------ instruction-opcode extension ---------
        if (registry == RSP && mod == REGISTRY_INDIRECT)
            EXECUTABLE_PUSH(int8_t, SIB_RSP);
    }
    else // if (argument.type == TYPE_MEM_OFFSET)
    {
        const unsigned long long offset = argument.iconstant;
        
        EXECUTABLE_PUSH(int8_t, 0xF7);
        EXECUTABLE_PUSH(int8_t, get_modrm(REGISTRY_INDIRECT, 3, SIB));
        //  ------------------------------------------------ ^ -------
        //  -------------------------- instruction-opcode extension --
        EXECUTABLE_PUSH(int8_t,  SIB_DISP32);
        EXECUTABLE_PUSH(int32_t, offset);
    }
}

static inline void div_(const IntermediateArgument argument)
{
    if (argument.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry = argument.registry;
        
        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry, REX_R));
        EXECUTABLE_PUSH(int8_t, 0xF7);
        EXECUTABLE_PUSH(int8_t, get_modrm(REGISTRY_DIRECT, 6, get_rq(registry)));
        //  ---------------------------------------------- ^ --------------------
        //  ------------------------------ instruction-opcode extension ---------
    }
}

static inline void cmp (const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    if (argument1.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry1 = argument1.registry;
    
        if (argument2.type == TYPE_REGISTRY)
        {
            const IntermediateRegistry registry2 = argument2.registry;
        
            EXECUTABLE_PUSH(int8_t,  REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
            EXECUTABLE_PUSH(int8_t,  0x3B);
            EXECUTABLE_PUSH(int8_t,  get_modrm(REGISTRY_DIRECT, registry1, get_rq(registry2)));
        }
    }
}

static inline void call(const IntermediateArgument argument)
{
    if (argument.type == TYPE_REGISTRY)
    {
        const IntermediateRegistry registry = argument.registry;
    
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
    
        EXECUTABLE_PUSH(int8_t, 0xFF);
        EXECUTABLE_PUSH(int8_t, REGISTRY_DIRECT | (2 << 3) | get_rq(registry));
    }
    else if (argument.type == TYPE_INTEGER)
    {
        EXECUTABLE_PUSH(int8_t,  0xE8);
        EXECUTABLE_PUSH(int32_t, argument.iconstant); // CAN BE UNSAFE
    }
}

static inline void ret()
{
    EXECUTABLE_PUSH(int8_t, 0xC3);
}


static inline void write_execution_data()
{
    *(int64_t *)(bincode->data + PRINTF_LLD_SPECIFIER) = 0x000A646C6C25; // "%lld\n"
    *(int64_t *)(bincode->data + PRINTF_LG_SPECIFIER)  = 0x000A676C25;   // "%lg\n"
    *(int64_t *)(bincode->data + SCANF_LLD_SPECIFIER)  = 0x00646C6C25;   // "%lld"
    *(int64_t *)(bincode->data + SCANF_LG_SPECIFIER)   = 0x00676C25;     // "%lg"
}

static inline void write_execution_prologue()
{
    mov(ARGUMENT(TYPE_MEM_OFFSET, (unsigned long long)(bincode->data + INITIAL_RSP_STORAGE)), ARGUMENT(TYPE_REGISTRY, RSP));
}


static void resolve_transitions(const Transition *const restrict unresolved_transitions)
{
    const unsigned char *restrict destination_address = NULL;
    const unsigned char *restrict operand_address     = NULL;
    
    for (const Transition *restrict transition = unresolved_transitions;
         transition != unresolved_transitions_free;
         transition += 1)
    {
        destination_address = transition->reference->argument2.address;
        operand_address     = transition->operand_address;
        
        *(int32_t *)operand_address = (int32_t)(destination_address - operand_address - sizeof(int32_t)); // CAN BE UNSAFE
    }
}


static CompilationResult compile_intermediate(Intermediate *const restrict intermediate)
{
    unsigned char *const restrict address = executable_free;
    
    switch (intermediate->opcode)
    {
        //  -v- bytecode-based intermediates -v-
        case RELATIVE_PUSH:
        {
            if (intermediate->argument1.type == TYPE_MEM_REGISTRY)
            {
                add(ARGUMENT(TYPE_REGISTRY, intermediate->argument1.registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
                push(intermediate->argument1);
                sub(ARGUMENT(TYPE_REGISTRY, intermediate->argument1.registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
            }
            else // if (intermediate->argument1.type == TYPE_MEM_RELATIVE)
            {
                convert_relative(&intermediate->argument1);
                push(intermediate->argument1);
            }
            
            break;
        }
        
        case PUSH:
        {
            push(intermediate->argument1);
            break;
        }
    
        case RELATIVE_POP:
        {
            if (intermediate->argument1.type == TYPE_MEM_REGISTRY)
            {
                add(ARGUMENT(TYPE_REGISTRY, intermediate->argument1.registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
                pop(intermediate->argument1);
                sub(ARGUMENT(TYPE_REGISTRY, intermediate->argument1.registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
            }
            else // if (intermediate->argument1.type == TYPE_MEM_RELATIVE)
            {
                convert_relative(&intermediate->argument1);
                pop(intermediate->argument1);
            }
            
            break;
        }
    
        case POP:
        {
            pop(intermediate->argument1);
            break;
        }
        
        case O0_ADD:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RAX));
    
            pop(ARGUMENT(TYPE_REGISTRY,     RAX));
            add(ARGUMENT(TYPE_MEM_REGISTRY, RSP), ARGUMENT(TYPE_REGISTRY, RAX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
            
            break;
        }
        
        case O0_IMUL:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RAX));
    
            pop (ARGUMENT(TYPE_REGISTRY,     RAX));
            imul(ARGUMENT(TYPE_REGISTRY,     RAX), ARGUMENT(TYPE_MEM_REGISTRY, RSP));
            mov (ARGUMENT(TYPE_MEM_REGISTRY, RSP), ARGUMENT(TYPE_REGISTRY,     RAX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
        
            break;
        }
        
        case O0_IN:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RDX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)), ARGUMENT(TYPE_REGISTRY, RBX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(2)), ARGUMENT(TYPE_REGISTRY, RCX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(3)), ARGUMENT(TYPE_REGISTRY, RAX));
    
            push(ARGUMENT(TYPE_REGISTRY, RAX));
            mov(ARGUMENT(TYPE_REGISTRY, RDX), ARGUMENT(TYPE_REGISTRY, RSP));
    
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_INTEGER, 32));
            sub(ARGUMENT(TYPE_REGISTRY, RSP), ARGUMENT(TYPE_REGISTRY, RBX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_INTEGER, 0));
            mov(ARGUMENT(TYPE_REGISTRY, RCX), ARGUMENT(TYPE_INTEGER, (unsigned long long)(bincode->data + SCANF_LLD_SPECIFIER)));
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_INTEGER, (unsigned long long)scanf));
    
            call(ARGUMENT(TYPE_REGISTRY, RBX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_INTEGER, 32));
            add(ARGUMENT(TYPE_REGISTRY, RSP), ARGUMENT(TYPE_REGISTRY, RBX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RDX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)));
            mov(ARGUMENT(TYPE_REGISTRY, RCX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(2)));
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(3)));
            
            break;
        }
        
        case O0_PRINTF:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RDX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)), ARGUMENT(TYPE_REGISTRY, RBX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(2)), ARGUMENT(TYPE_REGISTRY, RCX));
    
            pop(ARGUMENT(TYPE_REGISTRY, RDX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_INTEGER, 40));
            sub(ARGUMENT(TYPE_REGISTRY, RSP), ARGUMENT(TYPE_REGISTRY, RBX));
            mov(ARGUMENT(TYPE_REGISTRY, RCX), ARGUMENT(TYPE_INTEGER, (long long) (bincode->data + PRINTF_LLD_SPECIFIER)));
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_INTEGER, (long long) printf));
    
            call(ARGUMENT(TYPE_REGISTRY, RBX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_INTEGER,  40));
            add(ARGUMENT(TYPE_REGISTRY, RSP), ARGUMENT(TYPE_REGISTRY, RBX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RDX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)));
            mov(ARGUMENT(TYPE_REGISTRY, RCX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(2)));
            
            break;
        }
        
        case O0_JMP:
        {
            EXECUTABLE_PUSH(int8_t, 0xE9);
            ADD_TRANSITION(intermediate->argument1.reference);
            
            break;
        }
        
        case O0_JE:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RAX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)), ARGUMENT(TYPE_REGISTRY, RBX));
    
            pop(ARGUMENT(TYPE_REGISTRY, RAX));
            pop(ARGUMENT(TYPE_REGISTRY, RBX));
    
            cmp(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_REGISTRY, RBX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)));
            
            EXECUTABLE_PUSH(int8_t, 0x0F);
            EXECUTABLE_PUSH(int8_t, 0x84);
            ADD_TRANSITION(intermediate->argument1.reference);
            
            break;
        }
        
        case O0_JA:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RAX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)), ARGUMENT(TYPE_REGISTRY, RBX));
    
            pop(ARGUMENT(TYPE_REGISTRY, RAX));
            pop(ARGUMENT(TYPE_REGISTRY, RBX));
    
            cmp(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_REGISTRY, RBX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)));
    
            EXECUTABLE_PUSH(int8_t, 0x0F);
            EXECUTABLE_PUSH(int8_t, 0x87);
            ADD_TRANSITION(intermediate->argument1.reference);
            
            break;
        }
        
        case O0_HLT:
        {
            mov(ARGUMENT(TYPE_REGISTRY, RSP), ARGUMENT(TYPE_MEM_OFFSET, (unsigned long long)(bincode->data + INITIAL_RSP_STORAGE)));
            ret();
            
            break;
        }
        
        case O0_CALL:
        {
            EXECUTABLE_PUSH(int8_t, 0xE8);
            ADD_TRANSITION(intermediate->argument1.reference);
            
            break;
        }
        
        case O0_RET:
        {
            ret();
            break;
        }
        
        case O0_SQRT:
        {
            return COMPILATION_FAILURE;
        }
    
        case O0_SUB:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RAX));
    
            pop(ARGUMENT(TYPE_REGISTRY,     RAX));
            sub(ARGUMENT(TYPE_MEM_REGISTRY, RSP), ARGUMENT(TYPE_REGISTRY, RAX));
            neg(ARGUMENT(TYPE_MEM_REGISTRY, RSP));
    
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
        
            break;
        }
        
        case O0_DIV:
        {
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, RAX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)), ARGUMENT(TYPE_REGISTRY, RBX));
            mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(2)), ARGUMENT(TYPE_REGISTRY, RDX));
    
            pop (ARGUMENT(TYPE_REGISTRY, RAX));
            pop (ARGUMENT(TYPE_REGISTRY, RBX));
            div_(ARGUMENT(TYPE_REGISTRY, RBX));
            push(ARGUMENT(TYPE_REGISTRY, RAX));
    
            mov(ARGUMENT(TYPE_REGISTRY, RAX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
            mov(ARGUMENT(TYPE_REGISTRY, RBX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(1)));
            mov(ARGUMENT(TYPE_REGISTRY, RDX), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(2)));
            
            break;
        }
        
        case O0_PIX:
        {
            return COMPILATION_FAILURE;
        }
        
        case O0_SHOW:
        {
            return COMPILATION_FAILURE;
        }
        
        
        case ADD:
        {
            add(intermediate->argument1, intermediate->argument2);
            break;
        }
        
        case SUB:
        {
            sub(intermediate->argument1, intermediate->argument2);
            break;
        }
        
        
        case RELATIVE_IMUL:
        {
            if (intermediate->argument2.type == TYPE_MEM_REGISTRY)
            {
                const IntermediateRegistry registry = intermediate->argument2.registry;
        
                add (ARGUMENT(TYPE_REGISTRY, intermediate->argument2.registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
                imul(intermediate->argument1, intermediate->argument2);
                sub (ARGUMENT(TYPE_REGISTRY, intermediate->argument2.registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
            }
            else // if (intermediate->argument2.type == TYPE_MEM_RELATIVE)
            {
                convert_relative(&intermediate->argument1);
                imul(intermediate->argument1, intermediate->argument2);
            }
            
            break;
        }
        
        case IMUL:
        {
            imul(intermediate->argument1, intermediate->argument2);
            break;
        }
        
        //  -v- non-bytecode-based intermediates -v-
        case RELATIVE_MOV:
        {
            if (intermediate->argument1.type == TYPE_MEM_REGISTRY || intermediate->argument2.type == TYPE_MEM_REGISTRY)
            {
                const IntermediateRegistry registry1 = intermediate->argument1.registry;
                const IntermediateRegistry registry2 = intermediate->argument2.registry;
                
                if (registry1 == registry2)
                {
                    const IntermediateRegistry registry = registry1 == R14 ? R14 : R15;
                    
                    mov(ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)), ARGUMENT(TYPE_REGISTRY, registry));
                    
                    mov(ARGUMENT(TYPE_REGISTRY, registry), ARGUMENT(TYPE_REGISTRY, registry1));
                    add(ARGUMENT(TYPE_REGISTRY, registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
                    mov(intermediate->argument1, intermediate->argument2);
                    
                    mov(ARGUMENT(TYPE_REGISTRY, registry), ARGUMENT(TYPE_MEM_OFFSET, get_dark_registry(0)));
                }
                else
                {
                    const IntermediateRegistry registry  = intermediate->argument1.type == TYPE_MEM_REGISTRY
                                                           ? intermediate->argument1.registry
                                                           : intermediate->argument2.registry;
                    
                    add(ARGUMENT(TYPE_REGISTRY, registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
                    mov(intermediate->argument1, intermediate->argument2);
                    sub(ARGUMENT(TYPE_REGISTRY, registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
                }
            }
            else
            {
                convert_relative(&intermediate->argument1);
                convert_relative(&intermediate->argument2);
        
                mov(intermediate->argument1, intermediate->argument2);
            }
        }
        
        case MOV:
        {
            mov(intermediate->argument1, intermediate->argument2);
            break;
        }
        
        case RELATIVE_XCHG:
        {
            if (intermediate->argument1.type == TYPE_MEM_REGISTRY || intermediate->argument2.type == TYPE_MEM_REGISTRY)
            {
                const IntermediateRegistry registry = intermediate->argument1.type == TYPE_MEM_REGISTRY
                                                      ? intermediate->argument1.registry
                                                      : intermediate->argument2.registry;
        
                add(ARGUMENT(TYPE_REGISTRY, registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
                mov(intermediate->argument1, intermediate->argument2);
                sub(ARGUMENT(TYPE_REGISTRY, registry), ARGUMENT(TYPE_INTEGER, (unsigned long long)bincode->data));
            }
            else
            {
                convert_relative(&intermediate->argument1);
                convert_relative(&intermediate->argument2);
        
                xchg(intermediate->argument1, intermediate->argument2);
            }
            
            break;
        }
        
        case XCHG:
        {
            xchg(intermediate->argument1, intermediate->argument2);
            break;
        }
        
        case NEG:
        {
            neg(intermediate->argument1);
            break;
        }
        
        default:
        {
            return COMPILATION_FAILURE;
        }
    }
    
    intermediate->argument2.address = address;  // save instruction address to the second argument
    intermediate->is_compiled = 1;
    
    return COMPILATION_SUCCESS;
}

static CompilationResult compile_intermediates(IR *const restrict IR)
{
    CompilationResult compilation_result = COMPILATION_PLUG;
    
    for (list_index_t i = list_reset_iterator(IR); i != 0; i = list_iterate_forward(IR))
    {
        compilation_result = compile_intermediate(&IR->nodes[i].item);
        
        if (compilation_result == COMPILATION_FAILURE)
            return COMPILATION_FAILURE;
    }
    
    return COMPILATION_SUCCESS;
}


/*
 * +---------------------------------------------------------------------+
 * |                       x86-64 instructions                           |
 * +---------------------------------------------------------------------+
 *
 * legacy prefixes | opcode with prefixes | mod R/M     |  SIB        | displacement | immediate
 * ----------------|----------------------|-------------|-------------|--------------|------------
 * optional        | required             | if required | if required | if required  | if required
 * 1-4             | 1-4                  | 1           | 1           | 1, 2, 4, 8   | 1, 2, 4, 8
 *
 * --------------------- Legacy opcodes ---------------------
 * Mandatory prefix:
 * - probably for SIMD instructions
 *
 * REX prefix:
 * - instructions, that default to 64-bit operand size in long mode:
 *   CALL (near), ENTER, Jcc, JMP (near), LEAVE, LOOPcc, MOV CR, MOV DR, POP, PUSH, RET (near)
 *
 *
 * Opcode:
 * - can be 1, 2, 3 length
 *
 * --------------------- ModRM ---------------------
 * It's used to specify:
 * - 2 regs
 * - 1 reg, 1 mem-based operand, addressing mode
 *
 * Format:
 * 8   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * |  mod  |    reg    |     rm    |
 * +---+---+---+---+---+---+---+---+
 * \-- 2 --\---- 3 ----\---- 3 ----\
 *
 * mod:
 * - mod  = 0b11 - reg-direct addressing mode
 * - mod != 0b11 - reg-indirect addressing mode
 *
 * reg:
 * - reg = rax, rcx, rdx, rbx, others...
 *
 * rm:
 * - mod  = 0b11 => rm = reg
 * - mod != 0b11 => rm = [reg]
 *
 *
 * --------------------- SIB ---------------------
 * Formula:
 * - effective_address = scale * index + base + moffset
 *
 * Format:
 * 8   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * | scale |   index   |   base    |
 * +---+---+---+---+---+---+---+---+
 * \-- 2 --\---- 3 ----\---- 3 ----\
 *
 * Table:
 * - scale = 1, 2, 4, 8
 * - index = rax, rcx, rdx, rbx, others...
 * - base  = rax, rcx, rdx, rbx, others...
 *
 * Jokes:
 * - if base = 0b101, moffset can be added
 *
 * --------------------- REX ------------------------
 * Format:
 * 8   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * | 0   1   0   0 | W | R | X | B |
 * +---+---+---+---+---+---+---+---+
 * \------ 4 ------\ 1 \ 1 \ 1 \ 1 \
 *
 * Table:
 * - W = 0 - default operand size, 1 - 64-bit operand size
 * - R - extension of the ModRM.reg
 * - X - extension of the SIB.index
 * - B - extension of the ModRM.rm, SIB.index, or opcode.reg
 *
 * --------------------- Displacement ---------------------
 * Description:
 * - signed value that is added to the base of CS or to RIP
 *
 * Size:
 * - 1, 2, 4 bytes
 *
 * --------------------- Immediate ---------------------
 * Size:
 * - 1, 2, 4, 8 bytes
 * */

// lea eax, [rdi + rsi * 1]
// 0x8d | 00 000 100 | 00 110 111
/* ---^   -^ --^ --^   -^ --^ --^
 *    |    |   |   |    |   |   |
 *    /    |   |   |    |   |   \
 * lea     /   |   |    |   \    rdi
 *    reg-ind  /   |    \    rsi
 *          rax   SIB    scale = 1
 *
 */