void __cdecl vm_incValue(int a1)
{
  Vm_Value_T_2 *pPtrValue; // ebx
  int type; // eax
  const char *v4; // eax
  const char *v5; // eax
  const char *v6; // [esp-8h] [ebp-10h]

  pPtrValue = vm_stack[vm_sp].v.pPtrValue;
  type = pPtrValue->type;
  if ( type == 2 )
  {
    pPtrValue->v.number += a1;
  }
  else
  {
    if ( type != 3 )
    {
      v4 = (const char *)&unk_82ACF70;
      if ( a1 <= 0 )
        v4 = "--";
      v6 = v4;
      v5 = vm_valueType((int)"--");
      vm_error("Bad type(%s) to %s.\n", v5, v6);
    }
    pPtrValue->v.real = pPtrValue->v.real + (long double)a1;
  }
  vm_unrefValue(pPtrValue);
  vm_stack[vm_sp].type = 0;
  vm_stack[vm_sp++].attrib = 0;
}