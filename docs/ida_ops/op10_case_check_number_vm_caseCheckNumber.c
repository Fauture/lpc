void __cdecl vm_caseCheckNumber(int offset, int number)
{
  Vm_Value_T_2 *v2; // edx

  v2 = &vm_stack[vm_sp];
  if ( v2->type == 2 && v2->v.number == number )
  {
    vm_popStack();
    vm_currentContext->ip += offset;
  }
}