#include "debug.h"

#include "math/utils.h"
#include "color_table.h"

void ClearFrame(debug_frame* Frame)
{
  TIMED_FUNCTION();
  Frame->BeginClock = 0;
  Frame->EndClock = 0;
  Frame->WallSecondsElapsed = 0;
  Frame->FrameBarLaneCount = 0;
  ZeroArray(ArrayCount(Frame->Threads), Frame->Threads); 
  Frame->FirstFreeBlock = 0;
  Frame->Statistics.Clear();
}

internal void ResetCollation()
{
  debug_state* DebugState = DEBUGGetState();
  DebugState->CurrentFrameIndex = 0;
  DebugState->SelectedFrame = 0;
  DebugState->ThreadSelected = true;
  DebugState->SelectedThreadIndex = 0;
  DebugState->FunctionList.Clear();
  for(u32 FrameIndex = 0;
          FrameIndex < ArrayCount(DebugState->Frames);
          ++FrameIndex)
  {
    debug_frame* Frame = DebugState->Frames+FrameIndex;
    ClearFrame(Frame);
  }
}

MENU_EVENT_CALLBACK(DebugToggleButton)
{
  v4 InactiveColor = V4(0.3,0.1,0.1,1);
  v4 ActiveColor = V4(0.1,0.3,0.1,1);

  b32* ButtonFlag = (b32*) Data;
  *ButtonFlag = !*ButtonFlag;
  color_attribute* ColorAttr =  (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  ColorAttr->Color = (*ButtonFlag) ?  ActiveColor : InactiveColor;
}

MENU_EVENT_CALLBACK(DebugRecompileButton)
{
  DebugRewriteConfigFile();
}

internal debug_state*
DEBUGGetState()
{
  debug_state* DebugState = DebugGlobalMemory->DebugState;
  if(!DebugState)
  {
    DebugGlobalMemory->DebugState = BootstrapPushStruct(debug_state, Arena);

    DebugGlobalMemory->DebugState->FunctionList = vector_list<debug_record_entry>(&DebugGlobalMemory->DebugState->Arena, MAX_DEBUG_RECORD_COUNT*MAX_DEBUG_TRANSLATION_UNITS);

    for(u32 FrameIndex = 0;
        FrameIndex < ArrayCount(DebugGlobalMemory->DebugState->Frames);
        ++FrameIndex)
    {
      debug_frame* Frame = DebugGlobalMemory->DebugState->Frames+FrameIndex;
      Frame->MaxBlockCount = MAX_BLOCKS_PER_FRAME;
      Frame->Blocks = PushArray( &DebugGlobalMemory->DebugState->Arena, Frame->MaxBlockCount, debug_block);
      Frame->Statistics = vector_list<debug_statistics>(&DebugGlobalMemory->DebugState->Arena, MAX_DEBUG_FUNCTION_COUNT);
    }
    DebugState = DebugGlobalMemory->DebugState;
  }

  if(!DebugState->Initialized)
  {
    // Transient Memory Begin
    DebugState->Initialized = true;
    DebugState->Paused = false;
    DebugState->ThreadSelected = true;

    // Config state
    DebugState->ConfigMultiThreaded = MULTI_THREADED;
    DebugState->ConfigCollisionPoints = SHOW_COLLISION_POINTS;
    DebugState->ConfigCollider = SHOW_COLLIDER;
    DebugState->ConfigAABBTree = SHOW_AABB_TREE;

    // Set menu window
    u32 FontSize = 18;
    rect2f ButtonSize = GetTextSize(0, 0, "CollisionPoints", FontSize);
    v4 TextColor = V4(1,1,1,1);

    ButtonSize.W += 0.02f;
    ButtonSize.H += 0.02f;
    r32 ContainerWidth = 0.7;
    container_node* SettingsPlugin = 0;
    {
      // Create Option Window
      container_node* ButtonContainer =  NewContainer(GlobalGameState->MenuInterface, container_type::Grid);
      grid_node* Grid = GetGridNode(ButtonContainer);
      Grid->Col = 1;
      Grid->Row = 0;
      Grid->TotalMarginX = 0.0;
      Grid->TotalMarginY = 0.0;

      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, ButtonContainer, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      auto CreateButton = [&ButtonSize, &FontSize, &TextColor]( b32* ButtonFlag, c8* ButtonText)
      {
        container_node* ButtonNode = NewContainer(GlobalGameState->MenuInterface);
        text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_TEXT);
        Assert(str::StringLength( ButtonText ) < ArrayCount(Text->Text) );
        str::CopyStringsUnchecked( ButtonText, Text->Text );
        Text->FontSize = FontSize;
        Text->Color = TextColor;

        size_attribute* SizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_SIZE);
        SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE, ButtonSize.W);
        SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE, ButtonSize.H);
        SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->TopOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->XAlignment = menu_region_alignment::CENTER;
        SizeAttr->YAlignment = menu_region_alignment::CENTER;

        v4 InactiveColor = V4(0.3,0.1,0.1,1);
        v4 ActiveColor = V4(0.1,0.3,0.1,1);

        color_attribute* ColorAttr = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_COLOR);
        ColorAttr->Color = *ButtonFlag ?  ActiveColor : InactiveColor;

        RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, ButtonNode, ButtonFlag, DebugToggleButton, 0 );

        return ButtonNode;
      };

      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigMultiThreaded, "Multithread"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigCollisionPoints, "CollisionPoints"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigCollider, "Colliders"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigAABBTree, "AABBTree"));
      container_node* RecompileButton = ConnectNodeToBack(ButtonContainer, NewContainer(GlobalGameState->MenuInterface));
      {
        color_attribute* Color = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_COLOR);
        Color->Color = V4(0.2,0.1,0.3,1);
        
        text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_TEXT);
        str::CopyStringsUnchecked( "Recompile", Text->Text );
        Text->FontSize = FontSize;
        Text->Color = TextColor;

        size_attribute* SizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_SIZE);
        SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE, ButtonSize.W);
        SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE, 1);
        SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->TopOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->XAlignment = menu_region_alignment::CENTER;
        SizeAttr->YAlignment = menu_region_alignment::CENTER;

        RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, RecompileButton, 0, DebugRecompileButton, 0 );
      }
      SettingsPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Settings", V4(0.5,0.5,0.5,1), ButtonContainer);
    }

    // Create graph window
    container_node* FunctionPlugin = 0;
    {

      container_node* FunctionContainer = NewContainer(GlobalGameState->MenuInterface);
      FunctionContainer->Functions.Draw = DeclareFunction(menu_draw, DrawStatistics);

      FunctionPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Functions", HexCodeToColorV4( 0xABF74F ), FunctionContainer);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, FunctionPlugin, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);
    }

    // Create graph window
    container_node* GraphPlugin = 0;
    {
      container_node* ChartContainer = NewContainer(GlobalGameState->MenuInterface);
      ChartContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFunctionTimeline);

      container_node* FrameContainer = NewContainer(GlobalGameState->MenuInterface);
      FrameContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFrameFunctions);

      container_node* SplitNode  = NewContainer(GlobalGameState->MenuInterface, container_type::Split);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, SplitNode, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      container_node* BorderNode = CreateBorderNode(GlobalGameState->MenuInterface, false, 0.7);
      ConnectNodeToBack(SplitNode, BorderNode);
      ConnectNodeToBack(SplitNode, FrameContainer);
      ConnectNodeToBack(SplitNode, ChartContainer);

      GraphPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Profiler", HexCodeToColorV4( 0xAB274F ), SplitNode);
    }

    menu_tree* WindowsDropDownMenu = RegisterMenu(GlobalGameState->MenuInterface, "Windows");
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, SettingsPlugin);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, GraphPlugin);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, FunctionPlugin);
    //ToggleWindow(GlobalGameState->MenuInterface, "Functions");
    //ToggleWindow(GlobalGameState->MenuInterface, "Settings");
    //ToggleWindow(GlobalGameState->MenuInterface, "Profiler");
  }
  return DebugState;
}

