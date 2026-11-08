void vm_createArgumentArray()
{
  unsigned int v0; // esi
  int i; // ebx

  v0 = vm_argumentInThisContext();
  for ( i = v0 - 1; i != -1; --i )
    vm_pushLocalVar(i);
  vm_createArray(v0);
}