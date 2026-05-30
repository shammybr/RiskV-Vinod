#pragma once
#include "SDLNTSC.h"
#include "SDL3/SDL.h"
#include <iostream>
#include <vector>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "NES.h"
#include <format>
#include "misc/cpp/imgui_stdlib.h"

SDLNTSC::SDLNTSC(int windowW, int windowH) : nesBuffer(NES_WIDTH* NES_HEIGHT, 0),
                    ntscOutput(NTSC_OUT_WIDTH* NTSC_OUT_HEIGHT, 0) 
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
    }

    else {
       

        window = SDL_CreateWindow("CAVALO NES", windowW, windowH, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, NULL);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        mem_edit.OptShowDataPreview = true;
        mem_edit.ReadOnly = false;

        gameRect = { (float)windowW / 4 , 0.0f, 640.0f, 480.0f };
        cpuRect = { 0.0f , (480.0f / 2) + 30 , 640.0f / 2, 480.0f };
        regFlags = 0;
        regFlags |= ImGuiWindowFlags_NoTitleBar;   
        regFlags |= ImGuiWindowFlags_NoResize;     
        regFlags |= ImGuiWindowFlags_NoMove;       
        regFlags |= ImGuiWindowFlags_NoCollapse;   
        regFlags |= ImGuiWindowFlags_NoScrollbar;

        regFlagsScroll = 0;
        regFlagsScroll |= ImGuiWindowFlags_NoTitleBar;
        regFlagsScroll |= ImGuiWindowFlags_NoResize;
        regFlagsScroll |= ImGuiWindowFlags_NoMove;
        regFlagsScroll |= ImGuiWindowFlags_NoCollapse;


        calculateLines();
     

        //vsync
        SDL_SetRenderVSync(renderer, 1);

        SDL_SetRenderLogicalPresentation(
            renderer,
            windowW,
            windowH,
            SDL_LOGICAL_PRESENTATION_LETTERBOX
        );
        texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            NTSC_OUT_WIDTH,
            NTSC_OUT_HEIGHT);

        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);

        CPUtexture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            NTSC_OUT_WIDTH,
            NTSC_OUT_HEIGHT);
        SDL_SetTextureScaleMode(CPUtexture, SDL_SCALEMODE_PIXELART);

        ntsc = (nes_ntsc_t*)malloc(sizeof(nes_ntsc_t));
        setup = nes_ntsc_composite; 
        nes_ntsc_init(ntsc, &setup);


       
        spec.freq = 44100;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 1;

       
        audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        SDL_ResumeAudioStreamDevice(audioStream);



        burst_phase = 0;

    }



}

