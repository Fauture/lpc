void __cdecl vm_createArray(unsigned int a1)
{
  int v2; // ebx
  unsigned int v3; // esi
  unsigned int v4; // ebx
  Vm_Array_T *EmptyArray; // eax
  Vm_Value_T **pValues; // esi
  Vm_Value_T *v7; // ecx
  unsigned int v8; // eax
  unsigned int v9; // edi
  unsigned int v10; // eax
  Vm_Array_T *v11; // ebx
  Vm_Array_T *pArray; // [esp+8h] [ebp-10h]

  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  if ( a1 )
  {
    v2 = 0;
    do
    {
      v3 = v2 + 1;
      if ( vm_stack[v2 + vm_sp].type == 8 )
        vm_error("Bad value (got pointer) of element %d to create array.\n", v2 + 1);
      ++v2;
    }
    while ( v3 < a1 );
    v4 = 0;
    EmptyArray = vm_allocateEmptyArray(v3);
    pArray = EmptyArray;
    pValues = EmptyArray->pValues;
    do
    {
      pValues[v4]->type = vm_stack[v4 + vm_sp].type;
      pArray->pValues[v4]->attrib = vm_stack[v4 + vm_sp].attrib;
      pValues = pArray->pValues;
      v7 = pValues[v4];
      v8 = v4 + vm_sp;
      ++v4;
      v7->v.number = vm_stack[v8].v.number;
    }
    while ( v4 < a1 );
    v9 = a1 - 2;
    if ( a1 == 1 )
    {
      vm_stack[vm_sp].type = 6;
      vm_stack[vm_sp].v.number = (int)pArray;
    }
    else
    {
      v10 = vm_sp;
      do
      {
        vm_stack[v10].type = 0;
        vm_stack[vm_sp].attrib = 0;
        --v9;
        v10 = ++vm_sp;
      }
      while ( v9 != -1 );
      vm_stack[v10].type = 6;
      vm_stack[vm_sp].v.number = (int)pArray;
    }
  }
  else
  {
    v11 = vm_allocateEmptyArray(vm_sp);
    if ( !vm_sp )
      vm_error("Stack overflow when calculating.\n");
    vm_stack[--vm_sp].type = 6;
    vm_stack[vm_sp].v.number = (int)v11;
  }
}