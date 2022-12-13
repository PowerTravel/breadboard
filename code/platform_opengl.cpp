
// TODO: Decide upon a render design.
// Use asset manager or no?
// Simplify adding programs and shaders.
// Pass a render state?
// Expose buffers? (Make the caller responsible for sending data and textures to gpu?)
// Make shaders part of an asset to be passed with a render object?
// Render passes.
// Maybe keep it simple and disorganized for now till later and you know better what the requirements are.

// Could have some functions for sending geometry and shaders to the gpu and get a handle back which if you supply with some 
// data will make sure the buffers are set up, and the rendering is done correctly.


#include "render_push_buffer.h"
#include "math/affine_transformations.h"
#include "bitmap.h"
#include "assets.cpp"

internal void SendDataToBuffer(GLenum BufferType, glHandle BufferHandle, u32 BufferOffset, u32 DataSizeBytes, void* Data)
{
  glBindBuffer(BufferType, BufferHandle);
  glBufferSubData(BufferType,                    // Target
                  BufferOffset,                       // Offset into buffer to start writing data, starting at beginning
                  DataSizeBytes,                      // Size
                  (GLvoid*) Data);                    // Data
  glBindBuffer(BufferType, 0);
}

inline internal void OpenGLPrintProgramLog(GLuint Program)
{
  GLint ProgramMessageSize;
  glGetProgramiv( Program, GL_INFO_LOG_LENGTH, &ProgramMessageSize );
  char* ProgramMessage = (char*) VirtualAlloc(0, ProgramMessageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  glGetProgramInfoLog(Program, ProgramMessageSize, NULL, (GLchar*) ProgramMessage);
  OutputDebugStringA(ProgramMessage);
  VirtualFree(ProgramMessage, 0, MEM_RELEASE);
}

inline internal void OpenGLPrintShaderLog(GLuint Shader)
{
  GLint CompileMessageSize;
  glGetShaderiv( Shader, GL_INFO_LOG_LENGTH, &CompileMessageSize );
  char* CompileMessage = (char*) VirtualAlloc(0, CompileMessageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  glGetShaderInfoLog(Shader, CompileMessageSize, NULL, (GLchar*) CompileMessage);
  OutputDebugStringA(CompileMessage);
  VirtualFree(CompileMessage, 0, MEM_RELEASE);
}


static GLuint OpenGLLoadShader(u32 ShaderCount, const char** SourceCode, GLenum ShaderType )
{
  GLuint Shader = glCreateShader(ShaderType);
  glShaderSource( Shader, ShaderCount, (const GLchar**) SourceCode, NULL );
  glCompileShader( Shader );
  GLint Compiled;
  glGetShaderiv(Shader, GL_COMPILE_STATUS, &Compiled);
  if(!Compiled)
  {
    OpenGLPrintShaderLog(Shader);
    exit(1);
  }

  return Shader;
};

static GLuint OpenGLCreateProgram(char* Defines, char* VertexShaderBody, char* FragmentShaderBody )
{
  const char* VertexShaderCode[] = 
  {
    Defines,
    VertexShaderBody
  };

  const char* FragmentShaderCode[] = 
  {
    Defines,
    FragmentShaderBody
  };

  GLuint VertexShader   = OpenGLLoadShader(ArrayCount(VertexShaderCode), VertexShaderCode,   GL_VERTEX_SHADER   );
  GLuint FragmentShader = OpenGLLoadShader(ArrayCount(FragmentShaderCode), FragmentShaderCode, GL_FRAGMENT_SHADER );
  
  GLuint Program = glCreateProgram();

  // Attatch the shader to the program
  glAttachShader(Program, VertexShader);
  glAttachShader(Program, FragmentShader);
  glLinkProgram(Program);

  GLint Linked;
  glGetProgramiv(Program, GL_LINK_STATUS, &Linked);
  if(!Linked)
  {
    OpenGLPrintProgramLog(Program);
    exit(1);
  }

  glValidateProgram(Program);
  GLint Valid;
  glGetProgramiv(Program, GL_VALIDATE_STATUS, &Valid);
  if(!Valid)
  {
    OpenGLPrintProgramLog(Program);
    exit(1);
  }

  glDetachShader(Program, VertexShader);
  glDetachShader(Program, FragmentShader);
  
  glDeleteShader(VertexShader);
  glDeleteShader(FragmentShader);
  
  
  return Program;
}

// glDrawElementsInstanced is used when we have two buffers (1 Buffer with a geometry and another buffer with Instance Data )
// https://stackoverflow.com/questions/24516993/is-it-possible-to-use-index-buffer-objects-ibo-with-the-function-glmultidrawe
// glDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws the VertexBuffer using an offset into IndexBuffer
// glMultiDrawElements uses 1 VertexBuffer and 1 IndexBuffer and draws n number of whats in the VertexBuffer using an offset into IndexBuffer specific for each n


// Draws a textured 2-D Quad on the X-Y Plane.
// Expects geometry to be a Quad with side 1.
// Options:
//     - Texture
//     - Color
// -1,1                        1,1
//   |------X,Y Screen Space----|
//   |                          |
//   |   -0.5,0.5     0.5,0.5   |
//   |       |-----------|      |
//   |       |---QUAD----|      |
//   |       |----0.0----|      |
//   |       |-----------|      |
//   |       |-----------|      |
//   |   -0.5,-0.5    0.5,0.5   |
//   |                          |
//   |--------------------------|
// -1,-1                       1,1

opengl_program OpenGLQuad2DProgram(u32 UseTexture, u32 UseColor, u32 SpecialTexture)
{
  c8 Headers[4096] = {};

  // We havent initialized Platform.DEBUGFormatString yet, so this  is a quick workaround for now
  u32 Size = FormatString(Headers, sizeof(Headers),
  "#version 330 core\n#define TEXTURE %d\n#define COLORIZE %d\n #define SPECIAL_TEXTURE %d\n",
    UseTexture,
    UseColor,
    SpecialTexture);

  char VertexShaderCode[] = R"FOO(
uniform mat4 ProjectionMat;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
uniform mat4 ViewMat;        // View Matrix - Transforms points from WorldSpace to ScreenSpace.
layout (location = 0) in vec3 vertice;
layout (location = 1) in vec3 normal; // Wasted in 2D. Always points in z+, Possible optimization
layout (location = 2) in vec2 uv;
layout (location = 3) in int TexSlot;
layout (location = 4) in vec4 QuadRect; // X,Y,W,H
layout (location = 5) in float RotationAngle; // X,Y,W,H
layout (location = 6) in vec4 UVRect;   // X,Y,W,H
layout (location = 7) in vec4 Color;    // R,G,B,A
layout (location = 8) in vec2 RotationCenterOffset;    // x,y
out vec4 VertexColor;
out vec3 ArrayUV;
void main()
{
#if 0
  float CosAngle = cos(RotationAngle);
  float SinAngle = sin(RotationAngle);
  float Width = QuadRect.z;
  float Height = QuadRect.w;
  float X = QuadRect.x;
  float Y = QuadRect.y;
  float dRX = RotationCenterOffset.x;
  float dRY = RotationCenterOffset.y;

  float E11 = CosAngle * Width;
  float E12 = -SinAngle * Height;
  float E21 = SinAngle * Width;
  float E22 = CosAngle * Height;

  float R13 = X - dRX - E11*dRX - E12*dRY;
  float R23 = Y - dRY - E21*dRX - E22*dRY;

  mat4 ModelMatrix;
  ModelMatrix[0] = vec4(E11, E21, 0, 0);
  ModelMatrix[1] = vec4(E12, E22, 0, 0);
  ModelMatrix[2] = vec4(0, 0, 1, 0);
  ModelMatrix[3] = vec4(R13, R23, 0, 1);
  gl_Position = ProjectionMat * ViewMat * ModelMatrix * vec4(vertice.xyz,1);

#else

  float CosAngle = cos(RotationAngle);
  float SinAngle = sin(RotationAngle);
  float Width = QuadRect.z;
  float Height = QuadRect.w;
  float X = QuadRect.x;
  float Y = QuadRect.y;
  float dRX = RotationCenterOffset.x;
  float dRY = RotationCenterOffset.y;

  mat4 RotationMatrix;
  RotationMatrix[0] = vec4(CosAngle, SinAngle, 0, 0);
  RotationMatrix[1] = vec4(-SinAngle, CosAngle, 0, 0);
  RotationMatrix[2] = vec4(0, 0, 1, 0);
  RotationMatrix[3] = vec4(0, 0, 0, 1);

  mat4 ScaleMatrix;
  ScaleMatrix[0] = vec4(Width, 0, 0, 0);
  ScaleMatrix[1] = vec4(0, Height, 0, 0);
  ScaleMatrix[2] = vec4(0, 0, 1, 0);
  ScaleMatrix[3] = vec4(0, 0, 0, 1);

  mat4 TransMat;
  TransMat[0] = vec4(1, 0, 0, 0);
  TransMat[1] = vec4(0, 1, 0, 0);
  TransMat[2] = vec4(0, 0, 1, 0);
  TransMat[3] = vec4(X, Y, 0, 1);

  mat4 TransMatInv;
  TransMatInv[0] = vec4(1, 0, 0, 0);
  TransMatInv[1] = vec4(0, 1, 0, 0);
  TransMatInv[2] = vec4(0, 0, 1, 0);
  TransMatInv[3] = vec4(-X, -Y, 0, 1);

  mat4 TransCmMat;
  TransCmMat[0] = vec4(1, 0, 0, 0);
  TransCmMat[1] = vec4(0, 1, 0, 0);
  TransCmMat[2] = vec4(0, 0, 1, 0);
  TransCmMat[3] = vec4(dRX, dRY, 0, 1);

  mat4 TransCmMatInv;
  TransCmMatInv[0] = vec4(1, 0, 0, 0);
  TransCmMatInv[1] = vec4(0, 1, 0, 0);
  TransCmMatInv[2] = vec4(0, 0, 1, 0);
  TransCmMatInv[3] = vec4(-dRX, -dRY, 0, 1);

  mat4 ModelMatrix = TransMat * RotationMatrix * TransCmMatInv * ScaleMatrix;
  gl_Position = ProjectionMat * ViewMat * ModelMatrix * vec4(vertice.xyz,1);

#endif
  mat3 TextureTransform;
  TextureTransform[0] = vec3(UVRect.z, 0, 0);
  TextureTransform[1] = vec3(0, UVRect.w, 0);
  TextureTransform[2] = vec3(UVRect.xy, 1);
  vec2 TextureCoordinate = (TextureTransform*vec3(uv.xy,1)).xy;
  ArrayUV = vec3(TextureCoordinate.x, TextureCoordinate.y, TexSlot);

  VertexColor = Color;
}
)FOO";
  
  char* FragmentShaderCode = R"FOO(
out vec4 fragColor;
in vec4 VertexColor;
in vec3 ArrayUV;
uniform sampler2D SpecialTexture;      // One per draw call (unused atm), Need to bind before draw
uniform sampler2DArray TextureSampler; // 2D array of textures
void main() 
{
  fragColor = vec4(1,1,1,1);

#if TEXTURE
  #if SPECIAL_TEXTURE
    fragColor = fragColor * texture(SpecialTexture, ArrayUV.xy);
  #else
    fragColor = fragColor * texture(TextureSampler, ArrayUV);
  #endif
#endif

#if COLORIZE
  fragColor = fragColor * VertexColor;
#endif

}
)FOO";

  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram(Headers, VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.ProjectionMat = glGetUniformLocation(Result.Program, "ProjectionMat");
  Result.ViewMat = glGetUniformLocation(Result.Program, "ViewMat");
  glUseProgram(0);
  
  return Result;
}

opengl_program OpenGLSolid2D()
{
  char VertexShaderCode[] = R"FOO(
uniform mat4 ViewMat;        // View Matrix - Transforms points from WorldSpace to ScreenSpace.
uniform mat4 ProjectionMat;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
layout (location = 0)  in vec2 vertice;
layout (location = 1)  in vec2 value;
layout (location = 3)  in vec2 Position;
layout (location = 4)  in vec2 Scale;
layout (location = 5)  in vec4 Color;
layout (location = 6)  in float RotationAngle;
out vec4 VertColor;
void main()
{

  float CosAngle = cos(RotationAngle);
  float SinAngle = sin(RotationAngle);
  float Width = Scale.x;
  float Height = Scale.y;
  float X = Position.x;
  float Y = Position.y;

  mat4 RotationMatrix;
  RotationMatrix[0] = vec4(CosAngle, SinAngle, 0, 0);
  RotationMatrix[1] = vec4(-SinAngle, CosAngle, 0, 0);
  RotationMatrix[2] = vec4(0, 0, 1, 0);
  RotationMatrix[3] = vec4(0, 0, 0, 1);

  mat4 ScaleMatrix;
  ScaleMatrix[0] = vec4(Width, 0, 0, 0);
  ScaleMatrix[1] = vec4(0, Height, 0, 0);
  ScaleMatrix[2] = vec4(0, 0, 1, 0);
  ScaleMatrix[3] = vec4(0, 0, 0, 1);

  mat4 TransMat;
  TransMat[0] = vec4(1, 0, 0, 0);
  TransMat[1] = vec4(0, 1, 0, 0);
  TransMat[2] = vec4(0, 0, 1, 0);
  TransMat[3] = vec4(X, Y, 0, 1);

  mat4 ModelMatrix = TransMat * RotationMatrix * ScaleMatrix;
  gl_Position = ProjectionMat * ViewMat * ModelMatrix * vec4(vertice.xy, 0, 1);

  VertColor = Color;
}
 )FOO";

   char* FragmentShaderCode = R"FOO(
out vec4 fragColor;
in vec4 VertColor;
void main() 
{
  fragColor = VertColor;
}
)FOO";

  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram("#version 330 core\n", VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.ProjectionMat = glGetUniformLocation(Result.Program, "ProjectionMat");
  Result.ViewMat = glGetUniformLocation(Result.Program, "ViewMat");
  glUseProgram(0);
  
  return Result;
}

