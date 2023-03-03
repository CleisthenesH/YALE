// Copyright 2023 Kieran W Harvie. All rights reserved.
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

varying vec3 local_position;

// General State
uniform float current_timestamp;
uniform int effect_id;
uniform int selection_id;
uniform float variation;
uniform vec2 display_dimensions;
uniform vec2 object_scale;

// Material Selection Variables
uniform vec3 selection_color;
uniform float selection_cutoff;

// Material Effect Variables
uniform vec2 effect_point;
uniform vec3 effect_color;

// Global Effect Variables
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

float hash(vec2 p)
{
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);   
}

vec2 hash2(vec2 p)
{
	return vec2(hash(p),hash(vec2(p.y,p.x)));
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

vec4 voronoi(vec2 position)
{
    vec2 p = floor(position);
    vec2 f = fract(position);

	float closest_distance = 100.0;
	vec2 closest_point;
	vec2 closest_cell;

	for(int i = -1; i <= 1; i++)
	for(int j = -1; j <= 1; j++)
	{
		vec2 r = hash2(p+vec2(i,j));
		r += vec2(i,j);

		float d = distance(f,r);

		if(d < closest_distance)
		{
			closest_distance = d;
			closest_cell = vec2(i,j);
			closest_point = r;
		}	
	}

	return vec4(closest_cell,closest_point);
}

vec3 noised( vec2 p )
{
    vec2 i = floor( p );
    vec2 f = fract( p );

    vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
    vec2 du = 30.0*f*f*(f*(f-2.0)+1.0);
    
    vec2 ga = hash2( i+ vec2(0.0,0.0) );
    vec2 gb = hash2( i+ vec2(1.0,0.0) );
    vec2 gc = hash2( i+ vec2(0.0,1.0) );
    vec2 gd = hash2( i+ vec2(1.0,1.0) );
    
    float va = dot( ga, f - vec2(0.0,0.0) );
    float vb = dot( gb, f - vec2(1.0,0.0) );
    float vc = dot( gc, f - vec2(0.0,1.0) );
    float vd = dot( gd, f - vec2(1.0,1.0) );

    return vec3( va + u.x*(vb-va) + u.y*(vc-va) + u.x*u.y*(va-vb-vc+vd),   // value
                 ga + u.x*(gb-ga) + u.y*(gc-ga) + u.x*u.y*(ga-gb-gc+gd) +  // derivatives
                 du * (u.yx*(va-vb-vc+vd) + vec2(vb,vc) - va));
}

vec3 voronoi_sdf(vec2 position)
{
    vec2 cell = floor(position);
    vec2 point = position - cell;

	vec2 closest_cell;
	vec2 closest_point;
	float closest_distance = 100.0;

	for(int i = -1; i <= 1; i++)
	for(int j = -1; j <= 1; j++)
	{
		vec2 r = hash2(cell+vec2(i,j));
		r += vec2(i,j);

		float d = distance(point,r);

		if(d < closest_distance)
		{
			closest_distance = d;
			closest_cell = vec2(i,j)+cell;
			closest_point = r+cell;
		}	
	}

	closest_distance = 100.0;

	for(int i = -2; i <= 2; i++)
	for(int j = -2; j <= 2; j++)
	{
		vec2 r = hash2(closest_cell+vec2(i,j));
		r += vec2(i,j)+closest_cell;

		float d = dot(position-0.5*(closest_point + r),normalize(closest_point-r));

		closest_distance = min(closest_distance,d);
	}

	return vec3(closest_cell,closest_distance);
}

vec4 snake()
{
	vec2 position = local_position.xy * object_scale;

	position -= vec2(0.5,0.5);
	position *= 10.0;

	position += vec2(1,-2)*current_timestamp;

	vec3 voronoi = voronoi_sdf(position);

	if(voronoi.z < 0.1)
		return vec4(vec3(0),1);

	voronoi.z = clamp(0.1,1.0, voronoi.z);

	vec3 color = hsl2rgb(vec3(100.0/360.0,.3,0.15+.2*hash(voronoi.xy)));

	return vec4(mix(color,vec3(0),voronoi.z),1);
}

vec4 magma()
{
	vec2 position = local_position.xy * object_scale;

	position -= vec2(0.5,0.5);
	position *= 10.0;

	position += vec2(1,-2)*current_timestamp;

	vec3 voronoi = voronoi_sdf(position);

	if(voronoi.z < 0.1)
		return vec4((1-voronoi.z*10)*hsl2rgb(vec3(0.1/2*(sin(current_timestamp)+1),1,.5)),1-voronoi.z*10);
	else
		return vec4(0,0,0,0);
}

vec4 burn()
{
	vec2 p = local_position.xy * object_scale;

	float ref = p.x+p.y +3 - max(0,current_timestamp-3);

	float noise1 = 0.2+0.4*noised(0.02*gl_FragCoord.xy).x;

	ref += noise1;

	if(ref >= 1)
		return vec4(1,0,0,1);

	if(ref < 0.05 && ref > -0.05)
	{
		ref = (ref + 0.05)*10.0;
		return vec4(hsl2rgb(vec3(35.0/360.0,1.0, .54 +.1*ref)),1);
	}

	if(ref > 0)
		return vec4(ref,0,0,1);

	float a= min((1-max(abs(p.x),abs(p.y))),1);
	float noise2 = 0.2+0.4*noised(0.02*gl_FragCoord.xy+vec2(1,-1)*current_timestamp).x;
	float noise3 = 0.2+0.4*noised(0.2*gl_FragCoord.xy+vec2(1,-1)*current_timestamp).x;

	noise3 *= noise3;
	
	ref -= noise2+noise3-a;
	ref *= -1.0;

	if(ref > 1.0 )
		return vec4(0);

	ref = clamp(0.0,1.0,ref);

	ref = ref*ref*(3.0-2.0*ref);

	return vec4(vec3(1.0-ref),1.0-ref);
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
	// Run selection first and fill gl_FragColor with the "selection color" and a blend coeffiecent.
	// then if the blend is bigger than zero
	// 
	normal_behaviour();
	vec4 normal_color = gl_FragColor;

	switch(effect_id)
	{
	case 0: // No effects
	break;

	case 1: // Plain foil
		float ref = 0.5*(gl_FragCoord.x/display_dimensions.x + gl_FragCoord.y/display_dimensions.y);
		ref += (variation+current_timestamp)*0.1;
		ref = fract(ref);

		if(ref < 0.01)
		{	
			ref /= 0.01;
			normal_color.xyz = (1-ref)*vec3(1)+ref*gl_FragColor.xyz;
		}
	break;

	case 2: // Radial rgb
		vec2 displacement = gl_FragCoord.xy - vec2(effect_point.x,display_dimensions.y-effect_point.y);
		float angle = atan(displacement.y, displacement.x);
		angle = angle/3.14159265*2+1;
		angle += (variation+current_timestamp)*0.1;

		normal_color = vec4(hsl2rgb(vec3(angle,0.5,0.5)),1);
		break;

	case 3: // Voronoi
		normal_color = snake();// voronoi_edge();
	break;

	case 4:
		normal_color =  burn();
	break;
	}

	switch(selection_id)
	{
	case 0: // FULL Selection
		gl_FragColor = normal_color;
	break;
	case 1: // COLOR BAND
		vec3 displacement = filtered().xyz - selection_color;
		float ref = dot(displacement,displacement);

		if(ref < 0.3)
		{
			ref /= 0.3;
			ref = ref*ref*(3-2*ref);
			gl_FragColor.xyz = mix(normal_color.xyz,gl_FragColor.xyz,ref);
		}
		
	break;
	}

	// "Buff" effect

	// Saturate effect
	gl_FragColor.xyz = max(gl_FragColor.xyz,saturate);

}


