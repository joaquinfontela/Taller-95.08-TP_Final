#ifndef COMMANDEXECUTER_H_
#define COMMANDEXECUTER_H_

#include <atomic>
#include <map>
#include <mutex>
#include <vector>

#include "player.h"
#include "audiomanager.h"
#include "ClientMapLoader.h"
#include "../../common/includes/Socket/SocketCommunication.h"
#include "../../common/includes/Socket/SocketException.h"
#include "../../common/includes/Map/Map.h"
#include "../../common/includes/Thread/Thread.h"

typedef std::map<uint32_t,Player*>::iterator iterator_t;

class CommandExecuter : public Thread {
 public:
  CommandExecuter(SocketCommunication& s, std::atomic<bool>& alive, std::vector<Drawable*>& sprites,
                  std::map<uint32_t,Player*>& players, std::mutex& lock, int selfId,
                  AudioManager& audiomanager, Map& matrix, ClientMapLoader& loader);
  ~CommandExecuter();
  void run();
  void eraseSprite(uint32_t itemId);
  void playExplosionSound();
 private:
  void removeSpriteWithId(uint32_t itemId);
  void renderExplosionAnimation(uint32_t itemId);
  void renderDeathAnimation(uint32_t yamlId);
  void renderMovingSprite(double x, double y, uint32_t itemId);
  void loadNewTexture(double x, double y, uint32_t yamlId, uint32_t uniqueId);
  void playShootingSounds(int shooterId);
  void playDoorOpeningSound(int x, int y);
  SocketCommunication& socket;
  std::atomic<bool>& alive;
  std::vector<Drawable*>& sprites;
  std::map<uint32_t,Player*>& players;
  std::mutex& lock;
  int selfId;
  AudioManager& audiomanager;
  Map& matrix;
  ClientMapLoader& loader;
};

#endif  // COMMANDEXECUTER_H_
