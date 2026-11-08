int __cdecl vm_startProcess(Vm_PCB_T *pIdent)
{
  Vm_PCB_T *v2; // edx
  Vm_PCB *pPrev; // eax

  if ( SLOBYTE(pIdent->flags) < 0 )
    vm_removeTimerFromTimerList(1, pIdent);
  v2 = vm_pActiveProcess;
  if ( vm_pActiveProcess )
  {
    pPrev = vm_pActiveProcess->pPrev;
    pPrev->pNext = pIdent;
    pIdent->pPrev = pPrev;
    pIdent->holdTicks = 0;
    pIdent->pNext = v2;
    v2->pPrev = pIdent;
    ++vm_currentActiveProcess;
  }
  else
  {
    pIdent->pNext = pIdent;
    pIdent->pPrev = pIdent;
    ++vm_currentActiveProcess;
    vm_pActiveProcess = pIdent;
  }
  return 1;
}