opengl_program OpenGLCircle2D()
{
  char VertexShaderCode[] = R"FOO(
uniform mat4 ViewMat;        // View Matrix - Transforms points from WorldSpace to ScreenSpace.
uniform mat4 ProjectionMat;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
layout (location = 0)  in vec2 v;
layout (location = 3)  in vec2 Position;
layout (location = 4)  in vec2 Scale;
layout (location = 5)  in vec4 Color;
layout (location = 6)  in float Thickness;
out vec4 VertColor;
out vec2 VertValue;
out float VertThickness;
void main()
{
  mat4 ModelMatrix;
  ModelMatrix[0] = vec4(Scale.x, 0, 0, Position.x);
  ModelMatrix[1] = vec4(0, Scale.y, 0, Position.y);
  ModelMatrix[2] = vec4(0, 0, 1, 0);
  ModelMatrix[3] = vec4(0, 0, 0, 1);
  ModelMatrix = transpose(ModelMatrix);

  gl_Position = ProjectionMat*ViewMat*ModelMatrix*vec4(v.xy,0,1);

  VertColor = Color;
  VertValue = v;
  VertThickness = Thickness;
}
 )FOO";

   char* FragmentShaderCode = R"FOO(
out vec4 fragColor;
in vec2 VertValue;
in vec4 VertColor;
in float VertThickness;
void main() 
{
  float OuterRadius = 0.5;
  float InnerRadius = OuterRadius - VertThickness;
  
  #if 1
  float Dist = sqrt(dot(VertValue, VertValue));
  if(Dist >= OuterRadius || Dist <= InnerRadius)
  {
    discard;
  }
  float sm = smoothstep(OuterRadius,OuterRadius-0.01,Dist);
  float sm2 = smoothstep(InnerRadius,InnerRadius+0.01,Dist);
  float alpha = sm*sm2 * VertColor.w;
  fragColor = vec4(VertColor.xyz, alpha);

  #else

  if(abs(VertValue.x) <= InnerRadius && abs(VertValue.y) <= InnerRadius)
  {
    discard;
  }

  fragColor = VertColor;
  #endif
}
)FOO";

  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram("#version 330 core\n", VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.ProjectionMat = glGetUniformLocation(Result.Program, "ProjectionMat");
  Result.ViewMat = glGetUniformLocation(Result.Program, "ViewMat");
  glUseProgram(0);
  
  return Result;
}

