void __cdecl vm_checkArgNum(int a1)
{
  const char **fptr; // ebx
  int v3; // esi
  Vm_Value_T_2 *v4; // edi
  int ValueType; // edx
  int v6; // edx
  const char *v7; // eax
  int argNum; // edx
  char v9; // al
  unsigned __int8 type; // dl
  Vm_Fun_Arg_T *pArgs; // ebx
  int i; // ecx
  int defaultActionNum; // eax
  const char *v14; // [esp-Ch] [ebp-24h]
  int v15; // [esp+0h] [ebp-18h]
  Vm_Fun_T *pFun; // [esp+4h] [ebp-14h]
  Vm_FunCall_Context_T *pContext; // [esp+8h] [ebp-10h]

  pContext = &vm_funContext[(_DWORD)vm_funCp];
  fptr = (const char **)pContext->f.pEfun[1].fptr;
  v3 = 0;
  for ( pFun = pContext->f.pFun; fptr; fptr = (const char **)fptr[4] )
  {
    if ( pContext->argNum <= v3 )
      break;
    v4 = &vm_stack[v3 + vm_sp];
    if ( ((_BYTE)fptr[1] & 1) != 0 )
    {
      v15 = v3 + 1;
      if ( v4->type != 8 )
        vm_error(
          "Argument %d for \"%s\" of %s need reference type, but got a normal value.\n",
          v3 + 1,
          *fptr,
          pFun->name);
      if ( *((_BYTE *)fptr + 5) != 0xFF )
      {
        ValueType = vm_getValueType((Vm_Value_T_2 *)fptr);
        if ( ValueType != *((unsigned __int8 *)fptr + 5) && (ValueType || (*((_BYTE *)fptr + 6) & 8) == 0) )
        {
LABEL_10:
          v14 = vm_valueType(ValueType);
          v7 = vm_valueType(v6);
          vm_error(
            "Type mismatched for argument %d \"%s\" of %s.\nExpected %s, got %s.\n",
            v15,
            *fptr,
            pFun->name,
            v7,
            v14);
        }
      }
    }
    else
    {
      if ( (*((_BYTE *)fptr + 6) & 2) != 0 )
        vm_convertValueToConst(vm_stack);
      v9 = *((_BYTE *)fptr + 5);
      v15 = v3 + 1;
      if ( v9 == -1 )
        goto LABEL_11;
      type = v4->type;
      if ( type == 8 )
        vm_error("Argument %d for \"%s\" of %s got a reference value.\n", v3 + 1, *fptr, pFun->name);
      if ( type == v9 )
      {
        if ( type == 10 && *(char *)(v4->v.number + 13) < 0 && (*((_BYTE *)fptr + 6) & 8) == 0 )
          vm_error(
            "Type mismatched for argument %d \"%s\" of %s.\nExpected object, got undefined (object is destructed).\n",
            v15,
            *fptr,
            pFun->name);
        goto LABEL_11;
      }
      if ( v9 == 3 )
      {
        if ( type == 2 )
        {
          v4->v.real = (float)v4->v.number;
          v4->type = 3;
          goto LABEL_11;
        }
      }
      else if ( v9 == 2 && type == 3 )
      {
        v4->v.number = (int)v4->v.real;
        v4->type = 2;
        goto LABEL_11;
      }
      if ( vm_getVarType((Vm_Value_T_2 *)fptr) || (*((_BYTE *)fptr + 6) & 8) == 0 )
        goto LABEL_10;
    }
LABEL_11:
    v3 = v15;
  }
  if ( pFun->minArgNo > v3 )
    vm_error("No input argument %d for \"%s\" of %s.\n", v3 + 1, *fptr, pFun->name);
  argNum = pContext->argNum;
  if ( argNum < pFun->maxArgNo )
  {
    pArgs = pFun->pArgs;
    for ( i = 0; pArgs; i += defaultActionNum )
    {
      if ( pArgs->no >= argNum )
        break;
      defaultActionNum = pArgs->defaultActionNum;
      pArgs = pArgs->pNext;
    }
    vm_currentContext->ip += i;
  }
  else
  {
    if ( argNum > v3 && (pFun->attrib & 1) == 0 )
      vm_error("Too many arguments (%d) passed to the function %s.\n", pContext->argNum, pFun->name);
    vm_currentContext->ip += a1;
  }
}