void vm_convertToPtr()
{
  Vm_Value_T_2 *v0; // esi
  unsigned int v1; // eax
  Vm_Value_T_2 *TinyBlock; // ebx
  Vm_Value_T_2 *v3; // eax

  v0 = &vm_stack[vm_sp];
  v1 = v0->type - 4;
  switch ( v0->type )
  {
    case 4u:
    case 5u:
      v0->attrib |= 1u;
      goto LABEL_3;
    case 6u:
    case 7u:
LABEL_3:
      TinyBlock = (Vm_Value_T_2 *)pdb_allocateTinyBlock(v1);
      if ( !TinyBlock )
        vm_error("Not eough memory to create value.\n");
      memset(TinyBlock, 0, sizeof(Vm_Value_T_2));
      TinyBlock->type = 0;
      TinyBlock->attrib = 0;
      v3 = vm_allValues;
      TinyBlock->ref = 1;
      ++vm_totalValues;
      if ( v3 )
      {
        TinyBlock->pPrev = 0;
        TinyBlock->pNext = v3;
        v3->pPrev = TinyBlock;
        vm_allValues = TinyBlock;
      }
      else
      {
        vm_allValues = TinyBlock;
        TinyBlock->pPrev = 0;
        TinyBlock->pNext = 0;
      }
      TinyBlock->type = v0->type;
      TinyBlock->attrib = v0->attrib;
      TinyBlock->v.number = v0->v.number;
      TinyBlock->pThis = TinyBlock;
      v0->type = 8;
      v0->v.number = (int)TinyBlock;
      break;
    case 8u:
      return;
    default:
      vm_error("Bad type to index.\n");
      return;
  }
}