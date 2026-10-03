void vm_stopRpcServer()
{
  if ( rpcFd != -1 )
  {
    vm_stdout("RPC server is stopped.\n");
    vm_removeSocketFromScan(rpcFd);
    vm_closeSocket(rpcFd);
    rpcFd = -1;
  }
}