void SDLNTSC::calculateLines() {

    lineLeft = { cpuRect.x + 20.0f , cpuRect.y , lineThicc, 100.0f };
    lineRight = { cpuRect.x + cpuRect.w - 30.0f , cpuRect.y , lineThicc, 100.0f };

    lineTop = { cpuRect.x + 20.0f , cpuRect.y , cpuRect.w - 50.0f + lineThicc ,  lineThicc };
    lineBottom = { cpuRect.x + 20.0f , cpuRect.y + 100.0f, cpuRect.w - 50.0f + lineThicc,  lineThicc };

    float startX = lineBottom.x + lineThicc;
    float endX = (lineBottom.x + lineBottom.w) - (lineThicc * 2.0f);
    float totalDistance = endX - startX;
    float step = totalDistance / 19.0f;

    for (int i = 0; i < 20; i++) {
        SDL_FRect pinTop = { startX + (i * step),
        lineLeft.y,
        lineThicc,
        -5.0f };

        cpuTopPins[i] = pinTop;

        SDL_FRect pinBot = { startX + (i * step),
        lineBottom.y + lineThicc,
        lineThicc,
        5.0f };

        cpuBotPins[i] = pinBot;




        cpuBotWires[i] = new SWire();
        cpuTopWires[i] = new SWire();

    }






    //RAM


    RAMlineLeft = { cpuRect.x + 50.0f , cpuRect.y + (cpuRect.h / 2), lineThicc, 50.0f };
    RAMlineRight = { cpuRect.x + cpuRect.w - 75.0f  , cpuRect.y + (cpuRect.h / 2), lineThicc, 50.0f };

    RAMlineTop = { cpuRect.x + 50.0f , cpuRect.y + (cpuRect.h / 2) , cpuRect.w - 125.0f + lineThicc ,  lineThicc };
    RAMlineBottom = { cpuRect.x + 50.0f , cpuRect.y + (cpuRect.h / 2) + 50.0f, cpuRect.w - 125.0f + lineThicc,  lineThicc };

    float RAMstartX = RAMlineBottom.x + lineThicc;
    float RAMendX = (RAMlineBottom.x + RAMlineBottom.w) - (lineThicc * 2.0f);
    float RAMtotalDistance = RAMendX - RAMstartX;
    float RAMstep = RAMtotalDistance / 11.0f;


    for (int i = 0; i < 12; i++) {

        SDL_FRect pinTop = { RAMstartX + (i * RAMstep),
        RAMlineLeft.y,
        lineThicc,
        -5.0f };
            
        RAMTopPins[i] = pinTop;


        SDL_FRect pinBot = { RAMstartX + (i * RAMstep),
               RAMlineBottom.y + lineThicc,
               lineThicc,
               5.0f };

        RAMBotPins[i] = pinBot;


    }



    //CPU WIRES


    //BOT
    float botWire3Y = 0;
    for (int i = 0; i < 8; i++) {
        SDL_FRect botWire = { (startX + ((10 - i) * step)) + (lineThicc / 2) - (wireThicc / 2),
        lineBottom.y + lineThicc + 15.0f,
        wireThicc,
        50.0f + (5 * i) };

        SDL_FRect botWire2 = { botWire.x,
        botWire.y + botWire.h,
        RAMTopPins[11 - i].x - botWire.x + wireThicc,
        wireThicc };

        botWire3Y = botWire2.y;

        SDL_FRect botWire3 = { RAMTopPins[11 - i].x + (lineThicc / 2) - (wireThicc / 2),
        botWire.y + botWire.h,
        wireThicc,
        RAMTopPins[11 - i].y - botWire2.y - 10.0f };


        cpuBotWires[10 - i]->insertLine(botWire);
        cpuBotWires[10 - i]->insertLine(botWire2);
        cpuBotWires[10 - i]->insertLine(botWire3);


    }


    //11
    SDL_FRect botWire = { startX + ((11) * step) + (lineThicc / 2) - (wireThicc / 2),
       lineBottom.y + lineThicc + 15.0f,
       wireThicc,
       50.0f - (5 ) };

    SDL_FRect botWire2 = { botWire.x,
    botWire.y + botWire.h,
    RAMBotPins[11].x - botWire.x + wireThicc + 20.0f,
    wireThicc };

    SDL_FRect botWire3 = { RAMBotPins[11].x + wireThicc + 20.0f,
    botWire.y + botWire.h,
    wireThicc,
    RAMBotPins[10].y - botWire2.y + 25.0f };

    SDL_FRect botWire4 = { RAMBotPins[11].x + (wireThicc * 2) + 20.0f,
    botWire3.y + botWire3.h,
    RAMBotPins[10].x - botWire3.x,
    wireThicc };

    SDL_FRect botWire5 = { RAMBotPins[10].x + (lineThicc / 2) - (wireThicc / 2) ,
    RAMBotPins[10].y + 10.0f,
    wireThicc,
    botWire4.y - RAMBotPins[11].y - 10.0f + wireThicc};

    cpuBotWires[11]->insertLine(botWire);
    cpuBotWires[11]->insertLine(botWire2);
    cpuBotWires[11]->insertLine(botWire3);
    cpuBotWires[11]->insertLine(botWire4);
    cpuBotWires[11]->insertLine(botWire5);


    //12
    botWire = { startX + ((12) * step) + (lineThicc / 2) - (wireThicc / 2),
    lineBottom.y + lineThicc + 15.0f,
    wireThicc,
    50.0f - (10) };

    botWire2 = { botWire.x,
    botWire.y + botWire.h,
    RAMBotPins[11].x - botWire.x + wireThicc + 30.0f,
    wireThicc };

    botWire3 = { RAMBotPins[11].x + wireThicc + 30.0f,
    botWire.y + botWire.h,
    wireThicc,
    RAMBotPins[9].y - botWire2.y + 35.0f };

    botWire4 = { RAMBotPins[11].x + (wireThicc * 2) + 30.0f,
    botWire3.y + botWire3.h,
    RAMBotPins[9].x - botWire3.x,
    wireThicc };

    botWire5 = { RAMBotPins[9].x + (lineThicc / 2) - (wireThicc / 2) ,
    RAMBotPins[9].y + 10.0f,
    wireThicc,
    botWire4.y - RAMBotPins[9].y - 10.0f + wireThicc };

    cpuBotWires[12]->insertLine(botWire);
    cpuBotWires[12]->insertLine(botWire2);
    cpuBotWires[12]->insertLine(botWire3);
    cpuBotWires[12]->insertLine(botWire4);
    cpuBotWires[12]->insertLine(botWire5);


    //13

    botWire = { startX + ((13) * step) + (lineThicc / 2) - (wireThicc / 2),
    lineBottom.y + lineThicc + 15.0f,
    wireThicc,
    50.0f - (15) };

    botWire2 = { botWire.x,
    botWire.y + botWire.h,
    RAMBotPins[11].x - botWire.x + wireThicc + 40.0f,
    wireThicc };

    botWire3 = { RAMBotPins[11].x + wireThicc + 40.0f,
    botWire.y + botWire.h,
    wireThicc,
    RAMBotPins[6].y - botWire2.y + 45.0f };

    botWire4 = { RAMBotPins[11].x + (wireThicc * 2) + 40.0f,
    botWire3.y + botWire3.h,
    RAMBotPins[6].x - botWire3.x,
    wireThicc };

    botWire5 = { RAMBotPins[6].x + (lineThicc / 2) - (wireThicc / 2) ,
    RAMBotPins[6].y + 10.0f,
    wireThicc,
    botWire4.y - RAMBotPins[10].y - 10.0f + wireThicc };

    cpuBotWires[13]->insertLine(botWire);
    cpuBotWires[13]->insertLine(botWire2);
    cpuBotWires[13]->insertLine(botWire3);
    cpuBotWires[13]->insertLine(botWire4);
    cpuBotWires[13]->insertLine(botWire5);


    //TOP
    //20 - 16

    int lastX = startX + ((19) * step) + (wireThicc / 2);

    for (int i = 0; i < 5; i++) {
        SDL_FRect topWire = { startX + ((19 - i) * step) + (wireThicc / 2),
        lineTop.y + lineThicc - 15.0f,
        wireThicc,
        -20.0f - (5 * i)};

        SDL_FRect topWire2 = { topWire.x,
        topWire.y + topWire.h,
        10.0f + (lastX - topWire.x) + (5 * i),
        wireThicc };

        SDL_FRect topWire3 = { topWire2.x + topWire2.w,
        topWire2.y,
        wireThicc ,
        RAMBotPins[4 - i].y - topWire2.y + 60.0f + (5 * i)};

        SDL_FRect topWire4 = { topWire3.x + wireThicc,
        topWire3.y + topWire3.h,
        RAMBotPins[4 - i].x - topWire3.x,
        wireThicc };

        SDL_FRect topWire5 = { RAMBotPins[4 - i].x + (lineThicc / 2) - (wireThicc / 2)  ,
        RAMBotPins[4 - i].y + 10.0f,
        wireThicc,
        topWire4.y - RAMBotPins[4].y  + wireThicc - 10.0f };

        cpuTopWires[19 - i]->insertLine(topWire);
        cpuTopWires[19 - i]->insertLine(topWire2);
        cpuTopWires[19 - i]->insertLine(topWire3);
        cpuTopWires[19 - i]->insertLine(topWire4);
        cpuTopWires[19 - i]->insertLine(topWire5);


    }
    
 

    
    for (int i = 0; i < 3; i++) {
        SDL_FRect topWire = { startX + ((14 - i) * step) + (lineThicc / 2) - (wireThicc / 2),
        lineTop.y + lineThicc - 15.0f,
        wireThicc,
        -20.0f + (5 * i)};

        SDL_FRect topWire2 = { topWire.x,
        topWire.y + topWire.h,
        cpuTopPins[0].x - topWire.x - 20.0f + (5 * i),
        wireThicc };

        SDL_FRect topWire3 = { topWire2.x + topWire2.w,
        topWire2.y,
        wireThicc,
        botWire3Y - topWire2.y + 15.0f - (5 * i)};

        SDL_FRect topWire4 = { topWire3.x,
        topWire3.y + topWire3.h,
        (RAMTopPins[1 + i].x + (lineThicc / 2) - (wireThicc / 2)) - topWire3.x,
        wireThicc };


        SDL_FRect topWire5 = { RAMTopPins[1 + i].x + (lineThicc / 2) - (wireThicc / 2) ,
        RAMTopPins[1 + i].y - 10.0f,
        wireThicc,
        topWire4.y - (RAMTopPins[1 + i].y - 10.0f) };



        cpuTopWires[14 - i]->insertLine(topWire);
        cpuTopWires[14 - i]->insertLine(topWire2);
        cpuTopWires[14 - i]->insertLine(topWire3);
        cpuTopWires[14 - i]->insertLine(topWire4);
        cpuTopWires[14 - i]->insertLine(topWire5);

    }
}


