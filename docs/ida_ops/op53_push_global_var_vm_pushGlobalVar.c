void __usercall vm_pushGlobalVar(Vm_Value_T_2 *a1@<esi>, int a2)
{
  Vm_Value_T *pValue; // ebx
  unsigned int v4; // eax

  pValue = vm_pGlobalVar[a2].pValue;
  v4 = vm_sp;
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  --vm_sp;
  vm_stack[v4 - 1].type = 0;
  vm_copyValue(a1, (Vm_Value_T_2 *)pValue);
}