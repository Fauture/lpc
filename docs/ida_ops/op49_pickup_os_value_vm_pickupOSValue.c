void __cdecl vm_pickupOSValue(int *a1, int a2)
{
  if ( a2 == 2 )
  {
    vm_pushNumber(*(unsigned __int16 *)a1);
  }
  else if ( a2 == 1 )
  {
    vm_pushNumber(*(unsigned __int8 *)a1);
  }
  else
  {
    if ( a2 != 4 )
      vm_error("Bad length %d of the value to read.\n", a2);
    vm_pushNumber(*a1);
  }
}