Vm_Value_T_2 *__cdecl vm_safeFindForInsert(Vm_Mapping_T *m, Vm_Value_T_2 *lv, int doTheFree)
{
  unsigned __int8 type; // al
  int v4; // ebx
  unsigned int tableSize; // edx
  unsigned int v6; // esi
  Vm_Value_T_2 *v7; // edi
  unsigned __int8 v8; // al
  bool v9; // zf
  bool v10; // al
  Vm_Value_T_2 *v11; // edx
  unsigned int v13; // eax
  char *TinyBlock; // eax
  char *v15; // esi
  char *v16; // ebx
  void *v17; // edx
  _DWORD *v18; // ecx
  int v19; // ecx
  _WORD *info; // esi
  unsigned __int16 v21; // ax
  unsigned __int16 v22; // ax
  void *v23; // eax
  unsigned int v24; // edx
  unsigned int v25; // edi
  int v26; // eax
  _DWORD *v27; // ebx
  _DWORD *v28; // edx
  int v29; // ecx
  Vm_Mapping_T *v30; // [esp+0h] [ebp-38h]
  Vm_Mapping_Node_T *pElt; // [esp+Ch] [ebp-2Ch]
  Vm_Mapping_Node_T **ppa; // [esp+28h] [ebp-10h]

  type = lv->type;
  if ( type == 4 )
  {
    if ( (m->attrib & 1) != 0 )
    {
      info = lv->v.info;
      v21 = info[4];
      v4 = v21;
      if ( !v21 )
      {
        v22 = vm_whashstr(*((_BYTE **)info + 3), 32);
        info[4] = v22;
        v4 = v22;
      }
LABEL_4:
      tableSize = m->tableSize;
      v6 = v4 & tableSize;
      v7 = (Vm_Value_T_2 *)m->table[v4 & tableSize];
      ppa = &m->table[v4 & tableSize];
      pElt = (Vm_Mapping_Node_T *)v7;
      if ( v7 )
        goto LABEL_7;
      goto LABEL_37;
    }
    v4 = (unsigned __int16)vm_whashstrc(*(_BYTE **)(lv->v.number + 12), 32);
  }
  else
  {
    if ( type != 5 )
    {
      v4 = lv->v.number >> 4;
      goto LABEL_4;
    }
    v23 = lv->v.info;
    v24 = *((_DWORD *)v23 + 3);
    if ( v24 >= 0x21 )
      v24 = 32;
    v4 = vm_whashbuf(*((const unsigned __int8 **)v23 + 4), v24);
  }
  tableSize = m->tableSize;
  v6 = v4 & tableSize;
  v7 = (Vm_Value_T_2 *)m->table[v4 & tableSize];
  ppa = &m->table[v4 & tableSize];
  pElt = (Vm_Mapping_Node_T *)v7;
  if ( v7 )
  {
    while ( 1 )
    {
LABEL_7:
      v8 = pElt->values[0].type;
      if ( v8 != lv->type )
        goto LABEL_6;
      if ( v8 == 3 )
      {
        v7 = lv;
        v10 = lv->v.real == pElt->values[0].v.real;
        goto LABEL_13;
      }
      if ( v8 <= 3u )
        break;
      if ( v8 != 4 )
      {
        if ( v8 == 5 )
        {
          v17 = pElt->values[0].v.info;
          v18 = lv->v.info;
          if ( *((_DWORD *)v17 + 3) != v18[3] )
            goto LABEL_6;
          v9 = 1;
          v6 = v18[4];
          v7 = (Vm_Value_T_2 *)*((_DWORD *)v17 + 4);
          v19 = *((_DWORD *)v17 + 3);
          do
          {
            if ( !v19 )
              break;
            v9 = *(_BYTE *)v6++ == LOBYTE(v7->ref);
            v7 = (Vm_Value_T_2 *)((char *)v7 + 1);
            --v19;
          }
          while ( v9 );
LABEL_12:
          v10 = v9;
LABEL_13:
          if ( v10 )
            goto LABEL_14;
          goto LABEL_6;
        }
LABEL_11:
        v9 = lv->v.number == pElt->values[0].v.number;
        goto LABEL_12;
      }
      v27 = lv->v.info;
      v28 = pElt->values[0].v.info;
      if ( v27 == v28 )
      {
LABEL_14:
        if ( doTheFree )
          vm_cleanupValue((Vm_Buffer_T *)&pElt->values[1], (Vm_String_T *)v6, (Vm_Value_T_2 *)&pElt->values[1]);
        return (Vm_Value_T_2 *)&pElt->values[1];
      }
      v6 = v27[4];
      if ( v6 == v28[4] )
      {
        if ( (m->attrib & 1) != 0 )
        {
          if ( ((_BYTE)v27[1] & (_BYTE)v28[1] & 1) != 0 )
            goto LABEL_6;
          v9 = 1;
          v6 = v27[3];
          v7 = (Vm_Value_T_2 *)v28[3];
          v29 = v27[4];
          do
          {
            if ( !v29 )
              break;
            v9 = *(_BYTE *)v6++ == LOBYTE(v7->ref);
            v7 = (Vm_Value_T_2 *)((char *)v7 + 1);
            --v29;
          }
          while ( v9 );
        }
        else
        {
          v9 = vm_stricmp((const char *)v27[3], (const char *)v7) == 0;
        }
        goto LABEL_12;
      }
LABEL_6:
      pElt = pElt->pNext;
      if ( !pElt )
      {
        pElt = *ppa;
        goto LABEL_19;
      }
    }
    if ( v8 == 2 )
    {
      v6 = (unsigned int)lv;
      v9 = lv->v.number == pElt->values[0].v.number;
      goto LABEL_12;
    }
    goto LABEL_11;
  }
LABEL_37:
  v9 = m->unfilled-- == 1;
  if ( v9 )
  {
    v25 = tableSize + 1;
    v26 = vm_growMapping(v30);
    v11 = 0;
    if ( !v26 )
      return v11;
    if ( (v4 & v25) != 0 )
      v6 |= v25;
    ppa = &m->table[v6];
    pElt = *ppa;
  }
LABEL_19:
  v13 = m->count + 1;
  if ( v13 <= 0x300000 )
  {
    m->count = v13;
    TinyBlock = (char *)pdb_allocateTinyBlock(v13);
    v11 = 0;
    v15 = TinyBlock;
    if ( TinyBlock )
    {
      v16 = TinyBlock + 4;
      vm_initValue((Vm_Value_T_2 *)(TinyBlock + 4));
      vm_initValue((Vm_Value_T_2 *)(v15 + 28));
      vm_copyValue(v16, (int)lv);
      *ppa = (Vm_Mapping_Node_T *)v15;
      *(_DWORD *)&vm_mapNodeToFree.values[0].type += 52;
      *(_DWORD *)v15 = pElt;
      ++vm_mapNodeToFree.values[0].ref;
      return (Vm_Value_T_2 *)(v15 + 28);
    }
    return v11;
  }
  return 0;
}