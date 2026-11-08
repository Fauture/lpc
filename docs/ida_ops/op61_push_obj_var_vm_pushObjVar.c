void __cdecl vm_pushObjVar(int valueNo)
{
  Vm_Value *v1; // ebx
  unsigned int v2; // eax

  v1 = &vm_currentContext->thisObject->pObjectValues[valueNo + vm_currentContext->consititution->baseVarOffset];
  v2 = vm_sp;
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  --vm_sp;
  vm_stack[v2 - 1].type = 0;
  vm_copyValue((char *)&vm_stack[vm_sp], (int)v1);
}