void __cdecl vm_queryMemberPtr(int flag)
{
  Vm_Value_T_2 *pPtrValue; // ebx
  Vm_Value_T_2 *v2; // edi
  void *info; // esi
  unsigned __int8 v4; // al
  char *v5; // eax
  Vm_Value_T_2 *v6; // eax
  unsigned __int8 type; // al
  int v8; // eax
  _DWORD *v9; // eax
  int offset; // [esp+4h] [ebp-14h]

  pPtrValue = &vm_stack[vm_sp];
  v2 = pPtrValue + 1;
  info = pPtrValue[1].v.info;
  if ( (pPtrValue[1].attrib & 1) != 0 )
    vm_error("Can't modify a const value.\n");
  v4 = *((_BYTE *)info + 4);
  if ( v4 == 5 )
    vm_error("Can't get pointer inside buffer, use efun put_sub_data to modify string.\n");
  if ( v4 <= 5u )
  {
    if ( v4 == 4 )
      vm_error("Can't get pointer inside string, use efun put_sub_data to modify string.\n");
LABEL_5:
    v5 = vm_valueType(v4);
    vm_error("Bad type(%s) to index.\n", v5);
  }
  if ( v4 == 6 )
  {
    type = pPtrValue->type;
    if ( type == 3 )
    {
      pPtrValue->v.number = vm_ftol(pPtrValue->v.real);
      pPtrValue->type = 2;
    }
    else if ( type != 2 )
    {
      vm_error("Index should be integer.\n");
    }
    if ( (flag & 1) != 0 )
    {
      offset = *(_DWORD *)(*((_DWORD *)info + 2) + 4) - pPtrValue->v.number;
    }
    else
    {
      pPtrValue = pPtrValue->v.pPtrValue;
      offset = (int)pPtrValue;
    }
    if ( offset < 0 || (v8 = *((_DWORD *)info + 2), *(_DWORD *)(v8 + 4) <= offset) )
      vm_error("Index of the array out of range.\n");
    v9 = *(_DWORD **)(*(_DWORD *)(v8 + 12) + 4 * offset);
    v2->v.number = (int)v9;
    ++*v9;
    vm_unrefValue((Vm_String_T *)info, (Vm_Buffer_T *)info);
    vm_popStack((Vm_Buffer_T *)pPtrValue, (Vm_String_T *)info);
  }
  else
  {
    if ( v4 != 7 )
      goto LABEL_5;
    v6 = vm_findForInsert(*((Vm_Mapping_T **)info + 2), &vm_stack[vm_sp], 0);
    pPtrValue[1].v.number = (int)v6;
    ++v6->ref;
    vm_unrefValue((Vm_String_T *)info, (Vm_Buffer_T *)info);
    vm_popStack((Vm_Buffer_T *)pPtrValue, (Vm_String_T *)info);
  }
}