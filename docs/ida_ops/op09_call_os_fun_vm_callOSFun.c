void __cdecl vm_callOSFun(
        int (__cdecl *ptrOsFun)(int, int, int, int, int, int, int, int, int, int, int, int, int),
        int a2)
{
  int v3; // ebx
  Vm_Value_T_2 *v4; // edx
  bool v5; // cc
  int v6; // ebx
  char *v7; // eax
  int argv[16]; // [esp+0h] [ebp-58h] BYREF
  Vm_Value_T_2 v9; // [esp+40h] [ebp-18h] BYREF

  v3 = 0;
  memset(argv, 0, sizeof(argv));
  if ( a2 > 0 )
  {
    while ( 2 )
    {
      v4 = &vm_stack[v3 + vm_sp];
      switch ( v4->type )
      {
        case 0u:
        case 1u:
          argv[v3++] = 0;
          v5 = v3 < a2;
          goto LABEL_5;
        case 2u:
          argv[v3] = v4->v.number;
          goto LABEL_4;
        case 3u:
          argv[v3++] = v4->v.number;
          v5 = v3 < a2;
          goto LABEL_5;
        case 4u:
          argv[v3] = *(_DWORD *)(v4->v.number + 12);
          goto LABEL_4;
        case 5u:
          argv[v3] = *(_DWORD *)(v4->v.number + 16);
LABEL_4:
          v5 = ++v3 < a2;
LABEL_5:
          if ( !v5 )
            break;
          continue;
        default:
          v7 = vm_valueType(v4->type);
          vm_error("Bad type (%s) of the argument for os function.\n", v7);
          return;
      }
      break;
    }
  }
  v6 = ptrOsFun(
         argv[0],
         argv[1],
         argv[2],
         argv[3],
         argv[4],
         argv[5],
         argv[6],
         argv[7],
         argv[8],
         argv[9],
         argv[10],
         argv[11],
         argv[12]);
  vm_popNStack(&v9, a2);
  vm_pushNumber(v6);
}