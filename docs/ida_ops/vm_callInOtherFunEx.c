void __usercall vm_callInOtherFunEx(char a1@<sil>, int a2, int a3)
{
  unsigned int v5; // esi
  Vm_Array_T *v6; // edx

  v5 = (vm_sp << 20) | ((a3 & 0xFFF) << 8) & 0xFFFFF | a2 & 1 | a1 & 0xFE;
  if ( vm_stackTopValue()->type == 6 )
  {
    vm_stackTopValue();
    vm_pushNumber(0);
    vm_allocateEmptyArray(v5);
    vm_pushArray(v6);
    vm_callInOtherArray2((void *)((vm_sp << 20) | v5 & 0xFFFFF));
    if ( (v5 & 1) != 0 )
      vm_ctrlFlags |= 0x10u;
  }
  else if ( vm_callInOtherFun(a2, a3) )
  {
    vm_hookCallbackWhenReturn(vm_callInOtherFunExReturn, (void *)v5);
  }
  else if ( (v5 & 1) != 0 )
  {
    vm_returnFromEfun();
  }
}