void BeginDebugStatistics(debug_statistics* Statistic, debug_record_entry* Record)
{
  Statistic->HitCount = 0;
  Statistic->Min =  R32Max;
  Statistic->Max = -R32Max;
  Statistic->Tot = 0;
  Statistic->Record = Record;
}

void AccumulateStatistic(debug_statistics* Statistic, r32 Value)
{
  if(Statistic->Min > Value)
  {
    Statistic->Min = Value;
  }
  if(Statistic->Max < Value)
  {
    Statistic->Max = Value;
  }
  Statistic->Tot += Value;
  ++Statistic->HitCount;
}


#define DebugRecords_Main_Count __COUNTER__

#if HANDMADE_PROFILE
global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;
#else
debug_table* GlobalDebugTable = 0;
#endif
internal debug_thread* GetDebugThread(game_memory* Memory, debug_frame* Frame, u16 ThreadID)
{
  Assert(ThreadID);
  debug_thread* Result = 0;
  for(u32 ThreadArrayIndex = 0;
      ThreadArrayIndex < MAX_THREAD_COUNT;
      ++ThreadArrayIndex)
  {
    debug_thread* Thread = Frame->Threads + ThreadArrayIndex;
    if(!Thread->ID)
    {
      Result = Thread;
      *Result = {};
      Result->ID = ThreadID;
      Result->LaneIndex = Frame->FrameBarLaneCount++;
      break;
    }else if((Thread->ID == ThreadID))
    {
      Result = Thread;
      break;
    }
  }
  Assert(Result);
  return Result;
}

internal debug_record_entry*
GetRecordFrom(debug_block* OpenBlock)
{
  debug_record_entry* Result = OpenBlock ? OpenBlock->Record : 0;
  return Result;
}

s32 CompareDebugRecordEntry(void* A, void* B)
{
  debug_record_entry* EntryA  = (debug_record_entry*) A;
  debug_record_entry* EntryB  = (debug_record_entry*) B;
  s32 Result = str::Compare(EntryA->BlockName, EntryB->BlockName);
  return Result;
}

s32 CompareDebugStatistics(void* A, void* B)
{
  debug_statistics* EntryA  = (debug_statistics*) A;
  debug_statistics* EntryB  = (debug_statistics*) B;
  s32 Result = str::Compare(EntryA->Record->BlockName, EntryB->Record->BlockName);
  return Result;
}

inline u32 GetRecordIndexFromEvent( debug_event* Event )
{
  u32 Result = Event->TranslationUnit*MAX_DEBUG_RECORD_COUNT + Event->DebugRecordIndex;
  return Result;
}

