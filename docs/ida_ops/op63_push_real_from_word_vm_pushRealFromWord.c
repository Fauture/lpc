void __cdecl vm_pushRealFromWord(int firstWord, int secondWord)
{
  if ( !vm_sp )
    vm_error("Stack overflow when calculating.\n");
  vm_stack[--vm_sp].type = 3;
  vm_stack[vm_sp].v.number = firstWord;
}