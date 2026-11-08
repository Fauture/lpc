void __fastcall vm_isType(int type)
{
  int v1; // esi
  Vm_Value_T_2 *v2; // ebx
  unsigned __int8 v3; // dl
  int v4; // eax
  bool v5; // zf
  int v6; // edi
  int v7; // eax
  int v8; // [esp+20h] [ebp+8h]

  v2 = &vm_stack[vm_sp];
  v3 = v2->type;
  switch ( v3 )
  {
    case 0u:
    case 1u:
    case 2u:
    case 3u:
      v2->type = 2;
      v2->v.number = v3 == v8;
      return;
    case 4u:
    case 5u:
    case 6u:
    case 7u:
    case 8u:
      goto LABEL_4;
    case 9u:
      v4 = *(_DWORD *)(v2->v.number + 12);
      if ( v4 && *(char *)(v4 + 13) < 0 )
        goto LABEL_10;
LABEL_4:
      v5 = v3 == v8;
      goto LABEL_5;
    case 0xAu:
      v6 = *(_DWORD *)(v2->v.number + 12);
      if ( (v6 & 0x8000) != 0 )
      {
LABEL_10:
        v5 = v8 == 0;
LABEL_5:
        v1 = v5;
      }
      else
      {
        v1 = v3 == v8;
        if ( v3 == v8 )
        {
          v7 = -1;
          if ( (v6 & 0x2000) != 0 )
            v1 = -1;
          LOBYTE(v7) = -2;
          if ( (v6 & 0x4000) != 0 )
            v1 = v7;
        }
      }
      vm_cleanupValue((Vm_Buffer_T *)v2, (Vm_String_T *)v1, &vm_stack[vm_sp]);
LABEL_7:
      v2->type = 2;
      v2->v.number = v1;
      return;
    default:
      goto LABEL_7;
  }
}