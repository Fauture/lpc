void __usercall vm_toUnreserveStackValue(Vm_String_T *a1@<esi>, Vm_Buffer_T *a2)
{
  int v4; // eax

  while ( 1 )
  {
    v4 = (int)a2;
    a2 = (Vm_Buffer_T *)((char *)a2 - 1);
    if ( v4 <= 0 )
      break;
    vm_popStack(a2, a1);
  }
}