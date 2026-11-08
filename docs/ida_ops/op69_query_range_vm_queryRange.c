void __cdecl vm_queryRange(int flag)
{
  Vm_Value_T_2 *v1; // ebx
  const char *v2; // eax
  int v3; // esi
  int v4; // ebx
  signed int number; // edi
  unsigned __int8 type; // al
  int v7; // esi
  size_t v8; // esi
  Vm_Buffer_T *Buffer; // ebx
  Vm_Buffer_T *v10; // edx
  int v11; // eax
  bool v12; // zf
  signed int v13; // esi
  Vm_Array_T *v14; // ebx
  int v15; // esi
  unsigned int v16; // ebx
  Vm_String_T *v17; // esi
  char *v18; // edi
  Vm_String_T *v19; // edx
  char *v20; // [esp-10h] [ebp-38h]
  int v21; // [esp-Ch] [ebp-34h]
  char *str; // [esp+Ch] [ebp-1Ch]
  Vm_Array_T *pNewArray; // [esp+10h] [ebp-18h]
  Vm_Value_T_2 *pValue; // [esp+14h] [ebp-14h]

  v1 = &vm_stack[vm_sp];
  pValue = v1 + 2;
  if ( v1[1].type == 3 )
  {
    v11 = vm_ftol(v1[1].v.real);
    v1[1].type = 2;
    v12 = v1->type == 3;
    v1[1].v.number = v11;
    if ( !v12 )
      goto LABEL_3;
LABEL_44:
    v1->v.number = vm_ftol(v1->v.real);
    v1->type = 2;
    goto LABEL_3;
  }
  if ( v1->type == 3 )
    goto LABEL_44;
LABEL_3:
  if ( v1[1].type != 2 || v1->type != 2 )
    vm_error("Index should be integer.\n");
  type = v1[2].type;
  number = v1[1].v.number;
  v4 = v1->v.number;
  if ( type == 5 )
  {
    v7 = *(_DWORD *)(pValue->v.number + 12);
    if ( (flag & 1) != 0 )
    {
      if ( v4 <= 0 )
        v4 = 1;
      v4 = v7 - v4;
    }
    if ( (flag & 2) != 0 )
    {
      if ( number <= 0 )
        number = 1;
      number = v7 - number;
    }
    if ( number < 0 )
      number = 0;
    if ( v7 <= v4 )
      v4 = v7 - 1;
    v8 = 0;
    if ( number <= v4 )
      v8 = v4 - number + 1;
    Buffer = vm_allocateBuffer(number);
    if ( v8 )
    {
      number += *(_DWORD *)(pValue->v.number + 16);
      memcpy(Buffer->ptr, (const void *)number, v8);
    }
    vm_popNStack((Vm_Value_T_2 *)number, 3);
    vm_pushBuffer(v10);
    return;
  }
  if ( type <= 5u )
  {
    if ( type != 4 )
      goto LABEL_8;
    v3 = *(_DWORD *)(pValue->v.number + 16);
    if ( (flag & 1) != 0 )
    {
      if ( v4 <= 0 )
        v4 = 1;
      v4 = v3 - v4;
    }
    if ( (flag & 2) != 0 )
    {
      if ( number <= 0 )
        number = 1;
      number = v3 - number;
    }
    if ( number < 0 )
      number = 0;
    if ( v3 <= v4 )
      v4 = v3 - 1;
    if ( number <= v4 )
    {
      v16 = v4 - number + 1;
      v17 = vm_newString(v16);
      str = v17->ptr;
      v18 = (char *)(*(_DWORD *)(pValue->v.number + 12) + number);
      strncpy(str, v18, v16);
      str[v16] = 0;
      v17->len = v16;
      vm_popNStack((Vm_Value_T_2 *)v18, 3);
      vm_pushAllocedString(v19);
    }
    else
    {
      vm_popNStack((Vm_Value_T_2 *)number, 3);
      vm_pushString("");
    }
  }
  else
  {
    if ( type != 6 )
    {
      if ( type == 7 )
        vm_error("Bad type(undefined) to index for range values.\n");
LABEL_8:
      v2 = vm_valueType(type);
      vm_error("Bad type(%s) to index for range values.\n", v2);
    }
    v13 = *(_DWORD *)(pValue->v.number + 4);
    if ( (flag & 1) != 0 )
    {
      if ( v4 <= 0 )
        v4 = 1;
      v4 = v13 - v4;
    }
    if ( (flag & 2) != 0 )
    {
      if ( number <= 0 )
        number = 1;
      number = v13 - number;
    }
    vm_popNStack((Vm_Value_T_2 *)number, 2);
    if ( number < 0 )
      number = 0;
    if ( v13 <= v4 )
      v4 = v13 - 1;
    if ( number > v4 )
      number = v4 + 1;
    v14 = (Vm_Array_T *)(v4 - number);
    pNewArray = vm_allocateEmptyArray(v13);
    v15 = 0;
    if ( (int)&v14->ref + 1 > 0 )
    {
      v14 = (Vm_Array_T *)((char *)v14 + 1);
      do
      {
        v21 = *(_DWORD *)(*(_DWORD *)(pValue->v.number + 12) + 4 * (number + v15));
        v20 = (char *)pNewArray->pValues[v15++];
        vm_copyValue(v20, v21);
      }
      while ( (int)v14 > v15 );
    }
    vm_unrefArray(v14);
    pValue->v.number = (int)pNewArray;
  }
}