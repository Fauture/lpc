void __usercall vm_f_destruct_object2(Vm_Object_T *pCookie@<eax>, Vm_Program_Consititution_T *pConsititutions@<ebx>)
{
  Vm_Program_T *pProgram; // edi
  int v3; // esi
  __int16 initStep; // cx
  int v5; // edx
  Vm_PCB_T *CurrentPCB; // eax
  Vm_FunCall_Context_T *v7; // ebx

  pProgram = pCookie->pProgram;
  v3 = -1;
  while ( 2 )
  {
    initStep = pCookie->initStep;
    do
    {
      if ( initStep <= 0 )
      {
        CurrentPCB = vm_getCurrentPCB();
        vm_clearCrashHook(pConsititutions, CurrentPCB->name);
        pCookie->flags &= ~0x2000u;
        vm_destructObject((Vm_String_T *)v3, pCookie);
        v7 = &vm_funContext[(_DWORD)vm_funCp];
        if ( v7->f.pEfun->retType != 1 )
          vm_hookCallbackWhenNextAction((Vm_Next_Action_Hook_T)vm_error, "Failed to construct object.\n");
        vm_popNStack((Vm_Value_T_2 *)pProgram, v7->argNum);
        vm_returnFromEfun();
        return;
      }
      pCookie->initStep = --initStep;
      pConsititutions = pProgram->pConsititutions;
      v5 = pProgram->initOrderList[initStep];
      LOWORD(v3) = pConsititutions[v5].pProgram->destructFunOffset;
    }
    while ( (_WORD)v3 == 0xFFFF );
    LOWORD(v3) = pConsititutions[v5].baseFunOffset + v3;
    if ( (_WORD)v3 == 0xFFFF )
      continue;
    break;
  }
  vm_newThreadCatch2((void (*)(void *))vm_f_destruct_func_failed, pCookie);
  vm_callInUserFunBody(0, (int)pCookie, v3, &pCookie->pProgram->pFunctions[(__int16)v3].pFun->name, 0);
  vm_hookCallbackWhenReturn((void (*)(void *))vm_f_destruct_func_ok, pCookie);
}