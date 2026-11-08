void __usercall vm_caseDefault(Vm_Buffer_T *a1@<ebx>, Vm_String_T *a2@<esi>, int a3)
{
  vm_popStack(a1, a2);
  vm_currentContext->ip += a3;
}