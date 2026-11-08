void __cdecl vm_pushGlobalVptr(int a1)
{
  Vm_Value_T *pValue; // ebx
  unsigned int v3; // eax
  Vm_Value_T_2 *v4; // ecx
  unsigned __int8 attrib; // dl
  unsigned int v6; // eax

  pValue = vm_pGlobalVar[a1].pValue;
  v3 = vm_sp;
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  --vm_sp;
  vm_stack[v3 - 1].type = 8;
  v4 = vm_stack;
  attrib = pValue->attrib;
  v6 = vm_sp;
  ++pValue->ref;
  v4[v6].v.number = (int)pValue;
  v4[v6].attrib = attrib;
}