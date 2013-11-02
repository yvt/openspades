#include <OpenSpades.h>
#include <iostream>
#include <sstream>
#include <Client/IRenderer.h>
#include <Client/IImage.h>
#include <Client/IFont.h>
#include <Client/FontData.h>
#include <Core/Settings.h>
#include <Client/ChatWindow.h>
#include <Client/PhysicsConstants.h>
#include <Imports/SDL.h>


SPADES_SETTING(cg_fov, "68");

spades::client::Quake3Font* designFont = NULL;
spades::client::Quake3Font* textFont = NULL;
spades::client::Quake3Font* bigTextFont = NULL;
spades::client::Quake3Font* orbitronFont = NULL;

spades::Handle<spades::client::IImage> whiteImage;
spades::client::ChatWindow* chatWindow;
spades::client::ChatWindow* killfeedWindow;

const char* weapName( int idx )
{
	switch( idx ) {
		case RIFLE_WEAPON: return "Rifle";
		case SMG_WEAPON: return "SMG";
		case SHOTGUN_WEAPON: return "Shotgun";
		default: return "-";
	}
}

const char* playName( int idx )
{
	switch( idx ) {
		case 0: return "PlayerGreen";
		case 1: return "PlayerBlue";
		case 2: return "PlayerYellow";
		default: return "-";
	}
}


void addKill()
{
	int team1 = rand() % 3;
	int team2 = rand() % 3;
	std::stringstream ss;
	ss << spades::client::ChatWindow::TeamColorMessage(playName(team1), team1);
	bool ff = team1 == team2;
	bool wasSelf = !(rand() % 10);
	if( wasSelf ) {
		ff = false;
	}

	std::string cause = " [";
	int killNum = rand() % 8;
	killNum = killNum > KillTypeClassChange ? 0 : killNum;	//we want 2 extra, because we have 3 weapons.
	cause += spades::client::ChatWindow::killImage( (KillType)killNum, rand() % 3 );
	cause += "] ";
	ss << (ff ? spades::client::ChatWindow::ColoredMessage(cause, spades::client::MsgColorRed) : cause);
			
	if(!wasSelf){
		ss << spades::client::ChatWindow::TeamColorMessage(playName(team2), team2);
	}
	killfeedWindow->AddMessage(ss.str());
	killNum++;
}

void addChat()
{
	std::stringstream ss;
	int idx = rand() % 5;
	switch( idx )
	{
	case 0: case 1: case 2:
		ss << "[Global] " << spades::client::ChatWindow::TeamColorMessage( "Deuce", idx) << ": Some chat";
		break;
	case 3:
		ss << spades::client::ChatWindow::ColoredMessage("Screenshot saved: ", spades::client::MsgColorSysInfo);
		break;
	case 4:
		ss << spades::client::ChatWindow::ColoredMessage("Screenshot failed: ", spades::client::MsgColorRed);
		break;
	}
	chatWindow->AddMessage( ss.str() );
}


Uint32 ot = 0;
Uint32 nextChat = 0;
Uint32 nextKill = 0;
void drawFrame( spades::client::IRenderer& renderer )
{
	if( !designFont ) {
		renderer.Init();
		designFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/UnsteadyOversteer.tga"), (const int *)UnsteadyOversteerMap, 30, 18);
		textFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/UbuntuCondensed.tga"), (const int*)UbuntuCondensedMap, 24, 4); 
		bigTextFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/UbuntuCondensedBig.tga"), (const int*)UbuntuCondensedBigMap, 48, 8);
		orbitronFont = new spades::client::Quake3Font(&renderer, renderer.RegisterImage("Gfx/Fonts/Orbitron.tga"), (const int*)OrbitronMap, 30, 18);
		whiteImage = renderer.RegisterImage("Gfx/White.tga");
		chatWindow = new spades::client::ChatWindow( NULL, &renderer, textFont, false );
		killfeedWindow = new spades::client::ChatWindow( NULL, &renderer, textFont, true );
		ot = SDL_GetTicks();
		srand( ot );
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
	renderer.SetColor( spades::MakeVector4( 0.5f, 0.5f, 0.5f, 1 ) );
	renderer.DrawImage( whiteImage, spades::AABB2(0, 0, scrSize.x, scrSize.y) );
	

	Uint32 dt = SDL_GetTicks() - ot;
	float fdt = (float)dt / 1000.f;
	chatWindow->Update( fdt );
	killfeedWindow->Update( fdt );
	ot += dt;
	
	if( !nextChat || ot >= nextChat ) {
		nextChat = ot + 1000 + (rand() % 2000);
		addChat();
	}

	if( !nextKill || ot >= nextKill ) {
		nextKill = ot + 200 + (rand() % 1000);
		addKill();
	}

	chatWindow->Draw();
	killfeedWindow->Draw();

	renderer.FrameDone();
	renderer.Flip();
}