opengl_program OpenGLQuad3DProgram()
{
  char VertexShaderCode[] = R"FOO(
uniform mat4 ViewMat;        // View Matrix - Transforms points from WorldSpace to ScreenSpace.
uniform mat4 ProjectionMat;  // Projection Matrix - Transforms points from ScreenSpace to UnitQube.
layout (location = 0)  in vec3 vertice;
layout (location = 2)  in vec2 uv;
layout (location = 3)  in int TexSlot;
// Since gl is Column Major each row passed in the buffer becomes a column here
// Something is fishy, I have to transpose the matrix to get the right behaviour...
// Each M_Col corresponds to a row in the actual program.
// So either, we arent in column major mode OR ModelMatrix[0] refers to a row, not a column
layout (location = 4)  in vec4 M_Col0;
layout (location = 5)  in vec4 M_Col1;
layout (location = 6)  in vec4 M_Col2;
layout (location = 7)  in vec4 M_Col3;
layout (location = 8)  in vec3 TM_Col0;
layout (location = 9)  in vec3 TM_Col1;
layout (location = 10) in vec3 TM_Col2;
out vec3 ArrayUV;
out vec4 TestColor;
void main()
{
  mat4 ModelMatrix;
  ModelMatrix[0] = M_Col0;
  ModelMatrix[1] = M_Col1;
  ModelMatrix[2] = M_Col2;
  ModelMatrix[3] = M_Col3;
  ModelMatrix = transpose(ModelMatrix);

  mat3 TextureMatrix;
  TextureMatrix[0] = TM_Col0;
  TextureMatrix[1] = TM_Col1;
  TextureMatrix[2] = TM_Col2;
  TextureMatrix = transpose(TextureMatrix);

  gl_Position = ProjectionMat*ViewMat*ModelMatrix*vec4(vertice.xyz,1);

  vec2 TextureCoordinate = (TextureMatrix*vec3(uv.xy,1)).xy;
  ArrayUV = vec3(TextureCoordinate.x, TextureCoordinate.y, TexSlot);

  TestColor = M_Col0;
}
)FOO";
  
  char* FragmentShaderCode = R"FOO(
out vec4 fragColor;
in vec3 ArrayUV;
in vec4 TestColor;
uniform sampler2DArray TextureSampler;
void main() 
{
  vec4 Sample = texture(TextureSampler, ArrayUV);
  fragColor = Sample;
  //fragColor = vec4(1,0,0,1);
}
)FOO";
  
  opengl_program Result = {};
  Result.Program = OpenGLCreateProgram("#version 330 core\n", VertexShaderCode, FragmentShaderCode );
  glUseProgram(Result.Program);
  Result.ProjectionMat = glGetUniformLocation(Result.Program, "ProjectionMat");
  Result.ViewMat = glGetUniformLocation(Result.Program, "ViewMat");
  glUseProgram(0);
  
  return Result;
}

internal opengl_info
OpenGLGetExtensions()
{
  opengl_info Result = {};
  
  Result.Vendor   = (char*) glGetString(GL_VENDOR);
  Result.Renderer = (char*) glGetString(GL_RENDERER);
  Result.Version  = (char*) glGetString(GL_VERSION);
  
  Result.ShadingLanguageVersion = (char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
  Result.Extensions = (char*) glGetString(GL_EXTENSIONS);
  
  u32 ExtensionsStringLength = str::StringLength( Result.Extensions );
  char* ExtensionStart = Result.Extensions;
  char* Tokens = " \t\n";
  char* ExtensionEnd   = str::FindFirstOf( Tokens, ExtensionStart );
//  Platform.DEBUGPrint("%s\n", Result.Extensions);
  while( ExtensionStart )
  {
    u64 ExtensionStringLength =  ExtensionEnd ? (u64) (ExtensionEnd - ExtensionStart) : (u64) (str::StringLength(ExtensionStart));
    
    if( str::Contains( str::StringLength( "GL_ARB_debug_output" ), "GL_ARB_debug_output",
                      ExtensionStringLength, ExtensionStart ) )
    {
      int a = 10;
    }

    // NOTE: EXT_texture_sRGB_decode has been core since 2.1
    if( str::Contains( str::StringLength( "EXT_texture_sRGB_decode" ), "EXT_texture_sRGB_decode",
                      ExtensionStringLength, ExtensionStart ) )
    {
      Result.EXT_texture_sRGB_decode = true;
    }
    if( str::Contains( str::StringLength( "GL_ARB_framebuffer_sRGB" ), "GL_ARB_framebuffer_sRGB",
                      ExtensionStringLength, ExtensionStart ) )
    {
      Result.GL_ARB_framebuffer_sRGB = true;
    }
    
    if( str::Contains( str::StringLength( "GL_ARB_vertex_shader" ), "GL_ARB_vertex_shader",
                      ExtensionStringLength, ExtensionStart ) )
    {
      Result.GL_ARB_vertex_shader = true;
    }
    
    if( str::Contains( str::StringLength( "GL_EXT_blend_func_separate" ), "GL_EXT_blend_func_separate",
                      ExtensionStringLength, ExtensionStart ) )
    {
      Result.GL_blend_func_separate = true;
    }
    
    ExtensionStart = str::FindFirstNotOf(Tokens, ExtensionEnd);
    ExtensionEnd   = str::FindFirstOf( Tokens, ExtensionStart );
  }
  
  return Result;
}

opengl_info OpenGLInitExtensions()
{
  opengl_info Info = OpenGLGetExtensions();
  if(Info.EXT_texture_sRGB_decode)
  {
    //OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
  }
  
  if(Info.GL_ARB_framebuffer_sRGB)
  {
    // Will take as input LinearRGB and convert to sRGB Space.
    // sRGB = Pow(LinearRGB, 1/2.2)
    // glEnable(GL_FRAMEBUFFER_SRGB);
  }
  
  if(Info.GL_blend_func_separate)
  {
    // Not sure I want this
    //glEnable(GL_BLEND);
  }
  
  if(Info.GL_blend_func_separate)
  {
    // Not sure I want this
    //glEnable(GL_BLEND);
  }
  return Info;
}

void ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
  Platform.DEBUGPrint("Source: %d, Type: %d, ID: %d, Severity %d, %s\n",source, type, id, severity, message);
}


glHandle EnableAttribArrayQuad2DData(glHandle ElementEABO, glHandle ElementVBO, glHandle InstanceVBO, u32 OffsetInBuffer)
{
  glHandle VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementEABO);
  glBindBuffer(GL_ARRAY_BUFFER, ElementVBO);
  // The textVAO now implicitly binds these parameters to whatever VBO is currently bound (ObjectKeeper->VBO)
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, v));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vn));
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(opengl_vertex), (GLvoid*) OffsetOf(opengl_vertex, vt));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ARRAY_BUFFER, InstanceVBO);
  // The textVAO now implicitly binds these parameters to whatever VBO is currently bound (quadInstanceVBO)
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);
  glEnableVertexAttribArray(6);
  glEnableVertexAttribArray(7);
  glEnableVertexAttribArray(8);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(quad_2d_data), (void *)(OffsetInBuffer + OffsetOf(quad_2d_data,TextureSlot)));
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(quad_2d_data), (void *)(OffsetInBuffer + OffsetOf(quad_2d_data,QuadRect)));
  glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(quad_2d_data), (void *)(OffsetInBuffer + OffsetOf(quad_2d_data,Rotation)));
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(quad_2d_data), (void *)(OffsetInBuffer + OffsetOf(quad_2d_data,UVRect)));
  glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(quad_2d_data), (void *)(OffsetInBuffer + OffsetOf(quad_2d_data,Color)));
  glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, sizeof(quad_2d_data), (void *)(OffsetInBuffer + OffsetOf(quad_2d_data,RotationCenterOffset)));
  glVertexAttribDivisor(3, 1);
  glVertexAttribDivisor(4, 1);
  glVertexAttribDivisor(5, 1);
  glVertexAttribDivisor(6, 1);
  glVertexAttribDivisor(7, 1);
  glVertexAttribDivisor(8, 1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glBindVertexArray(0);
  return VAO;
}


internal void DefineDataInVertexBuffer_Circle2D(glHandle VertexBufferHandle, u32 OffsetInBuffer)
{
  glBindBuffer(GL_ARRAY_BUFFER, VertexBufferHandle);
}

