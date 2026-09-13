#include "map_module.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <mutex>

namespace lpc {

namespace {
// The original allocates ppAllMap with allMapCount=16 entries and grows it;
// we grow std::vector naturally.
std::mutex g_mapMutex;

// as_string equivalent (interpreter's as_string is in an anonymous namespace).
std::string valToString(const Value& v) {
    return to_string(v, false);
}

int64_t valToInt(const Value& v) {
    if (v.index() == 2) return std::get<int64_t>(v);
    if (v.index() == 3) return static_cast<int64_t>(std::get<double>(v));
    if (v.index() == 4) {
        try { return std::stoll(std::get<SharedString>(v)); } catch (...) { return 0; }
    }
    return 0;
}

std::string valToStr(const Value& v) {
    if (v.index() == 4) return std::get<SharedString>(v);
    if (v.index() == 10) return std::get<std::shared_ptr<BufferValue>>(v)->data;
    return "";
}
} // namespace

MapModule& mapModule() {
    static MapModule inst;
    return inst;
}

MapModule::MapInfo* MapModule::findMapIndex(int idx) {
    if (idx > 0 && idx < (int)allMap_.size() && allMap_[idx]) return allMap_[idx];
    return nullptr;
}

MapModule::MapInfo* MapModule::getMapInfoByValue(const Value& v) {
    // mapIndex is an int; the original vm_getMapInfo also accepts a name
    // string, but the game always passes the int index.
    if (v.index() == 4) {
        auto it = mapByName_.find(std::get<SharedString>(v));
        if (it == mapByName_.end()) return nullptr;
        return findMapIndex(it->second);
    }
    return findMapIndex(static_cast<int>(valToInt(v)));
}

MapModule::MapInfo* MapModule::findMap(const Value& v) {
    std::lock_guard<std::mutex> lk(g_mapMutex);
    return getMapInfoByValue(v);
}

MapModule::MapObj* MapModule::findCell(MapInfo& m, int x, int y) {
    if (x < 0 || y < 0 || x >= m.width || y >= m.height) return nullptr;
    return m.cells[(x >> 3) * m.yblocks + (y >> 3)];
}

// ---------------------------------------------------------------------------
// ID pool
// ---------------------------------------------------------------------------

void MapModule::initIdPool(int64_t size, int64_t startId) {
    std::lock_guard<std::mutex> lk(g_mapMutex);
    if (size <= 0 || startId <= 0) return;
    int64_t n = (size + 31) & ~31LL;   // round up to multiple of 32
    idPool_.assign(n / 64 + 1, 0);
    idInfo_.assign(static_cast<size_t>(n), "");
    poolSize_ = n;
    poolStartId_ = startId;
    lastFree_ = 0;
    lastBitOffset_ = 0;
}

Value MapModule::allocateId(const Value& objVal) {
    std::lock_guard<std::mutex> lk(g_mapMutex);
    if (idPool_.empty() || poolSize_ <= 0) return make_int(0);
    std::string key;
    if (objVal.index() == 7) {
        key = std::get<std::shared_ptr<ObjectValue>>(objVal)->programName;
    }
    if (key.empty()) return make_int(0);
    // scan the bitmap for a free slot
    int64_t blockCount = static_cast<int64_t>(idPool_.size());
    for (int64_t i = 0; i < blockCount; ++i) {
        int64_t bi = (lastFree_ + i) % blockCount;
        uint64_t blk = idPool_[bi];
        if (blk == ~0ULL) continue;
        // find first zero bit at/after lastBitOffset_
        int startBit = (bi == lastFree_) ? lastBitOffset_ : 0;
        for (int b = startBit; b < 64; ++b) {
            if (bi * 64 + b >= static_cast<int64_t>(poolSize_)) break;
            if (!(blk & (1ULL << b))) {
                idPool_[bi] |= (1ULL << b);
                idInfo_[bi * 64 + b] = key;
                lastFree_ = bi;
                lastBitOffset_ = b + 1;
                return make_int(poolStartId_ + bi * 64 + b);
            }
        }
        lastBitOffset_ = 0;
    }
    return make_int(0);
}

Value MapModule::freeId(const Value& idVal) {
    int64_t id = valToInt(idVal);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    if (idPool_.empty()) return make_int(0);
    if (id < poolStartId_ || id >= poolStartId_ + poolSize_) return make_int(0);
    int64_t off = id - poolStartId_;
    int64_t bi = off >> 6;
    int b = static_cast<int>(off & 63);
    if (!(idPool_[bi] & (1ULL << b))) return make_int(0);   // already free
    idPool_[bi] &= ~(1ULL << b);
    idInfo_[static_cast<size_t>(off)] = "";
    return make_int(1);
}

Value MapModule::getObjectById(const Value& idVal) {
    int64_t id = valToInt(idVal);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    if (idPool_.empty()) return make_int(0);
    if (id < poolStartId_ || id >= poolStartId_ + poolSize_) return make_int(0);
    int64_t off = id - poolStartId_;
    int64_t bi = off >> 6;
    int b = static_cast<int>(off & 63);
    if (!(idPool_[bi] & (1ULL << b))) return make_int(0);
    const std::string& key = idInfo_[static_cast<size_t>(off)];
    if (key.empty()) return make_int(0);
    auto ov = std::make_shared<ObjectValue>();
    ov->programName = key;
    return ov;
}

// ---------------------------------------------------------------------------
// map registry
// ---------------------------------------------------------------------------

Value MapModule::buildMap(const std::vector<Value>& args) {
    // build_map(mapName, mapId, width, height, obstacleBuffer)
    if (args.size() < 4) {
        static int zN = 0;
        if (zN++ < 40) std::cerr << "[bm0] ZERO-ARGS size=" << args.size() << "\n";
        return make_int(0);
    }
    std::string name = valToStr(args[0]);
    int mapId = static_cast<int>(valToInt(args[1]));
    int width = static_cast<int>(valToInt(args[2]));
    int height = static_cast<int>(valToInt(args[3]));
    std::string buf = (args.size() >= 5) ? valToStr(args[4]) : std::string();
    std::lock_guard<std::mutex> lk(g_mapMutex);
    auto it = mapByName_.find(name);
    if (it != mapByName_.end()) {
        // A map with this name is already registered: return the EXISTING
        // index.  build_map must be idempotent per name - rooms are set up
        // through TWO independent paths (roomd::setup_room at load time and
        // room::init_room_info, which some rooms/zones call themselves), and
        // room::set_map stores the result straight into the room's obj var
        // `map_index`.  Returning 0 there left the room with map_index == 0,
        // and is_blocked(0,x,y) answers -1, which makes
        // feature/common/move.c::enter_room bail out on its first check
        // (`flag < 0 -> return`) - the room became permanently unenterable,
        // i.e. "过图一次以后再也过不去".  Rooms that really are re-created
        // (clone rooms) call destruct_map(object_name()) first, so their name
        // is free again and a fresh map is built.
        static int zN = 0;
        if (zN++ < 4000)
            std::cerr << "[bm0] name-exists name=" << name << " idx=" << it->second
                      << " -> reusing\n";
        return make_int(it->second);
    }
    if (width <= 0 || height <= 0) {
        static int zN = 0;
        if (zN++ < 40 || name.find("yunxiaogong") != std::string::npos)
            std::cerr << "[bm0] bad-dims name=" << name << " " << width << "x" << height << "\n";
        return make_int(0);
    }
    auto* m = new MapInfo();
    m->name = name;
    m->mapId = mapId;
    m->width = width;
    m->height = height;
    m->xblocks = (width + 7) >> 3;
    m->yblocks = (height + 7) >> 3;
    m->cells.assign(static_cast<size_t>(m->xblocks * m->yblocks), nullptr);
    // obstacle buffer: the original build_map takes a 2-bit-per-cell packed
    // buffer (bit pairs along the x axis, one byte per 4 cells; value &1 =
    // obstacle, (value>>1)&1 = water-ish) and expands it into a per-cell
    // byte map: 1 for obstacle, 0x20 for "water" (bit 1), 0 otherwise.
    if (!buf.empty()) {
        m->bits.assign(static_cast<size_t>(width * height), 0);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // packed index is y-major (x continuous): bit = x + y*width
                size_t bit = static_cast<size_t>(y * width + x);
                size_t byte = bit >> 2;
                if (byte >= buf.size()) break;
                int sh = static_cast<int>((bit & 3) * 2);
                int v = (buf[byte] >> sh) & 3;
                // store x-major (is_blocked reads bits[x*height+y])
                m->bits[static_cast<size_t>(x * height + y)] = (v & 1) ? 1 : (((v >> 1) & 1) ? 0x20 : 0);
            }
        }
    }
    // find a free mapIndex (like the original: scan from 1)
    // Index 0 is RESERVED/invalid (findMapIndex requires idx>0): when the
    // module is still empty the scan below finds nothing and idx would fall
    // back to size()==0 - the FIRST built map (云霄宫 50000, the alphabetically
    // first zone) got index 0, its zone stored map_index=0 and every placed
    // npc's is_blocked(0,x,y) returned -1 so enter_room bailed and the map
    // stayed empty (city_war's 通灵药王@云霄宫 was never findable).  Keep a
    // dummy at slot 0 so the first real map takes index 1.
    if (allMap_.empty()) allMap_.push_back(nullptr);
    int idx = 0;
    for (int i = 1; i < (int)allMap_.size(); ++i) {
        if (!allMap_[i]) { idx = i; break; }
    }
    if (idx == 0) {
        idx = static_cast<int>(allMap_.size());
        allMap_.push_back(nullptr);
    }
    allMap_[idx] = m;
    mapByName_[name] = idx;
    return make_int(idx);
}

