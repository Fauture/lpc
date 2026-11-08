void __usercall vm_subValue(Vm_Array_T *a1@<ebx>, Vm_Mapping_T *a2@<edi>)
{
  Vm_Value_T_2 *v2; // ecx
  Vm_Value_T_2 *v3; // esi
  unsigned __int8 type; // al
  char *v5; // eax
  int v6; // eax
  int v7; // eax
  int v8; // eax
  Vm_Mapping_T *v9; // eax
  float v10; // [esp-10h] [ebp-18h]

  v2 = &vm_stack[vm_sp];
  v3 = v2 + 1;
  type = v2[1].type;
  if ( type == 3 )
  {
    v7 = v2->type;
    if ( v7 == 2 )
    {
      v2[1].v.real = v2[1].v.real - (long double)v2->v.number;
      goto LABEL_5;
    }
    if ( v7 != 3 )
      goto LABEL_13;
    v2[1].v.real = v2[1].v.real - v2->v.real;
  }
  else
  {
    if ( type <= 3u )
    {
      if ( type != 2 )
        goto LABEL_4;
      v8 = v2->type;
      if ( v8 == 2 )
      {
        v2[1].v.number -= v2->v.number;
        goto LABEL_5;
      }
      if ( v8 == 3 )
      {
        v10 = (long double)v2[1].v.number - v2->v.real;
        v2[1].v.number = vm_ftol(v10);
        goto LABEL_5;
      }
LABEL_13:
      vm_error("Bad -, right isn't a number when left is.\n");
    }
    if ( type == 6 )
    {
      if ( v2->type != 6 )
        vm_error("Bad -, right can not be sub with an array.\n");
      a1 = vm_subArray(v2[1].v.array, v2->v.array);
      vm_unrefArray(a1);
      v3->v.number = (int)a1;
    }
    else
    {
      if ( type != 7 )
      {
LABEL_4:
        v5 = vm_valueType(type);
        vm_error("Bad -, type invalid (got %s) of left value.\n", v5);
      }
      v6 = v2->type;
      if ( v6 == 6 )
      {
        v9 = vm_subMappingWithArray(v2[1].v.mapping, v2->v.array, 1);
      }
      else
      {
        if ( v6 != 7 )
          vm_error("Bad -, right can not be sub with a mapping or an array.\n");
        v9 = vm_subMapping(v2[1].v.mapping, v2->v.mapping, 1);
      }
      a1 = (Vm_Array_T *)v9;
      vm_unrefMapping(a2);
      v3->v.number = (int)a1;
    }
  }
LABEL_5:
  vm_popStack((Vm_Buffer_T *)a1, (Vm_String_T *)v3);
}