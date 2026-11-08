void __cdecl vm_goto(int offset)
{
  vm_currentContext->ip += offset;
}