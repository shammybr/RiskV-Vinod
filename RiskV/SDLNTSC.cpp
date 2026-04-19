#pragma once
#include "SDLNTSC.h"
#include "SDL3/SDL.h"
#include <iostream>
#include <vector>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "NES.h"

SDLNTSC::SDLNTSC(int windowW, int windowH) : nesBuffer(NES_WIDTH* NES_HEIGHT, 0),
                    ntscOutput(NTSC_OUT_WIDTH* NTSC_OUT_HEIGHT, 0) 
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
    }

    else {
       

        window = SDL_CreateWindow("CAVALO NES", windowW, windowH, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, NULL);
                
        gameRect = { (float)windowW / 4 , 0.0f, 640.0f, 480.0f };
        cpuRect = { 0.0f , 480.0f / 2 , 640.0f / 2, 480.0f };

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

    lineLeft = { cpuRect.x + 25.0f , cpuRect.y , lineThicc, 100.0f };
    lineRight = { cpuRect.x + cpuRect.w - 25.0f , cpuRect.y , lineThicc, 100.0f };

    lineTop = { cpuRect.x + 25.0f , cpuRect.y , cpuRect.w - 50.0f + lineThicc ,  lineThicc };
    lineBottom = { cpuRect.x + 25.0f , cpuRect.y + 100.0f, cpuRect.w - 50.0f + lineThicc,  lineThicc };

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



    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_FirstUseEver);
  //  ImGui::SetNextWindowSize(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin("CPU Registers");
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


    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::Text("P. Counter");

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(-3.0f, defaultSpacing.y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 80, 255));

    drawList = ImGui::GetWindowDrawList();
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    ImGui::Text(" ");

    for (int i = 0; i < 8; i++) {
        if (nes->regPC & (1 << (7 - i))) {
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

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    if (ImGui::Button("FPS:")) {
      
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(alignX);
    ImGui::Text(" %i", frames);
    ImGui::End();


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
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, &gameRect);


    //scanlines
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 60);

    for (int y = 0; y < gameRect.h; y += 2) {
        SDL_RenderLine(renderer, gameRect.x, gameRect.y + (float)y, gameRect.x + gameRect.w, gameRect.y + (float)y);
    }



}

void SDLNTSC::DrawCPU() {


    SDL_SetRenderDrawColor(renderer, 138, 138, 138, 60); 



    SDL_FRect topRect = { cpuRect.x , cpuRect.y, cpuRect.w, lineThicc };
    SDL_FRect bopRect = { cpuRect.x , cpuRect.y + (cpuRect.h) - lineThicc, cpuRect.w, lineThicc };
    SDL_RenderFillRect(renderer, &topRect);
    SDL_RenderFillRect(renderer, &bopRect);


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
           
            
        prox = cpuTopWires[i]->firstLine;
        while (prox != NULL) {
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
    SDL_SetRenderDrawColor(renderer, 130, 193, 193, 255);

    for (int i = 0; i < 20; i++) {

      


        NoWire* prox = cpuTopWires[i]->firstLine;
        while (prox != NULL) {
            SDL_RenderFillRect(renderer, &prox->wire);

            prox = prox->prox;
        }



    }




}


bool SDLNTSC::poll(uint8_t *controller, std::vector<float>& audioBuffer) {
    while (SDL_PollEvent(&event)) {
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
            }

            if (pressed) {
                controller[0] |= bitmask;  
            }
            else {
                controller[0] &= ~bitmask; 
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
