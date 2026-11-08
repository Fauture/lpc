void __cdecl vm_pushLocalVptr(int a1)
{
  Vm_FunCall_Context_T *v2; // esi
  signed int v3; // ebx
  unsigned int v4; // edx
  Vm_Value_T_2 *v5; // ecx
  signed int v6; // eax
  Vm_Value_T_2 *v7; // ebx
  Vm_Value_T_2 *v8; // ecx
  unsigned __int8 attrib; // dl
  unsigned int v10; // eax

  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  if ( !vm_funCp )
    vm_error("No such argument.\n");
  v2 = &vm_funContext[(_DWORD)vm_funCp];
  if ( v2->argNum <= a1 )
    vm_error("No such argument.\n");
  v3 = v2->bp + a1;
  v4 = vm_sp;
  if ( v3 < (int)vm_sp )
    vm_error("No such argument.\n");
  v5 = vm_stack;
  v6 = v3;
  if ( vm_stack[v3].type == 8 )
  {
    if ( !vm_sp )
      vm_error("Stack overflow when calculating.\n");
    vm_stack[--vm_sp].type = 0;
    vm_copyValue((char *)&vm_stack[vm_sp], (int)&v5[v6]);
  }
  else
  {
    v7 = &vm_stack[v6];
    if ( !vm_sp )
      vm_error("Stack overflow when calculating.\n");
    --vm_sp;
    vm_stack[v4 - 1].type = 8;
    v8 = vm_stack;
    attrib = v7->attrib;
    v10 = vm_sp;
    ++v7->ref;
    v8[v10].v.number = (int)v7;
    v8[v10].attrib = attrib;
  }
}