Value MapModule::destructMap(const Value& mapIndexVal) {
    int idx = static_cast<int>(valToInt(mapIndexVal));
    std::lock_guard<std::mutex> lk(g_mapMutex);
    if (!mapIndexValid(idx)) return make_int(0);
    MapInfo* m = allMap_[idx];
    allMap_[idx] = nullptr;
    if (m) {
        mapByName_.erase(m->name);
        delete m;
    }
    return make_int(1);
}

Value MapModule::getMapInfo(const Value& mapIndexVal) {
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(mapIndexVal);
    if (!m) return make_int(0);
    auto mp = std::make_shared<MappingValue>();
    mp->insert(make_string("map_name"), make_string(m->name));
    mp->insert(make_string("map_id"), make_int(m->mapId));
    mp->insert(make_string("map_index"), make_int(static_cast<int>(findMapIndexByPtr(m))));
    mp->insert(make_string("height"), make_int(m->height));
    mp->insert(make_string("width"), make_int(m->width));
    return mp;
}

int MapModule::findMapIndexByPtr(const MapInfo* m) const {
    for (size_t i = 1; i < allMap_.size(); ++i)
        if (allMap_[i] == m) return static_cast<int>(i);
    return 0;
}

Value MapModule::getAllMap() {
    std::lock_guard<std::mutex> lk(g_mapMutex);
    auto arr = std::make_shared<ArrayValue>();
    for (size_t i = 1; i < allMap_.size(); ++i) {
        if (!allMap_[i]) continue;
        MapInfo* m = allMap_[i];
        auto mp = std::make_shared<MappingValue>();
        mp->insert(make_string("map_name"), make_string(m->name));
        mp->insert(make_string("map_id"), make_int(m->mapId));
        mp->insert(make_string("map_index"), make_int(static_cast<int>(i)));
        mp->insert(make_string("height"), make_int(m->height));
        mp->insert(make_string("width"), make_int(m->width));
        arr->items.push_back(mp);
    }
    return arr;
}

