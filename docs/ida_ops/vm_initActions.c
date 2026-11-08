int vm_initActions()
{
  Vm_Action_Info_T *v0; // eax
  int (*v1)(Vm_PCB *); // eax
  const char *v2; // edx

  if ( actionInfoTable[0].actionNo != 0xF0 )
  {
    v0 = actionInfoTable;
    do
      ++v0;
    while ( v0->actionNo != 0xF0 );
  }
  if ( vm_confEnableJit )
  {
    v1 = (int (*)(Vm_PCB *))vm_jitProcessor;
    v2 = "jit";
  }
  else
  {
    v1 = vm_regularProcessor;
    v2 = "regular";
  }
  if ( v1 != vm_processor )
  {
    vm_processor = v1;
    vm_processorName = v2;
    vm_ctrlFlags |= 0x20u;
  }
  return 1;
}