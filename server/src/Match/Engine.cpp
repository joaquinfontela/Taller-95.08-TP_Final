#include "../../includes/Match/Engine.h"
#include "../../../common/includes/Socket/SocketException.h"
#include <iostream>
#include <time.h>
#include <math.h>

Engine::Engine(WaitingQueue<Command*>& commandQ, std::atomic<bool>& c, std::map<int, ClientCommunication*>& play, Game& game)
    : commandQueue(commandQ), cont(c), players(play), thisGame(game) {}


void Engine::update(){

  this->thisGame.updatePositions();
  this->thisGame.sendUpdateMessages(this->notifications);

}
void Engine::run() {
  std::thread notificationThread(&Engine::sendNotifications, this);
  std::thread commandThread(&Engine::executeCommands, this);

  int rate = 1000/30;

  auto t1 = std::chrono::steady_clock::now();
  auto t2 = t1;
  std::chrono::duration<float, std::milli> diff;

  int rest = 0;
  int behind = 0;
  int lost = 0;

  while (cont) {

    this->gameLock.lock();
    update();
    this->gameLock.unlock();

    t2 = std::chrono::steady_clock::now();
    diff = t2 - t1;
    rest = rate - ceil(diff.count());

    if(rest < 0){
      behind -= rest;
      lost = rate + (behind - behind % rate);
      rest = rate - behind % rate;

      t1 += std::chrono::milliseconds(lost);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(rest));
    t1 += std::chrono::milliseconds(rate);


  }
    commandThread.join();
    notificationThread.join();
    std::cout << "[ENGINE] Joined threads succesfully" << std::endl;
}

void Engine::requestShutdown(){
  commandQueue.requestShutdown();
  notifications.requestShutdown();
}

void Engine::executeCommands(){

  while (cont) {
    Command* newCommand = nullptr;
    newCommand = commandQueue.pop();
    if (newCommand == nullptr) continue;
    this->gameLock.lock();
    newCommand->execute(notifications, this->thisGame);
    this->gameLock.unlock();
    delete newCommand;
  }
}
void Engine::sendNotifications() {
  while (cont) {
    Notification* newNotification = nullptr;
    newNotification = notifications.pop();
    if (newNotification == nullptr) continue;
    std::cout << "[ENGINE] Sending Notification" << std::endl;
    for (auto& it : players) {
      ClientCommunication* c = it.second;
      try {
        newNotification->send(c->getSock());
      } catch (SocketException& e) {
        // Deberia sacar del mapa al ClientCommunication que falla.
      }
    }

    delete newNotification;
  }
}
