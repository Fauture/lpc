void __cdecl vm_callInEfun(Vm_Index_T argn, int a2)
{
  Vm_Fun *pFun; // ebx
  Vm_FunCall_Context_T *v4; // edx
  bool v5; // zf
  Vm_PCB_T *CurrentPCB; // eax

  if ( (int)vm_funCp >= vm_funContextSize )
    vm_error("Too depth (%d) function call!\n", vm_funCp);
  if ( argn < 0 || vm_pGlobalProgram->maxFunctionNo <= argn )
    vm_error("External function (index = %d) is not implemented yet.\n", argn);
  pFun = vm_pGlobalProgram->pFunctions[argn].pFun;
  if ( vm_gatheringCallStackInfo )
    vm_enterPPCopyStackCriticalSection();
  vm_funCp = (Vm_PCB_T *)((char *)vm_funCp + 1);
  vm_updateCurrentFunCallContext();
  v4 = vm_currentContext;
  qmemcpy(vm_currentContext, &vm_currentContext[-1], sizeof(Vm_FunCall_Context_T));
  v4->bp = vm_sp;
  v4->argNum = a2;
  v5 = vm_gatheringCallStackInfo == 0;
  v4->flags = 1;
  v4->f.pEfun = (Vm_Efun *)pFun;
  if ( !v5 )
    vm_leavePPCopyStackCriticalSection();
  ((void (*)(void))pFun->codeSegment.nodes)();
  if ( !vm_ctrlFlags )
    goto LABEL_11;
  if ( (vm_ctrlFlags & 0x10) == 0 )
  {
    if ( (vm_ctrlFlags & 4) == 0 )
    {
LABEL_11:
      vm_returnFromEfun();
      return;
    }
    CurrentPCB = vm_getCurrentPCB();
    CurrentPCB->flags |= 0x40u;
  }
}