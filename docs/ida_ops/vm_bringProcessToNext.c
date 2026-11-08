void __fastcall vm_bringProcessToNext(Vm_PCB_T *pPCB)
{
  Vm_PCB_T *v1; // ebx
  int v2; // eax
  int v3; // edx
  Vm_PCB *pNext; // eax
  int v5; // [esp+Ch] [ebp+8h]

  v1 = vm_pActiveProcess;
  if ( (Vm_PCB_T *)v5 != vm_pActiveProcess )
  {
    v2 = *(_DWORD *)(v5 + 488);
    if ( v2 != v5 && (*(_BYTE *)(v5 + 40) & 6) == 0 )
    {
      v3 = *(_DWORD *)(v5 + 484);
      *(_DWORD *)(v3 + 488) = v2;
      *(_DWORD *)(*(_DWORD *)(v5 + 488) + 484) = v3;
      pNext = v1->pNext;
      *(_DWORD *)(v5 + 484) = v1;
      pNext->pPrev = (Vm_PCB *)v5;
      *(_DWORD *)(v5 + 488) = pNext;
      *(_DWORD *)(*(_DWORD *)(v5 + 484) + 488) = v5;
    }
  }
}