/*
 Copyright (c) 2014, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "Pistons.h"

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;

Piston::Piston()
	: mOffset( 0.0f )
	, mColor( 1.0f, 1.0f, 1.0f )
	, mPosition( 0.0f, 0.0f, 0.0f )
{}

Piston::Piston(float x, float z)
	: mOffset( ci::Rand::randFloat(0.0f, 10.0f) )
	, mColor(  ci::CM_HSV,  ci::Rand::randFloat(0.0f, 0.1f),  ci::Rand::randFloat(0.0f, 1.0f),  ci::Rand::randFloat(0.25f, 1.0f) )
	, mPosition(  ci::Vec3f(x, 0.0f, z) )
{}

void Piston::update(const ci::Camera& camera)
{
	mDistance = mPosition.distanceSquared(camera.getEyePoint());
}

void Piston::draw(float time)
{
	float t = mOffset + time;
	float height = 55.0f + 45.0f *  ci::math<float>::sin(t);
	mPosition.y = 0.5f * height;

	gl::color( mColor );
	gl::drawCube( mPosition,  ci::Vec3f(10.0f, height, 10.0f) );
}

/////////////////////////////////////

const char* Pistons::vs = 
		"varying vec3 V;"
		"varying vec3 N;"

		"void main()"
		"{"
		"	V = vec3(gl_ModelViewMatrix * gl_Vertex);"
		"	N = normalize(gl_NormalMatrix * gl_Normal);"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;"
		"	gl_Position = ftransform();"
		"	gl_FrontColor = gl_Color;"
		"}";

const char* Pistons::fs =
		"varying vec3 V;"
		"varying vec3 N;"

		"void main()"
		"{"
		"	vec2 uv = gl_TexCoord[0].st;"
		"	vec3 L = normalize(-V);"
		"	vec3 E = normalize(-V);"
		"	vec3 R = normalize(-reflect(L,N));"

			// diffuse term with fake ambient occlusion
		"	float occlusion = 0.5 + 0.5*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y);"
		"	vec4 diffuse = gl_Color * occlusion;"
		"	diffuse *= max(dot(N,L), 0.0);"

			// specular term
		"	vec4 specular = gl_Color;"
		"	specular *= pow(max(dot(R,E),0.0), 50.0);"

			// write gamma corrected final color
		"	vec3 final = sqrt(diffuse + specular).rgb;"
		"	gl_FragColor.rgb = final;"

			// write luminance in alpha channel (required for FXAA)
		"	const vec3 luminance = vec3(0.299, 0.587, 0.114);"
		"	gl_FragColor.a = dot( final, luminance );"
		"}";

void Pistons::setup()
{
	mPistons.clear();

	for(int x=-50; x<=50; x+=10)
		for(int z=-50; z<=50; z+=10)
			mPistons.push_back( Piston( float(x), float(z) ) );

	// Load and compile our shaders and textures
	try { 
		mShader = gl::GlslProg( vs, fs );
	}
	catch( const std::exception& e ) { console() << e.what() << std::endl; }
}

void Pistons::update(const ci::Camera& camera)
{
	for(auto &piston : mPistons)
		piston.update(camera);

	std::qsort( &mPistons.front(), mPistons.size(), sizeof(Piston), &Piston::CompareByDistanceToCamera );
}

void Pistons::draw(const ci::Camera& camera, float time)
{
	gl::enableDepthRead();
	gl::enableDepthWrite();
	{
		gl::pushMatrices();
		gl::setMatrices(camera);
		{
			mShader.bind();
			{
				for(auto &piston : mPistons)
					piston.draw(time);
			}
			mShader.unbind();
		}
		gl::popMatrices();
	}
	gl::disableDepthWrite();
	gl::disableDepthRead();
}