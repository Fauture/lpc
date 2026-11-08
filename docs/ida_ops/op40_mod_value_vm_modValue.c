void vm_modValue()
{
  Vm_Value_T_2 *v0; // ebx
  unsigned __int8 type; // al
  char *v2; // eax
  int v3; // eax
  int v4; // eax
  long double number; // fst7
  int v6; // edx

  v0 = &vm_stack[vm_sp];
  type = v0[1].type;
  if ( type != 2 )
  {
    if ( type != 3 )
    {
      v2 = vm_valueType(type);
      vm_error("Bad %, type invalid (got %s) of left value.\n", v2);
    }
    v4 = v0->type;
    if ( v4 == 2 )
    {
      number = (float)v0->v.number;
      goto LABEL_15;
    }
    if ( v4 == 3 )
    {
      number = v0->v.real;
LABEL_15:
      v0[1].v.real = fmod(v0[1].v.real, number);
      goto LABEL_4;
    }
LABEL_7:
    vm_error("Bad *, right isn't a real when left is.\n");
  }
  v3 = v0->type;
  if ( v3 == 2 )
  {
    v6 = v0->v.number;
    if ( !v6 )
      vm_error("Module by zero.\n");
    v0[1].v.number %= v6;
  }
  else
  {
    if ( v3 != 3 )
      goto LABEL_7;
    v0[1].v.real = fmod((float)v0[1].v.number, v0->v.real);
    v0[1].type = 3;
  }
LABEL_4:
  vm_popStack((Vm_Buffer_T *)v0, (Vm_String_T *)&v0[1]);
}