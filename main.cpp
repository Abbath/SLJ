#include <cmath>
#include <iostream>

#include <fmt/format.h>

#include <raylib.h>
#include <raymath.h>

constexpr size_t BULLETS = 1000;
constexpr size_t ENEMIES = 1000;
constexpr size_t BONUSES = 1000;
constexpr size_t BACKGROUNDS = 1000;
constexpr int FPS = 60;

struct Player {
  float x;
  float y;
  float speed;
  int cannons;
  int bullet_damage;
  int bullet_penetration;
};

struct Bullet {
  float x = 0;
  float y = 0;
  float dx = 0;
  float dy = -10;
  int penetration = 1;
  bool alive = false;
};

struct Enemy {
  float x = 0;
  float y = 0;
  float speed = 4;
  int hp = 10;
  bool is_boss = false;
  bool alive = false;
};

struct Bonus {
  float x = 0;
  float y = 0;
  float speed = 4;
  int typ = 0;
  bool alive = false;
};

struct Background {
  float x;
  float y;
  float size;
  float speed;
  int color = 128;
  bool alive = false;
};

struct Rocket {
  float x = 0;
  float y = 0;
  float dx = 0;
  float dy = 0;
  bool alive = false;
};

void DrawPlayer(Player p, bool blink = false) {
  DrawPoly(Vector2{p.x, p.y}, 3, 32, 180, blink ? BLUE : YELLOW);
}
void MovePlayer(Player &p) {
  auto w = GetScreenWidth();
  auto h = GetScreenHeight();
  if (IsKeyDown(KEY_LEFT)) {
    p.x = std::max(0.0f, p.x - p.speed);
  }
  if (IsKeyDown(KEY_RIGHT)) {
    p.x = std::min(float(w), p.x + p.speed);
  }
  if (IsKeyDown(KEY_DOWN)) {
    p.y = std::min(float(h), p.y + p.speed);
  }
  if (IsKeyDown(KEY_UP)) {
    p.y = std::max(0.0f, p.y - p.speed);
  }
}
void DrawBullet(Bullet b) { DrawCircle(int(b.x), int(b.y), 2, WHITE); }
void MoveBullet(Bullet &b) {
  auto w = GetScreenWidth();
  auto h = GetScreenHeight();
  b.x += b.dx;
  b.y += b.dy;
  if (b.x < 0 || b.y < 0 || b.x > w || b.y > h) {
    b.alive = false;
  }
}
void DrawEnemy(Enemy e) {
  auto hp = std::min(300, std::max(16, e.hp));
  auto color = ColorAlpha(MAROON, 128);
  DrawRectangle(e.x - hp - (e.is_boss ? e.speed : 0),
                e.y - hp - (e.is_boss ? 0 : e.speed * 2), hp * 2, hp * 2,
                color);
  DrawRectangle(e.x - hp, e.y - hp, hp * 2, hp * 2, e.is_boss ? MAROON : RED);
}
void MoveEnemy(Enemy &e) {
  auto w = GetScreenWidth();
  auto h = GetScreenHeight();
  if (e.is_boss) {
    e.x += e.speed;
    if (e.x < 0 || e.x > w) {
      e.speed *= -1;
      e.x += 2 * e.speed;
    }
  } else {
    e.y += e.speed;
  }
  if (e.x < 0 || e.y < -e.hp || e.x > w || e.y > h + e.hp) {
    e.alive = false;
  }
}
void DrawBonus(Bonus b) {
  DrawPoly(Vector2{b.x, b.y}, 6, 16, 0,
           b.typ == 1 ? GREEN
                      : (b.typ == 2 ? GOLD : (b.typ == 3 ? PINK : PURPLE)));
}
void MoveBonus(Bonus &b) {
  auto w = GetScreenWidth();
  auto h = GetScreenHeight();
  b.y += b.speed;
  if (b.x < 0 || b.y < 0 || b.x > w || b.y > h) {
    b.alive = false;
  }
}
void DrawBackground(Background b) {
  DrawPoly(Vector2{b.x, b.y}, 4, b.size, 45,
           Color{0, 0, uint8_t(b.color), 255});
}
void MoveBackground(Background &b) {
  auto w = GetScreenWidth();
  auto h = GetScreenHeight();
  b.y += b.speed;
  if (b.x < 0 || b.y < -b.size || b.x > w || b.y > h + b.size) {
    b.alive = false;
  }
}

