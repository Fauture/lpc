void __usercall vm_rshValue(Vm_Mapping_T *a1@<edi>)
{
  Vm_Value_T_2 *v1; // ebx
  Vm_Mapping_T *v2; // eax
  char *v3; // eax
  unsigned int info; // ecx

  v2 = (Vm_Mapping_T *)(3 * vm_sp);
  v1 = &vm_stack[vm_sp];
  LOBYTE(v2) = v1[1].type;
  if ( (unsigned __int8)v2 == 2 )
  {
    if ( v1->type != 2 )
      vm_error("Bad >>, right isn't a number when left is.\n");
    info = (unsigned int)v1->v.info;
    if ( info <= 0x1F )
      v1[1].v.number = (unsigned int)v1[1].v.number >> info;
    else
      v1[1].v.number = 0;
    vm_popStack((Vm_Buffer_T *)v1, (Vm_String_T *)&v1[1]);
  }
  else
  {
    if ( (unsigned __int8)v2 != 7 )
    {
      v3 = vm_valueType((unsigned __int8)v2);
      vm_error("Bad >>, type invalid (got %s) of left value.\n", v3);
    }
    if ( v1->type != 7 )
      vm_error("Bad >>, right isn't a mapping when left is.\n");
    if ( (v1->attrib & 1) != 0 )
      vm_error("Const mapping can concat other mapping.\n");
    vm_concatMapping(v1->v.mapping, v2);
    vm_unrefMapping(a1);
    v1[1].v.number = v1->v.number;
    vm_stack[vm_sp].type = 0;
    vm_stack[vm_sp++].attrib = 0;
  }
}