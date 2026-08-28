
#ifndef __YCC_H__
#define __YCC_H__ 1

/* Types */

typedef signed char         ycc_s8;
typedef signed short        ycc_s16;
typedef signed int          ycc_s32;
typedef signed long long    ycc_s64;

typedef unsigned char       ycc_u8;
typedef unsigned short      ycc_u16;
typedef unsigned int        ycc_u32;
typedef unsigned long long  ycc_u64;

typedef ycc_s64 ycc_ssize;
typedef ycc_u64 ycc_usize;

typedef ycc_u8 ycc_bool8;
typedef ycc_u32 ycc_bool32;

/* Constants */

#define YCC_TRUE  (1)
#define YCC_FALSE (0)

/* String */

typedef struct
{
    char*       data;
    ycc_usize   size; /* Does not account for null terminator */
} ycc_string;

#define YCC_NULL_STRING (ycc_string){0}

#define YCC_STRING(literal) (ycc_string){literal, sizeof(literal) - 1}

#define YCC_IS_NULL_STRING(string) (!(string.data) || !(string.size))

/* Handles */

#define YCC_HANDLE(name) \
    typedef struct name { \
        ycc_u32 value32[1]; \
        ycc_u16 value16[2]; \
        ycc_u8  value8 [4]; \
    } name

#define YCC_NULL_HANDLE(name) (name){0}

#define YCC_IS_NULL_HANDLE(handle) ((handle).value32[0] == 0)

/* Setup */

typedef ycc_usize   ycc_log_out     (void* data, ycc_usize size);
typedef ycc_usize   ycc_err_out     (void* data, ycc_usize size);
typedef void*       ycc_reserve_mem (ycc_usize size);
typedef ycc_bool32  ycc_commit_mem  (void* mem, ycc_usize size);

typedef struct ycc_setup_info {
    ycc_log_out*        log_out;
    ycc_err_out*        err_out;
    ycc_reserve_mem*    reserve_mem;
    ycc_commit_mem*     commit_mem;
} ycc_setup_info;

static ycc_bool32 ycc_setup(ycc_setup_info* info);

/* Logging */

static void ycc_info(ycc_string message);
static void ycc_err (ycc_string message);

/* Lexcial Analysis */

YCC_HANDLE(ycc_lexer);

typedef ycc_u8 ycc_token_kind;
enum {
    YCC_TOKEN_KIND_EOF          = 0,
    YCC_TOKEN_KIND_INTEGER      = 1,
    YCC_TOKEN_KIND_IDENTIFIER   = 2,

    YCC_TOKEN_KIND_PLUS         = '+',
    YCC_TOKEN_KIND_MINUS        = '-',
    YCC_TOKEN_KIND_STAR         = '*',
    YCC_TOKEN_KIND_SLASH        = '/',
    YCC_TOKEN_KIND_PERCENT      = '%',

    YCC_TOKEN_KIND_MAX          = 255,
};

typedef struct ycc_lexer_info {
    ycc_string code;
} ycc_lexer_info;

static ycc_lexer        ycc_make_lexer          (ycc_lexer_info* info);
static ycc_bool32       ycc_lexer_finished      (ycc_lexer lexer);
static ycc_token_kind   ycc_lexer_current_kind  (ycc_lexer lexer);
static ycc_string       ycc_lexer_current_str   (ycc_lexer lexer);
static void             ycc_lexer_next          (ycc_lexer lexer);

/* ========================================================================
 * IMPLEMENTATION
 * ========================================================================
 */

#if defined(YCC_IMPLEMENTATION)

/* Internal state*/

typedef struct ycc_token {
    ycc_token_kind  kind; /* token kind */
    ycc_usize       from; /* byte offset to first character of token in code string */
    ycc_usize       size; /* size in bytes of token in code string */
} ycc_token;

typedef struct ycc_lexer_state {
    ycc_string  code;       /* code string that is being handled by this lexer */
    ycc_usize   index;      /* current character that lexer is at */
    ycc_token   current;    /* current token */
} ycc_lexer_state;

typedef struct {
    ycc_log_out*        log_out;
    ycc_err_out*        err_out;
    ycc_reserve_mem*    reserve_mem;
    ycc_commit_mem*     commit_mem;

    ycc_u32             max_lexer_count;
    ycc_lexer_state*    lexer_states;
} ycc_state;

static ycc_state ycc_globals = {0};

/* Memory */

#define ycc_zero_object(pointer) ycc_zero_mem(pointer, sizeof(*(pointer)))

static void ycc_zero_mem(void* mem, ycc_usize size)
{
    ycc_u8* dest = (ycc_u8*)mem;
    while (size--)
        *dest++ = 0;
}

/* Setup */