void CollateDebugRecords(game_memory* Memory)
{
  debug_state* DebugState = DEBUGGetState();

  // Start on the frame after the one we are writing to
  u32 DebugTableFrame = GlobalDebugTable->CurrentEventArrayIndex - 1;
  if(GlobalDebugTable->CurrentEventArrayIndex == 0)
  {
    DebugTableFrame = MAX_DEBUG_EVENT_ARRAY_COUNT-1;
  }

  debug_frame* Frame = DebugState->Frames + DebugState->CurrentFrameIndex;

  // Get the  statistics list for the current frame;
  vector_list<debug_statistics>* FrameStatistics = &Frame->Statistics;

  // Get the persistent function list from the debug state
  vector_list<debug_record_entry>* FunctionList = &DebugState->FunctionList;

  BEGIN_BLOCK(ProfileCollation);
  u32 EventCount = GlobalDebugTable->EventCount[DebugTableFrame];
  for(u32 EventIndex = 0;
          EventIndex < EventCount;
          ++EventIndex)
  {
    // Get the record and event from the debug table
    debug_event* Event = GlobalDebugTable->Events[DebugTableFrame] + EventIndex;
    debug_record* DebugRecord = (GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex);

    u32 RecordIndex = GetRecordIndexFromEvent(Event);

    switch(Event->Type)
    {
      case DebugEvent_FrameMarker:
      {
        if(Frame->FirstFreeBlock)
        {
          Frame->EndClock = Event->Clock;
          Frame->WallSecondsElapsed = Event->SecondsElapsed;
        }

        u32 MaxFrameCount = ArrayCount(DebugState->Frames);
        ++Frame;
        ++DebugState->CurrentFrameIndex;
        if(DebugState->CurrentFrameIndex >= MaxFrameCount)
        {
          DebugState->CurrentFrameIndex = 0;
          Frame = DebugState->Frames;
        }
        
        ClearFrame(Frame);
        
        Frame->BeginClock = Event->Clock;
        Frame->FirstFreeBlock = Frame->Blocks;
        *Frame->FirstFreeBlock = {};
        
      }break;
      case DebugEvent_BeginBlock:
      {
        if(Frame->FirstFreeBlock)
        {
          debug_record_entry* RecordEntry = 0;
          if(!FunctionList->Exists(RecordIndex))
          {
            debug_record_entry Entry{};
            str::CopyStringsUnchecked(DebugRecord->BlockName, Entry.BlockName); 
            Entry.LineNumber = DebugRecord->LineNumber;
            RecordEntry = FunctionList->PushBack(Entry, RecordIndex);
          }else{
            RecordEntry = FunctionList->GetFromVector(RecordIndex);
          }
          
          debug_thread* Thread = GetDebugThread(Memory, Frame, Event->TC.ThreadID);
          debug_block* Block = Frame->FirstFreeBlock++;
          *Frame->FirstFreeBlock = {};
          midx BlockCount = Block - Frame->Blocks;
          if(BlockCount >= MAX_BLOCKS_PER_FRAME)
          {
            // TODO: Handle this case somehow by growing the Block-vector by chunks
            //       For now just change the MAX_BLOCKS_PER_FRAME if we hit this assert
            Assert(0);
          }

          Block->Record = RecordEntry;
          Block->ThreadIndex = Thread->ID;
          Block->BeginClock = Event->Clock - Frame->BeginClock;
          Block->EndClock = 0;
          Block->OpeningEvent = *Event;

          // Set the opening block for this thread
          if(!Thread->FirstBlock)
          {
            Thread->FirstBlock = Block;
          }

          // If we have an open block it means this event was called inside it -> we are going deeper into the stack
          if(Thread->OpenBlock && !Thread->OpenBlock->FirstChild)
          {
            Thread->OpenBlock->FirstChild = Block;
          }

          Block->Parent = Thread->OpenBlock;

          if(Thread->ClosedBlock && GetRecordFrom(Thread->ClosedBlock->Parent) == GetRecordFrom(Block->Parent))
          {
            Thread->ClosedBlock->Next = Block;
          }

          Thread->OpenBlock = Block;
        }
      }break;
      case DebugEvent_EndBlock:
      {
        if(Frame->FirstFreeBlock)
        {
          debug_record_entry* RecordEntry = FunctionList->GetFromVector(RecordIndex);
          Assert(FunctionList->Exists(RecordIndex));

          debug_thread* Thread = GetDebugThread(Memory, Frame, Event->TC.ThreadID);
          Assert(Thread->OpenBlock);
          debug_block* Block = Thread->OpenBlock;
          Block->EndClock = Event->Clock - Frame->BeginClock;

          debug_event* OpeningEvent = &Block->OpeningEvent;
          Assert(OpeningEvent->TC.ThreadID      == Event->TC.ThreadID);
          Assert(OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex);
          Assert(OpeningEvent->TranslationUnit  == Event->TranslationUnit);

          Thread->ClosedBlock = Block;
          
          Thread->OpenBlock = Block->Parent;

          u32 Hash = utils::djb2_hash(RecordEntry->BlockName);
          u32 HashedIndex = Hash % FrameStatistics->maxSize();
          debug_statistics* Statistic = 0;
          b32 EntryFound = false;
          while(FrameStatistics->Exists(HashedIndex))
          {
            Statistic = FrameStatistics->GetFromVector(HashedIndex);
            if(str::ExactlyEquals(Statistic->Record->BlockName, RecordEntry->BlockName))
            {
              EntryFound = true;
              break;
            }
            ++HashedIndex;
          }

          if(!EntryFound)
          {
            debug_statistics Stats = {};
            BeginDebugStatistics(&Stats, RecordEntry);
            Statistic = FrameStatistics->PushBack(Stats, HashedIndex);
          }

          AccumulateStatistic(Statistic, (r32)(Block->EndClock - Block->BeginClock));
        }
      }break;
    }
  }
  END_BLOCK(ProfileCollation);
}

inline void
DebugRewriteConfigFile()
{
  debug_state* DebugState = DEBUGGetState();

  c8 Buffer[4096] = {};
  u32 Size = Platform.DEBUGFormatString(Buffer, sizeof(Buffer), sizeof(Buffer)-1,
"#define MULTI_THREADED %d // b32\n\
#define SHOW_COLLISION_POINTS %d // b32\n\
#define SHOW_COLLIDER %d // b32\n\
#define SHOW_AABB_TREE %d // b32",
    DebugState->ConfigMultiThreaded,
    DebugState->ConfigCollisionPoints,
    DebugState->ConfigCollider,
    DebugState->ConfigAABBTree);
  thread_context Dummy = {};

  Platform.DEBUGPlatformWriteEntireFile(&Dummy, "W:\\handmade\\code\\debug_config.h", Size, Buffer);

  //DebugState->UpdateConfig = false;
  DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("W:\\handmade\\code", "C:\\windows\\system32\\cmd.exe", "/C build_game.bat");
  DebugState->Compiling = true;

}


extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
  if(!GlobalDebugTable) return 0;
  GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count; // This stores the amount of records found is the first translation unit, (The Game)

  // Increment which event-array we should  be writing to. (Each frame writes into it's own array)
  ++GlobalDebugTable->CurrentEventArrayIndex;
  if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
  {
    // Wrap if we reached the final array
    GlobalDebugTable->CurrentEventArrayIndex=0;
  }

  // Shift CurrentEventArrayIndex to the high bits of ArrayIndex_EventIndex.
  //  The old value of ArrayIndex_EventIndex is returned by AtomicExchangeu64
  // Note the low bits "EventIndex" is zeroed out since we wanna start writing from the top of the array next frame.
  // EventIndex is incremented each time we run through a TIMED_FUNCTION macro
  u64 ArrayIndex_EventIndex = AtomicExchangeu64(&GlobalDebugTable->EventArrayIndex_EventIndex,          // The new value is stored in this variable
                                               ((u64)GlobalDebugTable->CurrentEventArrayIndex << 32));  // This is the new value

  u32 EventArrayIndex = (ArrayIndex_EventIndex >> 32);         // The event array index we just finished writing to
  u32 EventCount = (ArrayIndex_EventIndex & 0xFFFFFFFF);       // The number of events encountered last frame
  GlobalDebugTable->EventCount[EventArrayIndex] = EventCount;  // The frame "EventArrayIndex" saw "EventCount" Recorded Events

  debug_state* DebugState = DEBUGGetState();
  if(DebugState)
  {
    if(Memory->GameState->Input->ExecutableReloaded)
    {
      ResetCollation();
    }
    if(!DebugState->Paused)
    {
      CollateDebugRecords(Memory);
    }
  }
  return GlobalDebugTable;
}



