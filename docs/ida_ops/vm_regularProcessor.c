int __cdecl vm_regularProcessor(Vm_PCB_T *pPCB)
{
  int v1; // edi
  unsigned int v2; // edx
  int v3; // eax
  Vm_Next_Action_Hook_T nextActionHook; // edx
  Vm_Code_Segment_T *cs; // esi
  int ip; // eax
  Vm_Node_T *v7; // ebx
  Vm_Action_Info_T *v8; // edx
  unsigned __int8 parameters; // al
  void *pNextActionCookie; // eax
  int v12; // ecx
  int par8; // [esp-Ch] [ebp-24h]
  int actionState; // [esp+8h] [ebp-10h]

  v1 = 2048;
  actionState = 0;
  vm_switchOutThread = 0;
  do
  {
    v2 = vm_ctrlFlags;
    if ( vm_ctrlFlags )
    {
      if ( (vm_ctrlFlags & 0x10) == 0 || (v2 = vm_ctrlFlags & 0xFFFFFFEF, v3 = 0, (vm_ctrlFlags = v2) != 0) )
      {
        if ( (v2 & 2) != 0 )
        {
          vm_ctrlFlags = v2 & 0xFFFFFFFD;
          v3 = -8103;
        }
        else if ( (v2 & 4) != 0 )
        {
          v3 = -8104;
          vm_ctrlFlags = v2 & 0xFFFFFFFB;
        }
        else if ( (v2 & 8) != 0 )
        {
          v3 = -8102;
          vm_ctrlFlags = v2 & 0xFFFFFFF7;
        }
        else if ( (v2 & 0x20) != 0 )
        {
          v3 = -8107;
          vm_ctrlFlags = v2 & 0xFFFFFFDF;
        }
        else if ( (v2 & 0x40) != 0 )
        {
          v3 = -8101;
          vm_ctrlFlags = v2 & 0xFFFFFFBF;
        }
        else
        {
          v3 = -8102;
          if ( (v2 & 0x80u) != 0 )
          {
            LOBYTE(v2) = v2 & 0x7F;
            LOBYTE(v3) = 84;
            vm_ctrlFlags = v2;
          }
        }
      }
      actionState = v3;
      if ( v3 )
        break;
    }
    if ( --v1 <= 0 )
    {
      v1 = 1024;
      if ( vm_isTimerTimeout(0) )
        break;
    }
    nextActionHook = pPCB->nextActionHook;
    if ( nextActionHook )
    {
      pNextActionCookie = pPCB->pNextActionCookie;
      pPCB->nextActionHook = 0;
      pPCB->pNextActionCookie = 0;
      nextActionHook(pNextActionCookie);
    }
    else
    {
      cs = vm_currentContext->cs;
      ip = vm_currentContext->ip;
      v7 = &cs->nodes[ip];
      vm_currentContext->ip = ip + 1;
      v8 = &actionInfoTable[v7->actionNo];
      if ( (v8->attrib & 0x80000000) != 0 )
      {
        v12 = v7->par16 | (v7->par8 << 16);
        if ( v8->parameters == 2 )
          ((void (__cdecl *)(_DWORD, _DWORD))v8->entry)(
            *(_DWORD *)&cs->metaData[v12],
            *(_DWORD *)&cs->metaData[v12 + 4]);
        else
          ((void (__cdecl *)(_DWORD, int))v8->entry)(*(_DWORD *)&cs->metaData[v12], par8);
      }
      else
      {
        parameters = v8->parameters;
        if ( parameters )
        {
          if ( parameters != 1 )
            par8 = v7->par8;
          ((void (__cdecl *)(_DWORD, int))v8->entry)(v7->par16, par8);
        }
        else
        {
          ((void (*)(void))v8->entry)();
        }
      }
    }
  }
  while ( !vm_switchOutThread );
  return actionState;
}