int __usercall vm_compareValue@<eax>(int a1@<eax>, int a2@<edx>)
{
  unsigned __int8 v6; // al
  bool v7; // cf
  bool v8; // cc
  int v9; // edx
  int v10; // edx
  int v11; // ebx
  int v12; // edx
  int v13; // eax
  unsigned int v15; // esi
  int v16; // eax
  int v17; // eax
  int v18; // edx
  unsigned int v19; // ecx
  unsigned int v20; // ebx
  bool v21; // zf
  char *v22; // ebx
  int v23; // eax
  int v24; // eax
  int v25; // ecx
  unsigned int v26; // [esp+Ch] [ebp-1Ch]
  char *p2; // [esp+18h] [ebp-10h]

  v6 = *(_BYTE *)(a1 + 4);
  v7 = v6 < *(_BYTE *)(a2 + 4);
  v8 = v6 <= *(_BYTE *)(a2 + 4);
  v9 = -1;
  if ( !v7 )
  {
    if ( v8 )
    {
      switch ( v6 )
      {
        case 0u:
        case 1u:
          return 0;
        case 2u:
          v24 = *(_DWORD *)(a2 + 8);
          v25 = 1;
          if ( *(_DWORD *)(a1 + 8) <= v24 )
            return (*(_DWORD *)(a1 + 8) >= v24) - 1;
          return v25;
        case 3u:
          v16 = 1;
          if ( *(float *)(a1 + 8) <= (long double)*(float *)(a2 + 8) )
            return (*(float *)(a2 + 8) <= (long double)*(float *)(a1 + 8)) - 1;
          return v16;
        case 4u:
          v22 = *(char **)(*(_DWORD *)(a1 + 8) + 12);
          p2 = *(char **)(*(_DWORD *)(a2 + 8) + 12);
          if ( !vm_isdigit((unsigned __int8)*v22) || !vm_isdigit((unsigned __int8)*p2) )
            goto LABEL_22;
          v11 = vm_strtol(v10, v22, 0, 10, 0);
          v13 = vm_strtol(v12, p2, 0, 10, 0);
          v9 = -1;
          if ( v11 < v13 )
            return v9;
          if ( v11 > v13 )
            return 1;
LABEL_22:
          v23 = strcmp(*(const char **)(*(_DWORD *)(a1 + 8) + 12), *(const char **)(*(_DWORD *)(a2 + 8) + 12));
          v9 = 1;
          if ( v23 <= 0 )
            v9 = v23 >> 31;
          break;
        case 5u:
          v17 = *(_DWORD *)(a1 + 8);
          v18 = *(_DWORD *)(a2 + 8);
          v19 = *(_DWORD *)(v18 + 12);
          v20 = *(_DWORD *)(v17 + 12);
          v26 = v19;
          if ( v19 > v20 )
            v19 = *(_DWORD *)(v17 + 12);
          if ( v19 )
          {
            v21 = memcmp(*(const void **)(v17 + 16), *(const void **)(v18 + 16), v19) == 0;
            v9 = !v21;
            if ( !v21 )
              return v9;
          }
          v16 = 1;
          if ( v20 <= v26 )
            return -(v20 < v26);
          return v16;
        case 6u:
        case 7u:
        case 8u:
        case 9u:
        case 0xAu:
          v15 = *(_DWORD *)(a2 + 8);
          v16 = 1;
          if ( *(_DWORD *)(a1 + 8) <= v15 )
            return -(*(_DWORD *)(a1 + 8) < v15);
          return v16;
        default:
          vm_syslogf(
            2,
            4,
            "Fatal error = %s\nFILE(%s)\nLINE(%d)\n",
            "Bad type of value when comparing.\n",
            "/home/cc/xxxx/gameshell/src/vm/vm_sort.c",
            401);
          vm_endProcess(0);
      }
    }
    else
    {
      return 1;
    }
  }
  return v9;
}