#include "component_hitbox.h"

void InitiateCircleHitboxComponent(component_hitbox* Hitbox, position_node* PositionNode, r32 Radius)
{
  Hitbox->Type = HitboxType::CIRCLE;
  Hitbox->Position = PositionNode;
  Hitbox->Circle.Radius = Radius;
}

void InitiateTriangleHitboxComponent(component_hitbox* Hitbox, position_node* PositionNode, r32 Base, r32 Height, r32 CenterPoint)
{
  Hitbox->Type = HitboxType::TRIANGLE;
  Hitbox->Position = PositionNode;
  Hitbox->Triangle.Base = Base;
  Hitbox->Triangle.Height = Height;
  Hitbox->Triangle.CenterPoint = CenterPoint;
}

internal void getTriangle2DPointsCentroidAtOrigin(hitbox_triangle* HitboxTriangle, v2* A, v2* B, v2* C)
{
  *A = V2(-HitboxTriangle->Base * HitboxTriangle->CenterPoint, 0);
  *B = V2( HitboxTriangle->Base * (1.f-HitboxTriangle->CenterPoint), 0);
  *C = V2(0, HitboxTriangle->Height);
  v2 Centroid = GetTriangleCentroid(*A,*B,*C);
  *A -= Centroid;
  *B -= Centroid;
  *C -= Centroid;
}

bool Intersects(component_hitbox* HitboxComponent, world_coordinate IntersectionPoint)
{
  b32 Result = false;
  switch(HitboxComponent->Type)
  {
    case HitboxType::CIRCLE:
    {
      r32 Radius = Norm(IntersectionPoint - HitboxComponent->Position->AbsolutePosition);
      Result = Radius < HitboxComponent->Circle.Radius;
    }break;
    case HitboxType::RECTANGLE:
    {
      // If the roatation angle of the htibox is off center, this is the vector from the center 
      // of the hitbox to the center of rotation, if it's zero, it means the rotation point is equal to 
      // the geometric center.
      world_coordinate RotationCenterOffset = {};

      hitbox_rectangle HitboxRect = HitboxComponent->Rectangle;

      // Translate IntersectionPoint to have origin at the rectangle
      world_coordinate PointToTest = IntersectionPoint - HitboxComponent->Position->AbsolutePosition;
      
      // Rotate the point we want to test "backwards" instead of rotating the hitbox "Forward"
      r32 Rotation = -HitboxComponent->Position->AbsoluteRotation;
      v3 RotatedPointToTest = M3(Cos(Rotation), -Sin(Rotation), 0,
                                 Sin(Rotation),  Cos(Rotation), 0,
                                 0,              0,             1) * PointToTest;
      // LowerLeft
      v3 A = V3(-HitboxRect.Width, -HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;
      // LowerRight
      v3 B = V3( HitboxRect.Width, -HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;
      // UpperRight
      v3 C = V3( HitboxRect.Width,  HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;
      // UpperLeft
      v3 D = V3(-HitboxRect.Width,  HitboxRect.Height, 0) * 0.5f - RotationCenterOffset;

      b32 InLowerLeftTriangle  = IsVertexInsideTriangle(RotatedPointToTest, V3(0, 0, 1), A, B, D);
      b32 InUpperRightTriangle = IsVertexInsideTriangle(RotatedPointToTest, V3(0, 0, 1), B, C, D);
      
      Result = InLowerLeftTriangle || InUpperRightTriangle;
  
    }break;
    case HitboxType::TRIANGLE:
    {
      // If the roatation angle of the htibox is off center, this is the vector from the center 
      // of the hitbox to the center of rotation, if it's zero, it means the rotation point is equal to 
      // the geometric center.
      world_coordinate RotationCenterOffset = {};

      hitbox_triangle HitboxTriangle = HitboxComponent->Triangle;

      v2 A,B,C;
      getTriangle2DPointsCentroidAtOrigin(&HitboxTriangle, &A,&B,&C);
      A -= V2(RotationCenterOffset);
      B -= V2(RotationCenterOffset);
      C -= V2(RotationCenterOffset);

      // Translate IntersectionPoint to have origin at the rectangle
      world_coordinate PointToTest = IntersectionPoint - (HitboxComponent->Position->AbsolutePosition);

      // Rotate the point we want to test "backwards" instead of rotating the hitbox "Forward"
      r32 Rotation = -HitboxComponent->Position->AbsoluteRotation;
      v3 RotatedPointToTest = M3(Cos(Rotation), -Sin(Rotation), 0,
                                 Sin(Rotation),  Cos(Rotation), 0,
                                 0,              0,             1) * PointToTest;
      
      v3 TriangleNormal = V3(0, 0, 1);
      Result  = IsVertexInsideTriangle(RotatedPointToTest, TriangleNormal, V3(A,0), V3(B,0), V3(C,0));
      if(Result)
      {
        int a = 0;
      }
    }break;
  }

  return Result;
}
