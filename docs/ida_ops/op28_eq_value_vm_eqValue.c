void __usercall vm_eqValue(Vm_Value_T_2 *a1@<edi>, Vm_Value_T_2 *a2@<esi>)
{
  Vm_String_T *isEq; // esi
  Vm_Value_T_2 *v3; // ebx

  isEq = (Vm_String_T *)vm_isEq(a1, a2);
  vm_popStack();
  v3 = &vm_stack[vm_sp];
  vm_cleanupValue((Vm_Buffer_T *)v3, isEq, v3);
  v3->type = 2;
  v3->v.number = (int)isEq;
}