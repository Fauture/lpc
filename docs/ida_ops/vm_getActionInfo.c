Vm_Action_Info_T *__cdecl vm_getActionInfo(unsigned int a1)
{
  Vm_Action_Info_T *result; // eax

  result = 0;
  if ( a1 <= 0x56 )
    return &actionInfoTable[a1];
  return result;
}