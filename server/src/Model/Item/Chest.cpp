#include "../../../includes/Model/Item/Chest.h"
#include <iostream>

Chest::Chest(int itemID, int id, int pointsSumWhenPickedUp) : Item(itemID, id) {
  this->pointsSumWhenPickedUp = pointsSumWhenPickedUp;
}

Chest::~Chest() {}

bool Chest::canBePickedUpBy(Player* p) { return true; }

void Chest::pickUp(Player* p) {
  std::cout << "Player picking up chest" << std::endl;
   p->addPoints(this->pointsSumWhenPickedUp); }
