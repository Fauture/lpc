void __usercall vm_popStack(Vm_Buffer_T *a1@<ebx>, Vm_String_T *a2@<esi>)
{
  vm_cleanupValue(a1, a2, &vm_stack[vm_sp]);
  ++vm_sp;
}