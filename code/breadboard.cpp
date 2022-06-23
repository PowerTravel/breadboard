
#if 0

  Example of how to build a temporrary bitmap, upload to card and render


rect2f PlotToBitmap( bitmap* Bitmap, r32 ChartXMin, r32 ChartXMax, r32 ChartYMin, r32 ChartYMax, r32 XValue, r32 YValue, s32 LineSize, v4 Color)
{
  if(XValue < ChartXMin || XValue >= ChartXMax ||
     YValue < ChartYMin || YValue >= ChartYMax)
  {
    return {};
  }

  r32 XPercent = (XValue-ChartXMin) / (ChartXMax - ChartXMin);
  r32 YPercent = (YValue-ChartYMin) / (ChartYMax - ChartYMin);
  s32 PixelXValue = (s32) Round(XPercent*Bitmap->Width);
  s32 PixelYValue = (s32) Round(YPercent*Bitmap->Height);

  u8 Alpha = (u8) (Color.W * 255.f);
  u8 Blue = (u8) (Color.X * 255.f);
  u8 Green = (u8) (Color.Y * 255.f);
  u8 Red = (u8) (Color.Z * 255.f);

  u32 PixelData = (Blue << 0) | (Green << 8) | (Red << 16) | Alpha << 24;

  r32 X = (r32) Maximum(0, PixelXValue - LineSize);
  r32 Y = (r32) Maximum(0, PixelYValue - LineSize);
  r32 W = (r32) Minimum(PixelXValue + LineSize, 2*LineSize);
  r32 H = (r32) Minimum(PixelYValue + LineSize, 2*LineSize);

  W = (r32) Minimum(Bitmap->Width-X, W);
  H = (r32) Minimum(Bitmap->Height-Y, H);

  rect2f Result = Rect2f(X,Y,W,H);
  u32* Pixels = (u32*) Bitmap->Pixels;

  for( s32 YPixel = (s32) Y;
           YPixel < (s32)(Y+H);
         ++YPixel)
  {
    for( s32 XPixel = (s32)X;
             XPixel < (s32)(X+W);
           ++XPixel)
    {
      u32 PixelIndex = YPixel * Bitmap->Width + XPixel;
      u32* Pixel = Pixels + PixelIndex;
      *Pixel = 0;
    }
  }

  for( s32 YPixel = (s32) Y;
           YPixel < (s32)(Y+H);
         ++YPixel)
  {
    for( s32 XPixel = (s32)X;
             XPixel < (s32)(X+W);
           ++XPixel)
    {
      u32 PixelIndex = YPixel * Bitmap->Width + XPixel;
      u32* Pixel = Pixels + PixelIndex;
      *Pixel = PixelData;
    }
  }

  return Result;
}


  bitmap_handle Plot;
  GetHandle(GlobalGameState->AssetManager, "energy_plot", &Plot);
  bitmap* PlotBitMap = GetAsset(GlobalGameState->AssetManager, Plot);

  BeginScopedEntityManagerMemory();
  component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_DYNAMICS);
  rect2f UpdatedRegion {};
  u32 PointSize = 1;

  r32 Ek = 0;
  r32 Er = 0;
  r32 Ep = 0;
  while(Next(EM, ComponentList))
  {
    component_spatial* Spatial = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL);
    component_dynamics* Dynamics = (component_dynamics*) GetComponent(EM, ComponentList, COMPONENT_FLAG_DYNAMICS);

    Ek += Dynamics->Mass *(Dynamics->LinearVelocity * Dynamics->LinearVelocity) * 0.5f;
    Er += Dynamics->Mass *(Dynamics->AngularVelocity * Dynamics->AngularVelocity) * 0.5f;
    Ep += Dynamics->Mass * 9.82f * (Spatial->Position.Y+1);
  }

  r32 Cycles = Floor(World->GlobalTimeSec/20.f);
  r32 Remainder = World->GlobalTimeSec - 20*Cycles;

  rect2f TmpRegion1 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Er, PointSize, HexCodeToColorV4(0xff0000));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion1);
  rect2f TmpRegion2 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Ek, PointSize, HexCodeToColorV4(0x00ff00));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion2);
  rect2f TmpRegion4 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Ep, PointSize, HexCodeToColorV4(0x0000ff));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion4);
  rect2f TmpRegion5 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Ep+Ek+Er, PointSize, HexCodeToColorV4(0xff00FF));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion5);

  Reupload(GlobalGameState->AssetManager, Plot, UpdatedRegion);
