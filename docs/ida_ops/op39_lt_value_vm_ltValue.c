void vm_ltValue()
{
  Vm_Value_T_2 *v0; // ebx
  Vm_Value_T_2 *v1; // esi
  unsigned __int8 type; // al
  unsigned __int8 v3; // cl
  char *v4; // eax
  Vm_Buffer_T *v5; // eax
  unsigned int v6; // ebx
  Vm_Real_T v7; // xmm0_4
  bool v8; // cf
  bool v9; // zf
  bool v10; // al
  long double real; // fst6
  long double number; // fst7
  long double v13; // fst6
  long double v14; // fst7

  v0 = &vm_stack[vm_sp];
  v1 = v0 + 1;
  type = v0[1].type;
  v3 = v0->type;
  if ( type == v3 )
  {
    if ( type == 2 )
      goto LABEL_19;
    if ( type == 4 )
      goto LABEL_11;
  }
  if ( type != 3 )
  {
    if ( type <= 3u )
    {
      if ( type != 2 )
        goto LABEL_5;
      if ( v3 != 2 )
      {
        if ( v3 == 3 )
        {
          real = v0->v.real;
          number = (float)v0[1].v.number;
          v8 = real < number;
          v9 = real == number;
          goto LABEL_17;
        }
LABEL_15:
        vm_error("Bad <, right isn't a number when left is.\n");
      }
LABEL_19:
      v10 = v0[1].v.number < v0->v.number;
      goto LABEL_18;
    }
    if ( type != 4 )
    {
      if ( type != 5 )
      {
LABEL_5:
        v4 = vm_valueType(type);
        vm_error("Bad <, type invalid (got %s) of left value.\n", v4);
      }
      v5 = (Vm_Buffer_T *)vm_compareBufWithBufStr((const char *)&v0[1], (int)&vm_stack[vm_sp], "<");
      goto LABEL_12;
    }
    if ( v3 != 4 )
      vm_error("Bad <, right isn't a string when left is.\n");
LABEL_11:
    v5 = (Vm_Buffer_T *)strcmp(*(const char **)(v0[1].v.number + 12), *(const char **)(v0->v.number + 12));
LABEL_12:
    v6 = (unsigned int)v5;
    vm_cleanupValue(v5, (Vm_String_T *)v1, v1);
    v6 >>= 31;
    v1->type = 2;
    v1->v.number = v6;
    vm_popStack((Vm_Buffer_T *)v6, (Vm_String_T *)v1);
    return;
  }
  if ( v3 != 2 )
  {
    if ( v3 == 3 )
    {
      v13 = v0->v.real;
      v14 = v0[1].v.real;
      v8 = v13 < v14;
      v9 = v13 == v14;
      v0[1].type = 2;
      goto LABEL_17;
    }
    goto LABEL_15;
  }
  v7 = (float)v0->v.number;
  v0[1].type = 2;
  v8 = v7 < v0[1].v.real;
  v9 = v7 == v0[1].v.real;
LABEL_17:
  v10 = !v8 && !v9;
LABEL_18:
  v0[1].v.number = v10;
  vm_popStack((Vm_Buffer_T *)v0, (Vm_String_T *)v1);
}