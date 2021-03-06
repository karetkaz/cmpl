/*******************************************************************************
 *   File: core.h
 *   Date: 2007/04/20
 *   Desc: internal header
 *******************************************************************************
 *
 *
*******************************************************************************/
#ifndef CC_INTERNAL_H
#define CC_INTERNAL_H

#include "cmplVm.h"
#include "cmplCc.h"
#include "parser.h"
#include "cmplDbg.h"
#include "printer.h"
#include <stdarg.h>

/* Debugging the compiler:
	0: show where errors were raised
	1: trace errors to their root
	2: include debug messages
	3: include debug messages from code generator
	4: include debug messages from code emitter
 	if not defined no extra messages and extra checks are performed.
*/
//#define DEBUGGING 0

enum Settings {
	// maximum token count in expressions
	maxTokenCount = 1024,

	// size of the hash table for the symbol names
	hashTableSize = 512,

	// limit the count of printed elements(stacktrace, array elements)
	maxLogItems = 25,

	// pre allocate space for argument on the stack
	// faster execution if each argument is pushed when calculated
	preAllocateArgs = 0,

	// uint8 a = 130;   // a: uint8(130)
	// uint32 b = a;    // b: uint32(4294967170)
	// by default load.m8 will sign extend to 32 bits
	zeroExtendUnsigned = 0
};

// linked list
typedef struct list {
	struct list *next;
	unsigned char *data;
} *list;

// native function call
typedef struct libc {
	vmError (*call)(nfcContext);
	const char *proto;
	symn sym;
	size_t offs;
	size_t in, out;		// stack parameters
} *libc;

/// Compiler context
struct ccContextRec {
	rtContext rt;
	astn root;                      // statements
	symn owner;                     // scope variables and functions
	symn scope;                     // scope variables and functions (next is symn->scope)
	symn global;                    // global variables and functions (next is symn->global)
	list native;                    // list of native functions
	astn jumps;                     // list of break and continue statements to fix

	int32_t nest;                   // nest level: modified by (enter/leave)
	int32_t inStaticIfFalse: 1;     // inside a static if false
	int32_t genDocumentation: 1;    // generate documentation
	int32_t genStaticGlobals: 1;    // generate global variables as static variables
	int32_t errPrivateAccess: 1;    // raise error accessing private data
	int32_t errUninitialized: 1;    // raise error for uninitialized variables
	astn scopeStack[maxTokenCount]; // scope stack used by enter leave

	// Lexer
	list stringTable[hashTableSize];// string table (hash)
	astn tokPool;                   // list of recycled tokens
	astn tokNext;                   // next token: look-ahead

	// Parser
	symn symbolStack[hashTableSize];// symbol stack (hash)
	const char *home;               // home folder
	const char *unit;               // unit file name
	const char *file;               // current file name
	int line;                       // current line number

	// Type cache
	symn type_vid;        // void
	symn type_bol;        // boolean
	symn type_chr;        // character
	symn type_i08;        //  8bit signed integer
	symn type_i16;        // 16bit signed integer
	symn type_i32;        // 32bit signed integer
	symn type_i64;        // 64bit signed integer
	symn type_u08;        //  8bit unsigned integer
	symn type_u16;        // 16bit unsigned integer
	symn type_u32;        // 32bit unsigned integer
	symn type_u64;        // 64bit unsigned integer
	symn type_f32;        // 32bit floating point
	symn type_f64;        // 64bit floating point
	symn type_ptr;        // pointer
	symn type_var;        // variant
	symn type_rec;        // typename
	symn type_fun;        // function
	symn type_obj;        // object
	symn type_str;        // string

	symn type_int;        // integer: 32/64 bit signed
	symn type_idx;        // length / index: 32/64 bit unsigned

	symn null_ref;        // variable null
	symn length_ref;      // slice length attribute

	symn emit_opc;        // emit intrinsic function, or whatever it is.
	astn void_tag;        // used to lookup function call with 0 argument

	symn libc_dbg;        // raise(char file[*], int line, int level, int trace, char message[*], variant inspect);
	symn libc_try;        // tryExec(pointer args, void action(pointer args));
	symn libc_mem;        // pointer.alloc(pointer old, int size);
	symn libc_new;        // object.create(typename type);
};

/// Debugger context
struct dbgContextRec {
	rtContext rt;
	vmError (*debug)(dbgContext ctx, vmError, size_t ss, void *sp, size_t caller, size_t callee);

	struct arrBuffer functions;
	struct arrBuffer statements;
	size_t freeMem, usedMem;
	symn tryExec;	// the symbol of tryExec function
};

