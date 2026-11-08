void __cdecl vm_queryMember(char a1)
{
  Vm_Value_T_2 *v2; // ebx
  Vm_Value_T_2 *v3; // edi
  unsigned __int8 type; // al
  const char *v5; // eax
  unsigned __int8 v6; // al
  int v7; // esi
  unsigned __int8 v8; // al
  int number; // esi
  unsigned __int8 v10; // al
  Vm_Value_T_2 *pPtrValue; // esi
  Vm_Value_T_2 *v12; // ebx
  Vm_Value_T_2 *v13; // esi
  unsigned __int8 inheritAttrib; // [esp+Fh] [ebp-19h]
  int len; // [esp+10h] [ebp-18h]
  int lena; // [esp+10h] [ebp-18h]
  Vm_Buffer_T *pBuffer; // [esp+14h] [ebp-14h]
  Vm_String_T *pString; // [esp+18h] [ebp-10h]

  v2 = &vm_stack[vm_sp];
  v3 = v2 + 1;
  inheritAttrib = v2[1].attrib;
  type = v2[1].type;
  if ( type == 5 )
  {
    v8 = v2->type;
    if ( v8 == 3 )
    {
      v2->v.number = vm_ftol(v2->v.real);
      v2->type = 2;
    }
    else if ( v8 != 2 )
    {
      vm_error("Index should be integer.\n");
    }
    pBuffer = v2[1].v.buffer;
    lena = pBuffer->realSize;
    if ( (a1 & 1) != 0 )
      number = *(_DWORD *)(v2[1].v.number + 12) - v2->v.number;
    else
      number = v2->v.number;
    vm_popStack();
    if ( number < 0 || number >= lena )
      vm_error("Index out of range.\n");
    v2[1].v.number = pBuffer->ptr[number];
    v2[1].type = 2;
    vm_freeBuffer((Vm_Buffer_T *)v2);
    v2[1].attrib = inheritAttrib;
  }
  else if ( type <= 5u )
  {
    if ( type != 4 )
    {
LABEL_5:
      v5 = vm_valueType(type);
      vm_error("Bad type(%s) to index.\n", v5);
    }
    v6 = v2->type;
    if ( v6 == 3 )
    {
      v2->v.number = vm_ftol(v2->v.real);
      v2->type = 2;
    }
    else if ( v6 != 2 )
    {
      vm_error("Index should be integer.\n");
    }
    pString = v2[1].v.string;
    len = pString->len;
    if ( (a1 & 1) != 0 )
      v7 = *(_DWORD *)(v2[1].v.number + 16) - v2->v.number;
    else
      v7 = v2->v.number;
    vm_popStack();
    if ( v7 < 0 || v7 >= len )
      vm_error("Index out of range.\n");
    v2[1].v.number = (unsigned __int8)pString->ptr[v7];
    v2[1].type = 2;
    vm_unrefString(pString);
    v2[1].attrib = inheritAttrib;
  }
  else if ( type == 6 )
  {
    v10 = v2->type;
    if ( v10 == 3 )
    {
      v2->v.number = vm_ftol(v2->v.real);
      v2->type = 2;
    }
    else if ( v10 != 2 )
    {
      vm_error("Index should be integer.\n");
    }
    if ( (a1 & 1) != 0 )
      pPtrValue = (Vm_Value_T_2 *)(*(_DWORD *)(v2[1].v.number + 4) - v2->v.number);
    else
      pPtrValue = v2->v.pPtrValue;
    if ( (int)pPtrValue < 0 || (unsigned int)pPtrValue >= *(_DWORD *)(v2[1].v.number + 4) )
      vm_error("Index of the array out of range.\n");
    vm_popStack();
    v12 = v2[1].v.pPtrValue;
    vm_copyValue(pPtrValue, v12);
    vm_unrefArray((Vm_Array_T *)v12);
    v3->attrib = inheritAttrib;
  }
  else
  {
    if ( type != 7 )
      goto LABEL_5;
    v13 = vm_findInMapping(v2[1].v.mapping, &vm_stack[vm_sp]);
    vm_popStack();
    if ( v13 )
    {
      vm_copyValue(v13, v2[1].v.pPtrValue);
      vm_unrefMapping((Vm_Mapping_T *)v3);
    }
    else
    {
      vm_unrefMapping((Vm_Mapping_T *)v3);
      v2[1].type = 0;
      v2[1].v.number = 0;
    }
    v2[1].attrib = inheritAttrib;
  }
}