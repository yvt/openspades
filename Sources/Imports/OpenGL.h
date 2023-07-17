//
//  OpenGL.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

// TODO: support other platform
#if __APPLE__
#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#ifndef __sun
#include <GL/glew.h>
#else
#include <glew.h>
#endif

// v3.3 / GL_ARB_occlusion_query2
#ifndef GL_ANY_SAMPLES_PASSED
#  define GL_ANY_SAMPLES_PASSED 0x8C2F
#endif

#endif
