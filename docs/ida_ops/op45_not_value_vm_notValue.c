void __fastcall vm_notValue(Vm_Value_T_2 *a1)
{
  Vm_Value_T_2 *v1; // ebx
  Vm_String_T *isZero; // esi

  v1 = &vm_stack[vm_sp];
  isZero = (Vm_String_T *)vm_isZero(a1);
  vm_cleanupValue((Vm_Buffer_T *)v1, isZero, v1);
  v1->type = 2;
  v1->v.number = (int)isZero;
}