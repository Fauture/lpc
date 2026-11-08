void __cdecl vm_pushShortNumber(int a1)
{
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  vm_stack[--vm_sp].type = 2;
  vm_stack[vm_sp].v.number = a1;
}