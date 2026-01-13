// 
// grcore/opengl.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_OPENGL_H
#define GRCORE_OPENGL_H

#ifndef __OPENGL
#error "__OPENGL not defined."
#endif

#if __OPENGL

#if __WIN32PC
# include "system/xtl.h"
#endif

#if __PPU
#define CGDLL_API			// Lame
#define CGGLDLL_API			// Lamer
#include <GLES/gl.h>
#include <GLES/glext.h>
#undef SUCCEEDED
#define glOrtho glOrthof
#define glDepthRange glDepthRangef
#define glClearDepth glClearDepthf
#else
#include <gl/GL.h>
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#define GL_STREAM_DRAW                    0x88E0
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893

typedef void (__stdcall * pfnglCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (__stdcall * pfnglBindBuffer)(GLenum target, GLuint name);
typedef void (__stdcall * pfnglGenBuffers)(GLsizei n, GLuint *buffers);
typedef void (__stdcall * pfnglDeleteBuffers)(GLsizei n, const GLuint *buffers);
typedef void (__stdcall * pfnglBufferData)(GLenum target, GLsizei size, const GLvoid * data, GLenum usage);

extern pfnglCompressedTexImage2D glCompressedTexImage2D;
extern pfnglBindBuffer glBindBuffer;
extern pfnglGenBuffers glGenBuffers;
extern pfnglDeleteBuffers glDeleteBuffers;
extern pfnglBufferData glBufferData;

#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7

#define GL_FRAMEBUFFER_EXT                                 0x8D40
#define GL_RENDERBUFFER_EXT                                0x8D41
#define GL_STENCIL_INDEX_EXT                               0x8D45
#define GL_STENCIL_INDEX1_EXT                              0x8D46
#define GL_STENCIL_INDEX4_EXT                              0x8D47
#define GL_STENCIL_INDEX8_EXT                              0x8D48
#define GL_STENCIL_INDEX16_EXT                             0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT                          0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT                         0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT                0x8D44
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT          0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT          0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT        0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT   0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                           0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                           0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                           0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                           0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                           0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                           0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                           0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                           0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                           0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                           0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT                          0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT                          0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT                          0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT                          0x8CED
#define GL_COLOR_ATTACHMENT14_EXT                          0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT                          0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                            0x8D00
#define GL_STENCIL_ATTACHMENT_EXT                          0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                        0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT           0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT   0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT           0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT              0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT          0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT          0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                     0x8CDD
#define GL_FRAMEBUFFER_STATUS_ERROR_EXT                    0x8CDE
#define GL_FRAMEBUFFER_BINDING_EXT                         0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT                        0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT                       0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT                       0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT               0x0506

typedef GLboolean (__stdcall * pfnglIsRenderbufferEXT) (GLuint renderbuffer);
typedef GLvoid (__stdcall * pfnglBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
typedef GLvoid (__stdcall * pfnglDeleteRenderbuffersEXT) (GLsizei n, const GLuint * renderbuffers);
typedef GLvoid (__stdcall * pfnglGenRenderbuffersEXT) (GLsizei n, GLuint * renderbuffers);
typedef GLvoid (__stdcall * pfnglRenderbufferStorageEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef GLvoid (__stdcall * pfnglGetRenderbufferParameterivEXT) (GLenum target, GLenum pname, GLint* params);
typedef GLboolean (__stdcall * pfnglIsFramebufferEXT) (GLuint framebuffer);
typedef GLvoid (__stdcall * pfnglBindFramebufferEXT) (GLsizei n, GLuint framebuffer);
typedef GLvoid (__stdcall * pfnglDeleteFramebuffersEXT) (GLsizei n, GLuint * framebuffers);
typedef GLvoid (__stdcall * pfnglGenFramebuffersEXT) (GLsizei n, GLuint * framebuffers);
typedef GLenum (__stdcall * pfnglCheckFramebufferStatusEXT) (GLenum target);
typedef GLvoid (__stdcall * pfnglFramebufferTexture1DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLvoid (__stdcall * pfnglFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLvoid (__stdcall * pfnglFramebufferTexture3DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef GLvoid (__stdcall * pfnglFramebufferRenderbufferEXT) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef GLvoid (__stdcall * pfnglGetFramebufferAttachmentParameterivEXT) (GLenum target, GLenum attachment, GLenum pname, GLint * params);
// typedef GLvoid (__stdcall * PFNGLGENERATEMIPMAPEXTPROC) (GLenum target);
extern pfnglIsRenderbufferEXT glIsRenderbufferEXT;
extern pfnglBindRenderbufferEXT glBindRenderbufferEXT;
extern pfnglDeleteRenderbuffersEXT glDeleteRenderbuffersEXT;
extern pfnglGenRenderbuffersEXT glGenRenderbuffersEXT;
extern pfnglRenderbufferStorageEXT glRenderbufferStorageEXT;
extern pfnglGetRenderbufferParameterivEXT glGetRenderbufferParameterivEXT;
extern pfnglIsFramebufferEXT glIsFramebufferEXT;
extern pfnglBindFramebufferEXT glBindFramebufferEXT;
extern pfnglDeleteFramebuffersEXT glDeleteFramebuffersEXT;
extern pfnglGenFramebuffersEXT glGenFramebuffersEXT;
extern pfnglCheckFramebufferStatusEXT glCheckFramebufferStatusEXT;
extern pfnglFramebufferTexture1DEXT  glFramebufferTexture1DEXT;
extern pfnglFramebufferTexture2DEXT glFramebufferTexture2DEXT;
extern pfnglFramebufferTexture3DEXT glFramebufferTexture3DEXT;
extern pfnglFramebufferRenderbufferEXT glFramebufferRenderbufferEXT;
extern pfnglGetFramebufferAttachmentParameterivEXT glGetFramebufferAttachmentParameterivEXT;

#define GL_EXTENDED 1
#define glDeclareProc(x)	pfn##x x
# if __WIN32PC
# define glGetProc(x) if ((x = (pfn##x) wglGetProcAddress(#x)) == NULL) grcErrorf("Function '%s' not available",#x)
# else
# error "Need glGetProc definition for this platform."
# endif
#else
#define GL_EXTENDED 0
#endif

#endif // __OPENGL

#endif // GRCORE_OPENGL_H