// EnableBufferAttribute enables a data value on the currently bound GL_ARRAY_BUFFER buffer
// Params:
// u32 DivisorId               : Id of divisor layout (location DivisorId), 0, 1, 2, 3, ...
// u32 BaseTypeCount           : How many of BaseType is required to read the whole attribute
// GLenum BaseType             : GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT and GL_FLOAT
// u32 UpdateRate              : 0 means once per vertex, 1 means once per instance, 2 means it updates every other instance etc...
// bptr ByteOffsetInStruct     : Byte-Pointer to where the value in the struct starts. Buffer starts at 0.
// u32 OffsetOfStruct          : Number of bytes from base of struct where the value starts
//     Note: OffsetOfStructInBytes and OffsetInStructOfBytes combine to point out the first occurance of the value
// u32 SizeOfStruct           : The size of the struct
//     Note: This is the stride between occurances of the data type
internal void EnableBufferAttribute(u32 DivisorId, u32 BaseTypeCount, GLenum BaseType, u32 UpdateRate, u32 OffsetOfStruct, umm OffsetInStruct, u32 SizeOfStruct)
{
  // glEnableVertexAttribArray(n) enables
  //     layout (location n) in dataType variableName
  // to be used in vertex shader
  // dataType : float, int, vec2, vec3, vec4 etc
  // n : 0,1,2,3,...
  glEnableVertexAttribArray(DivisorId);

  // glVertexAttribPointer defines how the data is to be interpreted:
  glVertexAttribPointer(
    DivisorId,                                    // Divisor ID
    BaseTypeCount,                                // Size (number of Type specified below)
    BaseType,                                     // Type
    GL_FALSE,                                     // Should value be normalized
    SizeOfStruct,                                 // Stride between entries
    (GLvoid*) (OffsetOfStruct + OffsetInStruct)   // Pointer to to first occurance in bytes. Base of buffer is at 0
    );

  // Specifies the rate at which each value is advanced. 0 means it's once per vertex. 1 means per once instance, 2 means every second instance etc.
  // It is 0 by default
  glVertexAttribDivisor(DivisorId, UpdateRate);  
}

internal void EnableBufferAttributePerVertex(u32 DivisorId, u32 BaseTypeCount, GLenum BaseType, u32 OffsetOfStruct, umm OffsetInStruct, u32 SizeOfStruct)
{
  EnableBufferAttribute(DivisorId, BaseTypeCount, BaseType, 0, OffsetOfStruct, OffsetInStruct, SizeOfStruct);
}

internal void EnableBufferAttributePerInstance(u32 DivisorId, u32 BaseTypeCount, GLenum BaseType, u32 OffsetOfStruct, umm ByteOffsetInStruct, u32 SizeOfStruct)
{
  EnableBufferAttribute(DivisorId, BaseTypeCount, BaseType, 1, OffsetOfStruct, ByteOffsetInStruct, SizeOfStruct);
}

glHandle EnableAttribArrayCircle2DData(glHandle IndexBufferHandle, glHandle VertexBufferHandle, glHandle InstanceBufferHandle, u32 OffsetInVertexBuffer, u32 OffsetInInstanceBuffer)
{

  glHandle VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  
  // The IndexBufferHandle is now implicitly bound to the VertexArrayObject (VAO)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferHandle);

  // This implicitly binds the VertexBufferHandle ArrayBuffer and it's data-structure is now implicitly bound to the bound VertexArrayObject
  glBindBuffer(GL_ARRAY_BUFFER, VertexBufferHandle);
  EnableBufferAttributePerVertex(0, 2, GL_FLOAT, OffsetInVertexBuffer, OffsetOf(circle_2d_vertex, v),     sizeof(circle_2d_vertex));
  EnableBufferAttributePerVertex(1, 2, GL_FLOAT, OffsetInVertexBuffer, OffsetOf(circle_2d_vertex, Value), sizeof(circle_2d_vertex));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // This implicitly binds the InstanceBufferHandle ArrayBuffer and it's data-structure is now implicitly bound to the bound VertexArrayObject
  glBindBuffer(GL_ARRAY_BUFFER, InstanceBufferHandle);
  EnableBufferAttributePerInstance(3, 2, GL_FLOAT, OffsetInInstanceBuffer, OffsetOf(circle_2d_data, Position), sizeof(circle_2d_data));
  EnableBufferAttributePerInstance(4, 2, GL_FLOAT, OffsetInInstanceBuffer, OffsetOf(circle_2d_data, Scale),    sizeof(circle_2d_data));
  EnableBufferAttributePerInstance(5, 4, GL_FLOAT, OffsetInInstanceBuffer, OffsetOf(circle_2d_data, Color),    sizeof(circle_2d_data));
  EnableBufferAttributePerInstance(6, 1, GL_FLOAT, OffsetInInstanceBuffer, OffsetOf(circle_2d_data, Thickness),sizeof(circle_2d_data));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);
  return VAO;
}


