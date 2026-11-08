void vm_leValue()
{
  Vm_Value_T_2 *v0; // ebx
  Vm_Value_T_2 *v1; // esi
  unsigned __int8 type; // al
  unsigned __int8 v3; // cl
  char *v4; // eax
  bool v5; // al
  int v6; // eax
  _BOOL4 v7; // ebx
  Vm_Real_T number; // xmm0_4
  bool v9; // cf
  Vm_Value_T_2 *v10; // [esp-10h] [ebp-28h]

  v0 = &vm_stack[vm_sp];
  v1 = v0 + 1;
  type = v0[1].type;
  v3 = v0->type;
  if ( type == v3 )
  {
    if ( type == 2 )
      goto LABEL_18;
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
          v9 = v0->v.real < (long double)(float)v0[1].v.number;
          goto LABEL_16;
        }
LABEL_14:
        vm_error("Bad <=, right isn't a number when left is.\n");
      }
LABEL_18:
      v5 = v0[1].v.number <= v0->v.number;
      goto LABEL_17;
    }
    if ( type != 4 )
    {
      if ( type != 5 )
      {
LABEL_5:
        v4 = vm_valueType(type);
        vm_error("Bad <=, type invalid (got %s) of left value.\n", v4);
      }
      v0 = (Vm_Value_T_2 *)vm_compareBufWithBufStr((const char *)&v0[1], (int)&vm_stack[vm_sp], "<=");
      vm_cleanupValue((Vm_Buffer_T *)v0, (Vm_String_T *)v1, v1);
      v5 = (int)v0 <= 0;
      v1->type = 2;
      goto LABEL_17;
    }
    if ( v3 != 4 )
      vm_error("Bad <=, right isn't a string when left is.\n");
LABEL_11:
    v6 = strcmp(*(const char **)(v0[1].v.number + 12), *(const char **)(v0->v.number + 12));
    v10 = v0 + 1;
    v7 = v6 <= 0;
    vm_cleanupValue((Vm_Buffer_T *)v7, (Vm_String_T *)v1, v10);
    v1->type = 2;
    v1->v.number = v7;
    vm_popStack((Vm_Buffer_T *)v7, (Vm_String_T *)v1);
    return;
  }
  if ( v3 != 2 )
  {
    if ( v3 == 3 )
    {
      v9 = v0->v.real < (long double)v0[1].v.real;
      v0[1].type = 2;
      goto LABEL_16;
    }
    goto LABEL_14;
  }
  number = (float)v0->v.number;
  v0[1].type = 2;
  v9 = number < v0[1].v.real;
LABEL_16:
  v5 = !v9;
LABEL_17:
  v1->v.number = v5;
  vm_popStack((Vm_Buffer_T *)v0, (Vm_String_T *)v1);
}