// ---------------------------------------------------------------------------
// cells
// ---------------------------------------------------------------------------

Value MapModule::putInMap(const std::vector<Value>& args) {
    // put_in_map(mapIndex, x, y, id)
    if (args.size() < 4) return make_int(0);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_int(0);
    int x = static_cast<int>(valToInt(args[1]));
    int y = static_cast<int>(valToInt(args[2]));
    int64_t id = valToInt(args[3]);
    if (x < 0 || y < 0 || x >= m->width || y >= m->height) return make_int(0);
    // remove any existing entry for this id first (put is idempotent-ish)
    for (int cy = 0; cy < m->height; ++cy) {
        for (int cx = 0; cx < m->width; ++cx) {
            (void)cx; (void)cy;
        }
    }
    MapObj*& head = m->cells[(x >> 3) * m->yblocks + (y >> 3)];
    // if the id is already present (anywhere), remove it first
    for (size_t bi = 0; bi < m->cells.size(); ++bi) {
        MapObj** pp = &m->cells[bi];
        while (*pp) {
            if ((*pp)->id == id) { MapObj* d = *pp; *pp = d->next; delete d; break; }
            pp = &(*pp)->next;
        }
    }
    auto* ob = new MapObj();
    ob->id = id;
    ob->x = x;
    ob->y = y;
    ob->next = head;
    head = ob;
    return make_int(1);
}