void DEBUGAddTextSTB(const c8* String, r32 LineNumber, u32 FontSize)
{
  TIMED_FUNCTION();
  game_window_size WindowSize = GameGetWindowSize();
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  r32 CanPosX = 1/100.f;
  r32 CanPosY = 1 - ((LineNumber+1) * FontMap->Ascent - LineNumber*FontMap->Descent)/WindowSize.HeightPx;
  PushTextAt(CanPosX, CanPosY, String, FontSize, V4(1,1,1,1));
}


MENU_DRAW(DrawStatistics)
{
  TIMED_FUNCTION();
  BEGIN_BLOCK(AllocatingMemory);
  rect2f Region = Shrink(Node->Region,0.01);
  debug_state* DebugState = DEBUGGetState();

  ScopedMemory Memory(GlobalGameState->TransientArena);
  memory_arena* Arena = GlobalGameState->TransientArena;
  
  vector_list<debug_statistics> CumulativeStats = vector_list<debug_statistics>(GlobalGameState->TransientArena, MAX_DEBUG_FUNCTION_COUNT);
  vector_list<debug_record_entry>* DebugFunctions = &DebugState->FunctionList;
  END_BLOCK(AllocatingMemory);
  // TODO: This loop is super slow and it's unecessary to loop through it all every frame.
  //       Instead we can store the sum of CycleCount and HitCount for all previous farmes
  //       in each frame and calculate the average by just going through the latest frame

  // For each frame and for each debug-record: 
  u32 RowCount = 0;
  u32 CurrentFrameIndex = DebugState->CurrentFrameIndex == 0 ? MAX_DEBUG_FRAME_COUNT-1 : DebugState->CurrentFrameIndex-1;
  u32 FrameCount = 0;
  BEGIN_BLOCK(SummingStats);
  for(u32 FrameIndex = 0; FrameIndex < ArrayCount(DebugState->Frames); ++FrameIndex)
  {
    FrameCount++;
    debug_frame* Frame = DebugState->Frames + FrameIndex;
    u64 FrameCycleCount = Frame->EndClock - Frame->BeginClock;

    vector_list<debug_statistics>* FrameStatistics = &Frame->Statistics;
    debug_statistics*  Stat = FrameStatistics->First();
    
    while(Stat)
    {
      Assert(Stat->HitCount);

      u32 ArrayIndex = FrameStatistics->GetIndexOfEntry(Stat);

      debug_statistics* CumulativeStat = CumulativeStats.GetFromVector(ArrayIndex);

      if(!CumulativeStats.Exists(ArrayIndex))
      {
        debug_statistics NewStatisticEntry{};
        BeginDebugStatistics(&NewStatisticEntry, Stat->Record);
        CumulativeStat = CumulativeStats.PushBack(NewStatisticEntry, ArrayIndex);
      }

      CumulativeStat->HitCount += Stat->HitCount;
      CumulativeStat->Min = Minimum(CumulativeStat->Min, Stat->Min);
      CumulativeStat->Max = Maximum(CumulativeStat->Max, Stat->Max);
      CumulativeStat->Tot += Stat->Tot;

      Stat = FrameStatistics->Next(Stat);
    }
  }

  debug_statistics* Stat = CumulativeStats.First();
  while(Stat)
  {
    Assert(Stat->HitCount);

    r32 HitCount = (Stat->HitCount / (r32) FrameCount);
    r32 CycleCount = (Stat->Tot / (r32) FrameCount);

    debug_record_entry* Record = Stat->Record;
    u32 ArrayIndex = DebugFunctions->GetIndexOfEntry(Record);
    Assert(DebugFunctions->Exists(ArrayIndex));

    Record->CycleCount = (u32) CycleCount;
    Record->HitCount = Ciel(HitCount);
    Record->HCCount = Ciel(CycleCount/HitCount);

    Stat = CumulativeStats.Next(Stat);
  }

  END_BLOCK(SummingStats);


  u32 Cols = 5;
  u32 FontSize = 8;
  r32 LineHeight = GetTextLineHeightSize(FontSize);
  rect2f HeaderRects[5] = {};
  char* Text[] = {"LineNumber ",
                  "BlockName  ",
                  "  CycleCount",
                  "  HitCount",
                  "Cy/Hi"};
  r32 ColWidthPercent[5] = {};
  r32 ColWidthSum[5] = {};

  r32 Width = GetTextWidth(Text[0], FontSize);
  ColWidthPercent[0] = Width / Region.W;
  ColWidthSum[1] = ColWidthPercent[0];
  ColWidthPercent[1] = 0.4;
  ColWidthSum[2] = ColWidthSum[1] + ColWidthPercent[1];
  r32 RemainingWidth =1.f - (ColWidthPercent[0] + ColWidthPercent[1]);

  ColWidthPercent[2] = RemainingWidth/3.f;
  ColWidthSum[3] = ColWidthSum[2] + ColWidthPercent[2];
  ColWidthPercent[3] = RemainingWidth/3.f;
  ColWidthSum[4] =ColWidthSum[3] + ColWidthPercent[3];
  ColWidthPercent[4] = RemainingWidth/3.f;

  rect2f TextRect[] = {
    Rect2f(Region.X + Region.W * ColWidthSum[0], Region.Y + Region.H - LineHeight, Region.W * ColWidthPercent[0], LineHeight ),
    Rect2f(Region.X + Region.W * ColWidthSum[1], Region.Y + Region.H - LineHeight, Region.W * ColWidthPercent[1], LineHeight ),
    Rect2f(Region.X + Region.W * ColWidthSum[2], Region.Y + Region.H - LineHeight, Region.W * ColWidthPercent[2], LineHeight ),
    Rect2f(Region.X + Region.W * ColWidthSum[3], Region.Y + Region.H - LineHeight, Region.W * ColWidthPercent[3], LineHeight ),
    Rect2f(Region.X + Region.W * ColWidthSum[4], Region.Y + Region.H - LineHeight, Region.W * ColWidthPercent[4], LineHeight )
  };
  
  PushTextAt(TextRect[0].X, TextRect[0].Y, Text[0], FontSize, V4(1,1,1,1));
  PushTextAt(TextRect[1].X, TextRect[1].Y, Text[1], FontSize, V4(1,1,1,1));
  PushTextAt(TextRect[2].X + TextRect[2].W - GetTextWidth(Text[2], FontSize), TextRect[2].Y, Text[2], FontSize, V4(1,1,1,1));
  PushTextAt(TextRect[3].X + TextRect[3].W - GetTextWidth(Text[3], FontSize), TextRect[3].Y, Text[3], FontSize, V4(1,1,1,1));
  PushTextAt(TextRect[4].X + TextRect[4].W - GetTextWidth(Text[4], FontSize), TextRect[4].Y, Text[4], FontSize, V4(1,1,1,1));

  if(GlobalGameState->MenuInterface->MouseLeftButton.Active)
  {
    v2 DeltaMouse = Interface->PreviousMousePos - Interface->MousePos;
    if(Interface->MouseLeftButton.Edge)
    {
      if(Intersects(TextRect[0], Interface->MousePos))
      {
        if(DebugState->LineSorted == function_sorting::Ascending || DebugState->LineSorted == function_sorting::None){
          DebugState->LineSorted = function_sorting::Descending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;

            if(A->LineNumber <= B->LineNumber)
            {
              Result = true;
            }
            return Result;
          });
        }else if(DebugState->LineSorted == function_sorting::Descending)
        {
          DebugState->LineSorted = function_sorting::Ascending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;

            if(A->LineNumber >= B->LineNumber)
            {
              Result = true;
            }
            return Result;
          });
        }
      }else if(Intersects(TextRect[1], Interface->MousePos)){

        if(DebugState->BlockNameSorted == function_sorting::Ascending || DebugState->BlockNameSorted == function_sorting::None){
          DebugState->BlockNameSorted = function_sorting::Descending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = str::Compare(A->BlockName, B->BlockName) >= 0;
            return Result;
          });
        }else if(DebugState->BlockNameSorted == function_sorting::Descending)
        {
          DebugState->BlockNameSorted = function_sorting::Ascending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = str::Compare(A->BlockName,B->BlockName) <= 0;
            return Result;
          });
        }
      }else if(Intersects(TextRect[2], Interface->MousePos)){
        if(DebugState->CycleCountSorted == function_sorting::Descending || DebugState->CycleCountSorted == function_sorting::None)
        {
          DebugState->CycleCountSorted = function_sorting::Ascending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;
            if(A->CycleCount <= B->CycleCount)
            {
              Result = true;
            }
            return Result;
          });
        }else if(DebugState->CycleCountSorted == function_sorting::Ascending){
          DebugState->CycleCountSorted = function_sorting::Descending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;
            if(A->CycleCount >= B->CycleCount)
            {
              Result = true;
            }
            return Result;
          });
        }
      }else if(Intersects(TextRect[3], Interface->MousePos)){
        if(DebugState->HitCountSorted == function_sorting::Descending || DebugState->HitCountSorted == function_sorting::None)
        {
          DebugState->HitCountSorted = function_sorting::Ascending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;
            if(A->HitCount <= B->HitCount)
            {
              Result = true;
            }
            return Result;
          });
        }else if(DebugState->HitCountSorted == function_sorting::Ascending){
          DebugState->HitCountSorted = function_sorting::Descending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;
            if(A->HitCount >= B->HitCount)
            {
              Result = true;
            }
            return Result;
          });
        }
      }else if(Intersects(TextRect[4], Interface->MousePos)){
        if(DebugState->CyclePerHitSorted == function_sorting::Descending || DebugState->CyclePerHitSorted == function_sorting::None)
        {
          DebugState->CyclePerHitSorted = function_sorting::Ascending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;
            if(A->HCCount <= B->HCCount)
            {
              Result = true;
            }
            return Result;
          });
        }else if(DebugState->CyclePerHitSorted == function_sorting::Ascending){
          DebugState->CyclePerHitSorted = function_sorting::Descending;
          DebugFunctions->MergeSort(GlobalGameState->TransientArena, [](debug_record_entry* A, debug_record_entry* B)
          {
            b32 Result = false;
            if(A->HCCount >= B->HCCount)
            {
              Result = true;
            }
            return Result;
          });
        }

      }
    }
  }



  BEGIN_BLOCK(PaintingStats);
  r32 YPos = Region.Y + Region.H - 2.5f * LineHeight;

  v4 EventColor =  HexCodeToColorV4(0x00008B);
  EventColor.W = 0.5;
  v4 OddColor = HexCodeToColorV4(0x9400D3);
  OddColor.W = 0.5;
  b32 EvenRow = false;
  debug_record_entry* Entry = DebugFunctions->First();
  while(Entry)
  { 
    rect2f RowRect = Rect2f(Node->Region.X, YPos-LineHeight*0.5f, Node->Region.W, LineHeight*1.5f);
    PushOverlayQuad(RowRect, EvenRow ? EventColor : OddColor );
    EvenRow = !EvenRow;

    char StringBuffer[512]={};

    Platform.DEBUGFormatString(StringBuffer, ArrayCount(StringBuffer), ArrayCount(StringBuffer)-1,
    "%d: ", (u32)(Entry->LineNumber));
    Width = GetTextWidth(StringBuffer,FontSize);
    r32 XPos = TextRect[0].X + TextRect[0].W - Width;
    PushTextAt(XPos, YPos, StringBuffer, FontSize, V4(1,1,1,1));


    Platform.DEBUGFormatString(StringBuffer, ArrayCount(StringBuffer), ArrayCount(StringBuffer)-1,
    "%s", Entry->BlockName);

    XPos = TextRect[1].X;
    PushTextAt(XPos, YPos, StringBuffer, FontSize, V4(1,1,1,1));

    Platform.DEBUGFormatString(StringBuffer, ArrayCount(StringBuffer), ArrayCount(StringBuffer)-1,
    "%d", (u32) Entry->CycleCount);
    Width = GetTextWidth(StringBuffer,FontSize);
    
    XPos = TextRect[2].X + TextRect[2].W - Width;
    PushTextAt(XPos, YPos, StringBuffer, FontSize, V4(1,1,1,1));


    Platform.DEBUGFormatString(StringBuffer, ArrayCount(StringBuffer), ArrayCount(StringBuffer)-1,
    "%d", (u32) Entry->HitCount);
    Width = GetTextWidth(StringBuffer,FontSize);
    XPos = TextRect[3].X + TextRect[3].W - Width;
    PushTextAt(XPos, YPos, StringBuffer, FontSize, V4(1,1,1,1));


    Platform.DEBUGFormatString(StringBuffer, ArrayCount(StringBuffer), ArrayCount(StringBuffer)-1,
    "%d", (u32) Entry->HCCount);
    Width = GetTextWidth(StringBuffer,FontSize);
    XPos = TextRect[4].X + TextRect[4].W - Width;
    PushTextAt(XPos, YPos, StringBuffer, FontSize, V4(1,1,1,1));


    if(YPos-LineHeight < Region.Y){
      break;
    }

    YPos -= LineHeight*1.5f;

    Entry = DebugFunctions->Next(Entry);
    
  }

  END_BLOCK(PaintingStats);
}

