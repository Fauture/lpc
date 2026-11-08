int __cdecl vm_mainLoop(void *pPara)
{
  Vm_Posix_Sem_T *v1; // ebx
  Vm_Posix_Event_T *CurrentTick; // edi
  int v3; // eax
  int v4; // esi
  int v5; // eax
  Vm_Tick_T timeoutTicks; // eax
  int r3; // [esp+0h] [ebp-18h]
  int r2; // [esp+4h] [ebp-14h]
  unsigned int ti4; // [esp+8h] [ebp-10h]

  vm_alreadyStop = 0;
  if ( !tname_0.pThis )
  {
    tname_0.pThis = (Vm_Fun_Ptr::Vm_Value *)1;
    vm_mainTaskId = vm_getCurrentTaskId();
  }
  vm_scheduleProcessTicks = 0;
  vm_scheduleScanSocketTicks = 0;
  vm_scheduleScanTelnetTicks = 0;
  vm_scheduleIdleTicks = 0;
  while ( !vm_requestStop )
  {
    while ( vm_getVmMutex() )
    {
      CurrentTick = (Vm_Posix_Event_T *)vm_getCurrentTick();
      vm_isInSchduler = 1;
      vm_schedulerRuntimeState = 1;
      v3 = vm_scheduleProcessInternal();
      vm_schedulerRuntimeState = 0;
      vm_isInSchduler = 0;
      if ( v3 == -99 )
      {
        vm_printf("VM is shutting down(1)...\n");
        vm_releaseVmMutex();
        goto LABEL_23;
      }
      v4 = vm_getCurrentTick();
      vm_schedulerRuntimeState = 2;
      r2 = vm_scanSocketInMainLoop();
      v1 = (Vm_Posix_Sem_T *)vm_getCurrentTick();
      vm_schedulerRuntimeState = 3;
      r3 = vm_scanTelnet();
      vm_schedulerRuntimeState = 0;
      ti4 = vm_getCurrentTick();
      vm_scheduleProcessTicks += v4 - (_DWORD)CurrentTick;
      vm_scheduleScanSocketTicks += (unsigned int)v1 - v4;
      vm_scheduleScanTelnetTicks += ti4 - (_DWORD)v1;
      vm_releaseVmMutex();
      v5 = vm_forceVMRelinquish;
      if ( vm_forceVMRelinquish )
      {
        if ( vm_forceVMRelinquish > 10 )
        {
          vm_forceVMRelinquish = 10;
          v5 = 10;
        }
        v1 = 0;
        if ( v5 > 0 )
        {
          do
          {
            v1 = (Vm_Posix_Sem_T *)((char *)v1 + 1);
            vm_sleep(v4);
          }
          while ( (int)v1 < vm_forceVMRelinquish );
        }
      }
      if ( !vm_pActiveProcess && !vm_pInvokeList || (CurrentTick = (Vm_Posix_Event_T *)r2) != 0 || (v4 = r3) != 0 )
      {
        v1 = (Vm_Posix_Sem_T *)pPara;
        if ( pPara )
          return 0;
        timeoutTicks = vm_relinquishTick;
        if ( vm_pTimerList )
        {
          timeoutTicks = vm_pTimerList->timeoutTicks;
          if ( timeoutTicks > vm_relinquishTick )
            timeoutTicks = vm_relinquishTick;
        }
        if ( timeoutTicks )
          vm_waitEventByTime(CurrentTick, 0);
        else
          vm_sleep(v4);
      }
      vm_scheduleIdleTicks += vm_getCurrentTick() - ti4;
      if ( vm_requestStop )
        goto LABEL_23;
    }
  }
LABEL_23:
  vm_alreadyStop = 1;
  vm_giveSemaphore(v1);
  return 1;
}