// helper macros
#define lengthOf(__ARRAY) (sizeof(__ARRAY) / sizeof(*(__ARRAY)))
#define offsetOf(__TYPE, __FIELD) ((size_t) &((__TYPE*)NULL)->__FIELD)

/// Bit count: number of bits set to 1
static inline int32_t bitcnt(uint32_t value) {
	value -= ((value >> 1) & 0x55555555);
	value = (((value >> 2) & 0x33333333) + (value & 0x33333333));
	value = (((value >> 4) + value) & 0x0f0f0f0f);
	value += (value >> 8) + (value >> 16);
	return value & 0x3f;
}

/// Bit scan forward: position of the Least Significant Bit
static inline int32_t bitsf(uint32_t value) {
	if (value == 0) {
		return -1;
	}
	int32_t result = 0;
	if ((value & 0x0000ffff) == 0) {
		result += 16;
		value >>= 16;
	}
	if ((value & 0x000000ff) == 0) {
		result += 8;
		value >>= 8;
	}
	if ((value & 0x0000000f) == 0) {
		result += 4;
		value >>= 4;
	}
	if ((value & 0x00000003) == 0) {
		result += 2;
		value >>= 2;
	}
	if ((value & 0x00000001) == 0) {
		result += 1;
	}
	return result;
}

/// Bit scan reverse: position of the Most Significant Bit
static inline int32_t bitsr(uint32_t value) {
	if (value == 0) {
		return -1;
	}
	int32_t result = 0;
	if (value & 0xffff0000) {
		result += 16;
		value >>= 16;
	}
	if (value & 0x0000ff00) {
		result +=  8;
		value >>= 8;
	}
	if (value & 0x000000f0) {
		result += 4;
		value >>= 4;
	}
	if (value & 0x0000000c) {
		result += 2;
		value >>= 2;
	}
	if (value & 0x00000002) {
		result += 1;
	}
	return result;
}

/// Utility function to align offset
static inline size_t padOffset(size_t offs, size_t align) {
	return (offs + (align - 1)) & -align;
}

/// Utility function to align pointer
static inline void *padPointer(void *offs, size_t align) {
	return (void*)padOffset((size_t)offs, align);
}

/// Utility function to swap memory
static inline void memSwap(void *_a, void *_b, size_t size) {
	char *a = _a;
	char *b = _b;
	char *end = a + size;
	while (a < end) {
		char c = *a;
		*a = *b;
		*b = c;
		a += 1;
		b += 1;
	}
}

/// Check if the pointer is inside the vm.
static inline int isValidOffset(rtContext rt, void *ptr) {
	if ((unsigned char*)ptr > rt->_mem + rt->_size) {
		return 0;
	}
	if ((unsigned char*)ptr < rt->_mem) {
		return 0;
	}
	return 1;
}

void nfcCheckArg(nfcContext nfc, ccKind cast, char *name);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const char * const pluginLibInstall;
extern const char * const pluginLibDestroy;

extern const char * const type_fmt_signed32;
extern const char * const type_fmt_signed64;
extern const char * const type_fmt_unsigned32;
extern const char * const type_fmt_unsigned64;
extern const char * const type_fmt_float32;
extern const char * const type_fmt_float64;
extern const char * const type_fmt_character;
extern const char * const type_fmt_string;
extern const char * const type_fmt_typename;
extern const char type_fmt_string_chr;
extern const char type_fmt_character_chr;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

uint64_t timeMillis();
void sleepMillis(int64_t millis);

char *absolutePath(const char *path, char *buff, size_t size);

char *relativeToCWD(const char *path);

/** Calculate hash of string.
 * @brief calculate the hash of a string.
 * @param str string to be hashed.
 * @param len length of string.
 * @return hash code of the input.
 */
unsigned rehash(const char *str, size_t size);

/** Add string to string table.
 * @brief add string to string table.
 * @param cc compiler context.
 * @param str the string to be mapped.
 * @param len the length of the string, -1 recalculates using strlen.
 * @param hash pre-calculated hashcode, -1 recalculates.
 * @return the mapped string in the string table.
 */
char *ccUniqueStr(ccContext cc, const char *str, size_t size/* = -1*/, unsigned hash/* = -1*/);

// open log file for both compiler and runtime
void logFILE(rtContext ctx, FILE *file);
FILE *logFile(rtContext ctx, char *file, int append);

/**
 * Construct reference node.
 *
 * @param cc compiler context.
 * @param name name of the node.
 * @return the new node.
 */
