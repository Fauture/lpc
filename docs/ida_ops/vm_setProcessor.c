void __cdecl vm_setProcessor(int (*a1)(Vm_PCB *), const char *a2)
{
  int (*v4)(Vm_PCB *); // eax

  v4 = a1;
  if ( !a1 )
  {
    if ( vm_confEnableJit )
    {
      v4 = (int (*)(Vm_PCB *))vm_jitProcessor;
      a2 = "jit";
    }
    else
    {
      v4 = vm_regularProcessor;
      a2 = "regular";
    }
  }
  if ( v4 != vm_processor )
  {
    vm_processor = v4;
    vm_processorName = a2;
    vm_ctrlFlags |= 0x20u;
  }
}