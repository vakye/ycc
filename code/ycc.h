
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

#define YCC_U32_MAX ((ycc_u32)(0xFFFFFFFF))

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

static void ycc_write_log           (ycc_string message);
static void ycc_write_log_newline   (void);
static void ycc_write_log_usize     (ycc_usize value);

static void ycc_write_err           (ycc_string message);
static void ycc_write_err_newline   (void);
static void ycc_write_err_usize     (ycc_usize value);

static void ycc_info                (ycc_string message);
static void ycc_err                 (ycc_string message);

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

    YCC_TOKEN_KIND_LPAREN       = '(',
    YCC_TOKEN_KIND_RPAREN       = ')',

    YCC_TOKEN_KIND_MAX          = 255,
};

typedef struct ycc_lexer_info {
    ycc_string code;
} ycc_lexer_info;

static ycc_lexer        ycc_make_lexer          (ycc_lexer_info* info);
static ycc_bool32       ycc_lexer_finished      (ycc_lexer lexer);
static ycc_token_kind   ycc_lexer_current_kind  (ycc_lexer lexer);
static ycc_string       ycc_lexer_current_str   (ycc_lexer lexer);
static ycc_usize        ycc_lexer_current_int   (ycc_lexer lexer);
static void             ycc_lexer_next          (ycc_lexer lexer);

/* Parsing */

YCC_HANDLE(ycc_parser);

typedef ycc_u32 ycc_node_id;

#define YCC_NULL_NODE_ID (0)

typedef ycc_u8 ycc_node_kind;
enum {
    YCC_NODE_KIND_NIL           = 0,

    /* Uses ycc_integer_node in ycc_node_data */

    YCC_NODE_KIND_INTEGER,

    /* Uses ycc_binary_node in ycc_node_data */

    YCC_NODE_KIND_ADD,
    YCC_NODE_KIND_SUB,
    YCC_NODE_KIND_MUL,
    YCC_NODE_KIND_DIV,
    YCC_NODE_KIND_MOD,

    YCC_NODE_KIND_COUNT,
};

typedef struct ycc_integer_node {
    ycc_usize value;
} ycc_integer_node;

typedef struct ycc_binary_node {
    ycc_node_id left;
    ycc_node_id right;
} ycc_binary_node;

typedef struct ycc_node_data {
    ycc_integer_node integer;
    ycc_binary_node binary;
} ycc_node_data;

typedef struct ycc_parser_info {
    ycc_lexer lexer;
} ycc_parser_info;

static ycc_parser       ycc_make_parser         (ycc_parser_info* info);
static ycc_node_id      ycc_parser_expr         (ycc_parser parser);
static ycc_node_id      ycc_parser_sum          (ycc_parser parser);
static ycc_node_id      ycc_parser_factor       (ycc_parser parser);
static ycc_node_id      ycc_parser_primary      (ycc_parser parser);
static ycc_node_kind    ycc_get_node_kind       (ycc_parser parser, ycc_node_id node_id);
static ycc_node_data    ycc_get_node_data       (ycc_parser parser, ycc_node_id node_id);
static void             ycc_log_node            (ycc_parser parser, ycc_node_id node_id);

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

typedef struct ycc_node {
    ycc_node_kind kind;
    ycc_node_data data;
} ycc_node;

typedef struct ycc_parser_state {
    ycc_lexer   lexer;          /* current lexer that is being used for parsing */

    ycc_node*   nodes;          /* array of nodes parsed */
    ycc_u32     node_count;     /* count of nodes parsed so far */
    ycc_u32     nodes_commited; /* number of nodes whose memory has been commited */
    ycc_u32     nodes_reserved; /* maximum number of nodes whose memory has been reserved in `nodes` array */
} ycc_parser_state;

typedef struct {
    ycc_log_out*        log_out;
    ycc_err_out*        err_out;
    ycc_reserve_mem*    reserve_mem;
    ycc_commit_mem*     commit_mem;

    ycc_u32             max_lexer_count;
    ycc_u32             max_parser_count;

    ycc_lexer_state*    lexer_states;
    ycc_parser_state*   parser_states;
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
    ycc_usize lexer_states_size = 0;
    ycc_usize parser_states_size = 0;
    ycc_usize states_size = 0;

    if (good)
    {
        ycc_globals.max_lexer_count  = 512;
        ycc_globals.max_parser_count = 512;

        lexer_states_size  = ycc_globals.max_lexer_count * sizeof(ycc_lexer_state);
        parser_states_size = ycc_globals.max_parser_count * sizeof(ycc_parser_state);

        states_size = lexer_states_size + parser_states_size;
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
        ycc_zero_mem(states_mem, states_size);

        ycc_globals.lexer_states  = (ycc_lexer_state*) (states_mem + 0);
        ycc_globals.parser_states = (ycc_parser_state*)(states_mem + lexer_states_size);
    }

    ycc_info(YCC_STRING("ycc_setup() good"));

    return (good);
}

