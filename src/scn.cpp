/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/

#include "isa.h"
#include "consts.h"
#include "scn.h"
#include "scn_math.h"
#include "scn_gui.h"

#include "scn_gui.cpp"

isa_global struct bg_landscape
{
    u32_argb Color = { .U32 = (u32)PRUSSIAN_BLUE };
    i64 x, y;

} Bg;

// TODO(ingar): All of the static structs should proabably be part of
// the permanent memory so that they can be stored on disk
isa_global struct mouse_history
{
    bool LClicked = false;
    bool RClicked = false;

    scn_mouse_event Prev;
    scn_mouse_event PrevLClick;
    scn_mouse_event PrevRClick;

} MouseHistory;

isa_internal void
UpdateBg(u32_argb NewColor)
{
    Bg.Color = NewColor;    
}

isa_internal void 
UpdateBgColorWhenDragging(bg_landscape *Landscape, i64 x, i64 y)
{
   Landscape->x += x; 
   Landscape->y += y; 

   Landscape->Color.r += (u8)(x / 128);
   Landscape->Color.g += (u8)((x + y) / 128);
   Landscape->Color.b += (u8)(y / 128);
}

isa_internal scn_state *
InitScnState(scn_mem *Mem)
{
    scn_state *State = (scn_state *)Mem->Permanent;
    if(!Mem->Initialized)
    {
        State->Arena = IsaArenaCreate((u8 *)Mem->Permanent + sizeof(scn_state), Mem->PermanentMemSize - sizeof(scn_state)); 
        Mem->Initialized = true; 
    }

    return State;
}



// NOTE(ingar): Casey says that your code should not be split up in this way
// the code that updates state and then renders should be executed simultaneously
// so we might want to do that
// NOTE(ingar): This was also in the context of games. Sinuce we're a traditional app
// we might have different needs to handle events asynchronously

extern "C" RESPOND_TO_MOUSE(RespondToMouse)
{
    scn_state *ScnState = InitScnState(&Mem);

    // TODO(ingar): Convert to switch
    if(Event.Type == ScnMouseEvent_LDown)
    {
        MouseHistory.LClicked = true; 
    }
    else if(Event.Type == ScnMouseEvent_RDown)
    {
        MouseHistory.RClicked = true; 
    }

    if(Event.Type == ScnMouseEvent_LUp)
    {
        if(MouseHistory.Prev.Type == ScnMouseEvent_Move)
        {
            i64 PrevX = MouseHistory.PrevLClick.x;
            i64 PrevY = MouseHistory.PrevLClick.y;

            rect NewRectDim = { V2(0, 0), V2(0, 0) };

            if(PrevX < Event.x)
            {
                NewRectDim.min.x = (float)PrevX;
                NewRectDim.max.x = (float)Event.x;
            }
            else
            {
                NewRectDim.min.x = (float)Event.x;
                NewRectDim.max.x = (float)PrevX;
            }

            if(PrevY < Event.x)
            {
                NewRectDim.min.y = (float)PrevY;
                NewRectDim.max.y = (float)Event.y;
            }
            else
            {
                NewRectDim.min.y = (float)Event.y;
                NewRectDim.max.y = (float)PrevY;
            }
            
            // TODO(ingar): We need to push ui elements into memory which
            // UpdateBackBuffer then iterates over and draws
            // TODO(ingar): Since the functions are ran on timers, this
            // probably means that we need synchronization mechanisms so that
            // new elements are not pushed simultaneously with the drawing
            u32_argb NewRectColor;
            NewRectColor.U32 = GetRandu32();
            
            gui_rect *NewRect = IsaPushStructZero(&ScnState->Arena, gui_rect);
            NewRect->Dim = NewRectDim;
            //PushStruct(ScnState->Arena, NewRect);
                          
        }

        MouseHistory.LClicked = false; 
    }

    if(Event.Type == ScnMouseEvent_RUp) MouseHistory.RClicked = false; 

    MouseHistory.Prev = Event;
}

// NOTE(ingar): Man, this is overkill for this. Hoowee
extern "C" SEED_RAND_PCG(SeedRandPcg)
{
    SeedRandPcg_(Seed);
}

extern "C" UPDATE_BACK_BUFFER(UpdateBackBuffer)
{
    scn_state *ScnState = InitScnState(&Mem);

    /* Draw background */
    DrawRect(Buffer, 0, 0, Buffer.w, Buffer.h, Bg.Color); 
    
     
}