int main(int argc, char *argv[]) {
  SetRandomSeed(time(NULL));

  auto win_w = 1000;
  auto win_h = 1000;

  InitWindow(win_w, win_h, "SLJ");
  SetTargetFPS(FPS);

  Player p{float(win_w / 2), float(win_h - 40), 5, 1, 10, 1};
  Bullet bullets[BULLETS];
  Enemy enemies[ENEMIES];
  Bonus bonuses[BONUSES];
  Background backgrounds[BACKGROUNDS];
  Rocket rocket;

  size_t dead_bullet = 0;
  size_t dead_enemy = 0;
  size_t dead_bonus = 0;
  size_t dead_background = 0;
  size_t frame_counter = 0;
  size_t frame_counter_backup = 0;

  bool game_over = false;
  bool pause = true;
  bool boss_is_here = false;
  bool mute = true;
  int blink_player = 0;
  int score = 0;
  int stage = 1;

  InitAudioDevice();
  Sound psound = LoadSound("p.ogg");

  while (!WindowShouldClose()) {
    if (!game_over && !pause) {
      if (frame_counter % 6 == 0) {
        for (int i = 0; i < p.cannons; ++i) {
          bullets[dead_bullet].alive = true;
          float n = p.cannons == 1 ? 0 : float(i) - ((p.cannons - 1) / 2.0f);
          bullets[dead_bullet].x = p.x + n * 5;
          bullets[dead_bullet].y = p.y;
          bullets[dead_bullet].dx = n;
          bullets[dead_bullet].penetration = p.bullet_penetration;

          while (true) {
            dead_bullet = (dead_bullet + 1) % BULLETS;
            if (!bullets[dead_bullet].alive) {
              break;
            }
          }
        }
      }

      size_t difficulty = frame_counter / 360;
      size_t frequency = std::max(1ul, 30 - difficulty - stage);
      if (frame_counter % frequency == 0) {
        for (int i = 0; i < 1; ++i) {
          enemies[dead_enemy].alive = true;
          enemies[dead_enemy].x = GetRandomValue(0, GetScreenWidth());
          if (!boss_is_here && stage % 3 == 0) {
            boss_is_here = true;
            enemies[dead_enemy].speed = 5;
            enemies[dead_enemy].hp = 100000;
            enemies[dead_enemy].is_boss = true;
            enemies[dead_enemy].y = 0;
          } else {
            enemies[dead_enemy].speed =
                4 + (2 - log10(GetRandomValue(1, 1000)));
            enemies[dead_enemy].hp =
                GetRandomValue(10, 70) + difficulty * stage;
            enemies[dead_enemy].is_boss = false;
            enemies[dead_enemy].y = -enemies[dead_enemy].hp;
          }

          while (true) {
            dead_enemy = (dead_enemy + 1) % ENEMIES;
            if (!enemies[dead_enemy].alive) {
              break;
            }
          }
        }
      }

      if (frame_counter % 900 == 0) {
        bonuses[dead_bonus].alive = true;
        bonuses[dead_bonus].x = GetRandomValue(0, GetScreenWidth());
        bonuses[dead_bonus].y = 0;
        bonuses[dead_bonus].typ = GetRandomValue(1, 4);

        while (true) {
          dead_bonus = (dead_bonus + 1) % BONUSES;
          if (!bonuses[dead_bonus].alive) {
            break;
          }
        }
      }

      if (frame_counter % 120 == 0) {
        for (int i = 0; i < 2; ++i) {
          backgrounds[dead_background].alive = true;
          backgrounds[dead_background].size = GetRandomValue(60, 600);
          backgrounds[dead_background].x = GetRandomValue(0, GetScreenWidth());
          backgrounds[dead_background].y = -backgrounds[dead_background].size;
          backgrounds[dead_background].color = 128 + GetRandomValue(-64, 64);
          backgrounds[dead_background].speed = i ? 1 : 0.5;

          while (true) {
            dead_background = (dead_background + 1) % BACKGROUNDS;
            if (!backgrounds[dead_background].alive) {
              break;
            }
          }
        }
      }

      if (frame_counter % 1200 == 0) {
        if (!rocket.alive) {
          rocket.alive = true;
          rocket.x = p.x;
          rocket.y = p.y;
          rocket.dx = 0;
          rocket.dy = -5;
        }
      }

      MovePlayer(p);
      for (auto &background : backgrounds) {
        if (background.alive) {
          MoveBackground(background);
        }
      }
      for (auto &bullet : bullets) {
        if (bullet.alive) {
          MoveBullet(bullet);
        }
      }
      for (auto &bonus : bonuses) {
        if (bonus.alive) {
          MoveBonus(bonus);
        }
      }
      for (auto &enemy : enemies) {
        if (enemy.alive) {
          MoveEnemy(enemy);
        }
      }
      if (rocket.alive) {
        rocket.x += rocket.dx;
        rocket.y += rocket.dy;
      }
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (auto &background : backgrounds) {
      if (background.alive) {
        DrawBackground(background);
      }
    }
    for (auto &enemy : enemies) {
      if (enemy.alive) {
        DrawEnemy(enemy);
      }
    }
    for (auto &bullet : bullets) {
      if (bullet.alive) {
        DrawBullet(bullet);
      }
    }
    for (auto &bonus : bonuses) {
      if (bonus.alive) {
        DrawBonus(bonus);
      }
    }
    DrawPlayer(p, bool(blink_player));
    blink_player = std::max(0, blink_player - 1);
    if (rocket.alive) {
      DrawCircle(rocket.x - rocket.dx * (3.0 + 0.5 * GetRandomValue(1, 5)),
                 rocket.y - rocket.dy * (3.0 + 0.5 * GetRandomValue(1, 5)),
                 GetRandomValue(5, 15), ORANGE);
      DrawPoly(Vector2{rocket.x, rocket.y}, 3, 16,
               RAD2DEG * atan2(rocket.dx, rocket.dy), ORANGE);
    }
    if (game_over) {
      auto w = MeasureText("GAME OVER", 72);
      DrawText("GAME OVER", (GetScreenWidth() - w) / 2,
               GetScreenHeight() / 2 - 36, 72, RAYWHITE);
    }
    if (pause) {
      auto w = MeasureText("PAUSE", 72);
      DrawText("PAUSE", (GetScreenWidth() - w) / 2, GetScreenHeight() / 2 - 36,
               72, RAYWHITE);
    }
    DrawText(fmt::format("Score: {}\nStage: {}\nSpeed: {}\nBullet damage: "
                         "{}\nBullet penetration: {}\nCannons: {}\n",
                         score, stage, p.speed, p.bullet_damage,
                         p.bullet_penetration, p.cannons)
                 .c_str(),
             10, 10, 20, RAYWHITE);
    EndDrawing();
    if (IsKeyPressed(KEY_M)) {
      mute = !mute;
    }
    if (!game_over && IsKeyPressed(KEY_SPACE)) {
      pause = !pause;
      if (pause) {
        frame_counter_backup = frame_counter;
      } else {
        frame_counter = frame_counter_backup;
      }
    }
    if (game_over && IsKeyPressed(KEY_SPACE)) {
      score = 0;
      stage = 1;
      boss_is_here = false;
      for (auto &background : backgrounds) {
        background.alive = false;
      }
      for (auto &bullet : bullets) {
        bullet.alive = false;
      }
      for (auto &enemy : enemies) {
        enemy.alive = false;
      }
      for (auto &bonus : bonuses) {
        bonus.alive = false;
      }
      p = Player{float(win_w / 2), float(win_h - 40), 5, 1, 10, 1};
      rocket.alive = false;
      game_over = false;
      pause = true;
      frame_counter = 0;
    }
    if (!game_over && !pause) {
      float min_d = std::numeric_limits<float>::max();
      int counter = -1;
      int min_idx = 0;
      for (auto &enemy : enemies) {
        counter += 1;
        if (enemy.alive) {
          if (rocket.alive &&
              CheckCollisionRecs(
                  Rectangle{rocket.x - 10, rocket.y - 10, 20, 20},
                  Rectangle{enemy.x - 10, enemy.y - 10, 20, 20})) {
            enemy.alive = false;
            rocket.alive = false;
            continue;
          }
          if (rocket.alive) {
            auto d = std::sqrt(std::pow(rocket.x - enemy.x, 2.0f) +
                               std::pow(rocket.y - enemy.y, 2.0f));
            if (d < min_d) {
              min_d = d;
              min_idx = counter;
            }
          }
          auto hp = std::min(300, std::max(16, enemy.hp));
          if (CheckCollisionCircleRec(Vector2{p.x, p.y}, 12,
                                      Rectangle{enemy.x - hp, enemy.y - hp,
                                                hp * 2.0f, hp * 2.0f})) {
            game_over = true;
          }
        }
      }
      if (rocket.alive) {
        auto dx = enemies[min_idx].x - rocket.x;
        auto dy = enemies[min_idx].y - rocket.y;
        rocket.dx = 7 * dx / min_d;
        rocket.dy = 7 * dy / min_d;
      }
      for (auto &bonus : bonuses) {
        if (bonus.alive) {
          if (CheckCollisionRecs(
                  Rectangle{p.x - 15, p.y - 15, 30, 30},
                  Rectangle{bonus.x - 10, bonus.y - 10, 20, 20})) {
            switch (bonus.typ) {
            case 1:
              p.speed += 1;
              break;
            case 2:
              p.cannons += 1;
              break;
            case 3:
              p.bullet_damage += 1;
              break;
            case 4:
              p.bullet_penetration += 1;
              break;
            default:
              break;
            }
            bonus.alive = false;
            blink_player = 10;
          }
        }
      }
      for (auto &bullet : bullets) {
        if (bullet.alive) {
          for (auto &enemy : enemies) {
            if (enemy.alive) {
              auto hp = std::min(300, std::max(16, enemy.hp));
              auto rect =
                  Rectangle{enemy.x - hp, enemy.y - hp, hp * 2.0f, hp * 2.0f};
              if (CheckCollisionPointRec(Vector2{bullet.x, bullet.y}, rect)) {
                enemy.hp -= (p.bullet_damage -
                             (p.bullet_penetration - bullet.penetration));
                bullet.penetration -= 1;
                if (bullet.penetration == 0) {
                  bullet.alive = false;
                }
                if (enemy.hp <= 0) {
                  enemy.alive = false;
                  if (!mute && IsSoundReady(psound)) {
                    PlaySound(psound);
                  }
                  if (enemy.is_boss) {
                    score += 500;
                  }
                  score += 1;
                }
              }
            }
          }
        }
      }
    }

    auto old_frame_counter = frame_counter;
    frame_counter = (frame_counter + 1) % (FPS * 60);
    if (!game_over && !pause && old_frame_counter > frame_counter) {
      stage += 1;
      boss_is_here = false;
    }
  }

  return 0;
}