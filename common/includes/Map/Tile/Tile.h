#include <vector>

#include "../../../../server/includes/Model/Item/ItemDrop/ItemDrop.h"

class Tile {
 private:
  int x;
  int y;
  std::vector<ItemDrop> drops;

 public:
  Tile(int x, int y);
  void addDrop(ItemDrop itemDrop);
  std::vector<ItemDrop> getItemDrops();
};