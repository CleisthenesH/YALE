// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
//
// Modifyed from the default allegro shader source

#ifdef GL_ES
precision lowp float;
#endif

// ALLEGRO 
uniform sampler2D al_tex;
uniform bool al_use_tex;
uniform bool al_alpha_test;
uniform int al_alpha_func;
uniform float al_alpha_test_val;

varying vec4 varying_color;
varying vec2 varying_texcoord;

// General State
uniform float current_timestamp;
uniform int effect_id;
uniform int selection_id;
uniform float variation;

// Selection Variables
uniform vec3 selection_color;
uniform float cutoff;

// Effect Variables
uniform vec2 point;
uniform vec3 effect_color;

// Trump effect Variables
uniform float saturate;

vec3 hsl2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
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

void normal_behaviour()
{
	vec4 c;

	if (al_use_tex)
		c = varying_color * texture2D(al_tex, varying_texcoord);
	else
		c = varying_color;

	if (!al_alpha_test || alpha_test_func(c.a, al_alpha_func, al_alpha_test_val))
		gl_FragColor = c;
	else
		discard;
}

vec4 filtered()
{
//return varying_color * texture2D(al_tex, varying_texcoord+vec2(0.01,0));
return varying_color *(0.5*texture2D(al_tex, varying_texcoord) +
0.125*texture2D(al_tex, varying_texcoord+vec2(0.01,0)) +
0.125*texture2D(al_tex, varying_texcoord+vec2(-0.01,0)) +
0.125*texture2D(al_tex, varying_texcoord+vec2(0,0.01)) +
0.125*texture2D(al_tex, varying_texcoord+vec2(0,-0.01)));
}

void main()
{
	// 
	normal_behaviour();
	vec4 effect_color = gl_FragColor;

	switch(effect_id)
	{
	case 0: // No effects
	break;

	case 1: // Plain foil
		float ref = (gl_FragCoord.x/1920 + gl_FragCoord.y/1080)*0.5+(variation+current_timestamp)*0.1;
		ref = fract(ref);

		if(ref < 0.01)
		{	
			ref /= 0.01;
			effect_color.xyz = (1-ref)*vec3(1)+ref*gl_FragColor.xyz;
		}
	break;

	case 2: // Radial rgb
		vec2 displacement = gl_FragCoord.xy - vec2(-point.x,point.y);
		float angle = atan(displacement.y, displacement.x);
		//angle = angle/3.14159265*2+1;
		//angle += variation;

		effect_color = vec4(hsl2rgb(vec3(angle,0.5,0.5)),1);
	break;
	}

	switch(selection_id)
	{
	case 0: // FULL Selection
		gl_FragColor = effect_color;
	break;
	case 1: // COLOR BAND
		vec3 displacement = filtered().xyz - selection_color;
		float ref = dot(displacement,displacement);

		if(ref < 0.3)
		{
			ref /= 0.3;
			ref = ref*ref*(3-2*ref);
			gl_FragColor.xyz = mix(effect_color.xyz,gl_FragColor.xyz,ref);
		}
		
	break;
	}

	// "Buff" effect

	// Saturate effect
	gl_FragColor.xyz = max(gl_FragColor.xyz,saturate);
}


