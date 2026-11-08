void vm_negValue()
{
  Vm_Value_T_2 *v0; // ecx
  unsigned __int8 type; // al
  char *v2; // eax

  v0 = &vm_stack[vm_sp];
  type = v0->type;
  if ( type == 2 )
  {
    v0->v.number = -v0->v.number;
  }
  else
  {
    if ( type != 3 )
    {
      v2 = vm_valueType(type);
      vm_error("Bad -, type invalid of right value.\n", v2);
    }
    HIBYTE(v0->v.real) ^= 0x80u;
  }
}