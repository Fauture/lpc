void __usercall vm_addValue(Vm_Buffer_T *a1@<ebx>)
{
  Vm_Value_T_2 *v1; // esi
  unsigned __int8 type; // al
  int v3; // eax
  int v4; // eax
  int v5; // eax
  Vm_String_T *v6; // edx
  size_t v7; // ebx
  Vm_Mapping_T *mapping; // eax
  Vm_Array_T *v9; // ebx
  Vm_Mapping_T *v10; // ebx
  float v11; // [esp-10h] [ebp-28h]
  const char *v12; // [esp-Ch] [ebp-24h]

  v1 = &vm_stack[vm_sp];
  type = v1[1].type;
  switch ( type )
  {
    case 2u:
      v3 = v1->type;
      if ( v3 == 2 )
      {
        v1[1].v.number += v1->v.number;
      }
      else
      {
        if ( v3 != 3 )
          vm_error("Bad +, right isn't a number when left is.\n");
        v11 = v1->v.real + (long double)v1[1].v.number;
        v1[1].v.number = vm_ftol(v11);
      }
      goto LABEL_20;
    case 3u:
      v4 = v1->type;
      if ( v4 == 2 )
      {
        v1[1].v.real = v1[1].v.real + (long double)v1->v.number;
      }
      else
      {
        if ( v4 != 3 )
          vm_error("Bad +, right isn't a real when left is.\n");
        v1[1].v.real = v1->v.real + v1[1].v.real;
      }
      goto LABEL_20;
    case 4u:
      switch ( v1->type )
      {
        case 0u:
          vm_error("Bad +, string can not be add with undefined type.\n");
          return;
        case 1u:
          vm_error("Bad +, string can not be add with void.\n");
          return;
        case 2u:
        case 3u:
          vm_typeValue((Vm_Value_T_2 *)type);
          v1[1].v.number = (int)vm_concatStringWithCStr(v6, (char *)v1[1].v.info);
          break;
        case 4u:
          v1[1].v.number = (int)vm_concatString((Vm_String_T *)v1->type, v1[1].v.string);
          break;
        default:
          vm_error("Bad +, right can not be add with a string.\n");
          return;
      }
      goto LABEL_20;
    case 5u:
      v5 = v1->type;
      if ( v5 == 1 )
        vm_error("Bad +, buffer can not be add with void.\n");
      if ( v1->type <= 1u )
      {
        if ( !v1->type )
          vm_error("Bad +, buffer can not be add with undefined type.\n");
LABEL_16:
        vm_error("Bad +, right can not be add with a buffer.\n");
      }
      if ( v5 == 4 )
      {
        vm_dupBufferToModify((Vm_Buffer_T **)v1);
        vm_addStrToBuffer(a1, (char *)v1[1].v.info);
      }
      else
      {
        if ( v5 != 5 )
          goto LABEL_16;
        v7 = *(_DWORD *)(v1->v.number + 12);
        if ( v7 )
        {
          vm_dupBufferToModify((Vm_Buffer_T **)v1);
          if ( vm_extendBuffer((Vm_Buffer_T *)v7, (unsigned int)v1[1].v.info) != v7 )
            vm_error("Can not extend buffer for add.\n");
          memcpy(
            (void *)(*(_DWORD *)(v1[1].v.number + 16) + *(_DWORD *)(v1[1].v.number + 12)),
            *(const void **)(v1->v.number + 16),
            v7);
          *(_DWORD *)(v1[1].v.number + 12) += v7;
        }
      }
LABEL_20:
      vm_popStack();
      return;
    case 6u:
      if ( v1->type != 6 )
        vm_error("Bad +, right can not be add with an array.\n");
      v9 = vm_addArray(v1[1].v.array, v1->v.array);
      vm_unrefArray(v9);
      v1[1].v.number = (int)v9;
      goto LABEL_20;
    case 7u:
      if ( v1->type != 7 )
        vm_error("Bad +, right can not be add with a mapping.\n");
      mapping = v1[1].v.mapping;
      if ( mapping->ref > 1 )
      {
        v10 = vm_copyMapping((Vm_Mapping_T *)v1);
        vm_unrefMapping((Vm_Mapping_T *)&v1[1]);
        v1[1].v.number = (int)v10;
      }
      vm_concatMapping(v1[1].v.mapping, mapping);
      goto LABEL_20;
    default:
      v12 = vm_valueType(type);
      vm_error("Bad +, type invalid (got %s) of left value.\n", v12);
      return;
  }
}