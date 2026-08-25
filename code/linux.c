
void EntryPoint(void)
{
    __asm__ volatile ("syscall" :: "a"(60), "D"(0));
}

