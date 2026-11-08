void __cdecl vm_initForLoop(int varNo)
{
  Vm_Buffer_T *v1; // ebx
  Vm_String_T *v2; // esi
  int bp; // eax
  int v4; // eax
  Vm_Value_T_2 *v5; // eax
  int number; // edx
  _BYTE *info; // ecx
  Vm_Value_T_2 *v8; // ebx
  Vm_Value_T_2 *v9; // edi
  _DWORD *Memory; // eax
  _DWORD *v11; // ebx
  _DWORD *v12; // edi
  unsigned __int8 type; // al

  if ( vm_funCp )
    bp = vm_funContext[(_DWORD)vm_funCp].bp;
  else
    bp = vm_stackSize;
  v4 = varNo + bp;
  if ( v4 < 0 || v4 >= vm_stackSize )
    vm_error("Bad loop ctrl, failed to loop (in cond).\n");
  v9 = &vm_stack[v4];
  vm_cleanupValue(v1, v2, v9);
  Memory = pdb_allocateMemory(0x24u, "VM", "/home/cc/xxxx/gameshell/src/vm/vm_loop.c", 0x10Au);
  v11 = Memory;
  if ( !Memory )
    vm_error("Failed to create loop ctrl.\n");
  v9->v.number = (int)Memory;
  v9->type = 20;
  memset(Memory, 0, 0x24u);
  *Memory = 1;
  Memory[1] = 0;
  v12 = Memory;
  v5 = &vm_stack[vm_sp];
  number = v5->v.number;
  v11[2] = number;
  info = v5[2].v.info;
  v8 = v5 + 1;
  v12[1] = info;
  *((_BYTE *)v12 + 16) = v5[1].attrib & 1;
  if ( number == 1 )
  {
LABEL_15:
    if ( v5[1].type != 2 )
      vm_error("Bad value to loop, expected integer (in init).\n");
    v12[6] = v5[1].v.info;
    if ( info[4] != 2 )
      vm_error("Bad init value to loop, expected integer (in init).\n");
    v12[5] = *((_DWORD *)info + 2);
    vm_popNStack((Vm_Value_T_2 *)v12, 3);
    return;
  }
  if ( number <= 1 )
  {
    if ( number != -1 )
LABEL_9:
      vm_error("Bad step type for loop.\n");
    goto LABEL_15;
  }
  if ( number != 997 )
    goto LABEL_9;
  type = v5[1].type;
  if ( type == 6 )
  {
    v12[2] = 901;
    v12[5] = v8->v.info;
    ++*(_DWORD *)v8->v.info;
    v12[6] = 0;
  }
  else
  {
    if ( type != 7 )
      vm_error("Bad type of value to loop in, expected mapping or array.\n");
    v12[2] = 902;
    v12[5] = v8->v.info;
    ++*(_DWORD *)v8->v.info;
    v12[6] = 0;
    v12[7] = 0;
    v12[8] = 0;
  }
  vm_popNStack((Vm_Value_T_2 *)v12, 3);
}