Value MapModule::removeFromMap(const std::vector<Value>& args) {
    // remove_from_map(mapIndex, x, y, id)
    if (args.size() < 4) return make_int(0);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_int(0);
    int x = static_cast<int>(valToInt(args[1]));
    int y = static_cast<int>(valToInt(args[2]));
    int64_t id = valToInt(args[3]);
    if (x < 0 || y < 0 || x >= m->width || y >= m->height) return make_int(0);
    MapObj** pp = &m->cells[(x >> 3) * m->yblocks + (y >> 3)];
    while (*pp) {
        if ((*pp)->id == id && (*pp)->x == x && (*pp)->y == y) {
            MapObj* d = *pp; *pp = d->next; delete d;
            return make_int(1);
        }
        pp = &(*pp)->next;
    }
    return make_int(0);
}

Value MapModule::moveInMap(const std::vector<Value>& args) {
    // move_in_map(mapIndex, oldX, oldY, newX, newY, id)
    if (args.size() < 6) return make_int(0);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_int(0);
    int ox = static_cast<int>(valToInt(args[1]));
    int oy = static_cast<int>(valToInt(args[2]));
    int nx = static_cast<int>(valToInt(args[3]));
    int ny = static_cast<int>(valToInt(args[4]));
    int64_t id = valToInt(args[5]);
    // find in source cell
    if (ox < 0 || oy < 0 || ox >= m->width || oy >= m->height) return make_int(0);
    if (nx < 0 || ny < 0 || nx >= m->width || ny >= m->height) return make_int(0);
    MapObj** pp = &m->cells[(ox >> 3) * m->yblocks + (oy >> 3)];
    MapObj* ob = nullptr;
    while (*pp) {
        if ((*pp)->id == id && (*pp)->x == ox && (*pp)->y == oy) {
            ob = *pp; *pp = ob->next; break;
        }
        pp = &(*pp)->next;
    }
    if (!ob) return make_int(0);
    ob->x = nx;
    ob->y = ny;
    if (((ox >> 3) * m->yblocks + (oy >> 3)) != ((nx >> 3) * m->yblocks + (ny >> 3))) {
        MapObj*& head = m->cells[(nx >> 3) * m->yblocks + (ny >> 3)];
        ob->next = head;
        head = ob;
    }
    return make_int(1);
}

Value MapModule::getFromMap(const std::vector<Value>& args) {
    // get_from_map(mapIndex[, x, y])
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_array();
    auto arr = std::make_shared<ArrayValue>();
    if (args.size() >= 3) {
        // per-cell: array of ids
        int x = static_cast<int>(valToInt(args[1]));
        int y = static_cast<int>(valToInt(args[2]));
        if (x < 0 || y < 0 || x >= m->width || y >= m->height) return make_array();
        for (MapObj* ob = m->cells[(x >> 3) * m->yblocks + (y >> 3)]; ob; ob = ob->next) {
            if (ob->x == x && ob->y == y) arr->items.push_back(make_int(ob->id));
        }
    } else {
        // whole map: array of [id, x, y]
        for (int bx = 0; bx < m->xblocks; ++bx) {
            for (int by = 0; by < m->yblocks; ++by) {
                for (MapObj* ob = m->cells[bx * m->yblocks + by]; ob; ob = ob->next) {
                    auto e = std::make_shared<ArrayValue>();
                    e->items.push_back(make_int(ob->id));
                    e->items.push_back(make_int(ob->x));
                    e->items.push_back(make_int(ob->y));
                    arr->items.push_back(e);
                }
            }
        }
    }
    return arr;
}

