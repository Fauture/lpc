Vm_Program_T *__usercall vm_createProgram@<eax>(
        const char *pProgramName@<eax>,
        Vm_Code_Segment_T_0 *a2@<edx>,
        char *a3)
{
  Vm_Program_T *nodes; // edi
  Vm_Program_T *v7; // esi
  int v8; // eax
  Vm_Fun *ProgramFunctionByCStr; // eax
  Vm_Fun *v11; // ebx
  __int16 v12; // ax
  Vm_Fun *v13; // eax
  Vm_Fun *v14; // ebx
  int v15; // [esp+0h] [ebp-24h]
  Vm_Scratchpad_Info_T *pScratchpad; // [esp+14h] [ebp-10h] BYREF

  nodes = (Vm_Program_T *)a3;
  pScratchpad = 0;
  v7 = (Vm_Program_T *)vm_buildProgramHead(pProgramName);
  if ( v7 )
  {
    v8 = vm_compile(a2, a3, v7, &pScratchpad);
    if ( v8 )
    {
      vm_errprintf("Failed to compile \"%s\", errCode = %d.\n", a3, v8);
LABEL_4:
      vm_destructProgramHead(v7);
      goto LABEL_5;
    }
    ProgramFunctionByCStr = vm_findProgramFunctionByCStr(v7, (const char *)a2, (int)v7, (Vm_Index_T *)"create");
    v11 = ProgramFunctionByCStr;
    if ( ProgramFunctionByCStr )
    {
      v12 = ProgramFunctionByCStr->attrib | 1;
      v11->attrib = v12;
      if ( (v12 & 4) == 0 )
      {
        vm_stdprintf("Warning: Construction function \"create\" must be a private function.\n");
        v11->attrib |= 4u;
      }
      nodes = (Vm_Program_T *)v11->codeSegment.nodes;
      if ( !nodes )
      {
        vm_errprintf("Error: Construction function is not implemented.\n", v15, 1, &v7->createFunOffset);
        goto LABEL_4;
      }
    }
    else
    {
      v7->createFunOffset = -1;
    }
    v13 = vm_findProgramFunctionByCStr(v7, (const char *)v11, (int)v7, (Vm_Index_T *)"destruct");
    v14 = v13;
    if ( v13 )
    {
      if ( (v13->attrib & 4) == 0 )
      {
        vm_stdprintf("Warning: destruction function \"destruct\" must be a private function.\n");
        v14->attrib |= 4u;
      }
      if ( !v14->codeSegment.nodes )
      {
        vm_errprintf("Error: Destruction function is not implemented.\n");
        goto LABEL_4;
      }
    }
    else
    {
      v7->destructFunOffset = -1;
    }
    v7->ref = 0;
    vm_applyBreakpointsOnProgram(nodes);
    if ( !pScratchpad )
      return v7;
    goto LABEL_16;
  }
LABEL_5:
  v7 = 0;
  if ( !pScratchpad )
    return v7;
LABEL_16:
  vm_destructScratch((Vm_Scratchpad_Info_T *)v7);
  return v7;
}