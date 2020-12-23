#include "commandsender.h"

#include <SDL2/SDL.h>
#include <time.h>
#include <atomic>
#include <iostream>
#include "anglemanager.h"
#include "../../common/includes/protocol.h"
#include "player.h"

#define WIDTH 800
#define HEIGHT 600
#define DIMX 10
#define DIMY 13

CommandSender::CommandSender(SocketCommunication& s, std::atomic<bool>& alive) : 
        socket(s), x(370), y(370), alive(alive) {}

void CommandSender::update(uint32_t deltax, uint32_t deltay, double viewAngle, AngleManager& angles) {
  double theta = viewAngle - angles.ANGLE270;
  if (theta < 0) theta += angles.ANGLE360;
  if (theta > angles.ANGLE360) theta -= angles.ANGLE360;
  x -= deltax * angles.fCos(theta) - deltay * angles.fSin(theta);
  y -= deltax * angles.fSin(theta) - deltay * angles.fCos(theta);
  uint32_t protocol = PLAYER_POS_UPDATE;
  socket.send(&protocol, sizeof(protocol));
  socket.send(&x, sizeof(x));
  socket.send(&y, sizeof(y));
}

void CommandSender::run() {
  while (true) {
    try {
      uint32_t deltax = 0;
      uint32_t deltay = 0;
      uint32_t xmouse = 0;
      AngleManager angles;
      double viewAngle = angles.ANGLE270;

      SDL_Event event;
      SDL_WaitEvent(&event);
      if (event.type == SDL_QUIT) {
        socket.readShutdown();
        socket.writeShutdown();
        socket.close();
        this->alive = false;
        std::cout << "ESTOY MATANDO AL SENDER DE INFORMACIÓN." << std::endl;
        deltax = 0;
        deltay = 0;
        break;
      }
      if (event.type == SDL_KEYDOWN) {
        SDL_KeyboardEvent& key = (SDL_KeyboardEvent&)event;
        switch (key.keysym.sym) {
          case SDLK_a:
            if (x + 10 < DIMX * 64) deltax = 10; 
            this->update(deltax, deltay, viewAngle, angles);
            break;
          case SDLK_d:
            if (x - 10 > 0) deltax = -10; 
            this->update(deltax, deltay, viewAngle, angles);
            break;
          case SDLK_w:
            if (y + 10 < DIMY * 64) deltay = 10; 
            this->update(deltax, deltay, viewAngle, angles);
            break;
          case SDLK_s:
            if (y - 10 > 0) deltay = -10; 
            this->update(deltax, deltay, viewAngle, angles);
            break;
          case SDLK_LEFT:
            viewAngle -= angles.ANGLE10;
            if (viewAngle < 0) viewAngle += angles.ANGLE360;
            this->update(deltax, deltay, viewAngle, angles);
            break;
          case SDLK_RIGHT:
            viewAngle -= angles.ANGLE10;
            if (viewAngle > angles.ANGLE360) viewAngle -= angles.ANGLE360;
            this->update(deltax, deltay, viewAngle, angles);
            break;
        }
      } else if (event.type == SDL_MOUSEMOTION) {
        if (event.motion.x > xmouse) {
          viewAngle += angles.ANGLE1p5;
          if (viewAngle > angles.ANGLE360) viewAngle -= angles.ANGLE360;
          this->update(deltax, deltay, viewAngle, angles);
        } else {
          viewAngle -= angles.ANGLE1p5;
          if (viewAngle < 0) viewAngle += angles.ANGLE360;
          this->update(deltax, deltay, viewAngle, angles);
        }
        xmouse = event.motion.x;
      }

    } catch (SocketException& e) {
      break;
    }
  }
}
