// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
//
// Modifyed from the default allegro shader source

#ifdef GL_ES
precision lowp float;
#endif

uniform sampler2D al_tex;
uniform bool al_use_tex;
uniform bool al_alpha_test;
uniform int al_alpha_func;
uniform float al_alpha_test_val;
varying vec4 varying_color;
varying vec2 varying_texcoord;

uniform float variation;
uniform int foiling_type;

bool alpha_test_func(float x, int op, float compare);

void main()
{

  vec4 c;

  if (al_use_tex)
    c = vec4(varying_color.r,varying_color.b,varying_color.g, texture2D(al_tex, varying_texcoord).a);
  else
    c = varying_color;

  if (!al_alpha_test || alpha_test_func(c.a, al_alpha_func, al_alpha_test_val))
    gl_FragColor = c;
  else
    discard;

    // foil stripe
    float ref = (gl_FragCoord.x/1920 + gl_FragCoord.y/1080)*0.5+variation*0.1;
    ref = fract(ref);

    if(ref < 0.1)
        gl_FragColor.xyz = vec3(1);
}

bool alpha_test_func(float x, int op, float compare)
{
  if (op == 0) return false;
  else if (op == 1) return true;
  else if (op == 2) return x < compare;
  else if (op == 3) return x == compare;
  else if (op == 4) return x <= compare;
  else if (op == 5) return x > compare;
  else if (op == 6) return x != compare;
  else if (op == 7) return x >= compare;
  return false;
}
