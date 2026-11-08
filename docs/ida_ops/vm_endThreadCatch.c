void vm_endThreadCatch()
{
  Vm_Thread_Info_T *CurrentThreadInfo; // eax
  Vm_PCB_T *v1; // edx
  Vm_Thread_Info_T *v2; // ecx
  int v3; // eax
  int funCp; // ebx
  bool v5; // zf
  int topCatch; // edx

  CurrentThreadInfo = vm_getCurrentThreadInfo();
  v1 = vm_pActiveProcess;
  v2 = CurrentThreadInfo;
  v3 = vm_pActiveProcess->catchCp - 1;
  funCp = 0;
  v5 = vm_pActiveProcess->catchCp == 1;
  vm_pActiveProcess->catchCp = v3;
  if ( v3 >= 0 && !v5 )
    funCp = v1->catchContext[v3 - 1].funCp;
  topCatch = v2->topCatch;
  if ( topCatch > 0 )
    *(&v2->threadType + 41 * topCatch) = funCp;
}