void __cdecl vm_if(int a1, char a2)
{
  Vm_Value_T_2 *v4; // ecx

  vm_stackTopValue();
  if ( vm_isZero(v4) )
  {
    if ( (a2 & 1) != 0 )
      goto LABEL_3;
  }
  else if ( (a2 & 1) == 0 )
  {
LABEL_3:
    if ( (a2 & 2) == 0 )
      return;
LABEL_7:
    vm_popStack();
    return;
  }
  vm_currentContext->ip += a1;
  if ( (a2 & 2) != 0 )
    goto LABEL_7;
}