void __cdecl vm_sizeof(int type)
{
  Vm_Mapping_T *v1; // edi
  Vm_Value_T_2 *v2; // ebx
  int v3; // esi

  v2 = &vm_stack[vm_sp];
  switch ( v2->type )
  {
    case 0u:
    case 1u:
    case 2u:
    case 3u:
      v2->type = 2;
      v2->v.number = 0;
      return;
    case 4u:
      v3 = *(_DWORD *)(v2->v.number + 16);
      vm_unrefString((Vm_String_T *)vm_stack);
      goto LABEL_3;
    case 5u:
      v3 = *(_DWORD *)(v2->v.number + 12);
      vm_freeBuffer((Vm_Buffer_T *)v2);
      goto LABEL_3;
    case 6u:
      v3 = *(_DWORD *)(v2->v.number + 4);
      vm_unrefArray((Vm_Array_T *)v2);
      goto LABEL_3;
    case 7u:
      v3 = *(_DWORD *)(v2->v.number + 16);
      vm_unrefMapping(v1);
      goto LABEL_3;
    default:
      v3 = 0;
      vm_cleanupValue((Vm_Buffer_T *)v2, 0, &vm_stack[vm_sp]);
LABEL_3:
      v2->type = 2;
      v2->v.number = v3;
      return;
  }
}