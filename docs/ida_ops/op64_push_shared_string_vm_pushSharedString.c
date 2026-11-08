void __cdecl vm_pushSharedString(int a1)
{
  Vm_Program *pProgram; // edx
  Vm_String *v3; // ebx

  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  pProgram = vm_currentContext->consititution->pProgram;
  if ( a1 < 0 || pProgram->maxStringNo <= a1 || (v3 = pProgram->ppStrings[a1]) == 0 )
    vm_error("Bad shared string %d!\n", a1);
  vm_stack[--vm_sp].type = 4;
  vm_stack[vm_sp].v.number = (int)v3;
  ++v3->ref;
}