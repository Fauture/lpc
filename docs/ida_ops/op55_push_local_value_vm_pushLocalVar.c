void __cdecl vm_pushLocalVar(int a1)
{
  unsigned int v2; // ecx
  Vm_FunCall_Context_T *v3; // eax
  signed int v4; // ebx

  v2 = vm_sp;
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  if ( vm_funCp && (v3 = &vm_funContext[(_DWORD)vm_funCp], v3->argNum > a1) && (v4 = v3->bp + a1, v4 >= (int)vm_sp) )
  {
    --vm_sp;
    vm_stack[v2 - 1].type = 0;
    vm_copyValue((char *)&vm_stack[vm_sp], (int)&vm_stack[v4]);
    if ( vm_stack[vm_sp].type == 8 )
      vm_convertToValue();
  }
  else
  {
    if ( !vm_sp )
      vm_error("Stack overflow when calculating.\n");
    vm_stack[--vm_sp].type = 0;
    vm_stack[vm_sp].v.number = 0;
  }
}