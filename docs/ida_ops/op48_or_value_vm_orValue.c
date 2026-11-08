void vm_orValue()
{
  Vm_Value_T_2 *v0; // ebx
  Vm_Value_T_2 *v1; // esi
  unsigned __int8 type; // al
  char *v3; // eax
  Vm_Array_T *v4; // ebx

  v0 = &vm_stack[vm_sp];
  v1 = v0 + 1;
  type = v0[1].type;
  if ( type == 2 )
  {
    if ( v0->type != 2 )
      vm_error("Bad |, right isn't a number when left is.\n");
    v0[1].v.number |= (int)v0->v.info;
    vm_popStack((Vm_Buffer_T *)v0, (Vm_String_T *)v1);
  }
  else
  {
    if ( type != 6 )
    {
      v3 = vm_valueType(type);
      vm_error("Bad |, type invalid (got %s) of left value.\n", v3);
    }
    if ( v0->type != 6 )
      vm_error("Bad |, right is't an array when left is.\n");
    v4 = vm_orArray(v0[1].v.array, v0->v.array);
    vm_unrefArray(v4);
    v1->v.number = (int)v4;
    vm_popStack((Vm_Buffer_T *)v4, (Vm_String_T *)v1);
  }
}