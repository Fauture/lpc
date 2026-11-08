void __usercall vm_neValue(Vm_Buffer_T *a1@<ebx>)
{
  _BOOL4 v1; // esi
  Vm_Value_T_2 *v2; // ebx

  v1 = !vm_isEq((int)&vm_stack[vm_sp + 1], (int)&vm_stack[vm_sp]);
  vm_popStack(a1, (Vm_String_T *)v1);
  v2 = &vm_stack[vm_sp];
  vm_cleanupValue((Vm_Buffer_T *)v2, (Vm_String_T *)v1, v2);
  v2->type = 2;
  v2->v.number = v1;
}