Value MapModule::getInSight(const std::vector<Value>& args) {
    // get_in_sight(mapIndex, x, y): ids in the 9x7 block window
    if (args.size() < 3) return make_array();
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_array();
    int x = static_cast<int>(valToInt(args[1]));
    int y = static_cast<int>(valToInt(args[2]));
    if (x < 0 || y < 0 || x >= m->width || y >= m->height) return make_array();
    int xb1 = (x >> 3) - 4; if (xb1 < 0) xb1 = 0;
    int xb2 = (x >> 3) + 4; if (xb2 >= m->xblocks) xb2 = m->xblocks - 1;
    int yb1 = (y >> 3) - 3; if (yb1 < 0) yb1 = 0;
    int yb2 = (y >> 3) + 3; if (yb2 >= m->yblocks) yb2 = m->yblocks - 1;
    auto arr = std::make_shared<ArrayValue>();
    for (int bx = xb1; bx <= xb2; ++bx) {
        for (int by = yb1; by <= yb2; ++by) {
            for (MapObj* ob = m->cells[bx * m->yblocks + by]; ob; ob = ob->next)
                arr->items.push_back(make_int(ob->id));
        }
    }
    return arr;
}

Value MapModule::getIdInMap(const std::vector<Value>& args) {
    // get_id_in_map(id[, mapIndex]): list of maps (or that map) containing id
    if (args.empty()) return make_array();
    int64_t id = valToInt(args[0]);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    auto arr = std::make_shared<ArrayValue>();
    if (args.size() >= 2) {
        MapInfo* m = getMapInfoByValue(args[1]);
        if (!m) return make_array();
        for (size_t bi = 0; bi < m->cells.size(); ++bi) {
            for (MapObj* ob = m->cells[bi]; ob; ob = ob->next) {
                if (ob->id == id) arr->items.push_back(make_int(ob->id));
            }
        }
        return arr;
    }
    for (size_t i = 1; i < allMap_.size(); ++i) {
        if (!allMap_[i]) continue;
        MapInfo* m = allMap_[i];
        for (size_t bi = 0; bi < m->cells.size(); ++bi) {
            for (MapObj* ob = m->cells[bi]; ob; ob = ob->next) {
                if (ob->id == id) arr->items.push_back(make_int(static_cast<int>(i)));
            }
        }
    }
    return arr;
}

Value MapModule::getAllObjects() {
    std::lock_guard<std::mutex> lk(g_mapMutex);
    auto arr = std::make_shared<ArrayValue>();
    for (int64_t off = 0; off < poolSize_; ++off) {
        int64_t bi = off >> 6;
        int b = static_cast<int>(off & 63);
        if (bi < (int64_t)idPool_.size() && (idPool_[bi] & (1ULL << b)))
            arr->items.push_back(make_int(poolStartId_ + off));
    }
    return arr;
}

// ---------------------------------------------------------------------------
// queries
// ---------------------------------------------------------------------------

Value MapModule::isBlocked(const std::vector<Value>& args) {
    // is_blocked(mapIndex, x, y) -> bits & 0x21, or -1 out of range
    if (args.size() < 3) return make_int(-1);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_int(-1);
    int x = static_cast<int>(valToInt(args[1]));
    int y = static_cast<int>(valToInt(args[2]));
    if (x < 0 || y < 0 || x >= m->width || y >= m->height) return make_int(-1);
    if (m->bits.empty()) return make_int(0);
    return make_int(m->bits[static_cast<size_t>(x * m->height + y)] & 0x21);
}

