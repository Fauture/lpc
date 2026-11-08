void vm_pushArgNum()
{
  int v0; // ebx

  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  v0 = vm_argumentInThisContext();
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  vm_stack[--vm_sp].type = 2;
  vm_stack[vm_sp].v.number = v0;
}