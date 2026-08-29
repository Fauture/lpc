# -*- coding: utf-8 -*-
"""卸载原始 lunzhuanyu 对象,加载服务器内新的 /lunzhuanyu.c 并调用 apply()。"""
import random
import string
from gameshell_rpc import GameShellRpcClient


def rand_var(prefix='v'):
    return prefix + '_' + ''.join(random.choices(string.ascii_lowercase + string.digits, k=8))


def main():
    rpc = GameShellRpcClient(host='192.168.200.128', port=1024, password='123456', timeout=20.0)
    print('[*] 连接 192.168.200.128:1024 ...')
    rpc.connect()
    print('[+] 已认证, 进入 Shell>')

    # 1. 查找并卸载所有 lunzhuanyu 对象(根目录 + pak 内)
    print('\n========== 1. 卸载原始 lunzhuanyu 对象 ==========')
    paths_to_unload = [
        '/lunzhuanyu.c',
        '/gs/clone/item/hell/lunzhuanyu.c',
        '/gs/pack_data/lib_gs32.pak/gs/clone/item/hell/lunzhuanyu.c',
    ]
    for p in paths_to_unload:
        v_obj = rand_var('old')
        out = rpc.call(
            r'object ' + v_obj + r' = find_object("' + p + r'");'
            r'if (is_object(' + v_obj + r')) { destruct_object(' + v_obj + r'); write("destructed: ' + p + r'\\n"); }'
            r'else { write("not loaded: ' + p + r'\\n"); }',
            wait=5.0
        )
        print(out)

    # 2. 确认全部已卸载
    print('\n========== 2. 确认已卸载 ==========')
    for p in paths_to_unload:
        v_chk = rand_var('chk')
        out = rpc.call(
            r'object ' + v_chk + r' = find_object("' + p + r'");'
            r'write("' + p + r' = "); write(' + v_chk + r'); write("\\n");',
            wait=3.0
        )
        print(out)

    # 3. 加载服务器内新的 lunzhuanyu.c (根目录)
    print('\n========== 3. 加载服务器内新的 /lunzhuanyu.c ==========')
    v_new = rand_var('new')
    out = rpc.call(
        r'object ' + v_new + r' = load_object("/gs/clone/item/hell/lunzhuanyu.c");'
        r'if (is_object(' + v_new + r')) { write("loaded new ok\\n"); }'
        r'else { write("load FAIL\\n"); }',
        wait=15.0
    )
    print(out)

    # 4. 调用 apply()
    print('\n========== 4. 调用 apply() ==========')
    v_ret = rand_var('ret')
    out = rpc.call(
        r'int ' + v_ret + r' = ' + v_new + r'->apply();'
        r'write("apply() = "); write(' + v_ret + r'); write("\\n");',
        wait=5.0
    )
    print(out)

    # 5. 调用其他函数
    print('\n========== 5. 调用其他函数 ==========')
    v_r2 = rand_var('r2')
    out = rpc.call(
        r'int ' + v_r2 + r' = ' + v_new + r'->try_to_cost_amount();'
        r'write("try_to_cost_amount = "); write(' + v_r2 + r'); write("\\n");',
        wait=5.0
    )
    print(out)

    # 6. 查看对象信息
    print('\n========== 6. 对象信息 ==========')
    out = rpc.call(
        r'write("obj="); write(' + v_new + r'); write("\\n");'
        r'write("name="); write(' + v_new + r'->get_name()); write("\\n");',
        wait=5.0
    )
    print(out)

    rpc.close()
    print('\n[*] 已断开')


if __name__ == '__main__':
    main()
