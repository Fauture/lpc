void vm_pendCurrentProcess()
{
  unsigned int flags; // edx

  if ( (vm_pActiveProcess->flags & 0x200000) != 0 )
    vm_error("Thread \"%s\"(%d) can't be pended.\n", vm_pActiveProcess->name, vm_pActiveProcess->tid);
  flags = vm_pActiveProcess->flags;
  if ( (flags & 0x100000) != 0 && (flags & 0x80) == 0 )
    vm_error("Thread \"%s\"(%d) can't be pended forever.\n", vm_pActiveProcess->name, vm_pActiveProcess->tid);
  vm_ctrlFlags |= 4u;
}