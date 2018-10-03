

void
shutdown()
{
    asm volatile("cli");
    asm volatile("hlt");
}