void InitOpenGL(open_gl* OpenGL)
{
  OpenGL->Info = OpenGLInitExtensions();
  OpenGL->DefaultInternalTextureFormat = GL_RGBA8;
  OpenGL->DefaultTextureFormat = GL_RGBA;
  OpenGL->DefaultTextureFormat = GL_BGRA_EXT;
  OpenGL->MaxTextureCount = NORMAL_TEXTURE_COUNT;
  OpenGL->MaxSpecialTextureCount = SPECIAL_TEXTURE_COUNT;

  OpenGL->Quad2DProgram = OpenGLQuad2DProgram(true,true,false);
  OpenGL->Quad2DProgramSpecial = OpenGLQuad2DProgram(true,true,true);
  OpenGL->Colored2DQuadProgram = OpenGLQuad2DProgram(false,true,false);
  OpenGL->Circle2DProgram = OpenGLCircle2D();
  OpenGL->Solid2DProgram  = OpenGLSolid2D();

  // 
  OpenGL->BufferSize = Megabytes(32);
  
  // Enable 2D Textures
  glEnable(GL_TEXTURE_2D);
  // Enable 3D Textures
  glEnable(GL_TEXTURE_3D);

  // Enable backface culling  
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  ///
  
  OpenGL->SignleWhitePixelTexture = 0;
  glGenTextures(1, &OpenGL->SignleWhitePixelTexture);
  glBindTexture(GL_TEXTURE_2D, OpenGL->SignleWhitePixelTexture);
  u8 WhitePixel[4] = {0xFF,0xFF,0xFF,0xFF};
  glTexImage2D(GL_TEXTURE_2D, 0, OpenGL->DefaultInternalTextureFormat, 1, 1, 0, OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, WhitePixel);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  
  ////
  
  glGenTextures(OpenGL->MaxSpecialTextureCount, OpenGL->SpecialTextures);
  for (int i = 0; i < SPECIAL_TEXTURE_COUNT; ++i)
  {
    glBindTexture(GL_TEXTURE_2D, OpenGL->SpecialTextures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  }
  
  ////
  
  glGenTextures(1, &OpenGL->TextureArray);
  glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
  
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  
  b32 highesMipLevelReached = false;
  
  u32 ImageWidth = TEXTURE_ARRAY_DIM;
  u32 ImageHeight = TEXTURE_ARRAY_DIM;
  
#if 0
  // Using MipMaps
  u32 mipLevel = 0;
  while(!highesMipLevelReached)
  {
    glTexImage3D(GL_TEXTURE_2D_ARRAY, mipLevel, OpenGL->DefaultInternalTextureFormat,
                 ImageWidth, ImageHeight, OpenGL->MaxTextureCount, 0,
                 OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, 0);
    
    if((ImageWidth == 1) && (ImageHeight == 1))
    {
      highesMipLevelReached = true;
      ImageWidth  = 0;
      ImageHeight = 0;
    }
    mipLevel++;
    ImageWidth =  (ImageWidth>1)  ?  (ImageWidth+1)/2  : ImageWidth;
    ImageHeight = (ImageHeight>1) ?  (ImageHeight+1)/2 : ImageHeight;
  }
#else
  // Not using mipmaps until we need it
  glTexImage3D(GL_TEXTURE_2D_ARRAY,
               0,
               OpenGL->DefaultInternalTextureFormat,
               ImageWidth, ImageHeight, OpenGL->MaxTextureCount, 0,
               OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, 0);
#endif

  // These below handle fonts and shit, don't want to merge them yet into a single buffer
  // Remnant from handmade project
  glGenBuffers(1, &OpenGL->ElementVBO);
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  glBufferData(GL_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glGenBuffers(1, &OpenGL->ElementEBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  
  glGenBuffers(1, &OpenGL->InstanceVBO);
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO);
  glBufferData(GL_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_STREAM_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  OpenGL->Quad2DVAO = EnableAttribArrayQuad2DData(OpenGL->ElementEBO, OpenGL->ElementVBO, OpenGL->InstanceVBO, 0);
  


  circle_2d_vertex VertexData[7] = 
  {
    //   x, y,       vx,   vy
    {V2(-0.5,-0.5), V2(-0.5, -0.5)}, // 0 Square
    {V2( 0.5,-0.5), V2( 0.5, -0.5)}, // 1 Square
    {V2( 0.5, 0.5), V2( 0.5,  0.5)}, // 2 Square
    {V2(-0.5, 0.5), V2(-0.5,  0.5)}, // 3 Square
    {V2(-0.5f,  -0.866025/3.f), V2(-0.5f, -0.5f)}, // 4 Equilateral Triangle
    {V2( 0.5f,  -0.866025/3.f), V2(-0.5f,  0.5f)}, // 5 Equilateral Triangle
    {V2( 0.0f, 2*0.866025/3.f), V2(-0.5f,  0.5f)}, // 6 Equilateral Triangle
  };
  
  u32 IndiceData[9] =
  {
    0, 1, 3,
    1, 2, 3,
    4, 5, 6
  };
  
  u32 OffsetForVertexData = 0;
  OpenGL->OffsetForInstanceData = sizeof(VertexData);

  glGenBuffers(1, &OpenGL->VertexArrayBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->VertexArrayBuffer);
  glBufferData(GL_ARRAY_BUFFER, OpenGL->BufferSize, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  SendDataToBuffer(GL_ARRAY_BUFFER, OpenGL->VertexArrayBuffer, OffsetForVertexData, sizeof(VertexData), (void*) VertexData);
  
  glGenBuffers(1, &OpenGL->ElementArrayBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementArrayBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndiceData), IndiceData, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glGenVertexArrays(1, &OpenGL->Circle2DVAO);
  glBindVertexArray(OpenGL->Circle2DVAO);
  
  // The IndexBufferHandle is now implicitly bound to the Circle2DVAO (VAO)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementArrayBuffer);

  // This implicitly binds the VertexBufferHandle ArrayBuffer and it's data-structure is now implicitly bound to the bound Circle2DVAO
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->VertexArrayBuffer);
  EnableBufferAttributePerVertex(  0, 2, GL_FLOAT, OffsetForVertexData,           OffsetOf(circle_2d_vertex, v),       sizeof(circle_2d_vertex));
  EnableBufferAttributePerVertex(  1, 2, GL_FLOAT, OffsetForVertexData,           OffsetOf(circle_2d_vertex, Value),   sizeof(circle_2d_vertex));
  EnableBufferAttributePerInstance(3, 2, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(circle_2d_data, Position),  sizeof(circle_2d_data));
  EnableBufferAttributePerInstance(4, 2, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(circle_2d_data, Scale),     sizeof(circle_2d_data));
  EnableBufferAttributePerInstance(5, 4, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(circle_2d_data, Color),     sizeof(circle_2d_data));
  EnableBufferAttributePerInstance(6, 1, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(circle_2d_data, Thickness), sizeof(circle_2d_data));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);
  

  glGenVertexArrays(1, &OpenGL->Solid2DVAO);
  glBindVertexArray(OpenGL->Solid2DVAO);
  
  // The IndexBufferHandle is now implicitly bound to the Solid2DVAO (VAO)
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementArrayBuffer);

  // This implicitly binds the VertexBufferHandle ArrayBuffer and it's data-structure is now implicitly bound to the bound Solid2DVAO
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->VertexArrayBuffer);
  EnableBufferAttributePerVertex(  0, 2, GL_FLOAT, OffsetForVertexData,           OffsetOf(circle_2d_vertex, v),         sizeof(circle_2d_vertex));
  EnableBufferAttributePerVertex(  1, 2, GL_FLOAT, OffsetForVertexData,           OffsetOf(circle_2d_vertex, Value),     sizeof(circle_2d_vertex));
  EnableBufferAttributePerInstance(3, 2, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(triangle_2d_data, Position),  sizeof(triangle_2d_data));
  EnableBufferAttributePerInstance(4, 2, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(triangle_2d_data, Scale),     sizeof(triangle_2d_data));
  EnableBufferAttributePerInstance(5, 4, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(triangle_2d_data, Color),     sizeof(triangle_2d_data));
  EnableBufferAttributePerInstance(6, 1, GL_FLOAT, OpenGL->OffsetForInstanceData, OffsetOf(triangle_2d_data, Rotation),  sizeof(triangle_2d_data));
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);


#if HANDMADE_INTERNAL
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallbackARB(ErrorCallback, 0);
#endif
  
}

void OpenGLSetViewport( r32 ViewPortAspectRatio, s32 WindowWidth, s32 WindowHeight )
{
  r32 AspectRatio   = WindowWidth / (r32) WindowHeight;
  
  s32 OffsetX = 0;
  s32 OffsetY = 0;
  s32 ViewPortWidth  = 0;
  s32 ViewPortHeight = 0;
  
  // Bars on top and bottom
  if( ViewPortAspectRatio > AspectRatio )
  {
    ViewPortWidth  = WindowWidth;
    ViewPortHeight = (s32) (ViewPortWidth / ViewPortAspectRatio);
    
    Assert(ViewPortHeight < WindowHeight);
    
    OffsetX = 0;
    OffsetY = (WindowHeight - ViewPortHeight) / 2;
    
  }else{
    ViewPortHeight = WindowHeight;
    ViewPortWidth  = (s32) (ViewPortAspectRatio * ViewPortHeight);
    
    Assert(ViewPortWidth <= WindowWidth);
    
    OffsetX = (WindowWidth - ViewPortWidth) / 2;
    OffsetY = 0;
    
  }
  
  glViewport( OffsetX, OffsetY, ViewPortWidth, ViewPortHeight);
  
}

internal inline v4
Blend(v4* A, v4* B)
{
  v4 Result =  V4( A->X * B->X,
                  A->Y * B->Y,
                  A->Z * B->Z,
                  A->W * B->W );
  return Result;
}

b32 CompareU32Triplet( const u8* DataA, const  u8* DataB)
{
  u32* U32A = (u32*) DataA;
  const u32 A1 = *(U32A+0);
  const u32 A2 = *(U32A+1);
  const u32 A3 = *(U32A+2);
  
  u32* U32B = (u32*) DataB;
  const u32 B1 = *(U32B+0);
  const u32 B2 = *(U32B+1);
  const u32 B3 = *(U32B+2);
  
  return (A1 == B1) && (A2 == B2) && (A3 == B3);
}
b32 CompareU32(u8* DataA, u8* DataB)
{
  const u32 U32A = *( (u32*) DataA);
  const u32 U32B = *( (u32*) DataB);
  return (U32A == U32B);
}

u32 PushUnique( u8* Array, const u32 ElementCount, const u32 ElementByteSize,
               u8* NewElement, b32 (*CompareFunction)(const u8* DataA, const u8* DataB))
{
  for( u32 i = 0; i < ElementCount; ++i )
  {
    if( CompareFunction(NewElement, Array) )
    {
      return i;
    }
    Array += ElementByteSize;
  }
  
  // If we didn't find the element we push it to the end
  utils::Copy(ElementByteSize, NewElement, Array);
  
  return ElementCount;
}

struct gl_vertex_buffer
{
  u32 IndexCount;
  u32* Indeces;
  u32 VertexCount;
  opengl_vertex* VertexData;
};

