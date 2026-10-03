void __usercall vm_f_lodger_object(Vm_Value_T_2 *a1@<edi>, Vm_String_T *a2@<esi>)
{
  Vm_FunCall_Context_T *info; // eax
  Vm_Fun_Ptr::Vm_Value_0 *v3; // ebx
  int type; // eax
  Vm_Object *thisObject; // esi
  Vm_String_T *v6; // ecx
  Vm_Object *pLogderObject; // edx
  Vm_Array_T *EmptyArray; // ebx
  int v9; // ecx
  Vm_Object *i; // edx
  Vm_Value_T *v11; // eax
  Vm_Fun_Ptr::Vm_Value_0 *v12; // ebx
  unsigned __int8 v13; // al
  char *v14; // eax
  Vm_Buffer_T *v15; // ebx
  Vm_Fun_Ptr::Vm_Value_0 *v16; // eax

  info = vm_currentContext;
  if ( vm_currentContext->argNum > 1 )
    vm_error("Too many arguments to lodger_object.\n");
  if ( vm_currentContext->argNum <= 0 )
  {
    thisObject = vm_currentContext->thisObject;
  }
  else
  {
    if ( (int)vm_sp >= vm_stackSize )
      vm_error("Missed argument in stack.\n");
    v3 = vm_stackTopValue();
    type = v3->type;
    if ( type == 4 )
    {
      v12 = vm_stackTopValue();
      v13 = v12->type;
      if ( v13 != 4 )
      {
        v14 = vm_valueType(v13);
        vm_error("Bad type of value, expected string, got %s\n", v14);
      }
      v15 = (Vm_Buffer_T *)vm_lookupObjectByName((unsigned int)v12->v.info);
      vm_popStack(v15, a2);
      if ( !v15 || v15->realSize == 2048 )
        vm_pushNumber(0);
      else
        vm_pushObject(v15);
      v16 = vm_stackTopValue();
      if ( v16->type != 10 )
LABEL_6:
        vm_error("Not found the object for lodger_object.\n");
      info = (Vm_FunCall_Context_T *)v16->v.info;
    }
    else
    {
      if ( type != 10 )
        goto LABEL_6;
      info = (Vm_FunCall_Context_T *)v3->v.info;
      if ( (info->ip & 0x8000) != 0 )
        goto LABEL_6;
    }
    thisObject = (Vm_Object *)info;
  }
  if ( !thisObject )
    vm_error("Not found the object for lodger_object.\n");
  v6 = 0;
  pLogderObject = thisObject->pLogderObject;
  while ( pLogderObject )
  {
    pLogderObject = pLogderObject->pNeighbourObject;
    v6 = (Vm_String_T *)((char *)v6 + 1);
  }
  EmptyArray = vm_allocateEmptyArray((Vm_Value_T_2 *)info, v6);
  v9 = 0;
  for ( i = thisObject->pLogderObject; i; i = i->pNeighbourObject )
  {
    v11 = EmptyArray->pValues[v9];
    ++i->ref;
    v11->v.number = (int)i;
    EmptyArray->pValues[v9++]->type = 10;
  }
  vm_popNStack(a1, vm_currentContext->argNum);
  vm_pushArray((int)EmptyArray);
}