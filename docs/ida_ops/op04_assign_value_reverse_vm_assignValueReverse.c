void vm_assignValueReverse()
{
  Vm_Value_T_2 *v0; // ebx
  Vm_String_T *string; // esi

  v0 = &vm_stack[vm_sp];
  string = v0[1].v.string;
  vm_cleanupValue((Vm_Buffer_T *)v0, string, (Vm_Value_T_2 *)string);
  vm_copyValue((char *)string, (int)v0);
  vm_unrefValue(v0);
  v0[1].type = v0->type;
  v0[1].attrib = v0->attrib;
  v0[1].v.number = v0->v.number;
  vm_stack[vm_sp].type = 0;
  vm_stack[vm_sp++].attrib = 0;
}