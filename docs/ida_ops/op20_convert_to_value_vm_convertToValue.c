void __usercall vm_convertToValue(Vm_String_T *a1@<esi>)
{
  Vm_Buffer_T *buffer; // ebx
  bool v2; // zf
  int ref; // edx
  unsigned int realSize; // eax

  buffer = vm_stack[vm_sp].v.buffer;
  vm_copyValue((char *)&vm_stack[vm_sp], (int)buffer);
  v2 = buffer->ref-- == 1;
  if ( v2 && buffer )
  {
    vm_cleanupValue(buffer, a1, (Vm_Value_T_2 *)buffer);
    --vm_totalValues;
    if ( buffer == (Vm_Buffer_T *)vm_allValues )
    {
      vm_allValues = (Vm_Value_T_2 *)buffer->realSize;
      if ( vm_allValues )
        vm_allValues->pPrev = 0;
    }
    else
    {
      ref = buffer[1].ref;
      *(_DWORD *)(ref + 12) = buffer->realSize;
      realSize = buffer->realSize;
      if ( realSize )
        *(_DWORD *)(realSize + 20) = ref;
    }
    buffer->ptr = 0;
    pdb_freeTinyBlock(buffer);
  }
}