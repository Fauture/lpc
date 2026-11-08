Vm_Status __cdecl vm_run(
        int fileFd,
        int inFd,
        int outFd,
        int priority,
        int rpcClient,
        const char *pInFileName,
        int *pErrCode,
        int isFreeDefine,
        Vm_End_Execute_Hook_T endHook,
        void *pCookie)
{
  Vm_Status v10; // ebx
  int v12; // esi
  int v13; // edx
  Vm_Node_T *nodes; // edx
  int v15; // ecx
  Vm_Action_No_T actionNo; // al
  int started; // eax
  Vm_Status v18; // ebx
  int VmMutex; // eax
  int v20; // edx
  Vm_XNode_T *v21; // ecx
  Vm_PCB_Attribs_T *Memory; // eax
  const char *v23; // esi
  Vm_Tid_T v24; // edx
  unsigned int v25; // ecx
  const char *v26; // eax
  int v27; // ecx
  Vm_Tid_T v28; // edx
  int start; // [esp+8h] [ebp-40h] BYREF
  Vm_Scratchpad_Info_T *pScratchpad; // [esp+Ch] [ebp-3Ch] BYREF
  Vm_Code_Segment_T_0 finalCodeSegment; // [esp+10h] [ebp-38h] BYREF

  if ( !vm_runningInScheduler() || (v10 = Vm_Status::VM_RUNNING_IN_MAIN_LOOP, endHook) )
  {
    v10 = Vm_Status::VM_CAN_NOT_OPEN_INPUT;
    if ( fileFd )
    {
      LOBYTE(v10) = -104;
      if ( vm_getVmMutex() )
      {
        if ( vm_buildScratch(&pScratchpad) )
        {
          vm_inFd = (Vm_Io_Callback_Instance_T)inFd;
          vm_outFd = outFd;
          vm_rpcClient = (void *)rpcClient;
          currentFile = (Vm_String_T **)pInFileName;
          v12 = _setjmp(vm_shellEnv);
          if ( !v12 )
          {
            started = vm_startNewFile(0, 0);
            if ( started )
            {
              if ( started <= 0 )
              {
                if ( started == -1 )
                  v12 = -2;
              }
              else if ( started == 1 && !vm_loadBinary((int)&finalCodeSegment, (Vm_Code_Segment_T_0 *)fileFd) )
              {
                vm_errprintf("Failed to load binary: %s\n", pInFileName);
                v12 = -2;
              }
            }
            else if ( vm_yyparse(0) || vm_errors )
            {
              v12 = -2;
            }
            else
            {
              vm_genXAction(0, 0, 0x1Du, 0, 0);
              allActions = vm_appendXActions((Vm_XNode_T *)v10, v21);
              vm_postProcessXActionsTempMark(0);
              allActions = vm_optimize(allActions);
              vm_compilingProgram();
              vm_transformXActions(&finalCodeSegment, 0, (int)allActions);
            }
          }
          if ( !vm_endNewFile(v12 == 0, isFreeDefine, &finalCodeSegment) )
            v12 = -2;
          vm_releaseVmMutex();
          if ( !v12 )
          {
            v13 = 1;
            if ( finalCodeSegment.nodes )
            {
              nodes = finalCodeSegment.nodes;
              v15 = 0;
              if ( finalCodeSegment.nodesCount <= 0 )
              {
LABEL_16:
                v13 = 1;
              }
              else
              {
                while ( 1 )
                {
                  actionNo = nodes->actionNo;
                  if ( actionNo != 46 && actionNo != 29 )
                    break;
                  ++v15;
                  ++nodes;
                  if ( finalCodeSegment.nodesCount <= v15 )
                    goto LABEL_16;
                }
                v13 = 0;
              }
            }
            if ( v13 )
            {
              vm_destructScratch(0);
              if ( endHook )
                ((void (__cdecl *)(_DWORD, _DWORD, void *, Vm_End_Execute_Hook_T))endHook)(0, 0, pCookie, endHook);
              v12 = 0;
            }
            else
            {
              v18 = (Vm_Status)pScratchpad;
              VmMutex = vm_getVmMutex();
              v20 = -104;
              if ( VmMutex )
              {
                vm_inFd = (Vm_Io_Callback_Instance_T)inFd;
                vm_outFd = outFd;
                vm_rpcClient = (void *)rpcClient;
                Memory = (Vm_PCB_Attribs_T *)pdb_allocateMemory(
                                               0xCu,
                                               "VM",
                                               "/home/cc/xxxx/gameshell/src/vm/vm_shell.c",
                                               0x83Du);
                v23 = (const char *)Memory;
                if ( Memory )
                {
                  Memory->stackSize = v18;
                  Memory->funContextSize = (int)endHook;
                  Memory->catchContextSize = (int)pCookie;
                  v18 = vm_spawnProcess(
                          (const char *)&finalCodeSegment,
                          0,
                          (int)&start,
                          0,
                          (Vm_End_Hook_T)&finalCodeSegment,
                          vm_endOfActions,
                          Memory);
                  if ( v18 )
                  {
                    if ( (int)vm_outputFd > 0 )
                      vm_write(
                        (pthread_mutex_t *)v18,
                        "Can not spawn thread.\n",
                        strlen("Can not spawn thread.\n"),
                        (int)vm_outputFd);
                  }
                  else
                  {
                    vm_setProcessAttrib(v24, v25, 0, (unsigned int *)&finalCodeSegment);
                    vm_setThreadPriority(0xFFFFFFFF, v27);
                    vm_resumeProcess(start);
                    vm_bringupMainLoop();
                    if ( endHook )
                    {
                      if ( pErrCode )
                        *pErrCode = -2;
                      v18 = Vm_Status::VM_OK;
                    }
                    else
                    {
                      vm_waitProcessEnd(v28, (int *)start);
                      v18 = Vm_Status::VM_OK;
                    }
                  }
                }
                else
                {
                  v18 = Vm_Status::VM_CAN_NOT_PREPARE_TO_RUN;
                  if ( (int)vm_outputFd > 0 )
                    vm_write(
                      (pthread_mutex_t *)0xFFFFFED2,
                      "Can not build script-info before spawn thread.\n",
                      strlen("Can not build script-info before spawn thread.\n"),
                      (int)vm_outputFd);
                }
                if ( pErrCode )
                  *pErrCode = 0;
                vm_releaseVmMutex();
                if ( v18 && v23 )
                  pdb_freeMemory((void *)v18, v26, v23, (unsigned int)"VM");
                v20 = v18;
              }
              v12 = 0;
              if ( v20 )
              {
                if ( (int)vm_outputFd > 0 )
                  vm_write((pthread_mutex_t *)v18, "Execution abort.\n", strlen("Execution abort.\n"), (int)vm_outputFd);
                v12 = -3;
              }
            }
          }
          if ( pErrCode )
            *pErrCode = v12;
          v10 = Vm_Status::VM_OK;
          if ( v12 )
          {
            vm_destructScratch((Vm_Scratchpad_Info_T *)v12);
            return Vm_Status::VM_FAILED_TO_EXECUTE_SCRIPT;
          }
        }
        else
        {
          LOBYTE(v10) = -105;
          vm_releaseVmMutex();
        }
      }
    }
  }
  return v10;
}