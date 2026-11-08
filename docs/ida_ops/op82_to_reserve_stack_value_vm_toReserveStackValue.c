void __cdecl vm_toReserveStackValue(int a1)
{
  int v3; // eax

  while ( 1 )
  {
    v3 = a1--;
    if ( v3 <= 0 )
      break;
    vm_pushUndefined();
  }
}