static ycc_bool32 ycc_setup(ycc_setup_info* info)
{
    if (!info)
        return YCC_FALSE;

    ycc_bool32 good = YCC_TRUE;

    ycc_globals.log_out     = info->log_out;
    ycc_globals.err_out     = info->err_out;
    ycc_globals.reserve_mem = info->reserve_mem;
    ycc_globals.commit_mem  = info->commit_mem;

    if (!ycc_globals.reserve_mem)
    {
        ycc_err(YCC_STRING("ycc_setup() requires info.reserve_mem"));
        good = YCC_FALSE;
    }

    if (!ycc_globals.commit_mem)
    {
        ycc_err(YCC_STRING("ycc_setup() requires info.commit_mem"));
        good = YCC_FALSE;
    }

    ycc_u8* states_mem = 0;
    ycc_usize states_size = 0;

    if (good)
    {
        ycc_globals.max_lexer_count = 512;

        states_size =
            (ycc_globals.max_lexer_count * sizeof(ycc_lexer_state));

        states_mem = ycc_globals.reserve_mem(states_size);

        if (!states_mem)
        {
            ycc_err(YCC_STRING("failed to reserve state memory in ycc_setup()"));
            good = YCC_FALSE;
        }
    }

    if (good)
    {
        if (!ycc_globals.commit_mem(states_mem, states_size))
        {
            ycc_err(YCC_STRING("failed to reserve commit memory in ycc_setup()"));
            good = YCC_FALSE;
        }
    }

    if (good)
    {
        ycc_globals.lexer_states = (ycc_lexer_state*)(states_mem + 0);
    }

    ycc_info(YCC_STRING("ycc_setup() good"));

    return (good);
}

/* Logging */

static void ycc_info(ycc_string message)
{
    if (ycc_globals.log_out)
    {
        ycc_string prefix = YCC_STRING("[ycc_log]: ");
        char newline = '\n';

        ycc_globals.log_out(prefix.data,    prefix.size);
        ycc_globals.log_out(message.data,   message.size);
        ycc_globals.log_out(&newline,       sizeof(newline));
    }
}

static void ycc_err(ycc_string message)
{
    if (ycc_globals.log_out)
    {
        ycc_string prefix = YCC_STRING("[ycc_err]: ");
        char newline = '\n';

        ycc_globals.err_out(prefix.data,    prefix.size);
        ycc_globals.err_out(message.data,   message.size);
        ycc_globals.err_out(&newline,       sizeof(newline));
    }
}

/* Lexcial Analysis */

static ycc_lexer ycc_find_free_lexer_slot(void)
{
    ycc_lexer result = YCC_NULL_HANDLE(ycc_lexer);

    for (ycc_u32 index = 0; index < ycc_globals.max_lexer_count; index++)
    {
        ycc_lexer_state* state = ycc_globals.lexer_states + index;

        if (YCC_IS_NULL_STRING(state->code))
        {
            result.value32[0] = 1 + index;
            break;
        }
    }

    return (result);
}

static ycc_bool32 ycc_check_lexer_handle(ycc_lexer lexer)
{
    ycc_bool32 good =
        (lexer.value32[0] > 0) &&
        (lexer.value32[0] <= ycc_globals.max_lexer_count);

    return (good);
}

static ycc_lexer_state* ycc_get_lexer_state(ycc_lexer lexer)
{
    static ycc_lexer_state null_state = {0};
    ycc_lexer_state* state = &null_state;

    if (ycc_check_lexer_handle(lexer))
        state = ycc_globals.lexer_states + (lexer.value32[0] - 1);
    else
        ycc_err(YCC_STRING("invalid lexer handle passed to ycc_get_lexer_state()"));

    return (state);
}

static ycc_lexer ycc_make_lexer(ycc_lexer_info* info)
{
    if (!info)
    {
        ycc_err(YCC_STRING("null pointer to info passed to ycc_make_lexer()"));
        return YCC_NULL_HANDLE(ycc_lexer);
    }

    if (YCC_IS_NULL_STRING(info->code))
    {
        ycc_err(YCC_STRING("null code string passed to ycc_make_lexer()"));
        return YCC_NULL_HANDLE(ycc_lexer);
    }

    ycc_lexer lexer = ycc_find_free_lexer_slot();

    if (YCC_IS_NULL_HANDLE(lexer))
    {
        ycc_err(YCC_STRING("too many lexers. ycc_make_lexer() cannot find free lexer slot to use."));
        return YCC_NULL_HANDLE(ycc_lexer);
    }

    ycc_lexer_state* state = ycc_get_lexer_state(lexer);

    ycc_zero_object(state);

    state->code = info->code;

    ycc_lexer_next(lexer); /* Initialize state->current */

    ycc_info(YCC_STRING("ycc_make_lexer() good"));

    return (lexer);
}

