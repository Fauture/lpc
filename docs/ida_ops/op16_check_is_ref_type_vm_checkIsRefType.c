void __cdecl vm_checkIsRefType(int a1)
{
  unsigned int v2; // edx
  signed int v3; // ebx
  _BOOL4 v4; // ebx

  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  if ( vm_argumentInThisContext() <= a1 )
  {
    v2 = vm_sp;
LABEL_4:
    if ( !v2 )
      vm_error("Stack overflow when calculating.\n");
    vm_sp = v2 - 1;
    vm_stack[vm_sp].type = 2;
    vm_stack[vm_sp].v.number = 0;
    return;
  }
  v3 = vm_stackInThisContext() + a1;
  v2 = vm_sp;
  if ( v3 < (int)vm_sp )
    goto LABEL_4;
  v4 = vm_stack[v3].type == 8;
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  --vm_sp;
  vm_stack[v2 - 1].type = 2;
  vm_stack[vm_sp].v.number = v4;
}