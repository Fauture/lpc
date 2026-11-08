void __usercall vm_returnFromFakeFun(Vm_Value_T_2 *a1@<edi>)
{
  Vm_Object *thisObject; // esi
  Vm_FunCall_Context_T *v2; // ebx
  Vm_PCB_T *CurrentPCB; // eax

  thisObject = vm_currentContext->thisObject;
  v2 = &vm_funContext[(_DWORD)vm_funCp];
  if ( vm_gatheringCallStackInfo )
  {
    vm_enterPPCopyStackCriticalSection();
    vm_funCp = (Vm_PCB_T *)((char *)vm_funCp - 1);
    vm_updateCurrentFunCallContext();
    vm_leavePPCopyStackCriticalSection();
    if ( (v2->flags & 8) != 0 )
      goto LABEL_3;
  }
  else
  {
    vm_funCp = (Vm_PCB_T *)((char *)vm_funCp - 1);
    vm_updateCurrentFunCallContext();
    if ( (v2->flags & 8) != 0 )
      goto LABEL_3;
  }
  CurrentPCB = vm_getCurrentPCB();
  CurrentPCB->flags &= ~0x100u;
LABEL_3:
  vm_popNStack(a1, v2->argNum);
  if ( thisObject )
    vm_unExecRefObject((Vm_Object_T *)v2);
  if ( SLOBYTE(v2->flags) < 0 )
    v2->returnCallback(v2->pCookie);
}