void SDLNTSC::quit(){
    free(ntsc);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLNTSC::draw(int frames){

   
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();



    ImGui::NewFrame();
    

    ImGui::SetNextWindowPos(ImVec2(5, 0), ImGuiCond_FirstUseEver);
    //ImGui::SetNextWindowSize(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin("CPU Registers", NULL, regFlags);
    ImGui::Text("Accumulator");

    ImVec2 defaultSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-3.0f, defaultSpacing.y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 80, 255));

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine(0.0f, 20.0f);
    float alignX = ImGui::GetCursorPosX();
    ImGui::Text(" ");

    char bits[8];
    for (int i = 0; i < 8; i++) {
        if (nes->regA & (1 << (7 - i))) {
            bits[i] = '1';
        }
        else {
            bits[i] = '0';
        }
    }


    for (int i = 0; i < 8; i++) {
        ImGui::SameLine();

  
        ImVec2 p = ImGui::GetCursorScreenPos();

     
        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { bits[i], '\0'};
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f 
        );

    
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);

      
        ImGui::Dummy(ImVec2(25, 25));
    }




    ImGui::Text("Prog. Counter");


    drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    ImGui::Text(" ");

    char bits16[16];

    for (int i = 0; i < 16; i++) {
        if (nes->regPC & (1 << (15 - i))) {
            bits16[i] = '1';
        }
        else {
            bits16[i] = '0';
        }
    }



    for (int i = 0; i < 16; i++) {
        if (i == 8) {
            ImGui::SetCursorPosX(alignX);
            ImGui::Text(" ");
        }
        ImGui::SameLine();


        ImVec2 p = ImGui::GetCursorScreenPos();


        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { bits16[i], '\0'};
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f
        );


        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);


        ImGui::Dummy(ImVec2(25, 25));
    }




    ImGui::Text("Stack Pointer");


    drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    ImGui::Text(" ");

    for (int i = 0; i < 8; i++) {
        if (nes->regSP & (1 << (7 - i))) {
            bits[i] = '1';
        }
        else {
            bits[i] = '0';
        }
    }



    for (int i = 0; i < 8; i++) {
        ImGui::SameLine();


        ImVec2 p = ImGui::GetCursorScreenPos();


        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { bits[i], '\0' };
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f
        );


        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);


        ImGui::Dummy(ImVec2(25, 25));
    }


    ImGui::Text("Status Register");


    drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    ImGui::Text(" ");

    for (int i = 0; i < 8; i++) {
        if (nes->regP & (1 << (7 - i))) {
            bits[i] = '1';
        }
        else {
            bits[i] = '0';
        }
    }



    for (int i = 0; i < 8; i++) {
        ImGui::SameLine();


        ImVec2 p = ImGui::GetCursorScreenPos();


        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { bits[i], '\0' };
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f
        );


        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);


        ImGui::Dummy(ImVec2(25, 25));
    }

    ImGui::Text("Reg X");


    drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    ImGui::Text(" ");

    for (int i = 0; i < 8; i++) {
        if (nes->regX & (1 << (7 - i))) {
            bits[i] = '1';
        }
        else {
            bits[i] = '0';
        }
    }



    for (int i = 0; i < 8; i++) {
        ImGui::SameLine();


        ImVec2 p = ImGui::GetCursorScreenPos();


        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { bits[i], '\0' };
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f
        );


        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);


        ImGui::Dummy(ImVec2(25, 25));
    }

    ImGui::Text("Reg Y");


    drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    ImGui::Text(" ");

    for (int i = 0; i < 8; i++) {
        if (nes->regY & (1 << (7 - i))) {
            bits[i] = '1';
        }
        else {
            bits[i] = '0';
        }
    }



    for (int i = 0; i < 8; i++) {
        ImGui::SameLine();


        ImVec2 p = ImGui::GetCursorScreenPos();


        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { bits[i], '\0' };
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f
        );


        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);


        ImGui::Dummy(ImVec2(25, 25));
    }

  


    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::End();



    ImGui::SetNextWindowPos(ImVec2(lineLeft.x + lineThicc, lineLeft.y + lineThicc), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(lineTop.w - (lineThicc * 2), lineLeft.h - lineThicc), ImGuiCond_FirstUseEver);
    ImGui::Begin("Addr & Data", NULL, regFlags);
    ImGui::Text("Address");


    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-3.0f, defaultSpacing.y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 80, 255));

    drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine(0.0f, 10.0f);
    alignX = ImGui::GetCursorPosX();
    ImGui::Text(" ");

    char sixTeenBits[16];
    for (int i = 0; i < 16; i++) {
        if (nes->addressBus & (1 << (15 - i))) {
            sixTeenBits[i] = '1';
        }
        else {
            sixTeenBits[i] = '0';
        }
    }


    for (int i = 0; i < 16; i++) {
        if(i == 8){
            ImGui::SetCursorPosX(alignX);
            ImGui::Text(" ");
        }

        ImGui::SameLine();
        ImVec2 p = ImGui::GetCursorScreenPos();


        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { sixTeenBits[i], '\0' };
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f
        );

        if(i > 4)
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);
        else
        drawList->AddText(textPos, IM_COL32(196, 64, 47, 255), label);

        ImGui::Dummy(ImVec2(25, 25));
    }



    ImGui::Text("Data");
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    drawList = ImGui::GetWindowDrawList();
    ImGui::Text(" ");

    for (int i = 0; i < 8; i++) {
        if (nes->cpuDataBus & (1 << (7 - i))) {
            bits[i] = '1';
        }
        else {
            bits[i] = '0';
        }
    }


    for (int i = 0; i < 8; i++) {
        ImGui::SameLine();
        ImVec2 p = ImGui::GetCursorScreenPos();


        drawList->AddRectFilled(p, ImVec2(p.x + 20, p.y + 20), IM_COL32(60, 60, 60, 255));


        char label[2] = { bits[i], '\0' };
        ImVec2 textSize = ImGui::CalcTextSize(label);


        ImVec2 textPos = ImVec2(
            p.x + (20.0f - textSize.x) * 0.5f,
            p.y + (20.0f - textSize.y) * 0.5f + 1.0f
        );


        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), label);

        ImGui::Dummy(ImVec2(25, 25));
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(gameRect.x + gameRect.w, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(315, 30), ImGuiCond_FirstUseEver);

    ImGui::Begin("FPS", NULL, regFlags);
    ImGui::Text("FPS: %i", frames);



    ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(gameRect.x, gameRect.h), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2((gameRect.w / 2), 240), ImGuiCond_FirstUseEver);
  
    ImGui::Begin("Histórico", NULL, regFlags);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-3.0f, defaultSpacing.y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 80, 255));

    drawList = ImGui::GetWindowDrawList();

    alignX = ImGui::GetCursorPosX();
    for (int i = 0; i < 9; i++) {
        
        ImGui::Text(nes->history[(nes->historyN + i) % 9]);
    }


    


    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();


    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(gameRect.x + (gameRect.w / 2), gameRect.h), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(gameRect.w / 2, 240), ImGuiCond_FirstUseEver);
    ImGui::Begin("MICROOP", NULL, regFlags);
    ImGui::SetCursorPos(ImVec2(57.0f, 10.0f));

    if (nes->logMicroOps) {
        if (ImGui::Button("Micro Operations", ImVec2(200.0f, 50.0f))) {
            nes->logMicroOps = false;
        }
    }
    else {
        if (ImGui::Button("Macro Operations", ImVec2(200.0f, 50.0f))) {
            nes->logMicroOps = true;
        }
    }

    ImGui::Text("Play - F1");
    ImGui::Text("Frame Mode - F2");
    ImGui::Text("Next Step - F3");
    ImGui::Text("Next Frame - F4");

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(gameRect.x + gameRect.w, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(315, 350), ImGuiCond_FirstUseEver);
    ImGui::Begin("Memory", NULL, regFlagsScroll);

    if (ImGui::BeginTabBar("MemoryTabs")) {

        drawList = ImGui::GetWindowDrawList();

        if (ImGui::BeginTabItem("Zero Page")) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Quick RAM ($0000 - $00FF)");
            mem_edit.Cols = 4;

            mem_edit.DrawContents(nes->memory , 256, 0);

            ImGui::EndTabItem();
        }


        if (ImGui::BeginTabItem("Stack")) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Memory Stack ($0100 - $01FF)");
            mem_edit.Cols = 4;

            mem_edit.DrawContents(nes->memory + 0x0100, 256, 0x0100);

            ImGui::EndTabItem();
        }


        if (ImGui::BeginTabItem("General RAM")) {

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Internal RAM ($0200 - $07FF)");


            mem_edit.Cols = 4;

            mem_edit.DrawContents(nes->memory + 0x0200, 1536, 0x0200);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ROM")) {

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "ROM ($8000 - $BFFF)");


            mem_edit.Cols = 4;

            mem_edit.DrawContents(nes->currentRom->vPRGMemory.data(), nes->currentRom->vPRGMemory.size(), 0x8000);

            ImGui::EndTabItem();
        }



        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(gameRect.x + gameRect.w, 380), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(315, 340), ImGuiCond_FirstUseEver);
    ImGui::Begin("Roms", NULL, regFlagsScroll);

    if (ImGui::Button("Mario", ImVec2(70.0f, 30.0f))) {
        if (nes->currentRom->romName != "/mario."){
            nes->loadRom("ROM/mario.nes");
            isCustomMode = false;
        }
   
    }
    ImGui::SameLine();
    if (ImGui::Button("DragonQuest", ImVec2(120.0f, 30.0f))) {
        if (nes->currentRom->romName != "/Dragon Quest III.") {
            nes->loadRom("ROM/Dragon Quest III.nes");
            isCustomMode = false;
        }

    }

    ImGui::SameLine();
    if (ImGui::Button("Custom", ImVec2(70.0f, 30.0f))) {
        if (nes->currentRom->romName != "/custom.") {
       //     nes->loadCustom("ROM/Custom.nes");
            isCustomMode = true;
        }
    }

   

    if (isCustomMode) {
        char chrBuffer[5] = "";
        snprintf(chrBuffer, sizeof(chrBuffer), "%02X", savedChrBanks);

        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::InputText("CHR Banks", chrBuffer, IM_ARRAYSIZE(chrBuffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {

         
            if (chrBuffer[0] == '\0') {
                savedChrBanks = 0;
            }
            else {
                savedChrBanks = (uint8_t)strtol(chrBuffer, nullptr, 16);
            }
        }


        char flag6Buffer[5] = "";
        snprintf(flag6Buffer, sizeof(flag6Buffer), "%02X", savedFlags6);

        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::InputText("Flag 6", flag6Buffer, IM_ARRAYSIZE(flag6Buffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {


            if (flag6Buffer[0] == '\0') {
                savedChrBanks = 0;
            }
            else {
                savedChrBanks = (uint8_t)strtol(flag6Buffer, nullptr, 16);
            }
        }

        ImGui::SameLine();



        char flag7Buffer[5] = "";
        snprintf(flag7Buffer, sizeof(flag7Buffer), "%02X", savedFlags7);

        ImGui::SetNextItemWidth(50.0f);
        if (ImGui::InputText("Flag 7", flag7Buffer, IM_ARRAYSIZE(flag7Buffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {


            if (flag7Buffer[0] == '\0') {
                savedChrBanks = 0;
            }
            else {
                savedChrBanks = (uint8_t)strtol(flag7Buffer, nullptr, 16);
            }
        }


        int itemToDelete = -1;
        int itemToInsertAfter = -1;


        ImGui::BeginChild("InstructionList", ImVec2(0, 300.0f), true, ImGuiWindowFlags_HorizontalScrollbar);


        ImGuiListClipper clipper;
        clipper.Begin((int)customOps.size());


        while (clipper.Step()) {
            for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; index++) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%02d", index + 1);
                ImGui::SameLine(0.0f, 2.0f);


                MacroOp& operation = customOps[index];
                ImGui::SetNextItemWidth(120.0f);

                for (int iArgs = 0; iArgs < operation.argAmount; iArgs++) {
             
                    ImGui::PushID((index * 10) + iArgs);
           
                    if (iArgs == 0) {
                        if (operation.opName != "DATA") {



                            if (ImGui::BeginCombo("", operation.opName)) {
                                static char searchBuffer[8] = "";

                                if (ImGui::IsWindowAppearing()) {
                                    ImGui::SetKeyboardFocusHere();
                                }
                                ImGui::SetNextItemWidth(100.0f);

                                ImGui::InputText("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer), ImGuiInputTextFlags_CharsUppercase);
                                ImGui::Separator(); // D


                                for (int i = 0; i < 151; i++) {
                                    if (searchBuffer[0] == '\0' || strstr(macroOps[i].opName, searchBuffer) != nullptr) {
                                        if (ImGui::Selectable(macroOps[i].opName, false, 0, ImVec2(100.0f, 20.0f))) {
                                            customOps[index] = macroOps[i];
                                            searchBuffer[0] = '\0';
                                        }
                                    }
                                }
                                ImGui::EndCombo();
                            }
                        }
                        else {

                            ImGui::Button("RAW DATA", ImVec2(120.0f, 0.0f));

                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(30.0f);

                            char hexBuffer[5] = "";
                            if (operation.opArgs[0] >= 0) {
                                snprintf(hexBuffer, sizeof(hexBuffer), "%02X", operation.opArgs[0]);
                            }

                            ImGui::PushID("DataEdit");
                            if (ImGui::InputText("", hexBuffer, IM_ARRAYSIZE(hexBuffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {
                                if (hexBuffer[0] == '\0') {
                                    operation.opArgs[0] = 0;
                                }
                                else {
                                    operation.opArgs[0] = (int)strtol(hexBuffer, nullptr, 16);
                                }
                            }
                            ImGui::PopID();
                        }
                    }
                    else {
                   
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(30.0f);
                        char hexBuffer[5] = "";

                        if (operation.opArgs[iArgs] >= 0) {
                            snprintf(hexBuffer, sizeof(hexBuffer), "%02X", operation.opArgs[iArgs]);
                        }

                        if (ImGui::InputText("", hexBuffer, IM_ARRAYSIZE(hexBuffer), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {
                            if (hexBuffer[0] == '\0') {
                                operation.opArgs[iArgs] = 0;
                            }
                            else {
                                operation.opArgs[iArgs] = (int)strtol(hexBuffer, nullptr, 16);
                            }
                        }
                    }
                    ImGui::PopID();
                }

           
                ImGui::PushID((index * 10) + 8);
                ImGui::SameLine();
                if (ImGui::Button("+", ImVec2(15.0f, 30.0f))) {
                    itemToInsertAfter = index;
                }
                ImGui::PopID();

             
                ImGui::PushID((index * 10) + 9);
                ImGui::SameLine();
                if (ImGui::Button("-", ImVec2(15.0f, 30.0f))) {
                    itemToDelete = index;
                }
                ImGui::PopID();
            }
        }

    
        clipper.End();
        ImGui::EndChild();

      
        if (itemToDelete != -1) {
            customOps.erase(customOps.begin() + itemToDelete);
        }
        if (itemToInsertAfter != -1) {
            customOps.insert(customOps.begin() + itemToInsertAfter + 1, { "",  {-1, -1, -1}, 1 });
        }

        if (customOps.size() == 0) {
            ImGui::SetNextItemWidth(130.0f);
            if (ImGui::BeginCombo("+", "")) {
                for (int i = 0; i < 151; i++) {
                    if (ImGui::Selectable(macroOps[i].opName, false, 0, ImVec2(100.0f, 20.0f))) {
                        customOps.push_back(macroOps[i]);
                        break;
                    }
                }
                ImGui::EndCombo();
            }
        }



        if (ImGui::Button("Save")) {
            isSaving = true;
            isLoading = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Load")) {
            isLoading = true;
            isSaving = false;
            files.clear();

            std::string path = "ROM/";
            try {
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    if (entry.path().extension() == ".nes") {
                        files.push_back(entry.path().filename().string());
                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                printf("Failed to load ROMs!\n");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Compile & Run")) {
            std::vector<uint8_t> customCode;

            for (auto& operation : customOps) {
                if (operation.opName != "") {
                    for (int i = 0; i < operation.argAmount; i++) {
                        customCode.push_back(operation.opArgs[i]);

                        char hexBuffer[5] = "";
                        snprintf(hexBuffer, sizeof(hexBuffer), "%02X", operation.opArgs[i]);
                        printf("Operation: %s\n", hexBuffer);
                    }
                }
            }


            savedChrRom.clear();

            nes->makeRom("ROM/Custom.nes", customCode, savedChrBanks, savedChrRom, savedFlags6, savedFlags7);
            nes->loadRom("ROM/Custom.nes");
        }
    }

   // ImGui::ShowMetricsWindow();
    ImGui::End();

    if (isSaving) {
        ImGui::SetNextWindowPos(ImVec2(gameRect.x, gameRect.h / 2), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(gameRect.w, 100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Save", NULL, regFlagsScroll);


        saveName;

        ImGui::InputText("File Name", &saveName, ImGuiInputTextFlags_None);

        if (ImGui::Button("Save")) {
            std::vector<uint8_t> customCode;

            for (auto& operation : customOps) {
                if (operation.opName != "") {
                    for (int i = 0; i < operation.argAmount; i++) {
                        customCode.push_back(operation.opArgs[i]);

                        char hexBuffer[5] = "";
                        snprintf(hexBuffer, sizeof(hexBuffer), "%02X", operation.opArgs[i]);
                        printf("Operation: %s", hexBuffer);

                    }


                }
            }
            std::string fullName = "ROM/";
            fullName.append(saveName);
            fullName.append(".nes");
            nes->makeRom(fullName.c_str(), customCode, savedChrBanks, savedChrRom, savedFlags6, savedFlags7);
            isSaving = false;
            saveName = "";

        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            isSaving = false;
        }
        ImGui::End();
    }
    else if (isLoading) {
        ImGui::SetNextWindowPos(ImVec2(gameRect.x, gameRect.h / 2), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(gameRect.w, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Load", NULL, regFlagsScroll);




       

        int k = 0;
        for (auto& file : files) {
            ImGui::PushID(k);

            if (ImGui::Button(file.c_str())) {
                std::string fullName = "ROM/";
                fullName.append(file);
                std::vector<uint8_t> customCode = loadCustomCode(fullName);
                customOps.clear();

                for (int i = 0; i < customCode.size();) {
                 //   if (customCode[i] != 0x0) {
                    if (i + 3 < customCode.size() &&
                        customCode[i] == 0xEA &&
                        customCode[i + 1] == 0xEA &&
                        customCode[i + 2] == 0xEA &&
                        customCode[i + 3] == 0xEA){
                        break;
                    }


                        auto ptr = std::find_if(std::begin(macroOps), std::end(macroOps),
                        [&](const MacroOp& op) { return op.opArgs[0] == customCode[i]; });
                   
                        if (ptr != std::end(macroOps)) {
                            customOps.push_back(*ptr);
                            i++;

                            int argsToRead = ptr->argAmount - 1;
                            for (int amount = 1; amount <= argsToRead; amount++) {

                              
                                if (i < customCode.size()) {

                                    customOps.back().opArgs[amount] = customCode[i];
                                    i++;
                                }
                            }


                        }
                        else {
                            printf("Operation not found: %02X\n", customCode[i]);
                            MacroOp unknownData = { "DATA", {customCode[i], -1, -1}, 1 };
                            customOps.push_back(unknownData);
                            i++;
                        }


                       


                 //   }
                //    else {
                //        break;
                 //   }

                }
               
           //     nes->loadRom(fullName.c_str());
                isLoading = false;

            }
            ImGui::PopID();
            k++;
        }

       

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
           
            isLoading = false;
        }
        ImGui::End();




    }
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);











  
    DrawGame();

    DrawCPU();



    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);


    SDL_RenderPresent(renderer);
}

void SDLNTSC::DrawGame() {

    burst_phase ^= 1;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);


    SDL_RenderClear(renderer);
    nes_ntsc_blit(ntsc,
        nesBuffer.data(), NES_WIDTH,
        burst_phase,
        NES_WIDTH, NES_HEIGHT,
        ntscOutput.data(),
        NTSC_OUT_WIDTH * sizeof(uint16_t));

    SDL_UpdateTexture(texture, NULL, ntscOutput.data(), NTSC_OUT_WIDTH * sizeof(uint16_t));

    ImGui::SetNextWindowPos(ImVec2(gameRect.x, 0), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Display", NULL, regFlags);

    ImVec2 pos = ImGui::GetCursorScreenPos();

  
    float scale = 2.0f;


    float finalHeight = 240.0f * scale;


    float finalWidth = finalHeight * (4.0f / 3.0f);

    ImVec2 size = ImVec2(finalWidth, finalHeight);

    ImGui::Image((ImTextureID)texture, size);


    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 scanlineColor = IM_COL32(0, 0, 0, 60);


    for (int y = 0; y < size.y; y += 2) {
        drawList->AddLine(
            ImVec2(pos.x, pos.y + y),          
            ImVec2(pos.x + size.x, pos.y + y), 
            scanlineColor
        );
    }

    ImGui::End();
    ImGui::PopStyleVar(); 


}

void SDLNTSC::DrawCPU() {


    SDL_SetRenderDrawColor(renderer, 138, 138, 138, 60); 



  //  SDL_FRect topRect = { cpuRect.x , cpuRect.y, cpuRect.w, lineThicc };
   // SDL_FRect bopRect = { cpuRect.x , cpuRect.y + (cpuRect.h) - lineThicc, cpuRect.w, lineThicc };
    //SDL_RenderFillRect(renderer, &topRect);
   // SDL_RenderFillRect(renderer, &bopRect);


    SDL_SetRenderDrawColor(renderer, 138, 138, 138, 255);


    SDL_RenderFillRect(renderer, &lineLeft);
    SDL_RenderFillRect(renderer, &lineRight);
    SDL_RenderFillRect(renderer, &lineTop);
    SDL_RenderFillRect(renderer, &lineBottom);

    float startX = lineBottom.x + lineThicc;
    float endX = (lineBottom.x + lineBottom.w) - (lineThicc * 2.0f);
    float totalDistance = endX - startX;
    float step = totalDistance / 19.0f;


    for (int i = 0; i < 20; i++) {

        SDL_RenderFillRect(renderer, &cpuTopPins[i]);
        SDL_RenderFillRect(renderer, &cpuBotPins[i]);




    }
    








    SDL_RenderFillRect(renderer, &RAMlineLeft);
    SDL_RenderFillRect(renderer, &RAMlineRight);
    SDL_RenderFillRect(renderer, &RAMlineTop);
    SDL_RenderFillRect(renderer, &RAMlineBottom);





    for (int i = 0; i < 12; i++) {
      
        SDL_RenderFillRect(renderer, &RAMTopPins[i]);
        SDL_RenderFillRect(renderer, &RAMBotPins[i]);

    }




    //Wirezz
    //Address
    //OFF
    SDL_SetRenderDrawColor(renderer, 213, 213, 157, 100);

    for (int i = 3; i < 20; i++) {



       
        NoWire* prox = cpuBotWires[i]->firstLine;
        while (prox != NULL) {

  
            if (!(nes->addressBus & (1 << i - 3)))
            SDL_RenderFillRect(renderer, &prox->wire);

            prox = prox->prox;
        }
           
                    


    }

    //ON
     
    SDL_SetRenderDrawColor(renderer, 213, 213, 157, 255);

    for (int i = 3; i < 20; i++) {




        NoWire* prox = cpuBotWires[i]->firstLine;
        while (prox != NULL) {


            if (nes->addressBus & (1 << i - 3))
            SDL_RenderFillRect(renderer, &prox->wire);

            prox = prox->prox;
        }



    }
    
    //data
    //OFF
    SDL_SetRenderDrawColor(renderer, 130, 193, 193, 100);

    for (int i = 12; i < 20; i++) {

      


        NoWire* prox = cpuTopWires[i]->firstLine;
        while (prox != NULL) {
            if (!(nes->cpuDataBus & (1 << i - 12)))
            SDL_RenderFillRect(renderer, &prox->wire);

            prox = prox->prox;
        }



    }

    //ON
    SDL_SetRenderDrawColor(renderer, 130, 193, 193, 255);

    for (int i = 12; i < 20; i++) {




        NoWire* prox = cpuTopWires[i]->firstLine;
        while (prox != NULL) {
            if (nes->cpuDataBus & (1 << i - 12))
                SDL_RenderFillRect(renderer, &prox->wire);

            prox = prox->prox;
        }



    }


}


bool SDLNTSC::poll(uint8_t *controller, std::vector<float>& audioBuffer) {
    while (SDL_PollEvent(&event)) {
        SDL_ConvertEventToRenderCoordinates(renderer, &event);
        ImGui_ImplSDL3_ProcessEvent(&event);


        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            bool pressed = (event.type == SDL_EVENT_KEY_DOWN);
            uint8_t bitmask = 0;

            
            switch (event.key.key) {
            case SDLK_J:            bitmask = 0b10000000; break; // A
            case SDLK_K:            bitmask = 0b01000000; break; // B
            case SDLK_RSHIFT:  bitmask = 0b00100000; break; // Select
            case SDLK_RETURN:       bitmask = 0b00010000; break; // Start
            case SDLK_W:           bitmask = 0b00001000; break; // Up
            case SDLK_S:         bitmask = 0b00000100; break; // Down
            case SDLK_A:         bitmask = 0b00000010; break; // Left
            case SDLK_D:        bitmask = 0b00000001; break; // Right

            case SDLK_F5:
                if (pressed) { if (nes->frameMode) { ppu->pixelMode = true; };  nes->canStep = true; }
                break;


            }


            if (pressed) {
                controller[0] |= bitmask;  
            }
            else {
                controller[0] &= ~bitmask; 
            }

            if (!event.key.repeat) {
                switch (event.key.key) {

                case SDLK_F1:
                    if (pressed) { nes->frameMode = false; nes->canStep = true; }
                    break;
                case SDLK_F2:
                    if (pressed) { nes->frameMode = true ;  ppu->pixelMode = false; nes->canStep = true; }
                    break;
                case SDLK_F3:
                    if (pressed) { nes->stepWholeFrame = false;  ppu->pixelMode = false; nes->canStep = true; }
                    break;
                case SDLK_F4:
                    if (pressed) { nes->stepWholeFrame = true;  ppu->pixelMode = false; nes->canStep = true; }
                    break;

                }
            }


        }




        else if (event.type == SDL_EVENT_QUIT) {
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();

            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }
    }

    return true;
}

void SDLNTSC::playAudio(std::vector<float>& audioBuffer) {

    if (audioBuffer.size() > 0) {

        int max_queued_bytes = 4410 * sizeof(float);

        int currently_queued = SDL_GetAudioStreamQueued(audioStream);

        if (currently_queued < max_queued_bytes) {


            SDL_PutAudioStreamData(audioStream,
                audioBuffer.data(),
                audioBuffer.size() * sizeof(float));

        }

        audioBuffer.clear();
    }
}

void SDLNTSC::setNes(NES* _nes){
    nes = _nes;
    ppu = _nes->ppu;

}

std::vector<uint8_t> SDLNTSC::loadCustomCode(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};

    file.seekg(0, std::ios::beg);

    //header
    std::vector<uint8_t> header(16);
    file.read((char*)header.data(), 16);

    // Byte 4 is PRG banks (Code), Byte 5 is CHR banks (Graphics)
    int prgBanks = header[4];
    savedChrBanks = header[5];
    savedFlags6 = header[6];
    savedFlags7 = header[7];

    int prgBytes = prgBanks * 16384;
    int chrBytes = savedChrBanks * 8192;


    std::vector<uint8_t> prgData(prgBytes);
    file.read((char*)prgData.data(), prgBytes);

    // Graphics
    savedChrRom.resize(chrBytes);
    if (chrBytes > 0) {
        file.read((char*)savedChrRom.data(), chrBytes);
    }


    return prgData;
}