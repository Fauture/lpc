void vm_pushThisObject()
{
  if ( vm_currentContext->thisObject )
    vm_pushObject(&vm_currentContext->thisObject->ref);
  else
    vm_pushNumber(0);
}