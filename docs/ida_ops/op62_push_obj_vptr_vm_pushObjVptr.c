void __cdecl vm_pushObjVptr(int valueNo)
{
  Vm_Value *v1; // ebx
  unsigned int v2; // eax
  Vm_Value_T_2 *v3; // ecx
  unsigned __int8 attrib; // dl
  unsigned int v5; // eax

  v1 = &vm_currentContext->thisObject->pObjectValues[valueNo + vm_currentContext->consititution->baseVarOffset];
  v2 = vm_sp;
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  --vm_sp;
  vm_stack[v2 - 1].type = 8;
  v3 = vm_stack;
  attrib = v1->attrib;
  v5 = vm_sp;
  ++v1->ref;
  v3[v5].v.number = (int)v1;
  v3[v5].attrib = attrib;
}