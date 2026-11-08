void __usercall vm_setGlobalVar(Vm_String_T *a1@<esi>, int a2)
{
  Vm_Buffer_T *pValue; // ebx

  pValue = (Vm_Buffer_T *)vm_pGlobalVar[a2].pValue;
  vm_cleanupValue(pValue, a1, (Vm_Value_T_2 *)pValue);
  vm_copyValue((char *)pValue, (int)&vm_stack[vm_sp]);
}