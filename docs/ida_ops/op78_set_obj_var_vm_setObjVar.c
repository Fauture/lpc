void __cdecl vm_setObjVar(int valueNo)
{
  Vm_String_T *v1; // esi
  Vm_Buffer_T *v2; // ebx

  v2 = (Vm_Buffer_T *)&vm_currentContext->thisObject->pObjectValues[valueNo
                                                                  + vm_currentContext->consititution->baseVarOffset];
  vm_cleanupValue(v2, v1, (Vm_Value_T_2 *)v2);
  vm_copyValue((char *)v2, (int)&vm_stack[vm_sp]);
}