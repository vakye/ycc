
#define YCC_IMPLEMENTATION
#include "ycc.h"

#define __NR_write      (1)
#define __NR_mmap       (9)
#define __NR_mprotect   (10)
#define __NR_exit       (60)

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

#define PROT_NONE   (0x00)
#define PROT_READ   (0x01)
#define PROT_WRITE  (0x02)
#define PROT_EXEC   (0x04)

#define MAP_PRIVATE     (0x02)
#define MAP_ANONYMOUS   (0x20)

typedef struct linux_syscall_info
{
    ycc_usize syscall_num;
    ycc_usize arg0, arg1, arg2;
    ycc_usize arg3, arg4, arg5;
} linux_syscall_info;

#define linux_syscall(...) linux_syscall_with_info(&(linux_syscall_info){__VA_ARGS__})

static ycc_usize linux_syscall_with_info(linux_syscall_info* info)
{
    ycc_usize result = 0;

    register ycc_usize r10 __asm__("r10") = info->arg3;
    register ycc_usize r8  __asm__("r8")  = info->arg4;
    register ycc_usize r9  __asm__("r9")  = info->arg5;

    __asm__ volatile (
        "syscall" :
        "=a"(result) :
        "a"(info->syscall_num),
        "D"(info->arg0),
        "S"(info->arg1),
        "d"(info->arg2),
        "r"(r10),
        "r"(r8),
        "r"(r9)
    );

    return (result);
}

static ycc_usize linux_write_stdout(void* data, ycc_usize size)
{
    ycc_ssize written = (ycc_ssize)linux_syscall(__NR_write, STDOUT_FILENO, (ycc_usize)data, size);
    ycc_usize result = (written < 0) ? (0) : (written);
    return (result);
}

static ycc_usize linux_write_stderr(void* data, ycc_usize size)
{
    ycc_ssize written = (ycc_ssize)linux_syscall(__NR_write, STDERR_FILENO, (ycc_usize)data, size);
    ycc_usize result = (written < 0) ? (0) : (written);
    return (result);
}

static void* linux_reserve_mem(ycc_usize size)
{
    ycc_ssize map_result = (ycc_ssize)linux_syscall(
        __NR_mmap,
        0,
        size,
        PROT_NONE,
        MAP_PRIVATE|MAP_ANONYMOUS,
        -1,
        0
    );

    ycc_usize address = (map_result < 0) ? (0) : (map_result);
    void* result = (void*)address;

    return (result);
}

static ycc_bool32 linux_commit_mem(void* mem, ycc_usize size)
{
    ycc_ssize commit_result = (ycc_ssize)linux_syscall(
        __NR_mprotect,
        (ycc_usize)mem,
        size,
        PROT_READ|PROT_WRITE
    );

    ycc_bool32 good = (commit_result >= 0);
    return (good);
}

static void linux_exit(unsigned char exit_code)
{
    linux_syscall(__NR_exit, exit_code);
}

__attribute__((force_align_arg_pointer))
void entry_point(void)
{
    ycc_bool32 setup_good = ycc_setup(&(ycc_setup_info){
        .log_out        = linux_write_stdout,
        .err_out        = linux_write_stderr,
        .reserve_mem    = linux_reserve_mem,
        .commit_mem     = linux_commit_mem,
    });

    if (!setup_good)
        linux_exit(1);

    ycc_string code = YCC_STRING("int main() { int _my_number23__2 = 10; char hello = _my_number23__2  + 2; return 10 + 10 + hello; }");

    ycc_lexer lexer = ycc_make_lexer(&(ycc_lexer_info){
        .code = code,
    });

    ycc_info(YCC_STRING("lexer output:"));

    while (!ycc_lexer_finished(lexer))
    {
        ycc_info(ycc_lexer_current_str(lexer));
        ycc_lexer_next(lexer);
    }

    linux_exit(0);
}

void memset(void* dest_init, int byte, ycc_usize size)
{
    ycc_u8* dest = (ycc_u8*)dest_init;
    while (size--)
        *dest++ = (ycc_u8)byte;
}