v4 GetColorForRecord(debug_record_entry* Record)
{
  debug_state* DebugState = DEBUGGetState();
  u32 NameHash = utils::djb2_hash(Record->BlockName);
  v4 Result = GetColor(NameHash);
  return Result;
}


b32 DrawLane(u32 ArrayMaxCount, u32 BlockCount, debug_block*** BlockArray, u32* BufferCount, debug_block*** Buffer, debug_block** SelectedBlock,
             r32 StartX, r32 StartY, u32 LaneIndex, r32 LaneWidth, r32 CycleScaling, u64 CycleOffset, r32 PixelSize, v2 MousePos)
{
  debug_state* DebugState = DEBUGGetState();
  *BufferCount = 0;
  *SelectedBlock = 0;
  ZeroArray(ArrayMaxCount, *Buffer);
  b32 BlocksDrawn = false;
  for (u32 i = 0; i < BlockCount; ++i)
  {
    debug_block* Block = (*BlockArray)[i];
    while(Block)
    {
      if(Block->FirstChild)
      {
        (*Buffer)[(*BufferCount)++] = Block->FirstChild;
      }

      rect2f Rect{};
      Rect.X = StartX + CycleScaling * (r32)( Block->BeginClock - CycleOffset) + PixelSize*0.5f;
      Rect.Y = StartY + LaneWidth * LaneIndex + PixelSize*0.5f;
      r32 CycleCount = (r32)(Block->EndClock - Block->BeginClock);
      Rect.W = CycleScaling * CycleCount - PixelSize;
      Rect.H = LaneWidth - PixelSize;
      
      if(Rect.W > PixelSize)
      {
        v4 Color = GetColorForRecord(Block->Record);
        PushOverlayQuad(Rect, Color);
        BlocksDrawn = true;
        if(Intersects(Rect, MousePos))
        {
          c8 StringBuffer[1048] = {};
          Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
          "%s : %2.2f MCy", Block->Record->BlockName, CycleCount/1000000.f);
          PushTextAt(MousePos.X, MousePos.Y+0.02f, StringBuffer, 24, V4(1,1,1,1));
          *SelectedBlock = Block;
        }
      }
      Block = Block->Next;
    }
  }
  return BlocksDrawn;
}

