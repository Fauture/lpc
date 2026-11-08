int __usercall vm_doActions@<eax>(int a1@<ebx>, Vm_Value_T_2 *a2@<edi>, Vm_PCB *pPCB, Vm_Tick_T a4)
{
  Vm_Tick_T v5; // eax
  int catchCp; // eax
  int v7; // edx
  Vm_Catch_Buf *v8; // eax
  int v9; // eax
  int v10; // eax
  int v11; // eax
  signed int sp; // eax
  int catchIp; // edx
  Vm_Catch_Buf *v14; // eax
  unsigned int flags; // eax
  int v16; // edx
  unsigned int v17; // eax
  int ip; // edx
  signed int v19; // eax
  int v21; // eax
  int v22; // edx
  Vm_Catch_Buf *v23; // eax
  int v24; // eax
  Vm_Tick_T v25; // edx
  int actionState; // [esp+0h] [ebp-8h]
  Vm_Catch_Info_T *pCatch; // [esp+4h] [ebp-4h]

  v5 = 10000;
  if ( a4 )
    v5 = a4;
  totalTicks = v5;
  if ( (pPCB->flags & 0x200000) != 0 )
  {
    v24 = vm_getCostTimeCheck();
    v25 = 120000;
    if ( v24 )
      v25 = totalTicks;
    totalTicks = v25;
  }
  vm_startTimer(0, a1);
LABEL_5:
  catchCp = pPCB->catchCp;
  v7 = 0;
  if ( catchCp > 0 )
  {
LABEL_6:
    catchCp = (int)&pPCB->catchContext[catchCp];
    v7 = *(_DWORD *)(catchCp - 16);
  }
  v8 = vm_newCatch(v7, catchCp);
  v9 = _setjmp(v8->env);
  if ( !v9 )
  {
LABEL_14:
    flags = pPCB->flags;
    if ( (flags & 0x40) != 0 )
    {
      pPCB->flags = flags & 0xFFFFFFBF;
      vm_returnFromEfun();
    }
    if ( !vm_confEnableJit && pPCB->holdTicks > 0x927C0 )
    {
      if ( vm_getCostTimeCheck() )
        vm_error("Non-delay routine (primitive) cost too much time.\n");
      pPCB->holdTicks = 0;
    }
    actionState = -8107;
    if ( !vm_ctrlFlags )
      goto LABEL_23;
    if ( (vm_ctrlFlags & 0x10) == 0 || (v16 = 0, (vm_ctrlFlags &= ~0x10u) != 0) )
    {
      v17 = vm_ctrlFlags;
      if ( (vm_ctrlFlags & 2) != 0 )
      {
        vm_ctrlFlags &= ~2u;
        v16 = -8103;
      }
      else
      {
        if ( (vm_ctrlFlags & 4) != 0 )
        {
          vm_ctrlFlags &= ~4u;
          actionState = -8104;
          goto LABEL_23;
        }
        if ( (vm_ctrlFlags & 8) != 0 )
        {
          vm_ctrlFlags &= ~8u;
          actionState = -8102;
          goto LABEL_23;
        }
        if ( (vm_ctrlFlags & 0x20) != 0 )
        {
          vm_ctrlFlags &= ~0x20u;
          actionState = -8107;
          goto LABEL_23;
        }
        if ( (vm_ctrlFlags & 0x40) != 0 )
        {
          vm_ctrlFlags &= ~0x40u;
          actionState = -8101;
          goto LABEL_23;
        }
        v16 = -8102;
        if ( (vm_ctrlFlags & 0x80u) != 0 )
        {
          LOBYTE(v17) = vm_ctrlFlags & 0x7F;
          LOBYTE(v16) = 84;
          vm_ctrlFlags = v17;
          actionState = v16;
          goto LABEL_23;
        }
      }
    }
    actionState = v16;
LABEL_23:
    if ( actionState == -8107 )
    {
      while ( 1 )
      {
        ip = vm_currentContext->ip;
        if ( ip < 0 || ip >= vm_currentContext->cs->nodesCount )
          break;
        actionState = vm_processor(pPCB);
        if ( actionState != -8107 )
          goto LABEL_27;
      }
      actionState = -8102;
    }
LABEL_27:
    v19 = pPCB->flags;
    if ( (v19 & 0x200000) != 0 && v19 >= 0 && actionState != -8101 && actionState != -8102 )
      vm_error("Non-delay routine (callback) cost too much time.\n");
    vm_endCatch();
    return actionState;
  }
  while ( 1 )
  {
    actionState = v9;
    if ( v9 != -3 )
      return actionState;
    v10 = pPCB->catchCp;
    if ( v10 <= 0 )
      return actionState;
    v11 = v10 - 1;
    pPCB->catchCp = v11;
    pCatch = &pPCB->catchContext[v11];
    sp = pCatch->sp;
    actionState = 0;
    if ( sp > (int)vm_sp )
    {
      vm_popNStack(a2, sp - vm_sp);
      catchIp = pCatch->catchIp;
      if ( pCatch->catchIp == -1 )
      {
LABEL_35:
        v21 = pPCB->catchCp;
        v22 = 0;
        if ( v21 > 0 )
        {
          v21 = (int)&pPCB->catchContext[v21];
          v22 = *(_DWORD *)(v21 - 16);
        }
        v23 = vm_newCatch(v22, v21);
        if ( !_setjmp(v23->env) )
        {
          pCatch->catchCallback(pCatch->pCookie);
          vm_endCatch();
          goto LABEL_5;
        }
        return actionState;
      }
    }
    else
    {
      catchIp = pCatch->catchIp;
      if ( pCatch->catchIp == -1 )
        goto LABEL_35;
    }
    vm_currentContext->ip = catchIp;
    catchCp = pPCB->catchCp;
    if ( catchCp > 0 )
      goto LABEL_6;
    v14 = vm_newCatch(0, catchCp);
    v9 = _setjmp(v14->env);
    if ( !v9 )
      goto LABEL_14;
  }
}