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
    Value getIdInMap(const std::vector<Value>& args);
    Value getAllObjects();

    // ---- queries ----
    Value is_blocked(const std::vector<Value>& args) { return isBlocked(args); }
    Value isBlocked(const std::vector<Value>& args);
    Value readMapInfo(const std::vector<Value>& args);
    Value calcOffset(const std::vector<Value>& args);

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