internal gl_vertex_buffer
CreateGLVertexBuffer( memory_arena* TemporaryMemory,
                     const u32 IndexCount,
                     const u32* VerticeIndeces, const u32* TextureIndeces, const u32* NormalIndeces,
                     const v3* VerticeData,     const v2* TextureData,     const v3* NormalData)
{
  Assert(VerticeIndeces && VerticeData);
  u32* GLVerticeIndexArray  = PushArray(TemporaryMemory, 3*IndexCount, u32);
  u32* GLIndexArray         = PushArray(TemporaryMemory, IndexCount, u32);
  
  u32 VerticeArrayCount = 0;
  for( u32 i = 0; i < IndexCount; ++i )
  {
    const u32 vidx = VerticeIndeces[i];
    const u32 tidx = TextureIndeces ? TextureIndeces[i] : 0;
    const u32 nidx = NormalIndeces  ? NormalIndeces[i]  : 0;
    u32 NewElement[3] = {vidx, tidx, nidx};
    u32 Index = PushUnique((u8*)GLVerticeIndexArray, VerticeArrayCount, sizeof(NewElement), (u8*) NewElement,
                           [](const u8* DataA, const u8* DataB) {
                             u32* U32A = (u32*) DataA;
                             const u32 A1 = *(U32A+0);
                             const u32 A2 = *(U32A+1);
                             const u32 A3 = *(U32A+2);
                             u32* U32B = (u32*) DataB;
                             const u32 B1 = *(U32B+0);
                             const u32 B2 = *(U32B+1);
                             const u32 B3 = *(U32B+2);
                             b32 result = (A1 == B1) && (A2 == B2) && (A3 == B3);
                             return result;
                           });
    if(Index == VerticeArrayCount)
    {
      VerticeArrayCount++;
    }
    
  	GLIndexArray[i] = Index;
  }
  
  opengl_vertex* VertexData = PushArray(TemporaryMemory, VerticeArrayCount, opengl_vertex);
  opengl_vertex* Vertice = VertexData;
  for( u32 i = 0; i < VerticeArrayCount; ++i )
  {
    const u32 vidx = *(GLVerticeIndexArray + 3 * i + 0);
    const u32 tidx = *(GLVerticeIndexArray + 3 * i + 1);
    const u32 nidx = *(GLVerticeIndexArray + 3 * i + 2);
    Vertice->v  = VerticeData[vidx];
    Vertice->vt = TextureData ? TextureData[tidx] : V2(0,0);
    Vertice->vn = NormalData  ? NormalData[nidx]  : V3(0,0,0);
    ++Vertice;
  }
  
  gl_vertex_buffer Result = {};
  Result.IndexCount = IndexCount;
  Result.Indeces = GLIndexArray;
  Result.VertexCount = VerticeArrayCount;
  Result.VertexData = VertexData;
  return Result;
}

void PushObjectToGPU(open_gl* OpenGL, game_asset_manager* AssetManager, object_handle ObjectHandle)
{ 
  buffer_keeper* ObjectKeeper = 0;
  mesh_indeces* Object = GetAsset(AssetManager, ObjectHandle, &ObjectKeeper);
  
  mesh_data* MeshData = GetAsset(AssetManager, Object->MeshHandle);
  
  temporary_memory TempMem = BeginTemporaryMemory(&AssetManager->AssetArena);
  
  Assert(Object->Count && Object->vi && Object->ti && Object->ni && MeshData->v && MeshData->vt && MeshData->vn);
  
  // Each object within a mesh gets a copy of the mesh, not good.
  gl_vertex_buffer GLBuffer =  CreateGLVertexBuffer(&AssetManager->AssetArena, 
                                                    Object->Count,
                                                    Object->vi,
                                                    Object->ti,
                                                    Object->ni,
                                                    MeshData->v,
                                                    MeshData->vt,
                                                    MeshData->vn);
  
  
  
  u32 VBOOffset = OpenGL->ElementVBOOffset;
  u32 VBOSize = GLBuffer.VertexCount * sizeof(opengl_vertex);
  OpenGL->ElementVBOOffset += VBOSize;
  Assert(OpenGL->ElementVBOOffset < OpenGL->BufferSize );
  
  glBindBuffer(GL_ARRAY_BUFFER, OpenGL->ElementVBO);
  glBufferSubData(GL_ARRAY_BUFFER,                 // Target
                  VBOOffset,                       // Offset
                  VBOSize,                         // Size
                  (GLvoid*) GLBuffer.VertexData);  // Data
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  u32 EBOOffset = OpenGL->ElementEBOOffset;
  u32 EBOSize   = GLBuffer.IndexCount * sizeof(u32);
  OpenGL->ElementEBOOffset += EBOSize;
  Assert(OpenGL->ElementEBOOffset < OpenGL->BufferSize );
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL->ElementEBO);
  glBufferSubData( GL_ELEMENT_ARRAY_BUFFER,      // Target
                  EBOOffset,                    // Offset
                  EBOSize,                      // Size
                  (GLvoid*) GLBuffer.Indeces);  // Data
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  
  Assert((OpenGL->ElementEBOOffset + EBOSize) < OpenGL->BufferSize);
  
  Assert(ObjectKeeper->Referenced);
  ObjectKeeper->Count = GLBuffer.IndexCount;
  ObjectKeeper->Index = ((u8*) 0 + EBOOffset);
  ObjectKeeper->VertexOffset = VBOOffset/sizeof(opengl_vertex);
  
  EndTemporaryMemory(TempMem);
}

void CopyBitmapSubregion( u32 X, u32 Y, u32 Width, u32 Height, u32 Stride, u32* SrcPixels, u32* DstPixels )
{
  u32 PixelCount = Width*Height;
  u32 Xp = X;
  u32 Xf = X+Width;
  u32 Yp = Y;

  u32* DstEndPixel = DstPixels + PixelCount;
  u32* DstPixel = DstPixels;
  while(DstPixel < DstEndPixel)
  {
    u32 SrcPixelIndex = Yp * Stride + Xp++;
    u32* SrcPixel = SrcPixels + SrcPixelIndex;
    if(Xp >= Xf)
    {
      Xp = X;
      Yp++;
    }
    u32 SrcPixelValue = *SrcPixel;
    *DstPixel++ = SrcPixelValue;
  }
}

void PushBitmapToGPU(open_gl* OpenGL, game_asset_manager* AssetManager, bitmap_handle BitmapHandle)
{
  bitmap_keeper* BitmapKeeper = 0;
  bitmap* RenderTarget = GetAsset(AssetManager, BitmapHandle, &BitmapKeeper);
  
  Assert(RenderTarget->BPP == 32);
  
  if(!RenderTarget->Special)
  {
    BitmapKeeper->Special = false;
    if(!BitmapKeeper->TextureSlot)
    {
      glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
      u32 MipLevel = 0;
      BitmapKeeper->TextureSlot = OpenGL->TextureCount++;
      Assert(OpenGL->TextureCount < OpenGL->MaxTextureCount);
      
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                      MipLevel,
                      0, 0, BitmapKeeper->TextureSlot, // x0,y0,TextureSlot
                      RenderTarget->Width, RenderTarget->Height, 1,
                      OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, RenderTarget->Pixels);
    }else{
      glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);
      u32 MipLevel = 0;

      if(BitmapKeeper->UseSubRegion)
      {
        ScopedMemory M = ScopedMemory(&AssetManager->AssetArena);
        u32 X = (u32) BitmapKeeper->SubRegion.X;
        u32 Y = (u32) BitmapKeeper->SubRegion.Y;
        u32 W = (u32) BitmapKeeper->SubRegion.W;
        u32 H = (u32) BitmapKeeper->SubRegion.H;
        midx PixelCount = W * H;
        u32* Pixels = PushArray(&AssetManager->AssetArena, PixelCount, u32);

        CopyBitmapSubregion(X, Y, W, H, RenderTarget->Width, (u32*) RenderTarget->Pixels, Pixels);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        MipLevel,
                        X, Y, BitmapKeeper->TextureSlot, // x0,y0,TextureSlot
                        W, H, 1,
                        OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, Pixels);
      }else{

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        MipLevel,
                        0, 0, BitmapKeeper->TextureSlot, // x0,y0,TextureSlot
                        RenderTarget->Width, RenderTarget->Height, 1,
                        OpenGL->DefaultTextureFormat, GL_UNSIGNED_BYTE, RenderTarget->Pixels);
      }
    }
  }else{
    BitmapKeeper->Special = true;
    if(!BitmapKeeper->TextureSlot)
    {
      BitmapKeeper->TextureSlot = OpenGL->SpecialTextureCount++;
      Assert(OpenGL->SpecialTextureCount < OpenGL->MaxSpecialTextureCount);
      glBindTexture( GL_TEXTURE_2D, OpenGL->SpecialTextures[BitmapKeeper->TextureSlot] );
      glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA8,
                   RenderTarget->Width,  RenderTarget->Height,
                   0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                   RenderTarget->Pixels);
    }else{
      Assert(OpenGL->SpecialTextureCount < OpenGL->MaxSpecialTextureCount);
      glBindTexture( GL_TEXTURE_2D, OpenGL->SpecialTextures[BitmapKeeper->TextureSlot] );


      if(BitmapKeeper->UseSubRegion)
      {
        ScopedMemory M = ScopedMemory(&AssetManager->AssetArena);
        u32 X = (u32) BitmapKeeper->SubRegion.X;
        u32 Y = (u32) BitmapKeeper->SubRegion.Y;
        u32 W = (u32) BitmapKeeper->SubRegion.W;
        u32 H = (u32) BitmapKeeper->SubRegion.H;
        midx PixelCount = W * H;
        u32* Pixels = PushArray(&AssetManager->AssetArena, PixelCount, u32);

        CopyBitmapSubregion(X, Y, W, H, RenderTarget->Width, (u32*) RenderTarget->Pixels, Pixels);

        glTexSubImage2D ( GL_TEXTURE_2D,  0,
                   X,Y,W,H,
                   GL_RGBA, GL_UNSIGNED_BYTE,
                   Pixels);
      }else{
        glTexSubImage2D ( GL_TEXTURE_2D,  0, 
                   0,
                   0,
                   RenderTarget->Width,
                   RenderTarget->Height,
                   GL_RGBA, GL_UNSIGNED_BYTE,
                   RenderTarget->Pixels);
      }
    }
  }
}

