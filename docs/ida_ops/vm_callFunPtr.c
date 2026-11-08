int __cdecl vm_callFunPtr(int mayNull, int argn)
{
  Vm_Value_T_2 *v2; // edx
  Vm_Value_T_2 *v3; // ebx
  Vm_Value_T_2 *v4; // ecx
  Vm_Fun_Ptr_T *info; // esi
  unsigned __int8 attrib; // dl
  signed int v7; // edx
  int result; // eax
  Vm_Object *v9; // eax
  int v10; // edi
  Vm_Fun_T *ProgramFunction; // edi
  Vm_Value_1 *pArgs; // edx
  Vm_Index_T v13; // ax
  Vm_Object *v14; // eax
  Vm_Ident_Hash_Elem_T *v15; // ebx
  Vm_Object *thisObject; // edx
  Vm_Program_T *v17; // ebx
  int v18; // eax
  int v19; // ebx
  int v20; // ebx
  bool v21; // cc
  Vm_Value_T_2 *v22; // edx
  int v23; // eax
  unsigned int token; // eax
  const char *v25; // eax
  int v26; // ebx
  Vm_Efun *pInfo; // [esp+8h] [ebp-90h]
  int type; // [esp+Ch] [ebp-8Ch]
  int isGlobal; // [esp+10h] [ebp-88h]
  int isGlobala; // [esp+10h] [ebp-88h]
  Vm_Object *pObject; // [esp+14h] [ebp-84h]
  void *osPtr; // [esp+18h] [ebp-80h]
  Vm_Index_T funOffset; // [esp+1Eh] [ebp-7Ah]
  Vm_Index_T funOffseta; // [esp+1Eh] [ebp-7Ah]
  _DWORD v35[16]; // [esp+20h] [ebp-78h] BYREF
  Vm_Value_T_3 value; // [esp+60h] [ebp-38h] BYREF
  int argna; // [esp+A4h] [ebp+Ch]

  if ( argn <= 0 )
    vm_error("Missed function to call.\n");
  vm_initValue(v2);
  v4 = vm_stack;
  info = (Vm_Fun_Ptr_T *)vm_stack[vm_sp].v.info;
  value.type = vm_stack[vm_sp].type;
  attrib = vm_stack[vm_sp].attrib;
  argna = argn - 1;
  vm_stack[vm_sp].type = 0;
  value.attrib = attrib;
  vm_stack[vm_sp].attrib = 0;
  v7 = vm_sp + 1;
  value.v.number = (int)info;
  ++vm_sp;
  if ( value.type != 4 )
  {
    if ( value.type != 9 )
    {
      vm_destructStaticValue(v3);
      vm_error("Bad type of the value to call, expected a function or string.\n");
    }
    if ( info->argn > v7 )
    {
      vm_destructStaticValue(v3);
      vm_error("Stack overflow when do function call.\n");
    }
    v9 = info->pObject;
    if ( v9 && (v9->flags & 0x8000) != 0 )
    {
      vm_destructStaticValue(v3);
      vm_error("Owner object is destructed, can't invoke function.\n");
    }
    isGlobala = info->isGlobal;
    type = info->type;
    funOffseta = info->funOffset;
    pInfo = info->f.pEfun;
    pObject = 0;
    if ( !info->isGlobal )
      pObject = info->pObject;
    v10 = info->argn;
    if ( info->ref == 1 )
    {
      ProgramFunction = (Vm_Fun_T *)(v10 - 1);
      if ( ProgramFunction != (Vm_Fun_T *)-1 )
      {
        v3 = (Vm_Value_T_2 *)(24 * (_DWORD)ProgramFunction);
        do
        {
          pArgs = info->pArgs;
          vm_stack[--vm_sp].type = *(&pArgs->type + (_DWORD)v3);
          vm_stack[vm_sp].attrib = *(&v3->attrib + (unsigned int)info->pArgs);
          vm_stack[vm_sp].v.number = *(int *)((char *)&v3->v.number + (unsigned int)info->pArgs);
          *(&v3->type + (unsigned int)info->pArgs) = 0;
          ProgramFunction = (Vm_Fun_T *)((char *)ProgramFunction - 1);
          *(&v3->attrib + (unsigned int)info->pArgs) = 0;
          --v3;
        }
        while ( ProgramFunction != (Vm_Fun_T *)-1 );
      }
      vm_freeFunctionValue(info);
      value.type = 0;
      value.attrib = 0;
      vm_destructStaticValue(v3);
    }
    else
    {
      ProgramFunction = (Vm_Fun_T *)(v10 - 1);
      vm_destructStaticValue(v3);
      if ( ProgramFunction != (Vm_Fun_T *)-1 )
      {
        v19 = (int)ProgramFunction;
        do
        {
          ProgramFunction = (Vm_Fun_T *)((char *)ProgramFunction - 1);
          vm_pushValue((Vm_Value_T_2 *)&info->pArgs[v19--]);
        }
        while ( ProgramFunction != (Vm_Fun_T *)-1 );
      }
    }
    if ( type != 75 )
    {
      v13 = 0;
      if ( vm_currentContext->thisObject != pObject )
        v13 = (__int16)pObject;
      vm_callInUserFunBody(isGlobala, (Vm_Object_T *)info, v13, ProgramFunction, funOffseta);
      return 1;
    }
    v23 = pInfo->funOffset;
    goto LABEL_50;
  }
  v14 = info->pObject;
  v15 = 0;
  ProgramFunction = 0;
  if ( LOBYTE(v14->ref) == 36 )
  {
    osPtr = vm_lookupOSFun((char *)&v14->ref + 1);
    if ( !osPtr )
    {
      vm_destructStaticValue(0);
      vm_error("No such OS function.\n");
    }
    ProgramFunction = 0;
  }
  else
  {
    thisObject = vm_currentContext->thisObject;
    osPtr = 0;
    if ( !thisObject
      || (ProgramFunction = vm_findProgramFunction(thisObject->pProgram, 0, (int)info, (Vm_Index_T *)1),
          isGlobal = 0,
          !ProgramFunction) )
    {
      v17 = (Vm_Program_T *)value.v.info;
      if ( !*(_WORD *)(value.v.number + 8) )
      {
        *(_WORD *)(value.v.number + 8) = vm_whashstr((const char *)thisObject, (int)v4);
        v17 = (Vm_Program_T *)value.v.info;
      }
      v15 = vm_lookupIdentInProgram(v17, (const char *)vm_pGlobalProgram, *(unsigned __int16 *)&v17->attrib);
      isGlobal = 1;
    }
  }
  vm_destructStaticValue((Vm_Value_T_2 *)v15);
  if ( v15 )
  {
    if ( !ProgramFunction )
    {
      token = v15->token;
      if ( (token & 0x40000000) != 0 )
      {
        v23 = *((__int16 *)&v15->ii.v.stackOffset + 3);
LABEL_50:
        vm_callInEfun((Vm_Index_T)ProgramFunction, v23);
        return 1;
      }
      if ( (token & 0x8000000) == 0 )
        vm_error("The \"%s\" isn't a function.\n", v15->name);
      v18 = v15->ii.v.uf.funOffset;
LABEL_27:
      vm_callInUserFunBody(isGlobal, (Vm_Object_T *)info, 0, ProgramFunction, v18);
      return 1;
    }
LABEL_26:
    v18 = funOffset;
    goto LABEL_27;
  }
  if ( ProgramFunction )
    goto LABEL_26;
  if ( osPtr )
  {
    memset(v35, 0, sizeof(v35));
    v20 = 0;
    v21 = argna > 0;
    while ( v21 )
    {
      v22 = &vm_stack[v20 + vm_sp];
      switch ( v22->type )
      {
        case 0u:
        case 1u:
          v35[v20++] = 0;
          v21 = v20 < argna;
          continue;
        case 2u:
          v35[v20] = v22->v.info;
          goto LABEL_44;
        case 3u:
          v35[v20++] = v22->v.info;
          v21 = v20 < argna;
          continue;
        case 4u:
          v35[v20] = *(_DWORD *)(v22->v.number + 12);
          goto LABEL_44;
        case 5u:
          v35[v20] = *(_DWORD *)(v22->v.number + 16);
LABEL_44:
          v21 = ++v20 < argna;
          break;
        default:
          v25 = vm_valueType((int)v22);
          vm_error("Bad type (%s) of the argument for os function.\n", v25);
          return result;
      }
    }
    v26 = ((int (__cdecl *)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))osPtr)(
            v35[0],
            v35[1],
            v35[2],
            v35[3],
            v35[4],
            v35[5],
            v35[6],
            v35[7],
            v35[8],
            v35[9],
            v35[10],
            v35[11],
            v35[12]);
    vm_popNStack(&value, argna);
    vm_pushNumber(v26);
    return 1;
  }
  if ( !mayNull )
    vm_error("No such function.\n");
  vm_popNStack(0, argna);
  vm_pushVoid();
  return 0;
}