Value MapModule::readMapInfo(const std::vector<Value>& args) {
    // read_map_info(buffer): parse a server .map buffer -> (["width","height","obstacle"])
    // The server .map format (vm_f_read_map_info, buffer branch): 12-byte
    // header (map_id u32, width u32, height u32, each byte-swapped) followed by
    // the raw obstacle data.  The original copies the ENTIRE tail (size-12)
    // into the returned "obstacle" buffer WITHOUT unpacking; build_map then
    // interprets it as 2-bit-per-cell packed data (bit pair per cell, x-major,
    // one byte per 4 cells).  We replicate that: return the packed tail and let
    // buildMap expand it.
    if (args.empty()) return make_int(0);
    std::string buf = valToStr(args[0]);
    if (buf.size() < 12) return make_int(0);
    uint32_t w = 0, h = 0;
    std::memcpy(&w, buf.data() + 4, 4);
    std::memcpy(&h, buf.data() + 8, 4);
    // the original byte-swaps the width/height words (ROR16 on the low word
    // after ROR8) - equivalent to a big-endian read
    w = ((w & 0xFF) << 24) | ((w & 0xFF00) << 8) | ((w >> 8) & 0xFF00) | ((w >> 24) & 0xFF);
    h = ((h & 0xFF) << 24) | ((h & 0xFF00) << 8) | ((h >> 8) & 0xFF00) | ((h >> 24) & 0xFF);
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384) return make_int(0);
    auto mp = std::make_shared<MappingValue>();
    mp->insert(make_string("width"), make_int(w));
    mp->insert(make_string("height"), make_int(h));
    std::string obs = buf.substr(12);
    mp->insert(make_string("obstacle"), make_buffer(obs));
    return mp;
}

Value MapModule::calcOffset(const std::vector<Value>& args) {
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
}

// ---------------------------------------------------------------------------
// check_steps(mapIndex, x, y, steps, flag, move_type)
// move_v37.multi_move_to validates a client movement path before applying it:
// `steps` is a flat array of consecutive {x,y} cells (client pathfinding),
// the last pair is the destination.  Each cell must be inside the map and
// adjacent (chebyshev distance 1) to the previous one, and must be walkable
// (obstacle bits 0x21 clear) unless move_type == 1 (fly, which ignores the
// obstacle plane).  Returns 1 when the whole path is legal, else 0.  Left as
// a stub returning 0 this rejected EVERY move (the player could not walk).
// ---------------------------------------------------------------------------
Value MapModule::checkSteps(const std::vector<Value>& args) {
    if (args.size() < 4) return make_int(0);
    std::lock_guard<std::mutex> lk(g_mapMutex);
    MapInfo* m = getMapInfoByValue(args[0]);
    if (!m) return make_int(0);
    int x = static_cast<int>(valToInt(args[1]));
    int y = static_cast<int>(valToInt(args[2]));
    if (args[3].index() != 5) return make_int(1);   // no path data: allow
    const auto& items = std::get<std::shared_ptr<ArrayValue>>(args[3])->items;
    int moveType = (args.size() >= 6) ? static_cast<int>(valToInt(args[5])) : 0;
    bool fly = (moveType == 1);
    size_t n = items.size() / 2;
    for (size_t i = 0; i < n; ++i) {
        int nx = static_cast<int>(valToInt(items[i * 2]));
        int ny = static_cast<int>(valToInt(items[i * 2 + 1]));
        if (nx < 0 || ny < 0 || nx >= m->width || ny >= m->height) return make_int(0);
        int dx = nx - x, dy = ny - y;
        if (dx < -1 || dx > 1 || dy < -1 || dy > 1) return make_int(0);
        // A path may repeat the current cell (NPC auto-walk paths built by
        // inspect_cityd/auto_inspectd pass move_steps[0..<2] whose first point
        // is often the NPC's current cell) - that is not a failure.
        if (!fly && !m->bits.empty() &&
            (m->bits[static_cast<size_t>(nx * m->height + ny)] & 0x21) != 0)
            return make_int(0);
        x = nx; y = ny;
    }
    return make_int(1);
}

