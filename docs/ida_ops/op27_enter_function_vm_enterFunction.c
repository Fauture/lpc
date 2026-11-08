void __cdecl vm_enterFunction(int minArgNum)
{
  Vm_FunCall_Context_T *v1; // eax
  Vm_Efun *pEfun; // esi
  __int16 semValue_high; // dx
  int v4; // ebx

  v1 = &vm_funContext[(_DWORD)vm_funCp];
  pEfun = v1->f.pEfun;
  vm_currentContext->argNum = v1->argNum;
  semValue_high = HIWORD(pEfun->semValue);
  if ( semValue_high > (int)vm_sp )
    vm_error("Stack overflow when calling %s.\n", pEfun->word);
  v4 = 0;
  if ( semValue_high > 0 )
  {
    do
    {
      vm_pushUndefined();
      ++v4;
    }
    while ( SHIWORD(pEfun->semValue) > v4 );
  }
}