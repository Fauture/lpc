void __fastcall vm_bringupMainLoop(int a1, Vm_Posix_Event_T *a2)
{
  vm_raiseEvent(a2);
}