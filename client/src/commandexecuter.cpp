#include "../includes/commandexecuter.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "../../common/includes/Map/Map.h"
#include "../../common/includes/PlayerData.h"
#include "../../common/includes/Socket/SocketWrapper.h"
#include "../../common/includes/protocol.h"
#include "../includes/RaycastedAnimation.h"
#include "../includes/clientprotocol.h"
#include "../includes/scoreboard.h"
#include "../includes/texturemanager.h"

#define MUSIC_PATH "../audio/music.mp3"

CommandExecuter::CommandExecuter(SocketCommunication& s,
                                 std::atomic<bool>& alive,
                                 DrawableVector& sprites,
                                 std::map<uint32_t, Player*>& players,
                                 int selfId, AudioManager& audiomanager,
                                 Map& matrix, ClientMapLoader& loader,
                                 ScoreBoard* scoreboard)
    : CommandManager(scoreboard, s, alive),
      sprites(sprites),
      players(players),
      selfId(selfId),
      audiomanager(audiomanager),
      matrix(matrix),
      loader(loader),
      infogetter(s) {}

void CommandExecuter::loadNewTexture(double x, double y, uint32_t yamlId,
                                     uint32_t uniqueId) {
  unsigned int itemId =
      this->loader.convertYamlFileItemIdToProtocolItemSkinId(yamlId);
  this->sprites.push_back(new Drawable(y, x, itemId, uniqueId));
}

CommandExecuter::~CommandExecuter() {
  /*for (iterator_t it = this->players.begin(); it != this->players.end();
  ++it){ delete it->second;
  }*/
}

void CommandExecuter::playShootingSounds(int shooterId) {
  if (shooterId != this->selfId) {
    int weaponId = players.at(shooterId)->weaponId;
    int soundId = GET_WEAPON_SOUND(weaponId);
    double dist =
        players.at(this->selfId)->calculateDist(players.at(shooterId));
    this->audiomanager.playOnVariableVolumeWithId(soundId, dist);
  }
}

void CommandExecuter::playDoorOpeningSound(int x, int y) {
  double dist = players.at(this->selfId)->calculateDist(x, y);
  if (dist < 0) dist = 1;
  this->audiomanager.playOnVariableVolumeWithId(matrix.getDoorSound(x, y),
                                                dist * 10);
}

void CommandExecuter::playExplosionSound() {
  this->audiomanager.playWithId(EXPLOSION_SOUND);
}

void CommandExecuter::removeSpriteWithId(uint32_t itemId) {
  uint32_t soundId;
  this->sprites.removeSpriteWithIdAndGetSound(itemId, &soundId);
  this->audiomanager.playWithId(soundId);
}

void CommandExecuter::renderMovingSprite(double x, double y, uint32_t itemId) {
  this->sprites.swapCoords(x, y, itemId);
}

void CommandExecuter::renderExplosionAnimation(uint32_t itemId) {
  double x = ERROR;
  double y = ERROR;
  this->sprites.getCoordsAndErase(&x, &y, itemId);
  if (x == ERROR || y == ERROR)
    LOG("Error, no missile texture to explode found.");
  this->sprites.push_back(new RaycastedAnimation(
      x, y, EXPLOSION, itemId, FRAMES_PER_EXPLOSION_ANIMATION));
}

void CommandExecuter::renderDeathAnimation(uint32_t playerId) {
  Player* deadPlayer = this->players[playerId];
  int gunId = deadPlayer->weaponId;
  int deathSpriteId = GET_DEATH_ANIMATION_SPRITE(gunId);
  // I don't need to get a new uniqueId for the sprite when I can use -1 *
  // deadPlayer->playerId. Not only it's a negative number, which means that no
  // other texture could have the same id, But that no other players can have
  // the same id.
  this->playDyingSound(gunId, playerId);
  RaycastedAnimation* animation =
      new RaycastedAnimation(deadPlayer->x, deadPlayer->y, deathSpriteId,
                             -int(playerId), FRAMES_PER_DEATH_ANIMATION);
  this->sprites.push_back(animation);
}

void CommandExecuter::playDyingSound(int gunId, int playerId) {
  int soundId = GET_DEATH_SOUND(gunId);
  double dist = players.at(this->selfId)->calculateDist(players.at(playerId));
  this->audiomanager.playOrStopOnVariableVolumeWithId(soundId, dist);
}

void CommandExecuter::updateOrCreatePlayer() {
  PlayerData playerinfo;
  memset(&playerinfo, 0, sizeof(PlayerData));
  infogetter.receivePlayerData(playerinfo);
  uint32_t id = playerinfo.playerID;
  if (players.find(id) != players.end()) {
    players[id]->update(playerinfo);
  } else {
    Player* placeholder = new Player(playerinfo);
    if (!placeholder) {
      LOG(COULD_NOT_CREATE_PLAYER);
      return;
    }
    players[id] = placeholder;
    this->sprites.push_back(placeholder);
  }
}

