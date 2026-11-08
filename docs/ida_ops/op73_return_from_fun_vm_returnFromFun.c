void vm_returnFromFun()
{
  int v0; // eax
  Vm_PCB_T *CurrentPCB; // ebx
  int catchCp; // eax
  Vm_PCB_T *v3; // edx
  Vm_FunCall_Context_T *v4; // edx
  unsigned int flags; // eax
  const char **p_word; // esi
  int v7; // edi
  unsigned int v8; // edx
  Vm_Fun_Ptr::Vm_Value_0 *v9; // ebx
  int VarType; // eax
  int v11; // edx
  const char *v12; // eax
  Vm_Object *thisObject; // ebx
  Vm_PCB_T *v14; // eax
  const char *v15; // [esp-8h] [ebp-20h]
  const char *v16; // [esp-4h] [ebp-1Ch]
  Vm_FunCall_Context_T *pContext; // [esp+8h] [ebp-10h]

  if ( !vm_funCp )
  {
    v0 = vm_sp;
    if ( (int)vm_sp < vm_stackSize )
      v0 = 24 * vm_sp;
    vm_setReturnValue(v0);
    vm_printf("Return on the top layer of call stack. Execution abort.\n");
    vm_returnToBackend(-8102);
  }
  CurrentPCB = vm_getCurrentPCB();
  catchCp = CurrentPCB->catchCp;
  if ( catchCp <= 0 )
  {
LABEL_8:
    v3 = vm_funCp;
  }
  else
  {
    while ( 1 )
    {
      v3 = vm_funCp;
      if ( CurrentPCB->catchContext[catchCp - 1].funCp < (int)vm_funCp )
        break;
      vm_endThreadCatch();
      catchCp = CurrentPCB->catchCp;
      if ( catchCp <= 0 )
        goto LABEL_8;
    }
  }
  v4 = &vm_funContext[(_DWORD)v3];
  flags = v4->flags;
  pContext = v4;
  if ( (flags & 8) == 0 )
  {
    v14 = vm_getCurrentPCB();
    v14->flags &= ~0x100u;
    flags = pContext->flags;
  }
  if ( (flags & 4) == 0 )
  {
    vm_printf("Return on the top layer when do object initialzation, abort.\n");
    vm_popNStack((int)vm_funContext);
    goto LABEL_31;
  }
  p_word = &pContext->f.pEfun->word;
  v7 = *((unsigned __int8 *)p_word + 13);
  v8 = pContext->bp - *((__int16 *)p_word + 5) - vm_sp;
  if ( v7 == 1 )
  {
    if ( v8 )
      vm_error("The function \"%s\" shouldn't return any value.\n", *p_word);
  }
  else
  {
    if ( v7 != 255 )
    {
      if ( !v8 )
        vm_error("The function \"%s\" should return 1 value but not.\n", *p_word);
      if ( v8 != 1 )
      {
        vm_syslogf(
          2,
          4,
          "Fatal error = %s\nFILE(%s)\nLINE(%d)\n",
          "Stack error for function call.\n",
          "/home/cc/xxxx/gameshell/src/vm/vm_fun.c",
          1350);
        vm_endProcess(0);
      }
      if ( vm_stack[vm_sp].type == v7 )
        goto LABEL_26;
      v9 = vm_stackTopValue();
      VarType = vm_getVarType(v9);
      if ( v7 == 2 )
      {
        if ( VarType == 3 )
        {
          v9->v.number = (int)v9->v.real;
          v9->type = 2;
          goto LABEL_26;
        }
      }
      else if ( v7 == 3 && VarType == 2 )
      {
        v9->v.real = (float)v9->v.number;
        v9->type = 3;
        goto LABEL_26;
      }
      LOBYTE(v11) = *((_BYTE *)p_word + 12);
      if ( (v11 & 8) == 0 || VarType )
      {
        v16 = vm_valueType(v11);
        v15 = *p_word;
        v12 = vm_valueType((int)vm_stack);
        vm_error("Return type (%s) of function \"%s\" mismatched with the prototype (%s).\n", v12, v15, v16);
      }
      goto LABEL_27;
    }
    if ( v8 )
    {
      if ( v8 != 1 )
      {
        vm_syslogf(
          2,
          4,
          "Fatal error = %s\nFILE(%s)\nLINE(%d)\n",
          "Stack error for function call.\n",
          "/home/cc/xxxx/gameshell/src/vm/vm_fun.c",
          1342);
        vm_endProcess(0);
      }
      goto LABEL_26;
    }
  }
  vm_pushVoid();
LABEL_26:
  LOBYTE(v11) = *((_BYTE *)p_word + 12);
LABEL_27:
  if ( (v11 & 2) != 0 )
    vm_convertValueToConst(vm_stack);
  if ( vm_funContext[(_DWORD)vm_funCp].argNum + *((__int16 *)p_word + 5) )
    vm_takeOffValueFromStack(1, v7);
LABEL_31:
  thisObject = vm_currentContext->thisObject;
  if ( vm_gatheringCallStackInfo )
  {
    vm_enterPPCopyStackCriticalSection();
    vm_funCp = (Vm_PCB_T *)((char *)vm_funCp - 1);
    vm_updateCurrentFunCallContext();
    vm_leavePPCopyStackCriticalSection();
  }
  else
  {
    vm_funCp = (Vm_PCB_T *)((char *)vm_funCp - 1);
    vm_updateCurrentFunCallContext();
  }
  if ( thisObject )
    vm_unExecRefObject((Vm_Object_T *)thisObject);
  if ( SLOBYTE(pContext->flags) < 0 )
    pContext->returnCallback(pContext->pCookie);
}