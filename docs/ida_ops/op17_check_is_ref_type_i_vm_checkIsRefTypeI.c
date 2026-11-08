void __usercall vm_checkIsRefTypeI(Vm_String_T *a1@<esi>)
{
  Vm_Value_T_2 *ValueFromStack; // ebx
  unsigned __int8 type; // al
  Vm_Buffer_T *v3; // ebx

  ValueFromStack = vm_getValueFromStack(0, 255);
  type = ValueFromStack->type;
  if ( type == 3 )
  {
    ValueFromStack->v.number = vm_ftol(ValueFromStack->v.real);
    ValueFromStack->type = 2;
  }
  else if ( type != 2 )
  {
    vm_error("Bad index for argument.\n");
  }
  v3 = (Vm_Buffer_T *)(ValueFromStack->v.number - 1);
  vm_cleanupValue(v3, a1, &vm_stack[vm_sp]);
  ++vm_sp;
  vm_checkIsRefType((int)v3);
}