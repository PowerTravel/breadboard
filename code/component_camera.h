#pragma once

#include "math/affine_transformations.h"
#include "coordinate_systems.h"

struct world;

void CameraSystemUpdate( world* World );

// Todo, Can we use a location component instead of DeltaRot, DeltaPos etc?
struct component_camera
{
  r32 AngleOfView;
  r32 AspectRatio;
  r32 OrthoZoom; // Just used in orthographic projection to control how much of the screen is rendered
  m4  DeltaRot;
  v3  DeltaPos;
  m4  V;
  m4  P;
};

rect2f GetCameraScreenRect(r32 Zoom);

void LookAt( component_camera* Camera, v3 aFrom,  v3 aTo,  v3 aTmp = V3(0,1,0) );

struct ray
{
  v3 Origin;
  v3 Direction;
};

// Works only for 3d projection camera
ray GetRayFromCamera(component_camera* Camera, canonical_screen_coordinate MouseCanPos);
ray GetRayFromCamera(component_camera* Camera, v2 MousePos);
void GetCameraDirections(component_camera* Camera, v3* Up, v3* Right, v3* Forward);
void TranslateCamera( component_camera* Camera, const v3& DeltaP  );
void RotateCamera( component_camera* Camera, const r32 DeltaAngle, const v3& Axis );
void UpdateViewMatrix(  component_camera* Camera );
void SetOrthoProj( component_camera* Camera, r32 aNear, r32 aFar, r32 aRight, r32 aLeft, r32 aTop, r32 aBottom );
void SetOrthoProj( component_camera* Camera, r32 n, r32 f );
void SetPerspectiveProj( component_camera* Camera, r32 n, r32 f, r32 r, r32 l, r32 t, r32 b );
void SetPerspectiveProj( component_camera* Camera, r32 n, r32 f );
void UpdateViewMatrixAngularMovement(  component_camera* Camera );
#if 0
void setPinholeCamera( r32 filmApertureHeight, r32 filmApertureWidth,
                       r32 focalLength, r32 nearClippingPlane,
                       r32 inchToMM = 25.4f );
#endif

void SetCameraComponent(component_camera* Camera, r32 AngleOfView, r32 AspectRatio );
void gluPerspective(
  const r32& angleOfView, const r32& imageAspectRatio,
  const r32& n, const r32& f, r32& b, r32& t, r32& l, r32& r);
// set the OpenGL perspective projection matrix
void glFrustum( const r32& b, const r32& t,
                const r32& l, const r32& r,
                const r32& n, const r32& f, m4& M);