/* Logging */

#define YCC_INFO_PREFIX YCC_STRING("[ycc_log]: ")
#define YCC_ERR_PREFIX  YCC_STRING("[ycc_err]: ")

static void ycc_write_log(ycc_string message)
{
    if (ycc_globals.log_out)
        ycc_globals.log_out(message.data, message.size);
}

static void ycc_write_log_newline(void)
{
    ycc_write_log(YCC_STRING("\n"));
}

static void ycc_write_log_usize(ycc_usize value)
{
    char buffer[64] = {0};
    ycc_usize digit_index = sizeof(buffer);
    ycc_usize digit_count = 0;

    do
    {
        char digit = '0' + (char)(value % 10);
        value /= 10;

        digit_index--;
        digit_count++;

        buffer[digit_index] = digit;
    } while (value);

    ycc_write_log((ycc_string){
        .data = buffer + digit_index,
        .size = digit_count,
    });
}

static void ycc_write_err(ycc_string message)
{
    if (ycc_globals.err_out)
        ycc_globals.err_out(message.data, message.size);
}

static void ycc_write_err_newline(void)
{
    ycc_write_err(YCC_STRING("\n"));
}

static void ycc_write_err_usize(ycc_usize value)
{
    char buffer[64] = {0};
    ycc_usize digit_index = sizeof(buffer);
    ycc_usize digit_count = 0;

    do
    {
        char digit = '0' + (char)(value % 10);
        value /= 10;

        digit_index--;
        digit_count++;

        buffer[digit_index] = digit;
    } while (value);

    ycc_write_err((ycc_string){
        .data = buffer + digit_index,
        .size = digit_count,
    });
}

static void ycc_info(ycc_string message)
{
    ycc_write_log(YCC_INFO_PREFIX);
    ycc_write_log(message);
    ycc_write_log_newline();
}

