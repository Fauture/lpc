void __usercall vm_popNStack(Vm_Value_T_2 *a1@<edi>, int a2)
{
  int v3; // ebx

  if ( a2 > 0 )
  {
    v3 = a2;
    do
    {
      vm_cleanupValue(a1);
      --v3;
      ++vm_sp;
    }
    while ( v3 );
  }
}