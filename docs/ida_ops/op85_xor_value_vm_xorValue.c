void vm_xorValue()
{
  Vm_Value_T_2 *v0; // ebx
  char *v1; // eax

  v0 = &vm_stack[vm_sp];
  if ( v0[1].type != 2 )
  {
    v1 = vm_valueType(v0[1].type);
    vm_error("Bad ^, type invalid (got %s) of left value.\n", v1);
  }
  if ( v0->type != 2 )
    vm_error("Bad ^, right isn't a number when left is.\n");
  v0[1].v.number ^= (int)v0->v.info;
  vm_popStack((Vm_Buffer_T *)v0, (Vm_String_T *)&v0[1]);
}