static void ycc_err(ycc_string message)
{
    ycc_write_err(YCC_ERR_PREFIX);
    ycc_write_err(message);
    ycc_write_err_newline();
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

static ycc_usize ycc_lexer_current_int(ycc_lexer lexer)
{
    ycc_usize result = 0;

    if (ycc_lexer_current_kind(lexer) == YCC_TOKEN_KIND_INTEGER)
    {
        ycc_string string = ycc_lexer_current_str(lexer);
        for (ycc_usize index = 0; index < string.size; index++)
        {
            result *= 10;
            result += (string.data[index] - '0');
        }
    }

    return (result);
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

/* Parsing */

#define YCC_PARSER_NODES_COMMIT_BLOCK (16384)

static ycc_parser ycc_find_free_parser_slot(void)
{
    ycc_parser result = YCC_NULL_HANDLE(ycc_parser);

    for (ycc_u32 index = 0; index < ycc_globals.max_parser_count; index++)
    {
        ycc_parser_state* state = ycc_globals.parser_states + index;
        if (YCC_IS_NULL_HANDLE(state->lexer))
        {
            result.value32[0] = 1 + index;
            break;
        }
    }

    return (result);
}

static ycc_bool32 ycc_check_parser_handle(ycc_parser parser)
{
    ycc_bool32 good =
        (parser.value32[0] > 0) &&
        (parser.value32[0] <= ycc_globals.max_parser_count);

    return (good);
}

static ycc_parser_state* ycc_get_parser_state(ycc_parser parser)
{
    static ycc_parser_state nil_state = {0};

    ycc_parser_state* state = &nil_state;

    if (ycc_check_parser_handle(parser))
        state = ycc_globals.parser_states + (parser.value32[0] - 1);
    else
        ycc_err(YCC_STRING("invalid parser handle passed to ycc_get_parser_state"));

    return (state);
}

static ycc_parser ycc_make_parser(ycc_parser_info* info)
{
    if (!info)
    {
        ycc_err(YCC_STRING("null pointer to info passed to ycc_make_parser()"));
        return YCC_NULL_HANDLE(ycc_parser);
    }

    if (YCC_IS_NULL_HANDLE(info->lexer))
    {
        ycc_err(YCC_STRING("null lexer handle passed to ycc_make_parser()"));
        return YCC_NULL_HANDLE(ycc_parser);
    }

    ycc_parser parser = ycc_find_free_parser_slot();

    if (YCC_IS_NULL_HANDLE(parser))
    {
        ycc_err(YCC_STRING("too many parsers. ycc_make_parser() can't find free parser slot to use."));
        return YCC_NULL_HANDLE(ycc_parser);
    }

    ycc_parser_state* state = ycc_get_parser_state(parser);

    ycc_zero_object(state);

    state->lexer = info->lexer;

    state->nodes_reserved = YCC_U32_MAX;
    state->nodes_commited = YCC_PARSER_NODES_COMMIT_BLOCK;

    state->nodes = ycc_globals.reserve_mem(state->nodes_reserved * sizeof(ycc_node));

    if (!state->nodes)
    {
        ycc_err(YCC_STRING("failed to reserve memory for nodes in ycc_make_parser()"));
        return YCC_NULL_HANDLE(ycc_parser);
    }

    if (!ycc_globals.commit_mem(state->nodes, state->nodes_commited * sizeof(ycc_node)))
    {
        ycc_err(YCC_STRING("failed to commit memory for nodes in ycc_make_parser()"));
        return YCC_NULL_HANDLE(ycc_parser);
    }

    return (parser);
}

static ycc_token_kind ycc_parser_current(ycc_parser parser)
{
    ycc_parser_state* state = ycc_get_parser_state(parser);
    ycc_token_kind result = ycc_lexer_current_kind(state->lexer);

    return (result);
}

static void ycc_parser_next(ycc_parser parser)
{
    ycc_parser_state* state = ycc_get_parser_state(parser);
    ycc_lexer_next(state->lexer);
}

static ycc_bool32 ycc_parser_match(ycc_parser parser, ycc_token_kind kind)
{
    ycc_token_kind current_kind = ycc_parser_current(parser);
    ycc_bool32 result = (current_kind == kind);
    return (result);
}

static ycc_bool32 ycc_parser_next_if_match(ycc_parser parser, ycc_token_kind kind)
{
    ycc_bool32 matched = ycc_parser_match(parser, kind);
    if (matched)
        ycc_parser_next(parser);

    return (matched);
}

static ycc_node* ycc_parser_get_node(ycc_parser parser, ycc_node_id node_id)
{
    static ycc_node nil_node = {0};
    ycc_node* node = &nil_node;

    if (!YCC_IS_NULL_HANDLE(parser))
    {
        ycc_parser_state* state = ycc_get_parser_state(parser);

        if ((node_id > 0) && (node_id <= state->node_count))
            node = state->nodes + (node_id - 1);
    }

    return (node);
}

static ycc_node_id ycc_parser_make_node(ycc_parser parser)
{
    if (YCC_IS_NULL_HANDLE(parser))
        return (0);

    ycc_parser_state* state = ycc_get_parser_state(parser);

    if (state->node_count == state->nodes_commited)
    {
        ycc_usize   commit_size = YCC_PARSER_NODES_COMMIT_BLOCK * sizeof(ycc_node);
        void*       commit_at   = state->nodes + state->nodes_commited;

        state->nodes_commited += YCC_PARSER_NODES_COMMIT_BLOCK;
        if (state->nodes_commited >= state->nodes_reserved)
        {
            ycc_err(YCC_STRING("ran out of node memory for parser"));
            return (0);
        }

        if (!ycc_globals.commit_mem(commit_at, commit_size))
        {
            ycc_err(YCC_STRING("failed to commit more node memory for parser"));
            return (0);
        }
    }

    ycc_node_id node_id = 1 + state->node_count;
    state->node_count++;

    ycc_node* node = ycc_parser_get_node(parser, node_id);
    ycc_zero_object(node);

    return (node_id);
}

static ycc_node_id ycc_parser_integer_node(ycc_parser parser, ycc_usize value)
{
    ycc_node_id node_id = ycc_parser_make_node(parser);
    ycc_node* node = ycc_parser_get_node(parser, node_id);

    node->kind = YCC_NODE_KIND_INTEGER;
    node->data.integer.value = value;

    return (node_id);
}

static ycc_node_id ycc_parser_binary_node(ycc_parser parser, ycc_node_kind kind, ycc_node_id left, ycc_node_id right)
{
    ycc_node_id node_id = ycc_parser_make_node(parser);
    ycc_node* node = ycc_parser_get_node(parser, node_id);

    node->kind = kind;
    node->data.binary.left = left;
    node->data.binary.right = right;

    return (node_id);
}

static ycc_node_id ycc_parser_expr(ycc_parser parser)
{
    ycc_node_id node_id = ycc_parser_sum(parser);
    return (node_id);
}

static ycc_node_id ycc_parser_sum(ycc_parser parser)
{
    ycc_node_id node_id = ycc_parser_factor(parser);

    for (;;)
    {
        if (ycc_parser_next_if_match(parser, YCC_TOKEN_KIND_PLUS))
        {
            node_id = ycc_parser_binary_node(parser, YCC_NODE_KIND_ADD, node_id, ycc_parser_factor(parser));
        }
        else if (ycc_parser_next_if_match(parser, YCC_TOKEN_KIND_MINUS))
        {
            node_id = ycc_parser_binary_node(parser, YCC_NODE_KIND_SUB, node_id, ycc_parser_factor(parser));
        }
        else break;
    }

    return (node_id);
}

static ycc_node_id ycc_parser_factor(ycc_parser parser)
{
    ycc_node_id node_id = ycc_parser_primary(parser);

    for (;;)
    {
        if (ycc_parser_next_if_match(parser, YCC_TOKEN_KIND_STAR))
        {
            node_id = ycc_parser_binary_node(parser, YCC_NODE_KIND_MUL, node_id, ycc_parser_primary(parser));
        }
        else if (ycc_parser_next_if_match(parser, YCC_TOKEN_KIND_SLASH))
        {
            node_id = ycc_parser_binary_node(parser, YCC_NODE_KIND_DIV, node_id, ycc_parser_primary(parser));
        }
        else if (ycc_parser_next_if_match(parser, YCC_TOKEN_KIND_PERCENT))
        {
            node_id = ycc_parser_binary_node(parser, YCC_NODE_KIND_MOD, node_id, ycc_parser_primary(parser));
        }
        else break;
    }

    return (node_id);
}

static ycc_node_id ycc_parser_primary(ycc_parser parser)
{
    ycc_node_id node_id = 0;

    ycc_parser_state* state = ycc_get_parser_state(parser);

    if (ycc_parser_match(parser, YCC_TOKEN_KIND_INTEGER))
    {
        ycc_usize value = ycc_lexer_current_int(state->lexer);
        node_id = ycc_parser_integer_node(parser, value);
        ycc_parser_next(parser);
    }
    else if (ycc_parser_next_if_match(parser, YCC_TOKEN_KIND_LPAREN))
    {
        node_id = ycc_parser_expr(parser);

        if (!ycc_parser_next_if_match(parser, YCC_TOKEN_KIND_RPAREN))
            ycc_err(YCC_STRING("syntax error"));
    }
    else
    {
        ycc_err(YCC_STRING("syntax error"));
    }

    return (node_id);
}

static ycc_node_kind ycc_get_node_kind(ycc_parser parser, ycc_node_id node_id)
{
    ycc_node* node = ycc_parser_get_node(parser, node_id);
    ycc_node_kind result = node->kind;
    return (result);
}

static ycc_node_data ycc_get_node_data(ycc_parser parser, ycc_node_id node_id)
{
    ycc_node* node = ycc_parser_get_node(parser, node_id);
    ycc_node_data result = node->data;
    return (result);
}

static void ycc_log_node(ycc_parser parser, ycc_node_id node_id)
{
    static ycc_usize depth = 0;

    depth++;

    static ycc_string kind_names[YCC_NODE_KIND_COUNT] =
    {
        #define ycc_make_node_kind_name(name) \
            [YCC_NODE_KIND_##name] = {.data = #name, .size = sizeof(#name) - 1}

        ycc_make_node_kind_name(NIL),
        ycc_make_node_kind_name(INTEGER),
        ycc_make_node_kind_name(ADD),
        ycc_make_node_kind_name(SUB),
        ycc_make_node_kind_name(MUL),
        ycc_make_node_kind_name(DIV),
        ycc_make_node_kind_name(MOD),

        #undef ycc_make_node_kind_name
    };

    ycc_write_log(YCC_INFO_PREFIX);

    for (ycc_usize index = 0; index < depth; index++)
        ycc_write_log(YCC_STRING("    "));

    ycc_node_kind kind = ycc_get_node_kind(parser, node_id);
    ycc_node_data data = ycc_get_node_data(parser, node_id);

    ycc_write_log(kind_names[kind]);
    ycc_write_log(YCC_STRING(": "));

    switch (kind)
    {
        default: ycc_write_log_newline(); break;

        case YCC_NODE_KIND_INTEGER:
        {
            ycc_write_log_usize(data.integer.value);
            ycc_write_log_newline();
        } break;

        case YCC_NODE_KIND_ADD:
        case YCC_NODE_KIND_SUB:
        case YCC_NODE_KIND_MUL:
        case YCC_NODE_KIND_DIV:
        case YCC_NODE_KIND_MOD:
        {
            ycc_write_log_newline();
            ycc_log_node(parser, data.binary.left);
            ycc_log_node(parser, data.binary.right);
        } break;
    }

    depth--;
}

#endif /* YCC_IMPLEMENTATION */
#endif /* __YCC_H__ */