static inline astn tagNode(ccContext cc, const char *name) {
	astn ast = NULL;
	if (cc != NULL && name != NULL) {
		ast = newNode(cc, TOKEN_var);
		if (ast != NULL) {
			size_t len = strlen(name);
			ast->file = cc->file;
			ast->line = cc->line;
			ast->type = NULL;
			ast->ref.link = NULL;
			ast->ref.hash = rehash(name, len + 1) % hashTableSize;
			ast->ref.name = ccUniqueStr(cc, name, len + 1, ast->ref.hash);
		}
	}
	return ast;
}

/**
 * Construct arguments.
 *
 * @param cc compiler context.
 * @param lhs arguments or first argument.
 * @param rhs next argument.
 * @return if lhs is null return (rhs) else return (lhs, rhs).
 */
static inline astn argNode(ccContext cc, astn lhs, astn rhs) {
	if (lhs == NULL) {
		return rhs;
	}
	return opNode(cc, cc->type_vid, OPER_com, lhs, rhs);
}

/**
 * Chain the arguments trough ast.next link.
 * @param args root node of arguments tree.
 */
static inline astn chainArgs(astn args) {
	if (args == NULL) {
		return NULL;
	}
	if (args->kind == OPER_com) {
		astn lhs = chainArgs(args->op.lhso);
		astn rhs = chainArgs(args->op.rhso);
		args = lhs;
		while (lhs->next != NULL) {
			lhs = lhs->next;
		}
		lhs->next = rhs;
	} else {
		args->next = NULL;
	}
	return args;
}

/**
 * Optimize an assignment by removing extra copy of the value if it is on the top of the stack.
 * replace `dup x`, `set y` with a single `mov x, y` instruction
 * replace `dup 0`, `set x` with a single `set x` instruction
 *
 * @param Runtime context.
 * @param stkBegin Begin of stack.
 * @param offsBegin Begin of the byte code.
 * @param offsEnd End of the byte code.
 * @return non zero if the code was optimized.
 */
int foldAssignment(rtContext rt, size_t stkBegin, size_t offsBegin, size_t offsEnd);

/**
 * Test the virtual machine instruction set(compare implementation with definition `OPCODE_DEF`).
 *
 * @param cb callback executed for each instruction.
 * @return number of errors found during the test.
 */
int vmSelfTest(void cb(const char *error, const struct opcodeRec *info));

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Plugins
int importLib(rtContext rt, const char *path);
void closeLibs(rtContext rt);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Error and warning messages
void print_log(rtContext rt, raiseLevel level, const char *file, int line, rtValue *inspect, const char *msg, va_list vaList);

#define ERR_INTERNAL_ERROR "Internal Error"
#define ERR_MEMORY_OVERRUN "Memory Overrun"
#define ERR_OPENING_FILE "can not open file: %s"
#define ERR_UNIMPLEMENTED_FEATURE "Unimplemented feature"

// Lexer errors
#define ERR_EMPTY_CHAR_CONSTANT "empty character constant"
#define ERR_INVALID_CHARACTER "invalid character: '%c'"
#define ERR_INVALID_COMMENT "unterminated block comment"
#define ERR_INVALID_DOC_COMMENT "comment does not belong to a declaration"
#define ERR_INVALID_ESC_SEQ "invalid escape sequence '\\%c'"
#define ERR_INVALID_EXPONENT "no exponent in numeric constant"
#define ERR_INVALID_HEX_SEQ "hex escape sequence invalid '%c'"
#define ERR_INVALID_LITERAL "unclosed '%c' literal"
#define ERR_INVALID_STATEMENT "unterminated block statement"
#define ERR_INVALID_SUFFIX "invalid suffix in numeric constant '%s'"

// Parser errors
#define ERR_EXPECTED_BEFORE_TOK "expected `%s` before token `%?.t`"
#define ERR_EXPR_TOO_COMPLEX "expression too complex"
#define ERR_SYNTAX_ERR_BEFORE "syntax error before token `%?.t`"
#define ERR_UNEXPECTED_ATTR "unexpected attribute `%?K`"
#define ERR_UNEXPECTED_QUAL "unexpected qualifier `%.t` declared more than once"
#define ERR_UNEXPECTED_TOKEN "unexpected token `%.t`"
#define ERR_UNMATCHED_TOKEN "unexpected token `%?.t`, matching `%.k`"
#define ERR_UNMATCHED_SEPARATOR "unexpected separator `%?.t`, matching `%.k`"
#define ERR_DECLARATION_EXPECTED "declaration expected, got: `%t`"
#define ERR_INITIALIZER_EXPECTED "initializer expected, got: `%t`"
#define ERR_STATEMENT_EXPECTED "statement expected, got: `%t`"

