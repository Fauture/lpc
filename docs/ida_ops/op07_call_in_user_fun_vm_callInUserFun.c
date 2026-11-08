void __fastcall vm_callInUserFun(Vm_Index_T funOffset, int isGlobalTypeArgn)
{
  Vm_Object_T *v2; // esi
  Vm_Fun_T *v3; // edi
  __int16 v4; // cx
  Vm_Object_T *thisObject; // esi
  Vm_Index_T v6; // [esp+20h] [ebp+8h]
  unsigned int v7; // [esp+24h] [ebp+Ch]

  v3 = (Vm_Fun_T *)(v7 & 0x3F);
  if ( ((v7 >> 7) & 1) != 0 )
  {
    if ( v6 < 0 || vm_pGlobalProgram->maxFunctionNo <= v6 )
      vm_error("Global function (index = %d) is not existed!\n", v6);
    vm_callInUserFunBody((v7 >> 7) & 1, v2, 0, v3, (int)vm_pGlobalProgram->pFunctions[v6].pFun);
  }
  else
  {
    v4 = vm_currentContext->consititution->baseFunOffset + v6;
    if ( v4 < 0 )
      vm_error(
        "Function (index = %d) in %s is not existed!\n",
        v4,
        vm_currentContext->thisObject->pProgram->strName->ptr);
    thisObject = (Vm_Object_T *)vm_currentContext->thisObject;
    if ( thisObject->pProgram->maxFunctionNo <= v4 )
      vm_error("Function (index = %d) in %s is not existed!\n", v4, thisObject->pProgram->strName->ptr);
    vm_callInUserFunBody(0, thisObject, 0, v3, (int)vm_currentContext->thisObject->pProgram->pFunctions[v4].pFun);
  }
}