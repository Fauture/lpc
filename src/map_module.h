#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "value.h"

namespace lpc {

// Faithful re-implementation of the original driver's map module
// (vm_efun_map.c) + ID pool (vm_efun_id.c):
//
//   init_id_pool(poolSize, startId)   create the ID pool (bitmap + info table)
//   allocate_id(object)               allocate an id (poolStartId..) for obj
//   free_id(id)                       release the id back to the pool
//   get_object_by_id(id)              object value registered under the id
//   build_map(name, mapId, w, h, buf) create a map, return its mapIndex
//   put_in_map(mapIndex, x, y, id)    add an object id at (x,y)
//   remove_from_map(mapIndex,x,y,id)  remove it
//   move_in_map(mapIndex,ox,oy,nx,ny,id)  relocate it
//   get_from_map(mapIndex[,x,y])      whole-map [id,x,y] list or per-cell id list
//   get_id_in_map(id[,mapIndex])      where a given id lives
//   get_in_sight(mapIndex,x,y)        ids in the 9x7 block window
//   get_all_map()                     list of map info mappings
//   get_map_info(mapIndex)            single map info mapping
//   get_all_objects()                 all live object ids
//   read_map_info(buffer)             parse a server .map buffer -> info mapping
//   destruct_map(mapIndex)            release a map
//   is_blocked(mapIndex,x,y)          obstacle bit (0x21 mask), -1 out of range
//   calc_offset/check_steps/find_reachable/get_map_plot/... (see below)
//
// The ID pool stores the *object key* (programName of the ObjectValue, e.g.
// "clone:gs/clone/npc/basic.o#5608") so get_object_by_id can rebuild an
// ObjectValue that call_other resolves through the object registry.
class MapModule {
public:
    struct MapObj {           // one entry in a map cell's linked list
        int64_t id = 0;
        int x = 0;
        int y = 0;
        MapObj* next = nullptr;
    };
    struct MapInfo {
        std::string name;
        int mapId = 0;
        int width = 0;
        int height = 0;
        std::vector<uint8_t> bits;   // obstacle map, width*height bytes
        int xblocks = 0;             // (width+7)/8
        int yblocks = 0;             // (height+7)/8
        std::vector<MapObj*> cells;  // yblocks*xblocks cell heads
        ~MapInfo() {
            for (auto* c : cells) {
                while (c) { MapObj* n = c->next; delete c; c = n; }
            }
        }
    };

    // ---- ID pool ----
    void initIdPool(int64_t size, int64_t startId);
    Value allocateId(const Value& objVal);
    Value freeId(const Value& idVal);
    Value getObjectById(const Value& idVal);

    // ---- map registry ----
    Value buildMap(const std::vector<Value>& args);
    Value destructMap(const Value& mapIndexVal);
    Value getMapInfo(const Value& mapIndexVal);
    Value getAllMap();

    // ---- cells ----
    Value putInMap(const std::vector<Value>& args);
    Value removeFromMap(const std::vector<Value>& args);
    Value moveInMap(const std::vector<Value>& args);
    Value getFromMap(const std::vector<Value>& args);
    Value getInSight(const std::vector<Value>& args);
    // get_sight() for the calling object: reverse-resolve its allocated id,
    // locate its (map,x,y), then the same 9x7 block window as get_in_sight.
    // The engine primitive had no implementation, so move.c::enter computed
    // an empty sight list and no NPC/player appearance was ever sent.
    Value getSightByKey(const std::string& objKey);
    // new_view_point(map, ox, oy, nx, ny, &app, &dis): the sight-window
    // diff between the old and the new position — app = ids newly visible,
    // dis = ids no longer visible.  move_to gates every walking appear /
    // disappear notification on its result; the constant-0 stub meant
    // walking never revealed NPCs entering the view.
    Value newViewPoint(const std::vector<Value>& args);
    Value getIdInMap(const std::vector<Value>& args);
    Value getAllObjects();

    // ---- queries ----
    Value is_blocked(const std::vector<Value>& args) { return isBlocked(args); }
    Value isBlocked(const std::vector<Value>& args);
    Value readMapInfo(const std::vector<Value>& args);
    Value calcOffset(const std::vector<Value>& args);

    // check_steps(mapIndex, x, y, steps, flag, move_type): validate a movement
    // path (flat {x,y,...} pairs, consecutive cells must be adjacent and
    // walkable unless move_type == 1 (fly)).  Returns 1 valid / 0 invalid.
    Value checkSteps(const std::vector<Value>& args);
    // is_pt_in_poly(x, y, poly) - even-odd point-in-polygon test.
    static bool pointInPoly(int x, int y, const std::vector<int>& poly);
    // get_in_polygens(mapIndex, poly) - vm_f_get_in_polygens (0x807b148):
    // array of the ids of every object on that map whose (x,y) lies inside
    // the polygon (polygon coordinates are clamped to the map size).
    Value getInPolygens(const std::vector<Value>& args);

    // ---- map event hooks (driver hook_map_event / unhook_map_event) ----
    // An LPC hook is (mapIndex, polygon, idFilter, closure); the driver fires
    // closure(bound..., id, x, y, flag) when an object enters (flag=1) or
    // leaves (flag=0) the polygon on that map.
    struct MapHook {
        int64_t eventId = 0;
        int mapIndex = 0;
        std::vector<int> poly;
        int64_t idFilter = 0;
        Value closure;
    };
    int64_t hookAdd(int mapIndex, std::vector<int> poly, int64_t idFilter, const Value& closure);
    void hookRemove(int64_t eventId);
    std::vector<MapHook> hookSnapshot(int mapIndex) const;

    // helpers used by the LPC layer
    MapInfo* findMap(const Value& v);
    MapInfo* findMapIndex(int idx);
    int findMapIndexByPtr(const MapInfo* m) const;
    bool mapIndexValid(int idx) const { return idx > 0 && idx < (int)allMap_.size() && allMap_[idx]; }

private:
    MapInfo* getMapInfoByValue(const Value& v);
    MapObj* findCell(MapInfo& m, int x, int y);

    std::vector<MapInfo*> allMap_;          // ppAllMap, index 0 unused
    std::unordered_map<std::string, int> mapByName_;
    int allMapCount_ = 16;

    // ID pool: bitmap of free slots + object-key table
    std::vector<uint64_t> idPool_;
    std::vector<std::string> idInfo_;       // id slot -> object key
    int64_t poolSize_ = 0;
    int64_t poolStartId_ = 0;
    int64_t lastFree_ = 0;
    int lastBitOffset_ = 0;
};

MapModule& mapModule();

} // namespace lpc