bool MapModule::pointInPoly(int x, int y, const std::vector<int>& poly) {
    // Even-odd rule over the vertex list (flat {x1,y1,x2,y2,...}).
    size_t n = poly.size() / 2;
    if (n < 3) return false;
    bool inside = false;
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        int xi = poly[i * 2], yi = poly[i * 2 + 1];
        int xj = poly[j * 2], yj = poly[j * 2 + 1];
        if (((yi > y) != (yj > y)) &&
            (x < static_cast<double>(xj - xi) * (y - yi) / static_cast<double>(yj - yi) + xi))
            inside = !inside;
    }
    return inside;
}

// vm_f_get_in_polygens (0x807b148): get_in_polygens(mapIndex, poly) returns
// the ids of all objects on the map whose cell lies inside the polygon.  The
// original clamps the polygon coordinates to the map size, computes the
// bounding block range and walks every cell in it.
Value MapModule::getInPolygens(const std::vector<Value>& args) {
    auto out = std::make_shared<ArrayValue>();
    if (args.size() < 2 || args[1].index() != 5) return out;
    MapInfo* m = findMapIndex(static_cast<int>(valToInt(args[0])));
    if (!m) return out;
    const auto& items = std::get<std::shared_ptr<ArrayValue>>(args[1])->items;
    std::vector<int> poly;
    poly.reserve(items.size());
    for (const auto& v : items) poly.push_back(static_cast<int>(valToInt(v)));
    if (poly.size() < 6 || (poly.size() & 1)) return out;
    int minX = m->width, maxX = 0, minY = m->height, maxY = 0;
    for (size_t i = 0; i + 1 < poly.size(); i += 2) {
        int x = poly[i], y = poly[i + 1];
        if (x < 0) x = 0;
        if (x > m->width) x = m->width;
        if (y < 0) y = 0;
        if (y > m->height) y = m->height;
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }
    int bx0 = minX / 8, bx1 = maxX / 8, by0 = minY / 8, by1 = maxY / 8;
    if (bx0 < 0) bx0 = 0;
    if (by0 < 0) by0 = 0;
    if (bx1 > m->xblocks - 1) bx1 = m->xblocks - 1;
    if (by1 > m->yblocks - 1) by1 = m->yblocks - 1;
    for (int bx = bx0; bx <= bx1; ++bx) {
        for (int by = by0; by <= by1; ++by) {
            for (MapObj* o = m->cells[bx * m->yblocks + by]; o; o = o->next) {
                if (pointInPoly(o->x, o->y, poly)) out->items.push_back(make_int(o->id));
            }
        }
    }
    return out;
}

// ---- map event hooks ----
namespace {
std::mutex g_hookMutex;
std::vector<MapModule::MapHook> g_hooks;
std::atomic<int64_t> g_hookSeq{1};
}  // namespace

int64_t MapModule::hookAdd(int mapIndex, std::vector<int> poly, int64_t idFilter, const Value& closure) {
    std::lock_guard<std::mutex> lk(g_hookMutex);
    MapHook h;
    h.eventId = g_hookSeq.fetch_add(1);
    h.mapIndex = mapIndex;
    h.poly = std::move(poly);
    h.idFilter = idFilter;
    h.closure = closure;
    g_hooks.push_back(std::move(h));
    return g_hooks.back().eventId;
}

void MapModule::hookRemove(int64_t eventId) {
    std::lock_guard<std::mutex> lk(g_hookMutex);
    for (auto it = g_hooks.begin(); it != g_hooks.end(); ++it) {
        if (it->eventId == eventId) { g_hooks.erase(it); return; }
    }
}

std::vector<MapModule::MapHook> MapModule::hookSnapshot(int mapIndex) const {
    std::lock_guard<std::mutex> lk(g_hookMutex);
    std::vector<MapHook> out;
    for (const auto& h : g_hooks)
        if (h.mapIndex == mapIndex) out.push_back(h);
    return out;
}

} // namespace lpc