internal void DrawElementsInstancedBaseVertex(u32 ProgramHandle,
  u32 ProjectionMatrixHandle, m4* ProjectionMatrix,
  u32 ViewMatrixHandle, m4* ViewMatrix,
   glHandle VAO,  u32 InstanceCount, buffer_keeper* ElementObjectKeeper)
{  
  glUseProgram(ProgramHandle);
  glUniformMatrix4fv(ProjectionMatrixHandle, 1, GL_TRUE, ProjectionMatrix->E);
  glUniformMatrix4fv(ViewMatrixHandle,       1, GL_TRUE, ViewMatrix->E);
  glBindVertexArray(VAO);
  glDrawElementsInstancedBaseVertex(GL_TRIANGLES,                           // Mode,
                                    ElementObjectKeeper->Count,             // Nr of Elements (Triangles*3)
                                    GL_UNSIGNED_INT,                        // Index Data Type  
                                    (GLvoid*)(ElementObjectKeeper->Index),  // Pointer somewhere in the index buffer
                                    InstanceCount,                          // How many Instances to draw
                                    ElementObjectKeeper->VertexOffset);     // Base Offset into the geometry vbo
  glBindVertexArray(0);
}


void DrawRenderGroup(open_gl* OpenGL, render_group* RenderGroup, game_asset_manager* AssetManager)
{
  if( !RenderGroup->First) {return;}
  temporary_memory TempMem = BeginTemporaryMemory(&RenderGroup->Arena);
  {
    push_buffer_header* StartEntry = RenderGroup->First;
    push_buffer_header* BreakEntry = 0;
    while(StartEntry)
    {
      u32 Quad2DCount = 0;
      u32 Quad2DColorCount = 0;
      u32 Quad2DCountSpecial = 0;
      u32 ElectricalComponentCount = 0;
      u32 TriangleCount = 0;
      for(push_buffer_header* Entry = StartEntry;
          Entry != 0;
          Entry = Entry->Next )
      {
        Quad2DCount                 += ((Entry->Type == render_buffer_entry_type::QUAD_2D)              ? 1 : 0);
        Quad2DColorCount            += ((Entry->Type == render_buffer_entry_type::QUAD_2D_COLOR)        ? 1 : 0);
        Quad2DCountSpecial          += ((Entry->Type == render_buffer_entry_type::QUAD_2D_SPECIAL)      ? 1 : 0);
        ElectricalComponentCount    += ((Entry->Type == render_buffer_entry_type::ELECTRICAL_COMPONENT) ? 1 : 0);
        TriangleCount               += ((Entry->Type == render_buffer_entry_type::ELECTRICAL_CONNECTOR_TRIANGLE) ? 1 : 0);
        Quad2DColorCount            += ((Entry->Type == render_buffer_entry_type::ELECTRICAL_CONNECTOR_SQUARE) ? 1 : 0);
        if(Entry->Type == render_buffer_entry_type::NEW_LEVEL)
        {
          BreakEntry = Entry;
          break;
        }
      }

      u32 Quad2DBufferSize         = sizeof(quad_2d_data)*Quad2DCount;
      u32 Quad2DColorBufferSize    = sizeof(quad_2d_data)*Quad2DColorCount;
      u32 Quad2DBufferSizeSpecial  = sizeof(quad_2d_data)*Quad2DCountSpecial;
      u32 ElectricalComponentSize  = sizeof(circle_2d_data)*ElectricalComponentCount;
      u32 TriangleDataSize         = sizeof(triangle_2d_data)*TriangleCount;
      u32 SpecialTextureSlot = 0;

      quad_2d_data* Quad2DBuffer          = PushArray(&RenderGroup->Arena, Quad2DCount,              quad_2d_data);
      quad_2d_data* Quad2DColorBuffer     = PushArray(&RenderGroup->Arena, Quad2DColorCount,         quad_2d_data);
      quad_2d_data* Quad2DSpecialBuffer   = PushArray(&RenderGroup->Arena, Quad2DCountSpecial,       quad_2d_data);
      circle_2d_data* Circle2DBuffer      = PushArray(&RenderGroup->Arena, ElectricalComponentCount, circle_2d_data);
      triangle_2d_data* Triangle2DBuffer   = PushArray(&RenderGroup->Arena, TriangleCount,           triangle_2d_data);
      

      u32 Quad2DBufferInstanceIndex = 0;
      u32 Quad2DBufferColorInstanceIndex = 0;
      u32 Quad2DBufferSpecialInstanceIndex = 0;
      u32 Circle2DBufferIndex = 0;
      u32 ConnectorBufferIndex = 0;
      for( push_buffer_header* Entry = StartEntry; Entry != BreakEntry; Entry = Entry->Next)
      {
        u8* Head = (u8*) Entry;
        u8* Body = Head + sizeof(push_buffer_header);
        switch(Entry->Type)
        {
          // A quad with a 3D-texture. 3D-texture meaning a fixed size texture, think it's 512 x 512 or something.
          case render_buffer_entry_type::TEXT:
          case render_buffer_entry_type::QUAD_2D:
          {
            entry_type_2d_quad* Quad = (entry_type_2d_quad*) Body;
            bitmap_keeper* BitmapKeeper;
            GetAsset(AssetManager, Quad->BitmapHandle, &BitmapKeeper);

            quad_2d_data* Quad2dData = &Quad2DBuffer[Quad2DBufferInstanceIndex++]; 
            Quad2dData->TextureSlot  = BitmapKeeper->TextureSlot;
            Quad2dData->QuadRect     = Quad->QuadRect;
            Quad2dData->UVRect       = Quad->UVRect;
            Quad2dData->Color        = Quad->Colour;
            Quad2dData->Rotation     = Quad->Rotation;
            Quad2dData->RotationCenterOffset = Quad->RotationCenterOffset;
          }break;
           // A quad with a special texture. Special meaning it is not part of a 3D-texture of fixed size but a arbitrary sized texture
          case render_buffer_entry_type::QUAD_2D_SPECIAL:
          {
            entry_type_2d_quad* Quad = (entry_type_2d_quad*) Body;
            bitmap_keeper* BitmapKeeper;
            GetAsset(AssetManager, Quad->BitmapHandle, &BitmapKeeper);

            quad_2d_data* Quad2dData = &Quad2DSpecialBuffer[Quad2DBufferSpecialInstanceIndex++]; 
            SpecialTextureSlot       = BitmapKeeper->TextureSlot;
            Quad2dData->QuadRect     = Quad->QuadRect;
            Quad2dData->UVRect       = Quad->UVRect;
            Quad2dData->Color        = Quad->Colour;
            Quad2dData->Rotation     = Quad->Rotation;
            Quad2dData->RotationCenterOffset = Quad->RotationCenterOffset;
          }break;
          // A quad with a No texture, just solid color
          case render_buffer_entry_type::QUAD_2D_COLOR:
          {
            entry_type_2d_quad* Quad = (entry_type_2d_quad*) Body; 
            quad_2d_data* Quad2dData = &Quad2DColorBuffer[Quad2DBufferColorInstanceIndex++];
            Quad2dData->QuadRect     = Quad->QuadRect;
            Quad2dData->Color        = Quad->Colour; // For some reason quads with color opacity < 0.5 ish won't render !?
            Quad2dData->Rotation     = Quad->Rotation;
            Quad2dData->RotationCenterOffset = Quad->RotationCenterOffset;
          }break;
          case render_buffer_entry_type::ELECTRICAL_COMPONENT:
          {
            entry_type_electrical_component* Component = (entry_type_electrical_component*) Body; 
            circle_2d_data* Circle2DData = &Circle2DBuffer[Circle2DBufferIndex++];
            Circle2DData->Position     = Component->Position;
            Circle2DData->Color        = V4(Component->Color,1);
            Circle2DData->Scale        = V2(1,1);
            Circle2DData->Thickness    = 0.1;
          }break;
          case render_buffer_entry_type::ELECTRICAL_CONNECTOR_TRIANGLE:
          {
            entry_type_electrical_connector_triangle* Connector = (entry_type_electrical_connector_triangle*) Body; 
            triangle_2d_data* ConnectorData = &Triangle2DBuffer[ConnectorBufferIndex++];
            ConnectorData->Position     = Connector->Position;
            ConnectorData->Color        = V4(Connector->Color,1);
            ConnectorData->Scale        = Connector->Scale;
            ConnectorData->Rotation     = Connector->Rotation;
          }break;
          case render_buffer_entry_type::ELECTRICAL_CONNECTOR_SQUARE:
          {
            entry_type_2d_quad* Quad = (entry_type_2d_quad*) Body; 
            quad_2d_data* Quad2dData = &Quad2DColorBuffer[Quad2DBufferColorInstanceIndex++];
            Quad2dData->QuadRect     = Quad->QuadRect;
            Quad2dData->Color        = Quad->Colour;
            Quad2dData->Rotation     = Quad->Rotation;
            Quad2dData->RotationCenterOffset = Quad->RotationCenterOffset;
          }break;
        }
      }

      SendDataToBuffer(GL_ARRAY_BUFFER, OpenGL->VertexArrayBuffer, OpenGL->OffsetForInstanceData, ElectricalComponentSize, (void*) Circle2DBuffer);

      glUseProgram(OpenGL->Circle2DProgram.Program);
      glUniformMatrix4fv(OpenGL->Circle2DProgram.ProjectionMat, 1, GL_TRUE, RenderGroup->ProjectionMatrix.E);
      glUniformMatrix4fv(OpenGL->Circle2DProgram.ViewMat,       1, GL_TRUE, RenderGroup->ViewMatrix.E);
      glBindVertexArray(OpenGL->Circle2DVAO);
      glDrawElementsInstancedBaseVertex(GL_TRIANGLES,                                 // Mode
                                        6,                                            // Nr of Elements (Triangles*3)
                                        GL_UNSIGNED_INT,                              // Index Data Type  
                                        0,                                            // Pointer somewhere in the index buffer
                                        ElectricalComponentCount,                     // How many Instances to draw
                                        0);                                           // Base Offset into the geometry vbo, starting from offset in attrib array
      glBindVertexArray(0);

      
      SendDataToBuffer(GL_ARRAY_BUFFER, OpenGL->VertexArrayBuffer, OpenGL->OffsetForInstanceData, TriangleDataSize, (void*) Triangle2DBuffer);
      glUseProgram(OpenGL->Solid2DProgram.Program);
      glUniformMatrix4fv(OpenGL->Solid2DProgram.ProjectionMat, 1, GL_TRUE, RenderGroup->ProjectionMatrix.E);
      glUniformMatrix4fv(OpenGL->Solid2DProgram.ViewMat,       1, GL_TRUE, RenderGroup->ViewMatrix.E);
      glBindVertexArray(OpenGL->Circle2DVAO);
      glDrawElementsInstancedBaseVertex(GL_TRIANGLES,                                 // Mode
                                        3,                                            // Nr of Elements (Triangles*3)
                                        GL_UNSIGNED_INT,                              // Index Data Type  
                                        (GLvoid*)( (u8*) 0 + 6*sizeof(u32)),          // Pointer somewhere in the index buffer
                                        TriangleCount,                                // How many Instances to draw
                                        0);                                           // Base Offset into the geometry vbo, starting from offset in attrib array
      glBindVertexArray(0);

      buffer_keeper* ElementObjectKeeper = 0;
      object_handle ObjectHandle = GetEnumeratedObjectHandle(AssetManager, predefined_mesh::QUAD);
      GetAsset(AssetManager, ObjectHandle, &ElementObjectKeeper);

      const u32 InstanceBufferOffset = 0;
      SendDataToBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO, InstanceBufferOffset, Quad2DColorBufferSize, (void*) Quad2DColorBuffer);
      DrawElementsInstancedBaseVertex(
        OpenGL->Colored2DQuadProgram.Program,
        OpenGL->Colored2DQuadProgram.ProjectionMat, &RenderGroup->ProjectionMatrix,
        OpenGL->Colored2DQuadProgram.ViewMat, &RenderGroup->ViewMatrix,
        OpenGL->Quad2DVAO, Quad2DColorCount, ElementObjectKeeper);

      SendDataToBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO, InstanceBufferOffset, Quad2DBufferSize, (void*) Quad2DBuffer);
      DrawElementsInstancedBaseVertex(
        OpenGL->Quad2DProgram.Program,
        OpenGL->Quad2DProgram.ProjectionMat, &RenderGroup->ProjectionMatrix,
        OpenGL->Quad2DProgram.ViewMat, &RenderGroup->ViewMatrix,
        OpenGL->Quad2DVAO, Quad2DCount, ElementObjectKeeper);

      SendDataToBuffer(GL_ARRAY_BUFFER, OpenGL->InstanceVBO, InstanceBufferOffset, Quad2DBufferSizeSpecial, (void*) Quad2DSpecialBuffer);
      DrawElementsInstancedBaseVertex(
        OpenGL->Quad2DProgramSpecial.Program,
        OpenGL->Quad2DProgramSpecial.ProjectionMat, &RenderGroup->ProjectionMatrix,
        OpenGL->Quad2DProgramSpecial.ViewMat, &RenderGroup->ViewMatrix,
        OpenGL->Quad2DVAO, Quad2DCountSpecial, ElementObjectKeeper);

      StartEntry = 0;
      if(BreakEntry)
      {
        StartEntry = BreakEntry->Next;
        BreakEntry = 0;
      }
    }
  }
  EndTemporaryMemory(TempMem);
}

