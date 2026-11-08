void vm_lshValue()
{
  Vm_Value_T_2 *v0; // ebx
  Vm_String_T *v1; // esi
  Vm_Mapping_T *v2; // eax
  char *v3; // eax
  unsigned int info; // ecx

  v2 = (Vm_Mapping_T *)(3 * vm_sp);
  v0 = &vm_stack[vm_sp];
  v1 = (Vm_String_T *)&v0[1];
  LOBYTE(v2) = v0[1].type;
  if ( (unsigned __int8)v2 == 2 )
  {
    if ( v0->type != 2 )
      vm_error("Bad <<, right isn't a number when left is.\n");
    info = (unsigned int)v0->v.info;
    if ( info <= 0x1F )
      v0[1].v.number <<= info;
    else
      v0[1].v.number = 0;
    vm_popStack((Vm_Buffer_T *)v0, v1);
  }
  else
  {
    if ( (unsigned __int8)v2 != 7 )
    {
      v3 = vm_valueType((unsigned __int8)v2);
      vm_error("Bad <<, type invalid (got %s) of left value.\n", v3);
    }
    if ( v0->type != 7 )
      vm_error("Bad <<, right isn't a mapping when left is.\n");
    if ( (v0[1].attrib & 1) != 0 )
      vm_error("Const mapping can't absorb other mapping.\n");
    vm_concatMapping(v0[1].v.mapping, v2);
    vm_popStack((Vm_Buffer_T *)v0, v1);
  }
}