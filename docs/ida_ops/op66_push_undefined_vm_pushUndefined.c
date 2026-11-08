void vm_pushUndefined()
{
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  vm_stack[--vm_sp].type = 0;
  vm_stack[vm_sp].v.number = 0;
}