#endif

#include "breadboard.h"

#include "assets.h"
#include "tiles_spritesheet.h"

#if HANDMADE_INTERNAL
game_memory* DebugGlobalMemory = 0;
#endif

#include "math/aabb.cpp"
#include "obj_loader.cpp"
#include "render_push_buffer.cpp"
#include "entity_components_backend.cpp"
#include "breadboard_entity_components.cpp"
#include "system_controller.cpp"
#include "system_camera.cpp"
#include "system_sprite_animation.cpp"
#include "assets.cpp"
#include "asset_loading.cpp"
#include "menu_interface.cpp"
#include "breadboard_tile.cpp"
#include "containers/chunk_list.cpp"
#include "containers/chunk_list_unit_tests.h"
#include "entity_components_backend_unit_tests.h"

#include "debug.h"



internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{

  local_persist r32 tSine = 0.f;

  s16 ToneVolume = 2000;
  int SamplesPerPeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  r32 twopi = (r32)(2.0*Pi32);

  s16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0;
  SampleIndex < SoundBuffer->SampleCount;
    ++SampleIndex)
  {

#if 0
    r32 SineValue = Sin(tSine);
    s16 SampleValue = (s16)(SineValue * ToneVolume);

    tSine += twopi / (r32)SamplesPerPeriod;
    if (tSine > twopi)
    {
      tSine -= twopi;
    }
#else
    s16 SampleValue = 0;
#endif
    *SampleOut++ = SampleValue; // Left Speker
    *SampleOut++ = SampleValue; // Rright speaker
  }
}

world* CreateWorld( )
{
  world* World = PushStruct(GlobalGameState->PersistentArena, world);
  InitializeTileMap( &World->TileMap );
  World->Arena = GlobalGameState->PersistentArena;

  electrical_component* Source = PushStruct(World->Arena, electrical_component);
  Source->Type = ElectricalComponentType_Source;
  electrical_component* res    = PushStruct(World->Arena, electrical_component);
  res->Type = ElectricalComponentType_Resistor;
  electrical_component* l      = PushStruct(World->Arena, electrical_component);
  l->Type = ElectricalComponentType_Led_Red;
  electrical_component* gr     = PushStruct(World->Arena, electrical_component);
  gr->Type = ElectricalComponentType_Ground;

  ConnectPin(Source, ElectricalPinType_Output, res, ElectricalPinType_A);
  ConnectPin(res, ElectricalPinType_B, l, ElectricalPinType_Positive);
  ConnectPin(l, ElectricalPinType_Negative, gr, ElectricalPinType_Input);

  World->Source = Source;

  return World;
}

