int vm_scheduleProcessInternal()
{
  int result; // eax
  Vm_Tick_T v1; // eax
  Vm_Tick_T v2; // ebx
  Vm_Timer_T *v3; // ecx
  Vm_Tick_T timeoutTicks; // eax
  Vm_Timer_T *v5; // edx
  int *v6; // ecx
  Vm_Timer_T *v7; // edx
  Vm_Timer_T **v8; // eax
  Vm_Timer_T *v9; // edx
  int *v10; // ecx
  Vm_Tick_T v11; // eax
  unsigned int v12; // eax
  Vm_Tick_T v13; // eax
  Vm_PCB_T *v14; // edi
  Vm_PCB_T *v15; // ebx
  int (__cdecl *resumeHook)(Vm_PCB_T *, void *); // edx
  int v17; // eax
  int v18; // esi
  bool v19; // cc
  Vm_PCB_T *v20; // ebx
  Vm_PCB *v21; // ecx
  Vm_PCB *v22; // edx
  Vm_PCB_T *v23; // eax
  int v24; // eax
  Vm_PCB_T *v25; // ebx
  void *pResumeCookie; // eax
  Vm_PCB_T *v27; // eax
  Vm_Tick_T v28; // edx
  Vm_Tick_T v29; // eax
  Vm_PCB_T *v30; // edx
  Vm_PCB_T *v31; // ebx
  char *v32; // edx
  Vm_PCB *pPrev; // eax
  Vm_PCB *pNext; // edx
  Vm_Pend_Object *pPendOnObject; // ecx
  Vm_PCB_T *v36; // ebx
  char *v37; // esi
  Vm_PCB *v38; // edx
  Vm_PCB *v39; // eax
  Vm_PCB_T *v40; // edx
  Vm_PCB_T *v41; // edx
  Vm_PCB_T *v42; // edx
  Vm_Tick_T runTicks; // [esp+0h] [ebp-18h]
  Vm_Tick_T currentTick; // [esp+4h] [ebp-14h]
  int returnCode; // [esp+8h] [ebp-10h]

  result = -99;
  if ( !vm_alreadyShutdown )
  {
    vm_processAllRespondingInQueue();
    if ( vm_pActiveProcess || vm_pTimerList || (result = 0, vm_pInvokeList) )
    {
      v1 = vm_getCurrentTick();
      v2 = v1 - vm_lastTick;
      vm_lastTick = v1;
      if ( vm_pActiveProcess && (vm_pActiveProcess->flags & 0x100) != 0 && !vm_forceScriptRelinquish )
      {
        v3 = vm_pTimerList;
        if ( vm_pTimerList )
        {
          while ( v2 )
          {
            timeoutTicks = v3->timeoutTicks;
            if ( timeoutTicks > v2 )
              break;
            v3->timeoutTicks = 0;
            v3 = v3->pNext;
            v2 -= timeoutTicks;
            if ( !v3 )
              goto LABEL_26;
          }
          if ( v3 && v2 )
            v3->timeoutTicks -= v2;
        }
LABEL_26:
        runTicks = 10;
        if ( vm_currentActiveProcess > 9 )
        {
          v12 = 2;
          if ( 0x64u / vm_currentActiveProcess > 1 )
            v12 = 0x64u / vm_currentActiveProcess;
          runTicks = v12;
        }
        v13 = vm_getCurrentTick();
        v14 = (Vm_PCB_T *)(vm_currentActiveProcess - 1);
        currentTick = v13;
        if ( vm_currentActiveProcess )
        {
          if ( vm_pActiveProcess )
          {
            while ( 1 )
            {
              if ( vm_getCurrentTick() - currentTick >= 20 * runTicks )
                goto LABEL_85;
              if ( vm_gatheringCallStackInfo )
                vm_enterPPCopyStackCriticalSection();
              vm_funCp = (Vm_PCB_T *)vm_pActiveProcess->funCp;
              vm_funContext = vm_pActiveProcess->funContext;
              vm_funContextSize = vm_pActiveProcess->funContextSize;
              vm_updateCurrentFunCallContext();
              v15 = vm_pActiveProcess;
              vm_ctrlFlags = vm_pActiveProcess->ctrlFlags;
              vm_inFd = (Vm_Io_Callback_Instance_T)vm_pActiveProcess->inFd;
              vm_outFd = vm_pActiveProcess->outFd;
              vm_rpcClient = (void *)vm_pActiveProcess->rpcClient;
              vm_sp = vm_pActiveProcess->sp;
              vm_stack = (Vm_Value_T_2 *)vm_pActiveProcess->stack;
              vm_stackSize = vm_pActiveProcess->stackSize;
              if ( vm_gatheringCallStackInfo )
              {
                vm_leavePPCopyStackCriticalSection();
                v15 = vm_pActiveProcess;
              }
              resumeHook = v15->resumeHook;
              if ( !resumeHook )
                break;
              pResumeCookie = v15->pResumeCookie;
              v15->resumeHook = 0;
              v15->pResumeCookie = 0;
              v17 = resumeHook(v15, pResumeCookie);
              if ( !v17 )
              {
                if ( (vm_ctrlFlags & 4) == 0 )
                  break;
                vm_ctrlFlags &= ~4u;
                v17 = -8104;
              }
LABEL_39:
              v18 = v17;
              if ( v17 )
              {
                v19 = v17 <= -8104;
                if ( v17 == -8104 )
                  goto LABEL_76;
              }
              else
              {
                vm_enterThreadTick = vm_getCurrentTick();
                vm_beginForBackend();
                v18 = vm_doActions((Vm_PCB_1 *)vm_pActiveProcess, v28);
                vm_endForBackend();
                v29 = vm_getCurrentTick() - vm_enterThreadTick;
                v30 = vm_pActiveProcess;
                vm_pActiveProcess->holdTicks += v29;
                v30->totalTicks += v29;
                v19 = v18 <= -8104;
                if ( v18 == -8104 )
                {
LABEL_76:
                  if ( vm_gatheringCallStackInfo )
                    vm_enterPPCopyStackCriticalSection();
                  v31 = vm_pActiveProcess;
                  vm_pActiveProcess->ctrlFlags = vm_ctrlFlags;
                  v32 = vm_gatheringCallStackInfo;
                  v31->sp = vm_sp;
                  v31->funCp = (int)vm_funCp;
                  vm_stack = 0;
                  vm_rpcClient = (void *)-1;
                  if ( v32 )
                  {
                    vm_leavePPCopyStackCriticalSection();
                    v31 = vm_pActiveProcess;
                  }
                  pPrev = v31->pPrev;
                  pPrev->pNext = v31->pNext;
                  pNext = v31->pNext;
                  pNext->pPrev = pPrev;
                  --vm_currentActiveProcess;
                  v27 = 0;
                  pPendOnObject = v31->pPendOnObject;
                  if ( pNext != v31 )
                    v27 = pNext;
                  v31->flags |= 4u;
                  vm_pActiveProcess = v27;
                  v31->pNext = 0;
                  v31->pPrev = 0;
                  if ( !pPendOnObject )
                    goto LABEL_74;
                  *pPendOnObject->ppLastPCB = v31;
                  pPendOnObject->ppLastPCB = &v31->pNext;
                  goto LABEL_55;
                }
              }
              if ( v19 )
              {
                if ( v18 == -8106 )
                {
                  if ( vm_gatheringCallStackInfo )
                    vm_enterPPCopyStackCriticalSection();
                  v41 = vm_pActiveProcess;
                  vm_pActiveProcess->ctrlFlags = vm_ctrlFlags;
                  v41->sp = vm_sp;
                  v41->funCp = (int)vm_funCp;
                  vm_rpcClient = (void *)-1;
                  vm_stack = 0;
                  if ( vm_gatheringCallStackInfo )
                    vm_leavePPCopyStackCriticalSection();
                  vm_alreadyShutdown = (Vm_Timer_T *)1;
                  vm_clearEnvironment();
                  return -99;
                }
                if ( v18 == -8105 )
                {
                  if ( vm_gatheringCallStackInfo )
                    vm_enterPPCopyStackCriticalSection();
                  v42 = vm_pActiveProcess;
                  vm_pActiveProcess->ctrlFlags = vm_ctrlFlags;
                  v42->sp = vm_sp;
                  v42->funCp = (int)vm_funCp;
                  vm_rpcClient = (void *)-1;
                  vm_stack = 0;
                  if ( vm_gatheringCallStackInfo )
                    vm_leavePPCopyStackCriticalSection();
                  vm_clearEnvironment();
                  return 0;
                }
              }
              else
              {
                if ( v18 == -8103 )
                {
                  if ( vm_gatheringCallStackInfo )
                    vm_enterPPCopyStackCriticalSection();
                  v36 = vm_pActiveProcess;
                  vm_pActiveProcess->ctrlFlags = vm_ctrlFlags;
                  v37 = vm_gatheringCallStackInfo;
                  v36->sp = vm_sp;
                  v36->funCp = (int)vm_funCp;
                  vm_stack = 0;
                  vm_rpcClient = (void *)-1;
                  if ( v37 )
                  {
                    vm_leavePPCopyStackCriticalSection();
                    v36 = vm_pActiveProcess;
                  }
                  v38 = v36->pPrev;
                  v38->pNext = v36->pNext;
                  v39 = v36->pNext;
                  v39->pPrev = v38;
                  --vm_currentActiveProcess;
                  v40 = 0;
                  if ( v39 != v36 )
                    v40 = v39;
                  vm_pActiveProcess = v40;
                  v36->flags |= 2u;
                  v36->pNext = 0;
                  v36->pPrev = 0;
                  v25 = v40;
                  goto LABEL_56;
                }
                if ( !v18 )
                {
                  if ( vm_gatheringCallStackInfo )
                    vm_enterPPCopyStackCriticalSection();
                  v25 = vm_pActiveProcess;
                  vm_pActiveProcess->ctrlFlags = vm_ctrlFlags;
                  v25->sp = vm_sp;
                  v25->funCp = (int)vm_funCp;
                  vm_rpcClient = (void *)-1;
                  vm_stack = 0;
                  if ( vm_gatheringCallStackInfo )
                  {
                    vm_leavePPCopyStackCriticalSection();
                    v25 = vm_pActiveProcess;
                  }
                  if ( (v25->flags & 0x400100) != 0 && !vm_forceScriptRelinquish )
                    goto LABEL_56;
                  v27 = v25->pNext;
                  v25->holdTicks = 0;
                  vm_pActiveProcess = v27;
LABEL_74:
                  v25 = v27;
                  goto LABEL_56;
                }
              }
              if ( vm_gatheringCallStackInfo )
                vm_enterPPCopyStackCriticalSection();
              v20 = vm_pActiveProcess;
              vm_pActiveProcess->ctrlFlags = vm_ctrlFlags;
              v20->sp = vm_sp;
              v20->funCp = (int)vm_funCp;
              vm_rpcClient = (void *)-1;
              vm_stack = 0;
              if ( vm_gatheringCallStackInfo )
              {
                vm_leavePPCopyStackCriticalSection();
                v20 = vm_pActiveProcess;
              }
              v21 = v20->pPrev;
              v21->pNext = v20->pNext;
              v22 = v20->pNext;
              v23 = 0;
              --vm_currentActiveProcess;
              if ( v22 != v20 )
                v23 = v22;
              v22->pPrev = v21;
              vm_pActiveProcess = v23;
              v24 = 0;
              v20->pNext = 0;
              v20->pPrev = 0;
              if ( v18 != -8101 )
                v24 = 3 * (v18 != -8102) - 4;
              if ( !v20->returnCode )
                v20->returnCode = v24;
              vm_deleteProcess(v14);
LABEL_55:
              v25 = vm_pActiveProcess;
LABEL_56:
              v14 = (Vm_PCB_T *)((char *)v14 - 1);
              if ( v14 == (Vm_PCB_T *)-1 )
                goto LABEL_86;
              if ( !v25 )
                goto LABEL_58;
            }
            v17 = 0;
            goto LABEL_39;
          }
        }
        else
        {
LABEL_85:
          v25 = vm_pActiveProcess;
LABEL_86:
          if ( v25 )
            return 1;
        }
LABEL_58:
        if ( !vm_pInvokeList )
        {
          result = 0;
          if ( vm_pTimerList )
            return -1;
          return result;
        }
        return 1;
      }
      v5 = vm_pInvokeList;
      if ( vm_pInvokeList )
      {
        while ( 1 )
        {
          v6 = (int *)v5;
          v7 = v5->pNext;
          v8 = &vm_pInvokeList;
          if ( v7 )
            v8 = vm_ppLastInvokeNode;
          vm_pInvokeList = v7;
          vm_ppLastInvokeNode = v8;
          if ( !vm_executeAndFreeTimer(0, v6) )
            break;
          v5 = vm_pInvokeList;
          if ( !vm_pInvokeList )
            goto LABEL_19;
        }
      }
      else
      {
LABEL_19:
        v9 = vm_pTimerList;
        v10 = (int *)vm_pTimerList;
        if ( !vm_pTimerList )
          goto LABEL_26;
        v11 = vm_pTimerList->timeoutTicks;
        if ( v11 > v2 )
        {
LABEL_24:
          if ( v9 )
            v9->timeoutTicks -= v2;
          goto LABEL_26;
        }
        while ( 1 )
        {
          vm_pTimerList = v9->pNext;
          v2 -= v11;
          if ( !vm_executeAndFreeTimer(0, v10) )
            break;
          v9 = vm_pTimerList;
          v10 = (int *)vm_pTimerList;
          if ( !vm_pTimerList )
            goto LABEL_26;
          v11 = vm_pTimerList->timeoutTicks;
          if ( v11 > v2 )
            goto LABEL_24;
        }
      }
      return returnCode;
    }
  }
  return result;
}