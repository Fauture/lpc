void __usercall vm_mulValue(Vm_String_T *a1@<esi>)
{
  Vm_Value_T_2 *v1; // ecx
  Vm_Buffer_T *v2; // ebx
  unsigned __int8 type; // al
  char *v4; // eax
  int v5; // eax
  int v6; // eax
  float v7; // [esp-14h] [ebp-18h]

  v1 = &vm_stack[vm_sp];
  v2 = (Vm_Buffer_T *)&v1[1];
  type = v1[1].type;
  if ( type == 2 )
  {
    v5 = v1->type;
    if ( v5 == 2 )
    {
      v1[1].v.number *= v1->v.number;
      goto LABEL_4;
    }
    if ( v5 == 3 )
    {
      v7 = v1->v.real * (long double)v1[1].v.number;
      v1[1].v.number = vm_ftol(v7);
      goto LABEL_4;
    }
    goto LABEL_7;
  }
  if ( type != 3 )
  {
    v4 = vm_valueType(type);
    vm_error("Bad *, type invalid (got %s) of left value.\n", v4);
  }
  v6 = v1->type;
  if ( v6 == 2 )
  {
    v1[1].v.real = v1[1].v.real * (long double)v1->v.number;
    goto LABEL_4;
  }
  if ( v6 != 3 )
LABEL_7:
    vm_error("Bad *, right isn't a number when left is.\n");
  v1[1].v.real = v1->v.real * v1[1].v.real;
LABEL_4:
  vm_popStack(v2, a1);
}