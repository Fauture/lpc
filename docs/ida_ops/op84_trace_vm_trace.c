void __cdecl vm_trace(int nArgNum)
{
  int v1; // esi
  unsigned __int8 type; // al
  char *v3; // eax
  char *v4; // edi
  int v5; // ebx
  char *name; // eax
  int v7; // ebx
  unsigned int v8; // eax
  Vm_PCB_T *v9; // edx
  char *prefix; // eax
  Vm_Index_T v11; // ax
  char **info; // [esp+8h] [ebp-A0h]
  Vm_Value_T_1 *pValue; // [esp+Ch] [ebp-9Ch]
  char indent[152]; // [esp+10h] [ebp-98h] BYREF

  pValue = vm_stackTopValue();
  type = pValue->type;
  if ( type != 4 )
  {
    v3 = vm_valueType(type);
    vm_error("Bad type of trace module, expected string got %s.\n", v3);
  }
  v4 = *(char **)(pValue->v.number + 12);
  info = (char **)pValue->v.info;
  if ( !v4 )
  {
LABEL_24:
    v7 = -1;
LABEL_25:
    vm_enableTraceModule(info[3], 1, "", "");
    goto LABEL_10;
  }
  v1 = 0;
  v5 = 0;
  while ( 1 )
  {
    name = traceModules[v5].name;
    if ( name )
    {
      if ( !strcmp(name, v4) )
        break;
    }
    ++v1;
    ++v5;
    if ( v1 > 255 )
      goto LABEL_24;
  }
  v7 = v1;
  if ( v1 < 0 )
    goto LABEL_25;
  if ( !traceModules[v1].level )
  {
    vm_popNStack((Vm_Value_T_2 *)v4, nArgNum - 1);
    return;
  }
LABEL_10:
  if ( enableTraceLevel )
  {
    v8 = 0;
    v9 = vm_funCp;
    if ( (int)vm_funCp > 0 )
    {
      do
        indent[v8++] = 46;
      while ( (int)v8 < (int)v9 && v8 <= 0x7E );
    }
    indent[v8] = 0;
  }
  else
  {
    indent[0] = 0;
  }
  prefix = "\a" + 1;
  if ( v7 >= 0 )
    prefix = traceModules[v7].prefix;
  vm_printf("%s[%s] %s", indent, *(const char **)(pValue->v.number + 12), prefix);
  vm_popStack((Vm_Buffer_T *)v7, (Vm_String_T *)v1);
  v11 = efunNoWrite;
  if ( nArgNum > 2 )
    v11 = efunNoPrintf;
  vm_callInEfun(v11, nArgNum - 1);
  if ( v7 >= 0 )
    vm_printf("%s", traceModules[v7].suffix);
}