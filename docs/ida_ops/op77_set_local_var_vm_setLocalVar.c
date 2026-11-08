void __cdecl vm_setLocalVar(int a1)
{
  Vm_FunCall_Context_T *v2; // esi
  signed int v3; // ebx
  Vm_Value_T_2 *v4; // ebx
  Vm_Value_T_2 *v5; // esi
  Vm_Buffer_T *buffer; // ebx

  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  if ( !vm_funCp )
    vm_error("No such argument.\n");
  v2 = &vm_funContext[(_DWORD)vm_funCp];
  if ( v2->argNum <= a1 )
    vm_error("No such argument.\n");
  v3 = v2->bp + a1;
  if ( v3 < (int)vm_sp )
    vm_error("No such argument.\n");
  v4 = &vm_stack[v3];
  v5 = &vm_stack[vm_sp];
  if ( v4->type == 8 )
  {
    if ( v5->type == 8 )
      vm_error("Can not set a value which is reference type.\n");
    buffer = v4->v.buffer;
    vm_cleanupValue(buffer, (Vm_String_T *)v5, (Vm_Value_T_2 *)buffer);
    vm_copyValue((char *)buffer, (int)v5);
  }
  else
  {
    vm_cleanupValue((Vm_Buffer_T *)v4, (Vm_String_T *)v5, v4);
    vm_copyValue((char *)v4, (int)v5);
  }
}