// Type checker errors
#define ERR_DECLARATION_COMPLEX "declaration too complex: `%T`"
#define ERR_DECLARATION_REDEFINED "redefinition of `%T`"
#define ERR_UNDEFINED_DECLARATION "reference `%t` is not defined"
#define ERR_PRIVATE_DECLARATION "declaration `%T` is not public"
#define ERR_INVALID_ARRAY_LENGTH "positive integer constant expected, got `%t`"
#define ERR_INVALID_ARRAY_VALUES "array initialized with too many values, only %t expected"
#define ERR_INVALID_INHERITANCE "type must be inherited from object, got `%t`"
#define ERR_INVALID_BASE_TYPE "invalid struct base type, got `%t`"
#define ERR_INVALID_PACK_SIZE "invalid struct pack size, got `%t`"
#define ERR_INVALID_INITIALIZER "invalid initialization: `%t`"
#define ERR_INVALID_VALUE_ASSIGN "invalid assignment: `%T` := `%t`"
#define ERR_INVALID_CONST_ASSIGN "constants can not be assigned `%t`"
#define ERR_INVALID_CONST_EXPR "constant expression expected, got: `%t`"
#define ERR_INVALID_DECLARATION "invalid declaration: `%s`"
#define ERR_INVALID_FIELD_ACCESS "object reference is required to access the member `%T`"
#define ERR_INVALID_STATIC_FIELD_INIT "static field can not be re-initialized `%t`"
#define ERR_INVALID_TYPE "can not type check expression: `%t`"
#define ERR_INVALID_CAST "can not use cast to construct instance: `%t`"
#define ERR_INVALID_OPERATOR "invalid operator %.t(%T, %T)"
#define ERR_MULTIPLE_OVERLOADS "there are %d overloads for `%t`"
#define ERR_UNIMPLEMENTED_FUNCTION "unimplemented function `%T`"
#define ERR_UNINITIALIZED_CONSTANT "uninitialized constant `%T`"
#define ERR_UNINITIALIZED_VARIABLE "uninitialized variable `%T`"
#define ERR_UNINITIALIZED_MEMBER "uninitialized member `%.T.%.T`"
#define ERR_PARTIAL_INITIALIZE_UNION "partial union initialization with `%.T.%.T`"

// Code generator errors
#define ERR_CAST_EXPRESSION "can not emit expression: %t, invalid cast(%K -> %K)"
#define ERR_EMIT_LENGTH "can not emit length of array: %t"
#define ERR_EMIT_VARIABLE "can not emit variable: %T"
#define ERR_EMIT_FUNCTION "can not emit function: %T"
#define ERR_EMIT_STATEMENT "can not emit statement: %t"
#define ERR_INVALID_JUMP "`%t` statement is invalid due to previous variable declaration within loop"
#define ERR_INVALID_OFFSET "invalid reference: %06x"

// Code execution errors
#define ERR_EXEC_INSTRUCTION "%s at .%06x in function: <%?.T%?+d> executing instruction: %.A"
#define ERR_EXEC_NATIVE_CALL "%s at .%06x in function: <%?.T%?+d> executing native call: %T"
#define ERR_EXEC_FUNCTION "can not execute function: %T"

#define WARN_EMPTY_STATEMENT "empty statement `;`"
#define WARN_USE_BLOCK_STATEMENT "statement should be a block statement {%t}"
//TODO: #define WARN_LOCAL_MIGHT_ESCAPE "local variable `%t` can not be referenced outside of its scope"
#define WARN_PASS_ARG_NO_CAST "argument `%t` is passed to emit without cast as `%T`"
#define WARN_PASS_ARG_BY_REF "argument `%t` is passed by reference to `%T`"
#define WARN_NO_CODE_GENERATED "no code will be generated for statement: %t"
#define WARN_PADDING_ALIGNMENT "padding `%?T` with %d bytes: (%d -> %d)"
#define WARN_ADDING_IMPLICIT_CAST "adding implicit cast %T(%t: %T)"
#define WARN_ADDING_TEMPORARY_VAR "adding temporary variable %T := %t"
#define WARN_USING_SIGNED_CAST "using signed cast for unsigned value: `%t`"
#define WARN_STATIC_FIELD_ACCESS "accessing static member using instance variable `%T`/ %T"

#define WARN_COMMENT_MULTI_LINE "multi-line comment: `%s`"
#define WARN_COMMENT_NESTED "ignoring nested comment"
#define WARN_NO_NEW_LINE_AT_END "expected <new line> before end of input"