static ycc_bool32 ycc_lexer_finished(ycc_lexer lexer)
{
    ycc_bool32 result = YCC_TRUE;

    if (ycc_check_lexer_handle(lexer))
    {
        ycc_lexer_state* state = ycc_get_lexer_state(lexer);
        result = (state->current.kind == YCC_TOKEN_KIND_EOF);
    }
    else
    {
        ycc_err(YCC_STRING("invalid lexer handle passed to ycc_lexer_finished()"));
    }

    return (result);
}

static ycc_token_kind ycc_lexer_current_kind(ycc_lexer lexer)
{
    ycc_token_kind kind = YCC_TOKEN_KIND_EOF;

    if (ycc_check_lexer_handle(lexer))
    {
        ycc_lexer_state* state = ycc_get_lexer_state(lexer);

        kind = state->current.kind;
    }
    else
    {
        ycc_err(YCC_STRING("invalid lexer handle passed to ycc_lexer_current_kind()"));
    }

    return (kind);
}

static ycc_string ycc_lexer_current_str(ycc_lexer lexer)
{
    ycc_string string = YCC_NULL_STRING;

    if (ycc_check_lexer_handle(lexer))
    {
        ycc_lexer_state* state = ycc_get_lexer_state(lexer);

        ycc_usize from = state->current.from;
        ycc_usize size = state->current.size;

        if (from + size <= state->code.size)
        {
            string = (ycc_string){
                .data = state->code.data + from,
                .size = size
            };
        }
        else
            ycc_err(YCC_STRING("out-of-bounds token string detected in ycc_lexer_current_str()"));
    }
    else
    {
        ycc_err(YCC_STRING("invalid lexer handle passed to ycc_lexer_current_kind()"));
    }

    return (string);
}

static ycc_bool32 ycc_is_whitespace(char character)
{
    ycc_bool32 result =
        (character == ' ') ||
        (character == '\t') ||
        (character == '\n') ||
        (character == '\r');

    return (result);
}

static ycc_bool32 ycc_is_digit(char character)
{
    ycc_bool32 result = (character >= '0') && (character <= '9');
    return (result);
}

static ycc_bool32 ycc_is_identifier_start(char character)
{
    ycc_bool32 result =
        ((character >= 'a') && (character <= 'z')) ||
        ((character >= 'A') && (character <= 'Z')) ||
        ((character == '_'));

    return (result);
}

static ycc_bool32 ycc_is_identifier(char character)
{
    ycc_bool32 result =
        ycc_is_identifier_start(character) ||
        ycc_is_digit(character);

    return (result);
}

static ycc_bool32 ycc_is_punctuation(char character)
{
    ycc_bool32 result =
        ((character >=  33) && (character <=  47))  ||
        ((character >=  58) && (character <=  64))  ||
        ((character >=  91) && (character <=  94))  ||
        ((character ==  96))                        ||
        ((character >= 123) && (character <= 126));

    return (result);
}

static void ycc_lexer_next(ycc_lexer lexer)
{
    if (!ycc_check_lexer_handle(lexer))
        ycc_err(YCC_STRING("invalid lexer handle passed to ycc_lexer_current_kind()"));

    ycc_lexer_state* state = ycc_get_lexer_state(lexer);

    ycc_token new_token =
    {
        .kind = YCC_TOKEN_KIND_EOF,
        .from = state->index,
        .size = 0,
    };

    while (state->index < state->code.size)
    {
        char character = state->code.data[state->index];

        if (!ycc_is_whitespace(character))
            break;
        else
            state->index++;
    }

    if (state->index < state->code.size)
    {
        char character = state->code.data[state->index];

        new_token.from = state->index;

        if (ycc_is_digit(character))
        {
            new_token.kind = YCC_TOKEN_KIND_INTEGER;

            state->index++;

            while (state->index < state->code.size)
            {
                char character = state->code.data[state->index];

                if (!ycc_is_digit(character))
                    break;
                else
                    state->index++;
            }
        }
        else if (ycc_is_identifier_start(character))
        {
            new_token.kind = YCC_TOKEN_KIND_IDENTIFIER;

            state->index++;

            while (state->index < state->code.size)
            {
                char character = state->code.data[state->index];

                if (!ycc_is_identifier(character))
                    break;
                else
                    state->index++;
            }
        }
        else if (ycc_is_punctuation(character))
        {
            new_token.kind = (ycc_token_kind)character;
            state->index++;
        }
        else
        {
            ycc_err(YCC_STRING("unknown character detected in lexer input code string"));
        }

        new_token.size = state->index - new_token.from;
    }

    state->current = new_token;
}

#endif /* YCC_IMPLEMENTATION */
#endif /* __YCC_H__ */

