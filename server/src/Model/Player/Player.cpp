#include "../../../includes/Model/Player/Player.h"

#include "../../../includes/Model/Item/Weapon/Weapon.h"

#define MAX_AMMO 1000
#define MAX_HEALTH 100

Player::Player(unsigned int hp, unsigned int lifes)
    : health(hp), lifeRemaining(lifes), ammo(0), key(false), score(0) {}

int Player::handleDeath() {
  if (this->lifeRemaining == 0) {
    this->health = 0;
    return -1;  // El jugador ya no puede respawnear.
  }

  this->lifeRemaining -= 1;
  this->health = 100;  // Deberia restaurar la vida al maximo.
  return 0;            // Devuelvo valor indicando que mi vida quedo en 0.
}
int Player::takeDamage(unsigned int damage) {
  if (damage >= this->health) {
    return handleDeath();
  }

  this->health -= damage;
  return this->health;
}

int Player::attack() {
  // Deberia pedirle a su arma que ataque, devolviendo el daño que hizo.
  // Luego le pregunto al arma cuantas balas tiene, si no tiene mas disponible,
  // cambio a cuchillo
}

void Player::equipWeapon(Item* weapon) { this->weapon = (Weapon*)weapon; }

void Player::pickupKey() { this->key = true; }
bool Player::hasKey() { return key; }

bool Player::hasMaxAmmo() { return ammo < MAX_AMMO; }
void Player::pickUpAmmo() {
  ammo += 5;
  if (ammo > 1000) ammo = 1000;
}

int Player::getHealth() { return this->health; }
bool Player::hasFullHealth() { return this->health == 100; }
void Player::addHealth(int health) {
  this->health += health;
  if (this->health > 100) this->health = 100;
}

void Player::addPoints(int points) { this->score += points; }
int Player::getScore() { return this->score; }