#define WARN_MULTI_CHAR_CONSTANT "multi character constant"
#define WARN_CHR_CONST_OVERFLOW "character constant truncated"
#define WARN_OCT_ESC_SEQ_OVERFLOW "octal escape sequence overflow"
#define WARN_VALUE_OVERFLOW "value overflow"
#define WARN_EXPONENT_OVERFLOW "exponent overflow"

#define WARN_FUNCTION_MARKED_STATIC "marking function to be static: `%T`"
#define WARN_USING_BEST_OVERLOAD "using overload `%T` of %d declared symbols"
#define WARN_USING_DEF_TYPE_INITIALIZER "using default type initializer: %T := %t"
#define WARN_USING_DEF_FIELD_INITIALIZER "using default field initializer: %T := %t"
#define WARN_DECLARATION_REDEFINED "variable `%T` hides previous declaration"
#define WARN_FUNCTION_TYPENAME "function name `%.t` is a type, but returns `%T`"
#define WARN_INLINE_FILE "inline file: `%s`"


#define prerr(__TAG, __FMT, ...) do { printFmt(stdout, NULL, "%?s:%?u: %s(" __TAG "): " __FMT "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0)
#define fatal(__FMT, ...) do { prerr("fatal", __FMT, ##__VA_ARGS__); abort(); } while(0)
#define dieif(__EXP, __FMT, ...) do { if (__EXP) { prerr(#__EXP, __FMT, ##__VA_ARGS__); abort(); } } while(0)

// compilation errors
#define warn(__ENV, __LEVEL, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, __LEVEL, __FILE, __LINE, NULL, __FMT, ##__VA_ARGS__); } while(0)
#define info(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raisePrint, __FILE, __LINE, NULL, __FMT, ##__VA_ARGS__); } while(0)

#ifndef DEBUGGING
#define error(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseError, __FILE, __LINE, NULL, __FMT, ##__VA_ARGS__); } while(0)
#else
// show also the line where the error is raised
#define error(__ENV, __FILE, __LINE, __FMT, ...) do { printLog(__ENV, raiseError, __FILE__, __LINE__, NULL, "%?s:%?u: "__FMT, __FILE, __LINE, ##__VA_ARGS__); } while(0)
#define logif(__EXP, __FMT, ...) do { if (__EXP) { prerr(#__EXP, __FMT, ##__VA_ARGS__); } } while(0)

#if DEBUGGING >= 1	// enable trace
#define trace(__FMT, ...) do { prerr("trace", __FMT, ##__VA_ARGS__); } while(0)
#endif

#if DEBUGGING >= 2	// enable debug
#define debug(__FMT, ...) do { prerr("debug", __FMT, ##__VA_ARGS__); } while(0)
#endif

#if DEBUGGING >= 3	// enable debug cgen
#define dbgCgen(__FMT, ...) do { prerr("debug", __FMT, ##__VA_ARGS__); } while(0)
#endif

#if DEBUGGING >= 4	// enable debug emit
#define dbgEmit(__FMT, ...) do { prerr("debug", __FMT, ##__VA_ARGS__); } while(0)
#endif

#endif

#ifndef logif
#define logif(__EXP, __FMT, ...) do {} while(0)
#endif
#ifndef trace
#define trace(__FMT, ...) do {} while(0)
#endif
#ifndef debug
#define debug(__FMT, ...) do {} while(0)
#endif
#ifndef dbgCgen
#define dbgCgen(__FMT, ...) do {} while(0)
#endif
#ifndef dbgEmit
#define dbgEmit(__FMT, ...) do {} while(0)
#endif

#define traceAst(__AST) do { trace("%.*t", prDbg, __AST); } while(0)

// Compiler specific settings
#ifdef __WATCOMC__
#pragma disable_message(136);	// Warning! W136: Comparison equivalent to 'unsigned == 0'

#include <math.h>
static inline float fmodf(float x, float y) { return (float) fmod(x, y); }
static inline float sinf(float x) { return (float) sin((float) x); }
static inline float cosf(float x) { return (float) cos((float) x); }
static inline float tanf(float x) { return (float) tan((float) x); }
static inline float logf(float x) { return (float) log((float) x); }
static inline float expf(float x) { return (float) exp((float) x); }
static inline float powf(float x, float y) { return (float) pow((float) x, (float) y); }
static inline float sqrtf(float x) { return (float) sqrt((float) x); }
static inline float atan2f(float x, float y) { return (float) atan2((float) x, (float) y); }
#endif

#ifdef _MSC_VER
// disable warning messages
// C4996: The POSIX ...
#pragma warning(disable: 4996)
#endif

#endif
