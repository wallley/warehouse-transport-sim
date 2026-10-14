#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <graphics.h>
#include <windows.h>
#include <conio.h>
#include "warehouse/gui.hpp"
#include "warehouse/simulation.hpp"
#include "warehouse/logging.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace warehouse {

void renderMenuButton(const UIControl& control, bool isHovered) {
    Color btnColor = control.active ?
        (isHovered ? NEON_PINK : NEON_CYAN) :
        TEXT_DIM;

    for (int layer = 2; layer >= 0; layer--) {
        int offset = layer * 2;
        setlinecolor(rgb(btnColor >> 16 & 0xFF >> layer,
                        btnColor >> 8 & 0xFF >> layer,
                        btnColor & 0xFF >> layer));
        setlinestyle(PS_SOLID, 2 - layer);
        rectangle(control.posX - offset, control.posY - offset,
                 control.posX + control.sizeW + offset,
                 control.posY + control.sizeH + offset);
    }

    setfillcolor(HOLOGRAM_BG);
    solidrectangle(control.posX, control.posY,
                  control.posX + control.sizeW, control.posY + control.sizeH);

    setlinecolor(btnColor);
    setlinestyle(PS_SOLID, 3);
    rectangle(control.posX, control.posY,
             control.posX + control.sizeW, control.posY + control.sizeH);

    setlinecolor(btnColor);
    setlinestyle(PS_DOT, 1);
    line(control.posX + 10, control.posY + 5,
         control.posX + control.sizeW - 10, control.posY + 5);
    line(control.posX + 10, control.posY + control.sizeH - 5,
         control.posX + control.sizeW - 10, control.posY + control.sizeH - 5);

    settextcolor(btnColor);
    settextstyle(24, 0, "Consolas");
    int textW = textwidth(control.label.c_str());
    int textH = textheight(control.label.c_str());
    outtextxy(control.posX + (control.sizeW - textW) / 2,
             control.posY + (control.sizeH - textH) / 2 - 5,
             control.label.c_str());

    std::string shortcutText = "[" + std::string(1, control.shortcut) + "]";
    settextcolor(control.active ? TEXT_NEON : TEXT_DIM);
    settextstyle(16, 0, "Consolas");
    int shortcutW = textwidth(shortcutText.c_str());
    outtextxy(control.posX + (control.sizeW - shortcutW) / 2,
             control.posY + control.sizeH - 25, shortcutText.c_str());
}

