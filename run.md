测试环境:MySQL 192.168.200.128:3306 root/123456;
aaa 表在 dl_adb_all_test(port=8001);dba 表 port=8120; 
ddb 表在 dl_ddb_1_test 注意dl_ddb_1为参考表不得修改
游戏服务端口由 DB 表决定,driver telnet 由 -p 决定,不得冲突。telnet 可以不开启
aaa 测试:orig linux\aaa\pack_data\lib_aaa32.pak -p 8101 -w 123456;dba 测试:orig linux\dba\pack_data\lib_dba32.pak -p 9120 -w 123456。
服务端启动顺序为 aaa  dba  ccs  gs ,注意 aaa dba ccs 任意一个不启动gs就无法真正启动
注意:当前不缺少任何配置和配置数据,部分数据是由启动服务端后定时/自动写入的,不要妄图修改任何配置文件

反编译工具 python tools/lpc_decompiler_v37.py <o文件> -m disasm -o out.txt -u(对象在 linux<svc>\pack_data\lib_<svc>32... 解包树,aaa 的 daemon 在 lib_aaa32\aaa\daemons);
lpc_decompiler_v37.py
lpc_decompiler_restore_vm_o_v2
\linux\目录包含了已经解包的o文件已经反编译出的伪代码的c文件
aaa\pack_data\lib_aaa32 o文件目录
aaa\pack_data\lib_aaa32-c c文件目录
以上类推

//当前aaa可用
tools内有telnet连接工具,可以直接连接原版服务端进行函数测试 1024 aaa 1025 dba 1026 ccs 1027 gs

注意不知道的功能一定要IDA MCP 查原版语义,不要胡乱思考
请勿使用多线程,原版为单线程,多线程会导致数据无法共享一定要按照原版语义执行代码

构建:"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\lpcvm.sln -p:Configuration=Debug -m /v:q /nologo(MSBuild 不在 PATH);编辑文件前必须杀 lpcvm/cl/MSBuild/link(否则 EIO)。
调试手段:LPCVM_TRACE/LPCVM_TRACE_FN/LPCVM_LOCK_TRACE 门控日志;

还有很多功能尚未实现,当运行一直处于循环时根据日志判断当前执行什么的导致无法继续的
原版运行日志在linux目录下后缀log文件
注意,如果有任何和原版日志不同的地方就尝试修复,除了DAS此服务为验证服务无需还原

当前服务端卡在gs无法继续运行,检查gs服务端为何卡死不再继续
注意可以启动全量日志检查



