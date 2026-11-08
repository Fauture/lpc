void __cdecl vm_newThreadCatch(int a1)
{
  Vm_Thread_Info_T *CurrentThreadInfo; // eax
  Vm_PCB_T *v3; // ecx
  Vm_Thread_Info_T *v4; // edi
  int catchCp; // eax
  Vm_Catch_Info_T *v6; // ebx
  int CurrentFunCp; // eax
  unsigned int v8; // edx
  int topCatch; // edx

  CurrentThreadInfo = vm_getCurrentThreadInfo();
  v3 = vm_pActiveProcess;
  v4 = CurrentThreadInfo;
  if ( vm_pActiveProcess->catchCp >= vm_pActiveProcess->catchContextSize )
    vm_error("Too depth catch (%d).\n", vm_pActiveProcess->catchCp);
  catchCp = vm_pActiveProcess->catchCp;
  vm_pActiveProcess->catchCp = catchCp + 1;
  v6 = &v3->catchContext[catchCp];
  v6->catchIp = vm_currentContext->ip + a1;
  CurrentFunCp = vm_getCurrentFunCp();
  v8 = vm_sp;
  v6->funCp = CurrentFunCp;
  v6->sp = v8;
  topCatch = v4->topCatch;
  v6->catchCallback = 0;
  v6->pCookie = 0;
  *(&v4->threadType + 41 * topCatch) = CurrentFunCp;
}