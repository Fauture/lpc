void __usercall vm_caseCheckString(Vm_String_T *a1@<edi>, int a2, int a3)
{
  Vm_Value_T_2 *v5; // ebx
  Vm_Program *pProgram; // edx
  Vm_String_T *v7; // esi

  v5 = &vm_stack[vm_sp];
  pProgram = vm_currentContext->consititution->pProgram;
  if ( a3 < 0 || pProgram->maxStringNo <= a3 || (v7 = pProgram->ppStrings[a3]) == 0 )
    vm_error("Bad shared string %d!\n", a3);
  if ( v5->type == 4 )
  {
    if ( vm_isSameString(v7, a1) )
    {
      vm_popStack((Vm_Buffer_T *)v5, v7);
      vm_currentContext->ip += a2;
    }
  }
}