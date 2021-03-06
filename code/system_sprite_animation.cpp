#include "breadboard_entity_components.h"
#include "random.h"

void SpriteAnimationSystemUpdate(world* World)
{
  TIMED_FUNCTION();
  entity_manager* EM = GlobalGameState->EntityManager;

  filtered_entity_iterator EntityIterator = GetComponentsOfType(EM, COMPONENT_FLAG_SPRITE_ANIMATION);
  while(Next(&EntityIterator))
  {
    component_sprite_animation* SpriteAnimation = (component_sprite_animation*) GetComponent(EM, &EntityIterator, COMPONENT_FLAG_SPRITE_ANIMATION);

    Assert(  SpriteAnimation->ActiveSeries );

    list<m4>* ActiveSeries = SpriteAnimation->ActiveSeries;
    local_persist u32 FrameCounter = 0;
    if( FrameCounter++ == 10)
    {
      FrameCounter = 0;
      ActiveSeries->Next();
      if(ActiveSeries->IsEnd())
      {
        ActiveSeries->First();
      }
    }

    local_persist u32 Timer = 0;

    if(Timer%100 == 0)
    {
      r32 r = GetRandomReal(Timer);
      r32 interval = 1.f/4.f;
      if(r < interval)
      {
        SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("jump");
      }else if(r  >= interval && r < 2*interval){
        SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("fall");
      }else if(r  >= 2*interval && r < 3*interval)
      {
        SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("idle1");
      }else{
        SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("run");
      }

      r = GetRandomReal(Timer);
      if( r < 0.5)
      {
        SpriteAnimation->InvertX = false;
      }else{
        SpriteAnimation->InvertX = true;
      }
    }
    Timer++;

  }
}