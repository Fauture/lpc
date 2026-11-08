void vm_revValue()
{
  Vm_Value_T_2 *v0; // eax
  char *v1; // eax

  v0 = &vm_stack[vm_sp];
  if ( v0->type != 2 )
  {
    v1 = vm_valueType(v0->type);
    vm_error("Bad ~, type invalid of right value.\n", v1);
  }
  v0->v.number = ~v0->v.number;
}