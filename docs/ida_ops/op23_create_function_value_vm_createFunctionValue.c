void __cdecl vm_createFunctionValue(Vm_Index_T a1, unsigned int a2)
{
  if ( ((a2 >> 7) & 1) != 0 )
    vm_createFunctionValueEx((a2 >> 7) & 1, a1, ((a2 >> 6) & 1) == 0 ? 99 : 75, a1, (Vm_Object_T *)(a2 & 0x3F));
  else
    vm_createFunctionValueEx(
      0,
      (__int16)(vm_currentContext->consititution->baseFunOffset + a1),
      ((a2 >> 6) & 1) == 0 ? 99 : 75,
      vm_currentContext->consititution->baseFunOffset + a1,
      (Vm_Object_T *)(a2 & 0x3F));
}