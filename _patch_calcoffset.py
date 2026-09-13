# -*- coding: utf-8 -*-
"""calc_offset(x1,y1,x2,y2) = 两点距离; 3-arg 形式仍是 (mapIndex,x,y)->位图下标。

LPC 里两种用法并存:
  move.c/enter_room.c/…: calc_offset(my_x,my_y,i_x,i_y)  与距离比较 (!= -1)
  plot/map 代码:         calc_offset(mapIndex, x, y)      取障碍位图下标
原实现只做了 3 参形式, 4 参会把 x1 当 mapIndex -> getMapInfoByValue 失败 -> 恒返回 -1,
于是 is_near 的 `calc_offset(...) > distance` 永远为假 => 距离判定彻底失效
(npc4 探针实录: (86,158)->(48,161) = -1)。
"""
import io

P = 'src/map_module.cpp'
d = io.open(P, 'rb').read()
CRLF = b'\r\n' in d


def B(s):
    return s.replace('\n', '\r\n' if CRLF else '\n').encode('ascii')


OLD = B("""Value MapModule::calcOffset(const std::vector<Value>& args) {
    // calc_offset(mapIndex, x, y) -> offset into bits (x*height+y); used by
    // the LPC layer for plot manipulation.
    if (args.size() < 3) return make_int(-1);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_int(-1);
    int x = static_cast<int>(valToInt(args[1]));
    int y = static_cast<int>(valToInt(args[2]));
    if (x < 0 || y < 0 || x >= m->width || y >= m->height) return make_int(-1);
    return make_int(x * m->height + y);
}""")

NEW = B("""Value MapModule::calcOffset(const std::vector<Value>& args) {
    // Two overloads share this efun name in the LPC layer:
    //   calc_offset(x1, y1, x2, y2)   -> distance between two cells
    //        (move.c::is_near `if (calc_offset(my_x,my_y,i_x,i_y) > distance)
    //         return 0;`, enter_room.c's nearest-exit search, shift.c, ...)
    //   calc_offset(mapIndex, x, y)   -> offset into the obstacle bits
    //        (x*height+y), used by the plot/map code.
    // Only the 3-arg form used to be implemented: the 4-arg call took x1 as a
    // map index, getMapInfoByValue failed and every call returned -1, so
    // `is_near`'s distance test was ALWAYS false -> the range check silently
    // did nothing (probe: [near-off] (86,158)->(48,161) = -1).
    if (args.size() >= 4) {
        int x1 = static_cast<int>(valToInt(args[0]));
        int y1 = static_cast<int>(valToInt(args[1]));
        int x2 = static_cast<int>(valToInt(args[2]));
        int y2 = static_cast<int>(valToInt(args[3]));
        int dx = x1 > x2 ? x1 - x2 : x2 - x1;
        int dy = y1 > y2 ? y1 - y2 : y2 - y1;
        // Chebyshev: "N cells away" on an 8-direction tile grid.  Matches the
        // engine's own adjacency test in checkSteps (dx,dy within 1).
        return make_int(dx > dy ? dx : dy);
    }
    if (args.size() < 3) return make_int(-1);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_int(-1);
    int x = static_cast<int>(valToInt(args[1]));
    int y = static_cast<int>(valToInt(args[2]));
    if (x < 0 || y < 0 || x >= m->width || y >= m->height) return make_int(-1);
    return make_int(x * m->height + y);
}""")
assert d.count(OLD) == 1, 'anchor not found (%d)' % d.count(OLD)
d = d.replace(OLD, NEW)
io.open(P, 'wb').write(d)
print('patched; CRLF' if CRLF else 'patched; LF')