void CommandExecuter::disconnectPlayer() {
  uint32_t id;
  this->socket.receive(&id, sizeof(id));
  if (id == this->selfId) return;
  Player* toKill = players[id];
  players.erase(id);
  this->sprites.popAndErase(toKill);
  delete toKill;
}

void CommandExecuter::shotsFired() {
  uint32_t shooterId;
  this->socket.receive(&shooterId, sizeof(shooterId));
  this->players.at(shooterId)->startShooting();
  this->playShootingSounds(shooterId);
}

void CommandExecuter::openDoor() {
  uint32_t x, y;
  this->socket.receive(&x, sizeof(x));
  this->socket.receive(&y, sizeof(y));
  matrix.switchDoorState(x, y);
  this->playDoorOpeningSound(x, y);
}

void CommandExecuter::pickUpItem() {
  uint32_t itemId;
  this->socket.receive(&itemId, sizeof(itemId));
  this->removeSpriteWithId(itemId);
}

void CommandExecuter::playerDied() {
  uint32_t playerId;
  this->socket.receive(&playerId, sizeof(playerId));
  if (playerId != this->selfId) this->renderDeathAnimation(playerId);
}

void CommandExecuter::dropItem() {
  uint32_t yamlId, uniqueId;
  double x, y;
  x = infogetter.receiveDouble();
  y = infogetter.receiveDouble();
  this->socket.receive(&yamlId, sizeof(yamlId));
  this->socket.receive(&uniqueId, sizeof(uniqueId));
  this->loadNewTexture(x, y, yamlId, uniqueId);
}

void CommandExecuter::elementSwitchPosition() {
  uint32_t uniqueId;
  this->socket.receive(&uniqueId, sizeof(uniqueId));
  double x = infogetter.receiveDouble();
  double y = infogetter.receiveDouble();
  this->renderMovingSprite(x, y, uniqueId);
}

void CommandExecuter::explodeMissile() {
  uint32_t uniqueId;
  this->socket.receive(&uniqueId, sizeof(uniqueId));
  this->audiomanager.playWithId(EXPLOSION_SOUND);
  this->renderExplosionAnimation(uniqueId);
}

void CommandExecuter::run() {
  this->audiomanager.playWithId(MUSIC);
  while (!this->scoreboard->hasEnded()) {
    try {
      uint32_t opcode;
      socket.receive(&opcode, sizeof(opcode));
      if (opcode == PLAYER_UPDATE_PACKAGE) {
        this->updateOrCreatePlayer();
      } else if (opcode == PLAYER_DISCONNECT) {
        this->disconnectPlayer();
      } else if (opcode == SHOTS_FIRED) {
        this->shotsFired();
      } else if (opcode == OPEN_DOOR) {
        this->openDoor();
      } else if (opcode == PLAYER_PICKUP_ITEM) {
        this->pickUpItem();
      } else if (opcode == PLAYER_DIED) {
        this->playerDied();
      } else if (opcode == PLAYER_DROP_ITEM) {
        this->dropItem();
      } else if (opcode == ELEMENT_SWITCH_POSITION) {
        this->elementSwitchPosition();
      } else if (opcode == MISSILE_EXPLOTION) {
        this->explodeMissile();
      } else if (opcode == ENDING_MATCH) {
        this->saveScores();
        this->saveKills();
        this->saveShotsFired();
        alive = false;
      }
    } catch (SocketException& e) {
    } catch (std::exception& e) {
      LOG(e.what());
      break;
    }
  }
}

std::tuple<uint32_t, uint32_t> static recvTuple(SocketCommunication& socket) {
  uint32_t value1;
  uint32_t value2;
  socket.receive(&value1, sizeof(value1));
  socket.receive(&value2, sizeof(value2));
  return std::make_tuple(value1, value2);
}

void CommandExecuter::saveShotsFired() {
  uint32_t numberOfPlayers;
  this->socket.receive(&numberOfPlayers, sizeof(numberOfPlayers));
  for (int i = 0; i < numberOfPlayers; i++) {
    this->scoreboard->pushShotsfired(recvTuple(this->socket));
  }
}

void CommandExecuter::saveScores() {
  uint32_t numberOfPlayers;
  this->socket.receive(&numberOfPlayers, sizeof(numberOfPlayers));
  for (int i = 0; i < numberOfPlayers; i++) {
    this->scoreboard->pushScore(recvTuple(this->socket));
  }
}

void CommandExecuter::saveKills() {
  uint32_t numberOfPlayers;
  this->socket.receive(&numberOfPlayers, sizeof(numberOfPlayers));
  for (int i = 0; i < numberOfPlayers; i++) {
    this->scoreboard->pushKills(recvTuple(this->socket));
  }
}
