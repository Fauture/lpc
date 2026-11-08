void __usercall vm_cmpAndJmpInLoop(int a1@<eax>, int a2@<edx>, Vm_String_T *a3@<esi>, int continueOffset)
{
  int bp; // eax
  int v7; // eax
  Vm_Buffer_T *v8; // ebx
  Vm_Value_T_2 *v9; // edi
  Vm_Value_T_2 *v10; // eax
  int v11; // esi
  Vm_FunCall_Context_T *v12; // eax
  int v13; // edx
  Vm_String_T *v14; // esi
  Vm_Buffer_T *v15; // ebx
  Vm_Array_T *EmptyArray; // ebx
  signed int v17; // ecx
  int v18; // edx
  Vm_String_T *i; // esi
  int v20; // eax
  int v21; // [esp+4h] [ebp-24h]
  int v22; // [esp+Ch] [ebp-1Ch]
  void *info; // [esp+10h] [ebp-18h]
  int to; // [esp+14h] [ebp-14h]

  if ( vm_funCp )
    bp = vm_funContext[(_DWORD)vm_funCp].bp;
  else
    bp = vm_stackSize;
  v7 = a1 + bp;
  if ( v7 < 0 || v7 >= vm_stackSize || (v10 = &vm_stack[v7], v10->type != 20) || (info = v10->v.info) == 0 )
    vm_error("Bad loop ctrl, failed to loop (in step).\n");
  v8 = (Vm_Buffer_T *)*((_DWORD *)info + 2);
  v9 = (Vm_Value_T_2 *)*((_DWORD *)info + 1);
  if ( v8 == (Vm_Buffer_T *)1 )
  {
LABEL_16:
    if ( v9->type != 2 )
    {
      vm_cleanupValue(v8, a3, *((Vm_Value_T_2 **)info + 1));
      v9->type = 2;
    }
    v11 = *((_DWORD *)info + 5);
    to = *((_DWORD *)info + 6);
    *((_DWORD *)info + 5) = (char *)v8 + v11;
    v9->v.number = v11;
    if ( *((_DWORD *)info + 3) || (int)v8 > 0 && v11 > to || (int)v8 < 0 && v11 < to )
    {
      v12 = vm_currentContext;
      v13 = continueOffset;
    }
    else
    {
      v12 = vm_currentContext;
      v13 = a2;
    }
    v12->ip += v13;
    if ( v11 == to )
      *((_DWORD *)info + 3) = 1;
    return;
  }
  if ( (int)v8 <= 1 )
  {
    if ( v8 != (Vm_Buffer_T *)-1 )
      return;
    goto LABEL_16;
  }
  if ( v8 == (Vm_Buffer_T *)901 )
  {
    v14 = (Vm_String_T *)*((_DWORD *)info + 6);
    v15 = (Vm_Buffer_T *)*((_DWORD *)info + 5);
    *((_DWORD *)info + 6) = (char *)&v14->ref + 1;
    if ( (unsigned int)v14 < v15->attrib )
    {
      vm_currentContext->ip += a2;
      vm_cleanupValue(v15, v14, v9);
      vm_copyValue((Vm_Value_T_2 *)v14, (Vm_Value_T_2 *)v15);
      v9->attrib |= *((_BYTE *)info + 16);
      return;
    }
    goto LABEL_28;
  }
  if ( v8 != (Vm_Buffer_T *)902 )
    return;
  v22 = *((_DWORD *)info + 8);
  *((_DWORD *)info + 8) = v22 + 1;
  EmptyArray = (Vm_Array_T *)*((_DWORD *)info + 5);
  v17 = *((_DWORD *)info + 6);
  v21 = *((_DWORD *)info + 7);
  *((_DWORD *)info + 7) = v21 + 1;
  v18 = v21;
LABEL_30:
  if ( (signed int)EmptyArray->maxSize < v17 )
  {
LABEL_28:
    vm_currentContext->ip += continueOffset;
    return;
  }
  for ( i = *(Vm_String_T **)(EmptyArray[1].size + 4 * v17); ; i = (Vm_String_T *)i->ref )
  {
    if ( !i )
    {
      *((_DWORD *)info + 6) = ++v17;
      v18 = 0;
      *((_DWORD *)info + 7) = 1;
      goto LABEL_30;
    }
    v20 = v18--;
    if ( v20 <= 0 )
      break;
  }
  if ( v9->type != 6 || (EmptyArray = v9->v.array, EmptyArray->size != 3) )
  {
    vm_cleanupValue((Vm_Buffer_T *)EmptyArray, i, v9);
    v9->attrib |= *((_BYTE *)info + 16);
    EmptyArray = vm_allocateEmptyArray((unsigned int)i);
    v9->type = 6;
    v9->v.number = (int)EmptyArray;
  }
  vm_cleanupValue((Vm_Buffer_T *)EmptyArray, i, *(Vm_Value_T_2 **)EmptyArray->pValues);
  vm_copyValue((Vm_Value_T_2 *)i, (Vm_Value_T_2 *)EmptyArray);
  vm_cleanupValue((Vm_Buffer_T *)EmptyArray, i, *((Vm_Value_T_2 **)EmptyArray->pValues + 1));
  vm_copyValue((Vm_Value_T_2 *)i, (Vm_Value_T_2 *)EmptyArray);
  vm_cleanupValue((Vm_Buffer_T *)EmptyArray, i, *((Vm_Value_T_2 **)EmptyArray->pValues + 2));
  *(_BYTE *)(*((_DWORD *)EmptyArray->pValues + 2) + 4) = 2;
  *(_DWORD *)(*((_DWORD *)EmptyArray->pValues + 2) + 8) = v22;
  vm_currentContext->ip += a2;
}