void OpenGLRenderGroupToOutput(game_render_commands* Commands)
{  
  TIMED_FUNCTION();
  game_asset_manager* AssetManager = Commands->AssetManager;
  open_gl* OpenGL = &Commands->OpenGL;
  
  for (u32 i = 0; i < AssetManager->ObjectPendingLoadCount; ++i)
  {
    // PushObject
    PushObjectToGPU(OpenGL, AssetManager, AssetManager->ObjectPendingLoad[i]);
  }
  AssetManager->ObjectPendingLoadCount = 0;
  
  for (u32 i = 0; i < AssetManager->BitmapPendingLoadCount; ++i)
  {
    // PushObject
    PushBitmapToGPU(OpenGL, AssetManager, AssetManager->BitmapPendingLoad[i]);
  }
  AssetManager->BitmapPendingLoadCount = 0;
  
  r32 R = 0x1E / (r32) 0xFF;
  r32 G = 0x46 / (r32) 0xFF;
  r32 B = 0x5A / (r32) 0xFF;
  glClearColor(R,G,B, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // No need to clearh the depth buffer if we disable depth test
  // glEnable(GL_DEPTH_TEST);
  // glDepthFunc(GL_LESS);

  // Bind Textures (Enabled in init)
  glActiveTexture(GL_TEXTURE0);
  glBindTexture( GL_TEXTURE_2D_ARRAY, OpenGL->TextureArray);      

  r32 DesiredAspectRatio = (r32)Commands->ScreenWidthPixels / (r32)Commands->ScreenHeightPixels;
  OpenGLSetViewport( DesiredAspectRatio, Commands->ScreenWidthPixels,  Commands->ScreenHeightPixels );
  
  DrawRenderGroup(OpenGL, Commands->WorldGroup, AssetManager);
  
  Commands->OverlayGroup->ViewMatrix = M4Identity();
  DrawRenderGroup(OpenGL, Commands->OverlayGroup, AssetManager);
}