void vm_assignValue()
{
  Vm_Value_T_2 *v0; // eax
  Vm_Buffer_T *buffer; // ebx
  Vm_Value_T_2 *v2; // esi

  v0 = &vm_stack[vm_sp];
  buffer = v0->v.buffer;
  v2 = v0 + 1;
  vm_cleanupValue(buffer, (Vm_String_T *)&v0[1], (Vm_Value_T_2 *)buffer);
  vm_copyValue(v2, (Vm_Value_T_2 *)buffer);
  vm_unrefValue((Vm_Value_T_2 *)buffer);
  vm_stack[vm_sp].type = 0;
  vm_stack[vm_sp++].attrib = 0;
}