MenuAction displaySystemMenu(SimulationContext& context, bool& patrolMode, int& unitCount) {

    setfillcolor(BG_DARK);
    solidrectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    updateMatrixRain(context);
    settextcolor(TEXT_MATRIX);
    settextstyle(12, 0, "Consolas");
    for (size_t i = 0; i < context.view.matrix_rain.size(); ++i) {
        MatrixColumn& col = context.view.matrix_rain[i];
        for (size_t j = 0; j < col.chars.size(); ++j) {
            int charY = col.y + j * 15;
            if (charY >= 0 && charY < DISPLAY_HEIGHT) {
                int brightness = 255 - (j * 20);
                if (brightness > col.brightness) brightness = col.brightness;
                if (brightness > 0) {
                    settextcolor(rgb(0, brightness, 0));
                    char str[2] = { col.chars[j], '\0' };
                    outtextxy(i * 15, charY, str);
                }
            }
        }
    }

    settextcolor(NEON_PINK);
    settextstyle(36, 0, "Consolas");
    std::string title = "Warehouse Transport Simulator";
    int titleW = textwidth(title.c_str());

    for (int layer = 3; layer >= 0; layer--) {
        int offset = layer * 2;
        Color glowColor = rgb(255 - layer * 50, 0, 255 - layer * 30);
        settextcolor(glowColor);
        outtextxy(DISPLAY_WIDTH / 2 - titleW / 2 + offset, 50 + offset, title.c_str());
    }

    int panelX = DISPLAY_WIDTH / 2 - 250;
    int panelY = 150;
    int panelW = 500;
    int panelH = 200;

    setfillcolor(HOLOGRAM_BG);
    solidrectangle(panelX, panelY, panelX + panelW, panelY + panelH);

    double borderPhase = GetTickCount() / 1000.0;
    for (int i = 0; i < 20; i++) {
        double t = i / 20.0 + fmod(borderPhase, 1.0);
        if (t > 1.0) t -= 1.0;

        int x = panelX + (int)(panelW * t);
        int y = panelY;

        setfillcolor(NEON_CYAN);
        solidcircle(x, y, 3);
        solidcircle(x, y + panelH, 3);

        y = panelY + (int)(panelH * t);
        x = panelX;
        solidcircle(x, y, 3);
        solidcircle(x + panelW, y, 3);
    }

    settextcolor(NEON_GREEN);
    settextstyle(24, 0, "Consolas");
    std::string modeText = "TASK: " + std::string(patrolMode ? "PATROL ANALYSIS" : "SCHEDULE SIMULATION");
    int modeW = textwidth(modeText.c_str());
    outtextxy(DISPLAY_WIDTH / 2 - modeW / 2, panelY + 30, modeText.c_str());

    settextcolor(NEON_YELLOW);
    settextstyle(20, 0, "Consolas");
    std::string unitText = patrolMode ?
        "HOVERCRAFTS: 3 [LOCKED]" :
        "HOVERCRAFTS: " + std::to_string(unitCount) + " [ADJUSTABLE]";
    int unitW = textwidth(unitText.c_str());
    outtextxy(DISPLAY_WIDTH / 2 - unitW / 2, panelY + 70, unitText.c_str());

    settextcolor(STATUS_INFO);
    settextstyle(16, 0, "Consolas");
    std::string statusText = "SYSTEM READY | ALL MODULES ONLINE";
    int statusW = textwidth(statusText.c_str());
    outtextxy(DISPLAY_WIDTH / 2 - statusW / 2, panelY + 110, statusText.c_str());

    settextcolor(TEXT_DIM);
    settextstyle(14, 0, "Consolas");
    std::string logText = "LOG: " + context.active_log_filename;
    int logW = textwidth(logText.c_str());
    outtextxy(DISPLAY_WIDTH / 2 - logW / 2, panelY + 140, logText.c_str());

    int centerX = DISPLAY_WIDTH / 2;
    int centerY = DISPLAY_HEIGHT / 2 + 50;
    int menuRadius = 180;

    struct RadialMenuItem {
        std::string label;
        std::string key;
        double angle;
        MenuAction action;
        bool active;
        Color color;
    };

    RadialMenuItem menuItems[] = {
        {"BEGIN", "[1]", -MATH_PI/2, MenuAction::BEGIN, true, NEON_GREEN},
        {"TASK", "[2]", 0, MenuAction::CHANGE_TASKS, true, NEON_PURPLE},
        {"CARS", "[3]", MATH_PI/2, MenuAction::CHANGE_CARS, !patrolMode, NEON_YELLOW},
        {"EXIT", "[4]", MATH_PI, MenuAction::EXIT, true, STATUS_ERROR}
    };

    auto drawRadialMenu = [&](int hovered) {

        for (int r = 3; r >= 0; r--) {
            setlinecolor(rgb(100 - r * 20, 0, 150 - r * 30));
            setlinestyle(PS_SOLID, 3 - r);
            circle(centerX, centerY, 50 + r * 10);
        }

        for (int i = 0; i < 4; i++) {
            RadialMenuItem& item = menuItems[i];

            int itemX = centerX + (int)(menuRadius * cos(item.angle));
            int itemY = centerY + (int)(menuRadius * sin(item.angle));

            if (item.active) {
                setlinecolor(i == hovered ? item.color : TEXT_DIM);
                setlinestyle(PS_SOLID, i == hovered ? 3 : 1);
                line(centerX, centerY, itemX, itemY);
            }

            int circleSize = i == hovered ? 60 : 50;
            if (item.active) {

                for (int g = 2; g >= 0; g--) {
                    setlinecolor(i == hovered ? item.color : TEXT_DIM);
                    setlinestyle(PS_SOLID, 2 - g);
                    circle(itemX, itemY, circleSize + g * 5);
                }

                setfillcolor(HOLOGRAM_BG);
                solidcircle(itemX, itemY, circleSize);
            }

            settextcolor(item.active ? (i == hovered ? item.color : TEXT_NEON) : TEXT_DIM);
            settextstyle(18, 0, "Consolas");
            int labelW = textwidth(item.label.c_str());
            outtextxy(itemX - labelW/2, itemY - 10, item.label.c_str());

            settextstyle(14, 0, "Consolas");
            int keyW = textwidth(item.key.c_str());
            outtextxy(itemX - keyW/2, itemY + 10, item.key.c_str());
        }

        settextcolor(NEON_CYAN);
        settextstyle(16, 0, "Consolas");
        std::string centerText = "NEXUS";
        int centerW = textwidth(centerText.c_str());
        outtextxy(centerX - centerW/2, centerY - 8, centerText.c_str());
    };

    int instructY = DISPLAY_HEIGHT - 150;
    setfillcolor(HOLOGRAM_BG);
    solidrectangle(100, instructY, DISPLAY_WIDTH - 100, instructY + 120);

    setlinecolor(NEON_CYAN);
    setlinestyle(PS_SOLID, 2);
    rectangle(100, instructY, DISPLAY_WIDTH - 100, instructY + 120);

    settextcolor(TEXT_NEON);
    settextstyle(14, 0, "Consolas");
    int textY = instructY + 15;
    outtextxy(120, textY, "[NAVIGATION] Click menu items or use keyboard shortcuts");
    outtextxy(120, textY + 25, "[SIMULATION] [P] Pause | [+/-] Speed Control | [D] Debug Export");
    std::string hoverText = "[HOVERCRAFTS] " + std::string(patrolMode ? "3 cars (Patrol mode)" : std::to_string(unitCount) + " cars (adjustable)");
    outtextxy(120, textY + 50, hoverText.c_str());
    outtextxy(120, textY + 75, "[STATUS] All systems operational | Logging active");

    drawRadialMenu(-1);

    MOUSEMSG mouseMsg;
    bool menuActive = true;
    MenuAction result = MenuAction::EXIT;
    int hoveredItem = -1;

    while (menuActive) {
        bool needRedraw = false;

        if (MouseHit()) {
            mouseMsg = GetMouseMsg();

            int newHover = -1;
            for (int i = 0; i < 4; i++) {
                if (!menuItems[i].active) continue;

                int itemX = centerX + (int)(menuRadius * cos(menuItems[i].angle));
                int itemY = centerY + (int)(menuRadius * sin(menuItems[i].angle));

                int dx = mouseMsg.x - itemX;
                int dy = mouseMsg.y - itemY;
                if (dx * dx + dy * dy <= 60 * 60) {
                    newHover = i;
                    break;
                }
            }

            if (newHover != hoveredItem) {
                hoveredItem = newHover;
                needRedraw = true;
            }

            if (mouseMsg.uMsg == WM_LBUTTONDOWN && hoveredItem != -1) {
                result = menuItems[hoveredItem].action;

                if (result == MenuAction::CHANGE_CARS && !patrolMode) {
                    unitCount = (unitCount == 3) ? 5 : ((unitCount == 5) ? 7 : 3);

                    setfillcolor(BG_DARK);
                    solidrectangle(120, instructY + 65, 800, instructY + 90);
                    settextcolor(TEXT_NEON);
                    settextstyle(14, 0, "Consolas");
                    std::string updateText = "[HOVERCRAFTS] " + std::to_string(unitCount) + " cars (adjustable)";
                    outtextxy(120, instructY + 65, updateText.c_str());
                } else if (result != MenuAction::CHANGE_CARS) {
                    menuActive = false;
                }
            }
        }

        if (_kbhit()) {
            char key = toupper(_getch());
            switch (key) {
                case '1': result = MenuAction::BEGIN; menuActive = false; break;
                case '2': result = MenuAction::CHANGE_TASKS; menuActive = false; break;
                case '3':
                    if (!patrolMode) {
                        unitCount = (unitCount == 3) ? 5 : ((unitCount == 5) ? 7 : 3);
                        result = MenuAction::CHANGE_CARS;

                        setfillcolor(BG_DARK);
                        solidrectangle(120, instructY + 65, 800, instructY + 90);
                        settextcolor(TEXT_NEON);
                        settextstyle(14, 0, "Consolas");
                        std::string updateText = "[HOVERCRAFTS] " + std::to_string(unitCount) + " cars (adjustable)";
                    outtextxy(120, instructY + 65, updateText.c_str());
                    }
                    break;
                case '4': result = MenuAction::EXIT; menuActive = false; break;
            }
        }

        if (needRedraw) {

            setfillcolor(BG_DARK);
            solidrectangle(centerX - 250, centerY - 250, centerX + 250, centerY + 250);
            drawRadialMenu(hoveredItem);
        }

        Sleep(10);
    }

    cleardevice();

    return result;
}

} // 命名空间 warehouse
