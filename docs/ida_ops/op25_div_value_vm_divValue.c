void vm_divValue()
{
  Vm_Value_T_2 *v0; // ebx
  unsigned __int8 type; // al
  char *v2; // eax
  int v3; // eax
  int v4; // eax
  int v5; // edx
  int number; // edx
  float v7; // [esp-10h] [ebp-18h]

  v0 = &vm_stack[vm_sp];
  type = v0[1].type;
  if ( type == 2 )
  {
    v3 = v0->type;
    if ( v3 == 2 )
    {
      number = v0->v.number;
      if ( !number )
        vm_error("divide by zero.\n");
      v0[1].v.number /= number;
    }
    else
    {
      if ( v3 != 3 )
        goto LABEL_7;
      if ( v0->v.real == 0.0 )
        vm_error("divide by zero.\n");
      v7 = (long double)v0[1].v.number / v0->v.real;
      v0[1].v.number = vm_ftol(v7);
    }
  }
  else
  {
    if ( type != 3 )
    {
      v2 = vm_valueType(type);
      vm_error("Bad /, type invalid (got %s) of left value.\n", v2);
    }
    v4 = v0->type;
    if ( v4 != 2 )
    {
      if ( v4 == 3 )
      {
        if ( v0->v.real == 0.0 )
          vm_error("divide by zero.\n");
        v0[1].v.real = v0[1].v.real / v0->v.real;
        goto LABEL_4;
      }
LABEL_7:
      vm_error("Bad /, right isn't a number when left is.\n");
    }
    v5 = v0->v.number;
    if ( !v5 )
      vm_error("divide by zero.\n");
    v0[1].v.real = v0[1].v.real / (long double)v5;
  }
LABEL_4:
  vm_popStack((Vm_Buffer_T *)v0, (Vm_String_T *)&v0[1]);
}