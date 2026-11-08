int __cdecl vm_resumeProcess(unsigned int a1)
{
  int result; // eax
  Vm_PCB_T *v3; // ebx
  unsigned int flags; // edx
  int suspendLevel; // eax
  Vm_PCB_T *v6; // edx
  Vm_PCB *pPrev; // eax

  result = 0;
  if ( a1 < vm_maxProcess )
  {
    v3 = &vm_allPCB[a1];
    flags = v3->flags;
    result = 0;
    if ( (flags & 1) != 0 && (flags & 0x20) == 0 && (flags & 2) != 0 )
    {
      suspendLevel = v3->suspendLevel;
      if ( suspendLevel )
      {
        v3->suspendLevel = suspendLevel - 1;
        return 1;
      }
      else
      {
        v3->flags = flags & 0xFFFFFFFD;
        if ( (flags & 4) == 0 )
        {
          if ( (flags & 0x80u) != 0 )
            vm_removeTimerFromTimerList(1, v3);
          v6 = vm_pActiveProcess;
          if ( vm_pActiveProcess )
          {
            pPrev = vm_pActiveProcess->pPrev;
            pPrev->pNext = v3;
            v3->pPrev = pPrev;
            v3->pNext = v6;
            v6->pPrev = v3;
            v3->holdTicks = 0;
          }
          else
          {
            v3->pNext = v3;
            v3->pPrev = v3;
            vm_pActiveProcess = v3;
          }
          ++vm_currentActiveProcess;
        }
        return 1;
      }
    }
  }
  return result;
}