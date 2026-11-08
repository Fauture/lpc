void vm_resortStack()
{
  unsigned int v0; // ebx
  int argNum; // ecx
  int i; // esi
  unsigned int v3; // edx
  int *p_ref; // ecx
  int v5; // esi
  unsigned int v6; // edi
  unsigned int v7; // eax
  Vm_Value_T_2 *v8; // ecx
  Vm_Value_T_2 *v9; // [esp+0h] [ebp-1Ch]
  int defaultPushed; // [esp+4h] [ebp-18h]
  int newArgNum; // [esp+8h] [ebp-14h]
  Vm_FunCall_Context_T *pContext; // [esp+Ch] [ebp-10h]

  pContext = &vm_funContext[(_DWORD)vm_funCp];
  v0 = vm_sp;
  argNum = pContext->argNum;
  defaultPushed = pContext->bp - vm_sp;
  newArgNum = argNum + defaultPushed;
  v9 = vm_stack;
  qmemcpy(&tempStack_0[vm_sp], &vm_stack[pContext->bp], 4 * ((unsigned int)(24 * argNum) >> 2));
  for ( i = 0; i < defaultPushed; ++i )
  {
    v3 = 24 * (v0 + i + pContext->argNum);
    p_ref = &v9[pContext->bp - i - 1].ref;
    tempStack_0[v3 / 0x18].ref = *p_ref;
    *(_DWORD *)(v3 + 138952644) = p_ref[1];
    *(_DWORD *)(v3 + 138952648) = p_ref[2];
    *(_DWORD *)(v3 + 138952652) = p_ref[3];
    *(_DWORD *)(v3 + 138952656) = p_ref[4];
    *(_DWORD *)(v3 + 138952660) = p_ref[5];
  }
  v5 = 0;
  if ( newArgNum > 0 )
  {
    while ( 1 )
    {
      v9[v0 + v5].type = tempStack_0[v0 + v5].type;
      vm_stack[v5 + vm_sp].attrib = tempStack_0[v5 + vm_sp].attrib;
      v6 = vm_sp;
      v7 = vm_sp + v5++;
      v8 = vm_stack;
      vm_stack[v7].v.number = *(_DWORD *)(v7 * 24 + 138952648);
      if ( v5 >= newArgNum )
        break;
      v0 = v6;
      v9 = v8;
    }
  }
  pContext->argNum = newArgNum;
  pContext->bp = vm_sp;
}