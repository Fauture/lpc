void __usercall vm_f_move_object(Vm_String_T *a1@<esi>)
{
  Vm_Fun_Ptr::Vm_Value_0 *v1; // ebx
  int type; // eax
  Vm_Value_T_2 *pPtrValue; // edi
  _DWORD *info; // esi
  _DWORD *i; // ebx
  int v6; // eax
  _DWORD *v7; // edx
  bool j; // zf
  int *p_v; // edx
  bool k; // zf
  Vm_FunCall_Context_T *v11; // eax
  Vm_Value_T_2 *Argument; // ebx
  int v13; // eax
  Vm_Fun_Ptr::Vm_Value_0 *v14; // ebx
  unsigned __int8 v15; // al
  char *v16; // eax
  Vm_Buffer_T *v17; // ebx
  Vm_Fun_Ptr::Vm_Value_0 *v18; // eax
  Vm_Fun_Ptr::Vm_Value_0 *v19; // esi
  unsigned __int8 v20; // al
  char *v21; // eax
  Vm_String_T *v22; // esi
  Vm_Fun_Ptr::Vm_Value_0 *v23; // esi

  if ( vm_currentContext->argNum > 2 )
    vm_error("Too many arguments to move_object\n");
  if ( vm_currentContext->argNum <= 0 )
    vm_error("Miss argument 1, expected object | string.\n");
  if ( (int)vm_sp >= vm_stackSize )
    vm_error("Missed argument in stack.\n");
  v1 = vm_stackTopValue();
  type = v1->type;
  if ( type == 4 )
  {
    v14 = vm_stackTopValue();
    v15 = v14->type;
    if ( v15 != 4 )
    {
      v16 = vm_valueType(v15);
      vm_error("Bad type of value, expected string, got %s\n", v16);
    }
    v17 = (Vm_Buffer_T *)vm_lookupObjectByName((unsigned int)v14->v.info);
    vm_popStack(v17, a1);
    if ( !v17 || v17->realSize == 2048 )
      vm_pushNumber(0);
    else
      vm_pushObject(v17);
    v18 = vm_stackTopValue();
    if ( v18->type != 10 )
LABEL_6:
      vm_error("Not found destination for move_object.\n");
    pPtrValue = v18->v.pPtrValue;
  }
  else
  {
    if ( type != 10 )
      goto LABEL_6;
    pPtrValue = v1->v.pPtrValue;
    if ( SBYTE1(pPtrValue->pNext) < 0 )
      goto LABEL_6;
  }
  if ( vm_currentContext->argNum > 1 )
  {
    Argument = vm_getArgument(1, (char *)0xFF);
    v13 = Argument->type;
    if ( v13 == 4 )
    {
      vm_pushValue(Argument);
      v19 = vm_stackTopValue();
      v20 = v19->type;
      if ( v20 != 4 )
      {
        v21 = vm_valueType(v20);
        vm_error("Bad type of value, expected string, got %s\n", v21);
      }
      v22 = (Vm_String_T *)vm_lookupObjectByName((unsigned int)v19->v.info);
      vm_popStack((Vm_Buffer_T *)Argument, v22);
      if ( !v22 || v22->ptr == (char *)2048 )
        vm_pushNumber(0);
      else
        vm_pushObject(v22);
      v23 = vm_stackTopValue();
      if ( v23->type != 10 )
        vm_error("Not found the object for move_object.\n");
      vm_destructValue((Vm_String_T *)v23, (Vm_Buffer_T *)Argument);
      Argument->type = v23->type;
      Argument->attrib = v23->attrib;
      Argument->v.number = v23->v.number;
      vm_stack[vm_sp].type = 0;
      vm_stack[vm_sp++].attrib = 0;
    }
    else if ( v13 != 10 )
    {
      vm_error("Not found the object for move_object.\n");
    }
    info = vm_getArgument(1, (char *)0xFF)->v.info;
  }
  else
  {
    info = &vm_currentContext->thisObject->ref;
  }
  if ( !info )
    vm_error("Not found the object for move_object.\n");
  for ( i = (_DWORD *)pPtrValue[2].ref; i; i = (_DWORD *)i[12] )
  {
    if ( i == info )
      vm_error("You can not move object to its lodger.\n");
  }
  v6 = info[12];
  if ( v6 )
  {
    v7 = (_DWORD *)(v6 + 56);
    for ( j = *(_DWORD *)(v6 + 56) == (_DWORD)info; !j; j = *v7 == (_DWORD)info )
      v7 = (_DWORD *)(*v7 + 52);
    *v7 = info[13];
    info[13] = 0;
  }
  p_v = (int *)&pPtrValue[2].v;
  for ( k = pPtrValue[2].v.number == 0; !k; k = *p_v == 0 )
    p_v = (int *)(*p_v + 52);
  v11 = vm_currentContext;
  *p_v = (int)info;
  info[12] = pPtrValue;
  vm_popNStack(pPtrValue, v11->argNum);
}