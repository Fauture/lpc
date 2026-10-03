void __usercall vm_f_destruct_object(Vm_Value_T_2 *a1@<edi>)
{
  Vm_Fun_Ptr::Vm_Value_0 *v1; // esi
  int type; // eax
  int number; // ebx
  int v4; // eax
  Vm_PCB_T *CurrentPCB; // eax
  Vm_Fun_Ptr::Vm_Value_0 *v6; // ebx
  unsigned __int8 v7; // al
  char *v8; // eax
  Vm_Buffer_T *v9; // ebx
  Vm_Fun_Ptr::Vm_Value_0 *v10; // eax

  if ( vm_currentContext->argNum > 1 )
    vm_error("Too many arguments to destruct_object.\n");
  if ( vm_currentContext->argNum <= 0 )
    vm_error("Miss argument 1, expected object | string.\n");
  if ( (int)vm_sp >= vm_stackSize )
    vm_error("Missed argument in stack.\n");
  v1 = vm_stackTopValue();
  type = v1->type;
  if ( type == 4 )
  {
    v6 = vm_stackTopValue();
    v7 = v6->type;
    if ( v7 != 4 )
    {
      v8 = vm_valueType(v7);
      vm_error("Bad type of value, expected string, got %s\n", v8);
    }
    v9 = (Vm_Buffer_T *)vm_lookupObjectByName((unsigned int)v6->v.info);
    vm_popStack(v9, (Vm_String_T *)v1);
    if ( !v9 || v9->realSize == 2048 )
      vm_pushNumber(0);
    else
      vm_pushObject(v9);
    v10 = vm_stackTopValue();
    if ( v10->type != 10 )
LABEL_6:
      vm_error("Not found the object for destruct_object.\n");
    number = v10->v.number;
  }
  else
  {
    if ( type != 10 )
      goto LABEL_6;
    number = v1->v.number;
    if ( *(char *)(number + 13) < 0 )
      goto LABEL_6;
  }
  v4 = *(_DWORD *)(number + 12);
  if ( (v4 & 0xE000) != 0 )
  {
    vm_popNStack(a1, vm_currentContext->argNum);
  }
  else
  {
    BYTE1(v4) |= 0x20u;
    *(_DWORD *)(number + 12) = v4;
    CurrentPCB = vm_getCurrentPCB();
    vm_setCrashHook((int)CurrentPCB, (void (__cdecl *)(int, int))vm_f_destruct_object_crash, number);
    vm_f_destruct_object2((void *)number);
    vm_ctrlFlags |= 0x10u;
  }
}