void InitiateGame(game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  if (Memory->GameState)
  {
    // Game is valid
    Assert(RenderCommands->OverlayGroup);
    return;
  }

  GlobalGameState = BootstrapPushStruct(game_state, PersistentArena);
  GlobalGameState->TransientArena = PushStruct(GlobalGameState->PersistentArena, memory_arena);
  GlobalGameState->TransientTempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);

  GlobalGameState->FunctionPool = PushStruct(GlobalGameState->PersistentArena, function_pool);

  GlobalGameState->RenderCommands = RenderCommands;

  GlobalGameState->AssetManager  = CreateAssetManager();
  GlobalGameState->EntityManager = CreateEntityManager();
  GlobalGameState->MenuInterface = CreateMenuInterface(GlobalGameState->PersistentArena, Megabytes(1));

  GlobalGameState->World = CreateWorld();

  GlobalGameState->Input = Input;

  u32 ControllableCamera = NewEntity( GlobalGameState->EntityManager );
  NewComponents( GlobalGameState->EntityManager, ControllableCamera, COMPONENT_FLAG_CONTROLLER | COMPONENT_FLAG_CAMERA);

  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;

  component_camera* Camera = GetCameraComponent(ControllableCamera);
  v3 From = V3(0,0,1);
  v3 To = V3(0,0,0);
  v3 Up = V3(0,1,0);
  LookAt(Camera, From,  To,  Up );

  Camera->OrthoZoom = 1;
  r32 Near = -1;
  r32 Far = 10;
  r32 Right = AspectRatio;
  r32 Left = -Right;
  r32 Top = 1.f;
  r32 Bot = -Top;
  SetOrthoProj(Camera, Near, Far, Right, Left, Top, Bot );

  r32 FieldOfView =  90;
  Camera->AngleOfView  = FieldOfView;
  Camera->AspectRatio = AspectRatio;
  Camera->DeltaRot = M4Identity();
  Camera->DeltaPos = V3(0,0,0);
  Camera->V = M4Identity();

  // Camera->P = GetCanonicalSpaceProjectionMatrix();  /// [0,0] -> Bot Left, [1,AspectRatio]
  //SetOrthoProj(Camera, -1, 1 );
  //LookAt(Camera, V3(0,1,0), V3(0,30,0), V3(1,0,0));

  component_controller* Controller = GetControllerComponent(ControllableCamera);
  Controller->Controller = GetController(Input, 0);
  Controller->Keyboard = &Input->Keyboard;
  Controller->Mouse = &Input->Mouse;
  Controller->Type = ControllerType_FlyingCamera;

  Memory->GameState = GlobalGameState;
  RenderCommands->AssetManager = GlobalGameState->AssetManager;

  for (s32 ControllerIndex = 0;
  ControllerIndex < ArrayCount(Input->Controllers);
    ++ControllerIndex)
  {
    GetController(Input, ControllerIndex)->IsAnalog;
  }

  GlobalGameState->IsInitialized = true;


  Assert(!RenderCommands->WorldGroup);
  Assert(!RenderCommands->OverlayGroup);
  RenderCommands->WorldGroup = InitiateRenderGroup();
  RenderCommands->OverlayGroup = InitiateRenderGroup();


  chunk_list_tests::RunUnitTests(GlobalGameState->TransientArena);
  entity_components_backend_tests::RunUnitTests(GlobalGameState->TransientArena);
}

#include "function_pointer_pool.h"

void BeginFrame(game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  Platform = Memory->PlatformAPI;

  InitiateGame(Memory, RenderCommands, Input);

#if HANDMADE_INTERNAL
  DebugGlobalMemory = Memory;
#endif

  Assert(Memory->GameState);
  Assert(Memory->GameState->AssetManager);

  GlobalGameState = Memory->GameState;

  EndTemporaryMemory(GlobalGameState->TransientTempMem);
  GlobalGameState->TransientTempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);

  if(Input->ExecutableReloaded)
  {
    ReinitiatePool();
  }

  ResetRenderGroup(RenderCommands->WorldGroup);
  ResetRenderGroup(RenderCommands->OverlayGroup);

  // Transforms from canonical space to scre
  RenderCommands->OverlayGroup->ProjectionMatrix = GetCanonicalSpaceProjectionMatrix();
  GlobalGameState->World->GlobalTimeSec += Input->dt;
}



/*
  Note:
  extern "C" prevents the C++ compiler from renaming the functions which it does for function-overloading reasons
  (among other things) by forcing it to use C conventions which does not support overloading.
  Also called 'name mangling' or 'name decoration'. The actual function names are visible in the outputted .map file
  i the build directory.
*/



// Signature is
//void game_update_and_render (thread_context* Thread,
//                game_memory* Memory,
//                render_commands* RenderCommands,
//                game_input* Input )


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  BeginFrame(Memory, RenderCommands, Input);

  TIMED_FUNCTION();

  world* World = GlobalGameState->World;
  World->dtForFrame = Input->dt;


  entity_manager* EM = GlobalGameState->EntityManager;
  ControllerSystemUpdate(World);
  CameraSystemUpdate(World);
  SpriteAnimationSystemUpdate(World);

  FillRenderPushBuffer(World);

  if(Memory->DebugState)
  {
    PushDebugOverlay(Input);
  }
  UpdateAndRenderMenuInterface(Input, GlobalGameState->MenuInterface);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  GameOutputSound(SoundBuffer, 400);
}

#include "debug.cpp"
