
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

#define YCC_STRING(literal) (ycc_string){literal, sizeof(literal) - 1}

/* Setup */

typedef ycc_usize   ycc_log_out(void* data, ycc_usize size);
typedef ycc_usize   ycc_err_out(void* data, ycc_usize size);
typedef void*       ycc_reserve_mem(ycc_usize size);
typedef ycc_bool32  ycc_commit_mem(void* mem, ycc_usize size);

typedef struct ycc_setup_info {
    ycc_log_out*        log_out;
    ycc_err_out*        err_out;
    ycc_reserve_mem*    reserve_mem;
    ycc_commit_mem*     commit_mem;
} ycc_setup_info;

static ycc_bool32 ycc_setup(ycc_setup_info* info);

/* Logging */

static void ycc_info(ycc_string message);
static void ycc_err(ycc_string message);

/* ========================================================================
 * IMPLEMENTATION
 * ========================================================================
 */

#if defined(YCC_IMPLEMENTATION)

/* Setup */

typedef struct
{
    ycc_log_out*        log_out;
    ycc_err_out*        err_out;
    ycc_reserve_mem*    reserve_mem;
    ycc_commit_mem*     commit_mem;
} ycc_state;

static ycc_state ycc_globals = {0};

static ycc_bool32 ycc_setup(ycc_setup_info* info)
{
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

#endif /* YCC_IMPLEMENTATION */
#endif /* __YCC_H__ */