u32 PushLaneToBuffer(debug_block* Block, u32 BufferCount, debug_block** Buffer)
{
  while(Block)
  {
    if(Block->FirstChild)
    {
      Buffer[BufferCount++] = Block->FirstChild;  
    }
    Block = Block->Next;
  }
  return BufferCount;
}

debug_block* DrawBlockChain(debug_block* Block, r32 StartX, r32 StartY, r32 LaneWidth, r32 PixelSize, r32 CycleScaling, u64 CycleOffset, v2 MousePos)
{
  debug_block* Result = 0;
  while(Block)
  {
    rect2f Rect{};
    Rect.X = StartX + CycleScaling * (r32)(Block->BeginClock - CycleOffset) + PixelSize*0.5f;
    Rect.Y = StartY + PixelSize*0.5f;
    r32 CycleCount = (r32)(Block->EndClock - Block->BeginClock);
    Rect.W = CycleScaling * CycleCount - PixelSize;
    Rect.H = LaneWidth - PixelSize;

    if(Rect.W >= PixelSize)
    {
      v4 Color = GetColorForRecord(Block->Record);
      PushOverlayQuad(Rect, Color);

      if(Intersects(Rect, MousePos))
      {
        c8 StringBuffer[1048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", Block->Record->BlockName, CycleCount/1000000.f);
        PushTextAt(MousePos.X, MousePos.Y+0.02f, StringBuffer, 24, V4(1,1,1,1));
        
        Result = Block;
      }
    }


    Block = Block->Next;
  }

  return Result;
}

u64 GetBlockChainCycleCount(debug_block* FirstBlock)
{
  debug_block* LastBlock = FirstBlock;
  while(LastBlock->Next)
  {
    LastBlock = LastBlock->Next;
  }
  u64 Result = LastBlock->EndClock - FirstBlock->BeginClock;
  return Result;
}

MENU_DRAW(DrawFrameFunctions)
{
  TIMED_FUNCTION();
  debug_state* DebugState = DEBUGGetState();

  r32 ThreadBreak = 0.005;

  rect2f Chart = Node->Region;
  Chart.H -= ThreadBreak;
  
  v2 MousePos = Interface->MousePos;

  debug_frame* Frame = DebugState->SelectedFrame;
  if(!Frame)
  {
    u32 FrameCount = ArrayCount(DebugState->Frames);
    u32 FrameIndex = (DebugState->CurrentFrameIndex-1) % FrameCount;
    Frame = DebugState->Frames + FrameIndex;
  };

  r32 ThreadCount = (r32)Frame->FrameBarLaneCount;
  r32 MaxLaneCountPerThread = 8;

  game_window_size WindowSize = GameGetWindowSize();
  r32 PixelSize = 1.f / WindowSize.HeightPx;

  r32 LaneWidth = Chart.H/(MaxLaneCountPerThread + ThreadCount);
  r32 dt = GlobalGameState->Input->dt;

  debug_block* SelectedBlock = 0;
  r32 ThreadHeight = 0;
  r32 StartY = Chart.Y;
  
  u32 SelectedThreadIndex = 0;

  r32 ThreadStartY = Chart.Y;

  debug_block* HotBlock = 0;
  debug_thread* HotThread = 0;

  debug_block* SwapBuffer[2][1048] = {};
  debug_block** ConsumeBuffer = SwapBuffer[0];
  debug_block** ProduceBuffer = SwapBuffer[1];
  u32 ConsumeCount = 0;
  u32 ProducedCount = 0;
  r32 LaneIndex = 0;
  for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
  {
    r32 CycleScaling = Chart.W / (r32) (Frame->EndClock - Frame->BeginClock);
    u64 CycleOffset = 0;

    debug_thread* Thread = Frame->Threads + ThreadIndex;
  
    Assert(Thread->FirstBlock);

    if(Thread->SelectedBlock)
    {
      debug_block* BaseBlock = Thread->SelectedBlock;

      CycleScaling = Chart.W / (r32) (BaseBlock->EndClock - BaseBlock->BeginClock);
      CycleOffset = BaseBlock->BeginClock;

      rect2f FirstBlockRect = {};
      FirstBlockRect.X = Chart.X;
      FirstBlockRect.Y = ThreadStartY+ ThreadBreak *0.5f;
      FirstBlockRect.W = Chart.W;
      FirstBlockRect.H = LaneWidth;

      v4 Color = GetColorForRecord(BaseBlock->Record);
      if(Intersects(FirstBlockRect, MousePos))
      {
        c8 StringBuffer[1048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", BaseBlock->Record->BlockName, (u32)(BaseBlock->EndClock - BaseBlock->BeginClock)/1000000.f);
        PushTextAt(MousePos.X, MousePos.Y+0.02f, StringBuffer, 24, V4(1,1,1,1));
        HotBlock = Thread->SelectedBlock;
      }
      PushOverlayQuad(FirstBlockRect, Color);
      ConsumeBuffer[ConsumeCount++] = Thread->SelectedBlock->FirstChild;
      LaneIndex = 1;
    }else{
      debug_block* MousedOverBlock = DrawBlockChain(Thread->FirstBlock, Chart.X, ThreadStartY + ThreadBreak *0.5f, LaneWidth, PixelSize, CycleScaling, CycleOffset, MousePos);
      if(!HotBlock)
      {
        HotBlock = MousedOverBlock;
      }
      LaneIndex = 1;
      if(DebugState->ThreadSelected)
      {
        if(ThreadIndex == DebugState->SelectedThreadIndex)
        {
          ConsumeCount = PushLaneToBuffer(Thread->FirstBlock, 0, ConsumeBuffer);
        }
      }
    }
    
    while(ConsumeCount && LaneIndex < MaxLaneCountPerThread )
    {
      for (u32 BlockIndex = 0; BlockIndex < ConsumeCount; ++BlockIndex)
      {
        debug_block* Block = ConsumeBuffer[BlockIndex];
        r32 YOffset = LaneWidth * LaneIndex + ThreadBreak*0.5f;
        debug_block* MousedOverBlock = DrawBlockChain(Block, Chart.X, ThreadStartY + YOffset, LaneWidth, PixelSize, CycleScaling, CycleOffset, MousePos);
        if(MousedOverBlock)
        {
          HotBlock = MousedOverBlock;
        }
        ProducedCount = PushLaneToBuffer(Block, ProducedCount, ProduceBuffer);
      }
      ++LaneIndex;

      ZeroArray(ConsumeCount, ConsumeBuffer);
      ConsumeCount = ProducedCount;
      ProducedCount = 0;

      debug_block** TmpBuffer = ConsumeBuffer;
      ConsumeBuffer = ProduceBuffer;
      ProduceBuffer = TmpBuffer;
    }

    rect2f ThreadRegion = {};
    ThreadRegion.X = Chart.X;
    ThreadRegion.Y = ThreadStartY;
    ThreadRegion.W = Chart.W;

    r32 ThreadRegionHeight = LaneWidth + ThreadBreak;
    if(DebugState->ThreadSelected && (ThreadIndex == DebugState->SelectedThreadIndex))
    {
      ThreadRegionHeight = LaneWidth*MaxLaneCountPerThread;
    }

    ThreadRegion.H = ThreadRegionHeight;
    ThreadStartY += ThreadRegionHeight;
    if(!HotThread && Intersects(ThreadRegion, MousePos))
    {
      HotThread = Thread;
    }

    if(ThreadIndex < ThreadCount)
    {
      rect2f ThreadBreakRect = {};
      ThreadBreakRect.X = Chart.X;
      ThreadBreakRect.Y = ThreadStartY - ThreadBreak * 0.5f;
      ThreadBreakRect.W = Chart.W;
      ThreadBreakRect.H = ThreadBreak;
      PushOverlayQuad(ThreadBreakRect, V4(0,0,1,1));
    }
  }

  // Click-logic!
  if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active && Intersects(Chart,MousePos))
  {
    // We clicked inside a thread-region
    if(HotThread)
    {
      // We klicked inside an already active thread-region
      if(DebugState->ThreadSelected && DebugState->SelectedThreadIndex == HotThread->LaneIndex)
      {
        // We klicked on a block inside an already active thread-region
        if(HotBlock)
        {
          // We klicked on an already selected block inside an already active thread-region
          if(HotThread->SelectedBlock == HotBlock)
          {
            // We set the selected block to it's parent
            HotThread->SelectedBlock = HotThread->SelectedBlock->Parent;
          }else{
            // We set the threads selected block
            HotThread->SelectedBlock = HotBlock;  
          }

        // We did not click on a block inside the already selected thread-region
        // And no block was selected
        }else if(!HotThread->SelectedBlock)
        {
          // Collapse the selected thread region
          DebugState->ThreadSelected = false;

        // We did not click on a block inside the already selected thread-region
        // But a block was selected
        }else if(HotThread->SelectedBlock)
        {
          // Reset the selected block
          HotThread->SelectedBlock = 0;
        }

      // We klicked inside a thread-region but it's not an already selected region
      }else{
        // Another region was already selected
        if(DebugState->ThreadSelected)
        {
          // Set it's selected block to 0
          debug_thread* PreviouslySelectedThread = Frame->Threads + DebugState->SelectedThreadIndex;
          PreviouslySelectedThread->SelectedBlock = 0;
        }
        DebugState->ThreadSelected = true;
      }
      DebugState->SelectedThreadIndex = HotThread->LaneIndex;

    }else{
      DebugState->ThreadSelected = false;
    }
  }
}


MENU_DRAW(DrawFunctionTimeline)
{
  TIMED_FUNCTION();
  debug_state* DebugState = DEBUGGetState();

  rect2f Chart = Node->Region;

  r32 dt = GlobalGameState->Input->dt;

  r32 FrameTargetHeight = Chart.H * 0.7f;
  r32 HeightScaling = FrameTargetHeight/dt;

  
  u32 MaxFramesToDisplay = ArrayCount(DebugState->Frames)-1;

  game_window_size WindowSize = GameGetWindowSize();
  r32 PixelSize = 1.f / WindowSize.HeightPx;

  r32 BarGap = PixelSize;

  v4 RedColor = V4(1,0,0,1);
  r32 FullRed = 1.2;
  r32 FullBlue = 1;
  v4 BlueColor =  V4(0,0,1,1);

  r32 MouseX = Interface->MousePos.X;
  r32 MouseY = Interface->MousePos.Y;

  u32 FrameCount =  ArrayCount(DebugState->Frames);
  u32 Count = 0;
  debug_frame* Frame = DebugState->Frames + DebugState->CurrentFrameIndex+1;
  debug_frame* SelectedFrame = 0;
  debug_thread* SelectedThread = 0;
  r32 BarWidth = Chart.W/(MaxFramesToDisplay);
  for(u32 BarIndex = 0; BarIndex < MaxFramesToDisplay; ++BarIndex)
  {
    u32 FrameIndex = (u32) (Frame-DebugState->Frames);
    if(FrameIndex >= FrameCount)
    {
      Frame = DebugState->Frames; 
    }
    
    debug_block* Block = Frame->Threads[0].FirstBlock;

    r32 FrameY = Chart.Y;

    r32 FrameHitRatio = Frame->WallSecondsElapsed / dt;
    r32 FrameCycleScaling = FrameTargetHeight * FrameHitRatio/(Frame->EndClock - Frame->BeginClock);

    r32 FrameX = Chart.X + (r32)BarIndex*BarWidth;

    if(DebugState->SelectedFrame && DebugState->SelectedFrame == Frame)
    {
      u64 LaneCycleCount = GetBlockChainCycleCount(Block);
      rect2f Rect = {};
      Rect.X = FrameX - 0.5f*PixelSize;
      Rect.Y = FrameY - 0.5f*PixelSize;
      Rect.W = BarWidth + PixelSize;
      Rect.H = FrameCycleScaling*LaneCycleCount + 2*PixelSize;
      PushOverlayQuad(Rect, V4(1,1,0,1));
    }

    while(Block)
    {
      rect2f Rect = {};
      Rect.X = FrameX + BarGap * 0.5f;
      Rect.Y = FrameY+FrameCycleScaling*Block->BeginClock;
      Rect.W = BarWidth - BarGap;
      Rect.H = FrameCycleScaling*(Block->EndClock - Block->BeginClock);

      v4 Color = GetColorForRecord(Block->Record);
      PushOverlayQuad(Rect, Color);
      
      if(Intersects(Rect, V2(MouseX,MouseY)))
      {
        c8 StringBuffer[2048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer),
        "Frame %d: %2.2f Sec (%s)", FrameIndex, Frame->WallSecondsElapsed, Block->Record->BlockName);

        PushTextAt(MouseX, MouseY+0.02f, StringBuffer, 24, V4(1,1,1,1));
      }
      Block = Block->Next;
    }  

    rect2f FrameRect = {};
    FrameRect.X = FrameX;
    FrameRect.Y = FrameY;
    FrameRect.W = BarWidth;
    FrameRect.H = Frame->WallSecondsElapsed * HeightScaling;

    if(Intersects(FrameRect, V2(MouseX,MouseY)))
    {
      SelectedFrame = Frame;
    }
    ++Frame;
  }

  if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
  {
    if(SelectedFrame)
    {
      DebugState->SelectedFrame = SelectedFrame;
      DebugState->Paused = true;
    }else if(Intersects(Chart, V2(MouseX,MouseY))){
      DebugState->SelectedFrame = 0;
      DebugState->Paused = false;
    }
  }
  

  rect2f Rect = {};
  Rect.X = Chart.X;
  Rect.Y = Chart.Y + FrameTargetHeight - 0.001f*0.5f;
  Rect.W = Chart.W;
  Rect.H = 0.001;
  PushOverlayQuad(Rect, V4(0,0,0,1));
}

void PushDebugOverlay(game_input* GameInput)
{
  TIMED_FUNCTION();

  debug_state* DebugState = DEBUGGetState();

  if(DebugState->Compiling)
  {
    debug_process_state ProcessState = Platform.DEBUGGetProcessState(DebugState->Compiler);
    DebugState->Compiling = ProcessState.IsRunning;
    if(DebugState->Compiling)
    {
      DEBUGAddTextSTB("Compiling", 0, 24);
    }
  }
}


#include "menu_functions.h"
