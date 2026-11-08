void __cdecl vm_opEqValue(Vm_Value_T_2 *pToValue)
{
  unsigned int v2; // edx
  Vm_Value_T_2 *v3; // ecx
  Vm_Value_T_2 *pPtrValue; // ebx
  Vm_Value_T_2 *v5; // edx
  Vm_Value_T_2 *v6; // ebx
  Vm_String_T *string; // esi
  Vm_Value_T_2 *v8; // ecx
  Vm_Buffer_T *v9; // ebx
  unsigned __int8 type; // al
  Vm_Value_T_2 *v11; // ebx
  int v12; // eax
  int v13; // eax
  float v14; // [esp-10h] [ebp-28h]
  const char *v15; // [esp-Ch] [ebp-24h]
  const char *v16; // [esp-Ch] [ebp-24h]

  v2 = vm_sp;
  if ( (int)vm_sp <= 0 )
    vm_error("Stack overflow when do op_eq.\n");
  v3 = &vm_stack[vm_sp];
  pPtrValue = v3[1].v.pPtrValue;
  --vm_sp;
  v5 = &vm_stack[v2 - 1];
  v5->type = v3->type;
  v5->attrib = v3->attrib;
  v5->v.number = v3->v.number;
  vm_copyValue(pToValue, pPtrValue);
  switch ( (unsigned int)pToValue )
  {
    case 2u:
      vm_addValue((Vm_Buffer_T *)pPtrValue);
      goto LABEL_4;
    case 3u:
      vm_subValue();
      goto LABEL_4;
    case 4u:
      v8 = &vm_stack[vm_sp];
      v9 = (Vm_Buffer_T *)&v8[1];
      type = v8[1].type;
      if ( type == 2 )
      {
        v12 = v8->type;
        if ( v12 == 2 )
        {
          v8[1].v.number *= v8->v.number;
          goto LABEL_28;
        }
        if ( v12 == 3 )
        {
          v14 = v8->v.real * (long double)v8[1].v.number;
          v8[1].v.number = vm_ftol(v14);
          goto LABEL_28;
        }
LABEL_24:
        vm_error("Bad *, right isn't a number when left is.\n");
      }
      if ( type != 3 )
      {
        v15 = vm_valueType(type);
        vm_error("Bad *, type invalid (got %s) of left value.\n", v15);
      }
      v13 = v8->type;
      if ( v13 == 2 )
      {
        v8[1].v.real = v8[1].v.real * (long double)v8->v.number;
        goto LABEL_28;
      }
      if ( v13 != 3 )
        goto LABEL_24;
      v8[1].v.real = v8->v.real * v8[1].v.real;
LABEL_28:
      vm_popStack(v9, (Vm_String_T *)pToValue);
LABEL_4:
      v6 = &vm_stack[vm_sp];
      string = v6[1].v.string;
      vm_cleanupValue((Vm_Buffer_T *)v6, string, (Vm_Value_T_2 *)string);
      vm_copyValue((Vm_Value_T_2 *)string, v6);
      vm_unrefValue(v6);
      v6[1].type = v6->type;
      v6[1].attrib = v6->attrib;
      v6[1].v.number = v6->v.number;
      vm_stack[vm_sp].type = 0;
      vm_stack[vm_sp++].attrib = 0;
      return;
    case 5u:
      vm_divValue();
      goto LABEL_4;
    case 6u:
      vm_andValue();
      goto LABEL_4;
    case 7u:
      vm_orValue();
      goto LABEL_4;
    case 8u:
      v11 = &vm_stack[vm_sp];
      if ( v11[1].type != 2 )
      {
        v16 = vm_valueType((int)vm_stack);
        vm_error("Bad ^, type invalid (got %s) of left value.\n", v16);
      }
      if ( v11->type != 2 )
        vm_error("Bad ^, right isn't a number when left is.\n");
      v11[1].v.number ^= (int)v11->v.info;
      vm_popStack((Vm_Buffer_T *)v11, (Vm_String_T *)&v11[1]);
      goto LABEL_4;
    case 9u:
      vm_rshValue();
      goto LABEL_4;
    case 0xAu:
      vm_lshValue();
      goto LABEL_4;
    case 0xBu:
      vm_modValue();
      goto LABEL_4;
    default:
      yyerror("Error operate code.\n");
      goto LABEL_4;
  }
}