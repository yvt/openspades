#include <OpenSpades.h>
#include <iostream>
#include <Client/IRenderer.h>
#include <Client/IImage.h>
#include <Client/IFont.h>
#include <Client/FontData.h>
#include <Core/Settings.h>


SPADES_SETTING(cg_fov, "68");

spades::client::Quake3Font* designFont = NULL;
spades::client::Quake3Font* textFont = NULL;
spades::client::Quake3Font* bigTextFont = NULL;
spades::client::Quake3Font* orbitronFont = NULL;

spades::Handle<spades::client::IImage> whiteImage;

void drawFrame( spades::client::IRenderer& renderer )
{
	if( !designFont ) {
		renderer.Init();
		designFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/UnsteadyOversteer.tga"), (const int *)UnsteadyOversteerMap, 30, 18);
		textFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/UbuntuCondensed.tga"), (const int*)UbuntuCondensedMap, 24, 4); 
		bigTextFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/UbuntuCondensedBig.tga"), (const int*)UbuntuCondensedBigMap, 48, 8);
		orbitronFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/Orbitron.tga"), (const int*)OrbitronMap, 30, 18);
		whiteImage = renderer.RegisterImage("Gfx/White.tga");
	}
	spades::client::SceneDefinition sceneDef;
	sceneDef.viewOrigin = spades::MakeVector3(0, 0, 0);
	sceneDef.viewAxis[0] = spades::MakeVector3(1, 0, 0);
	sceneDef.viewAxis[1] = spades::MakeVector3(0, 0, -1);
	sceneDef.viewAxis[2] = spades::MakeVector3(0, 0, 1);	
	sceneDef.fovY = (float)cg_fov * static_cast<float>(M_PI) /180.f;
	sceneDef.fovX = atanf(tanf(sceneDef.fovY * .5f) * renderer.ScreenWidth() / renderer.ScreenHeight()) * 2.f;		
	sceneDef.zNear = 0.05f;
	sceneDef.zFar = 130.f;
	sceneDef.skipWorld = true;	
	renderer.SetFogColor(spades::MakeVector3(0,0,0));

	renderer.StartScene(sceneDef);
	renderer.EndScene();

	spades::Vector2 scrSize = { renderer.ScreenWidth(), renderer.ScreenHeight() };
	renderer.DrawImage( whiteImage, spades::AABB2(0, 0, scrSize.x, scrSize.y) );
	
	spades::Vector2 pos = spades::MakeVector2( 10, 10 );
	for( int n = 0; n < 4; ++n ) {
		spades::client::IFont *font = n == 0 ? designFont : ( n == 1 ? textFont : (n == 2 ? bigTextFont : orbitronFont) );
		std::string str = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG\nthe quick brown fox jumps over the lazy dog\n0123456789";
		font->Draw(str, pos + spades::MakeVector2(1,1), 1.f, spades::MakeVector4(0,0,0,0.5));
		font->Draw(str, pos, 1.f, spades::MakeVector4(1,0,0,1));
		spades::Vector2 size = font->Measure(str);
		pos.y += size.y;
	}

	renderer.FrameDone();
	renderer.Flip();
}

