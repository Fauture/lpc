void __usercall vm_castType(Vm_Buffer_T *a1@<ebx>, unsigned int a2@<edi>, int attrib, char attriba)
{
  Vm_Value_T_2 *v5; // edx
  Vm_Value_T_2 *v6; // esi
  const char *v7; // ebx
  Vm_Real_T number; // xmm0_4
  Vm_Buffer_T *v9; // ebx
  void *v10; // eax
  int v11; // ebx
  int v12; // edx
  Vm_Buffer_T *v13; // ebx
  int v14; // edi
  Vm_String_T *v15; // eax
  _DWORD *v16; // eax
  Vm_Buffer_T *v17; // ebx
  unsigned int v18; // edi
  int v19; // eax
  _DWORD *info; // eax
  unsigned int v21; // ecx
  int v22; // ebx
  unsigned int v23; // edi
  int v24; // edx
  int v25; // eax
  Vm_Buffer_T *v26; // ebx
  Vm_Buffer_T *Array; // ebx
  Vm_Buffer_T *Mapping; // ebx
  unsigned int v29; // ecx
  Vm_Buffer_T *pBuffer; // [esp+4h] [ebp-14h]
  Vm_Buffer_T *pBufferc; // [esp+4h] [ebp-14h]
  Vm_Buffer_T *pBuffera; // [esp+4h] [ebp-14h]
  Vm_Buffer_T *pBufferb; // [esp+4h] [ebp-14h]
  float v34; // [esp+8h] [ebp-10h]

  v5 = vm_stack;
  v6 = &vm_stack[vm_sp];
  switch ( attrib )
  {
    case 2:
      switch ( v6->type )
      {
        case 0u:
        case 1u:
          v6->type = 2;
          v6->v.number = 0;
          break;
        case 2u:
          goto LABEL_3;
        case 3u:
          v6->type = 2;
          v6->v.number = vm_ftol(v6->v.real);
          break;
        case 4u:
          v26 = (Vm_Buffer_T *)__strtol_internal(*(const char **)(v6->v.number + 12), 0, 10, 0);
          vm_cleanupValue(v26, (Vm_String_T *)v6, v6);
          v6->type = 2;
          v6->v.number = (int)v26;
          break;
        case 5u:
          info = v6->v.info;
          v21 = info[3];
          v22 = 0;
          v23 = 0;
          if ( v21 )
          {
            v24 = info[4];
            do
            {
              v25 = *(unsigned __int8 *)(v24 + v23++);
              v22 = v25 | (v22 << 8);
            }
            while ( v21 > v23 );
          }
          vm_cleanupValue((Vm_Buffer_T *)v22, (Vm_String_T *)v6, &vm_stack[vm_sp]);
          v6->v.number = v22;
          v6->type = 2;
          break;
        default:
          vm_error("Can not cast the type to integer.\n");
          return;
      }
      goto LABEL_3;
    case 3:
      switch ( v6->type )
      {
        case 0u:
        case 1u:
          v6->type = 3;
          v6->v.number = 0;
          break;
        case 2u:
          number = (float)v6->v.number;
          v6->type = 3;
          v6->v.real = number;
          break;
        case 3u:
          goto LABEL_3;
        case 4u:
          v7 = *(const char **)(v6->v.number + 12);
          while ( vm_isspace((int)v5) )
            ++v7;
          v34 = __strtod_internal(v7, 0, 0);
          vm_cleanupValue((Vm_Buffer_T *)LODWORD(v34), (Vm_String_T *)v6, v6);
          v6->type = 3;
          v6->v.real = v34;
          break;
        default:
          vm_error("Can not cast the type to real.\n");
          return;
      }
      goto LABEL_3;
    case 4:
      switch ( v6->type )
      {
        case 0u:
        case 1u:
        case 2u:
        case 3u:
        case 0xAu:
          vm_typeValue(vm_stack);
          v9 = (Vm_Buffer_T *)vm_copyString((const char *)v6);
          vm_cleanupValue(v9, (Vm_String_T *)v6, v6);
          v6->v.number = (int)v9;
          v6->type = 4;
          goto LABEL_3;
        case 4u:
          goto LABEL_3;
        case 5u:
          v10 = v6->v.info;
          v11 = *((_DWORD *)v10 + 3);
          v12 = *((_DWORD *)v10 + 4);
          pBuffer = (Vm_Buffer_T *)v10;
          break;
        default:
          vm_error("Can not cast the type to string.\n");
          return;
      }
      return;
    case 5:
      switch ( v6->type )
      {
        case 0u:
        case 1u:
          pBufferc = vm_allocateBuffer(a2);
          vm_cleanupValue(a1, (Vm_String_T *)v6, v6);
          v6->v.number = (int)pBufferc;
          v6->type = 5;
          break;
        case 2u:
          pBufferb = vm_allocateBuffer(a2);
          pBufferb->ptr[3] = v6->v.number;
          v18 = 1;
          v19 = v6->v.number >> 8;
          do
          {
            v6->v.number = v19;
            pBufferb->ptr[4 - v18++ - 1] = v6->v.number;
            v19 = v6->v.number >> 8;
          }
          while ( v18 < 4 );
          v6->v.number = (int)pBufferb;
          v6->type = 5;
          break;
        case 4u:
          v17 = *(Vm_Buffer_T **)(v6->v.number + 16);
          pBuffera = vm_allocateBuffer(a2);
          if ( v17 )
            memcpy(pBuffera->ptr, *(const void **)(v6->v.number + 12), (size_t)v17);
          vm_cleanupValue(v17, (Vm_String_T *)v6, v6);
          v6->v.number = (int)pBuffera;
          v6->type = 5;
          break;
        case 5u:
          goto LABEL_3;
        default:
          vm_error("Can not cast the type to buffer.\n");
          return;
      }
      goto LABEL_3;
    case 6:
      if ( v6->type <= 1u )
      {
        Array = (Vm_Buffer_T *)vm_allocateArray((unsigned int)v6);
        vm_cleanupValue(Array, (Vm_String_T *)v6, v6);
        v6->v.number = (int)Array;
        v6->type = 6;
      }
      else if ( v6->type != 6 )
      {
        vm_error("Can not cast the type to array.\n");
      }
      goto LABEL_3;
    case 7:
      if ( v6->type <= 1u )
      {
        Mapping = (Vm_Buffer_T *)vm_allocateMapping(0);
        vm_cleanupValue(Mapping, (Vm_String_T *)v6, v6);
        v6->v.number = (int)Mapping;
        v6->type = 7;
      }
      else if ( v6->type != 7 )
      {
        vm_error("Can not cast the type to mapping.\n");
      }
      goto LABEL_3;
    default:
      vm_error("Can not cast to such type.\n");
      return;
  }
  do
  {
    if ( --v11 == -1 )
      goto LABEL_27;
  }
  while ( *(_BYTE *)(v12 + v11) );
  v29 = strlen((const char *)v12) + 1;
  v13 = (Vm_Buffer_T *)(v29 - 1);
  if ( !v29 )
LABEL_27:
    v13 = (Vm_Buffer_T *)*((_DWORD *)v10 + 3);
  if ( (unsigned int)v13 > 2 && *(_BYTE *)v12 == 0xEF && *(_BYTE *)(v12 + 1) == 0xBB && *(_BYTE *)(v12 + 2) == 0xBF )
  {
    v14 = 3;
    v13 = (Vm_Buffer_T *)((char *)v13 - 3);
  }
  else
  {
    v14 = 0;
  }
  v15 = vm_newString((unsigned int)&v13->ref + 1);
  v6->v.number = (int)v15;
  memcpy(v15->ptr, &pBuffer->ptr[v14], (size_t)v13);
  v16 = v6->v.info;
  v16[4] = v13;
  *((_BYTE *)&v13->ref + v16[3]) = 0;
  v6->type = 4;
  vm_freeBuffer(v13);
LABEL_3:
  if ( (attriba & 2) != 0 )
    v6->attrib |= 1u;
}