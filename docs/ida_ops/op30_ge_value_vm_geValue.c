void vm_geValue()
{
  Vm_Value_T_2 *v0; // ebx
  Vm_Value_T_2 *v1; // esi
  unsigned __int8 type; // al
  unsigned __int8 v3; // cl
  char *v4; // eax
  int v5; // eax
  unsigned int v6; // ebx
  Vm_Buffer_T *v7; // ebx
  float number; // xmm0_4
  bool v9; // cf
  bool v10; // al
  Vm_Value_T_2 *v11; // [esp-10h] [ebp-28h]

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
          v9 = (float)v0[1].v.number < v0->v.real;
          goto LABEL_17;
        }
LABEL_15:
        vm_error("Bad >=, right isn't a number when left is.\n");
      }
LABEL_19:
      v10 = v0[1].v.number >= v0->v.number;
      goto LABEL_18;
    }
    if ( type != 4 )
    {
      if ( type != 5 )
      {
LABEL_5:
        v4 = vm_valueType(type);
        vm_error("Bad >=, type invalid (got %s) of left value.\n", v4);
      }
      v5 = vm_compareBufWithBufStr((const char *)&v0[1], (int)&vm_stack[vm_sp], ">=");
      v11 = v0 + 1;
      v6 = ~v5;
      vm_cleanupValue((Vm_Buffer_T *)~v5, (Vm_String_T *)v1, v11);
      goto LABEL_12;
    }
    if ( v3 != 4 )
      vm_error("Bad >=, right isn't a string when left is.\n");
LABEL_11:
    v6 = ~strcmp(*(const char **)(v0[1].v.number + 12), *(const char **)(v0->v.number + 12));
    vm_cleanupValue((Vm_Buffer_T *)v6, (Vm_String_T *)v1, v1);
LABEL_12:
    v7 = (Vm_Buffer_T *)(v6 >> 31);
    v1->type = 2;
    v1->v.number = (int)v7;
    vm_popStack(v7, (Vm_String_T *)v1);
    return;
  }
  if ( v3 != 2 )
  {
    if ( v3 == 3 )
    {
      v9 = v0[1].v.real < (long double)v0->v.real;
      v0[1].type = 2;
      goto LABEL_17;
    }
    goto LABEL_15;
  }
  number = (float)v0->v.number;
  v0[1].type = 2;
  v9 = v0[1].v.real < (long double)number;
LABEL_17:
  v10 = !v9;
LABEL_18:
  v0[1].v.number = v10;
  vm_popStack((Vm_Buffer_T *)v0, (Vm_String_T *)v1);
}