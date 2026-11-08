void __cdecl vm_createMapping(unsigned int pValue)
{
  unsigned int i; // esi
  unsigned int v3; // eax
  unsigned int v4; // ebx
  Vm_String_T *v5; // esi
  unsigned int v6; // ebx
  Vm_Value_T_2 *v7; // eax
  Vm_Value_T_2 *v8; // edx
  Vm_Value_T_2 *v9; // eax
  unsigned int v10; // eax
  unsigned int v11; // edx
  Vm_Buffer_T *v12; // ebx
  Vm_Mapping_T *m; // [esp+8h] [ebp-10h]

  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  for ( i = 0; i < pValue; ++i )
  {
    if ( vm_stack[i + vm_sp].type == 8 )
      vm_error("Bad value (got pointer) of pair %d to create mapping.\n", (pValue >> 1) - (i >> 1));
  }
  m = vm_allocateMapping(pValue >> 1);
  v3 = vm_sp;
  v4 = vm_sp;
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  --vm_sp;
  vm_stack[v3 - 1].type = 7;
  v5 = 0;
  vm_stack[vm_sp].v.number = (int)m;
  if ( pValue )
  {
    v6 = v4;
    do
    {
      v7 = vm_findForInsert(m, &vm_stack[v6], 1);
      v7->type = vm_stack[v6 + 1].type;
      v7->attrib = vm_stack[v6 + 1].attrib;
      v8 = &vm_stack[v6 + 1];
      v7->v.number = vm_stack[v6 + 1].v.number;
      v8->type = 0;
      v5 = (Vm_String_T *)((char *)v5 + 2);
      v9 = &vm_stack[v6];
      v6 += 2;
      v9[1].attrib = 0;
    }
    while ( (unsigned int)v5 < pValue );
  }
  vm_stack[vm_sp].type = 0;
  vm_stack[vm_sp].attrib = 0;
  v10 = vm_sp + 1;
  v11 = vm_sp + 1;
  if ( (int)pValue > 0 )
  {
    v12 = (Vm_Buffer_T *)pValue;
    do
    {
      vm_sp = v11;
      vm_cleanupValue(v12, v5, &vm_stack[v10]);
      v10 = vm_sp + 1;
      v12 = (Vm_Buffer_T *)((char *)v12 - 1);
      v11 = vm_sp + 1;
    }
    while ( v12 );
  }
  vm_sp = v10 - 1;
  vm_stack[vm_sp].type = 7;
  vm_stack[vm_sp].v.number = (int)m;
}