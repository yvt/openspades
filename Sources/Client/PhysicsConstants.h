//
//  PhysicsConstants.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#define FALL_SLOW_DOWN	0.24f
#define FALL_DAMAGE_VELOCITY	0.58f
#define FALL_DAMAGE_SCALAR	4096
#define MINERANGE	3
#define SCPITCH 128
#define MAXSCANDIST 128
#define MAXSCANSQ (MAXSCANDIST*MAXSCANDIST)
#define BOUNCE_SOUND_THRESHOLD 0.1f
#define TC_CAPTURE_RATE 0.05f
#define TC_CAPTURE_DISTANCE 16.f

enum WeaponType {
	RIFLE_WEAPON,
	SMG_WEAPON,
	SHOTGUN_WEAPON
};

enum BlockActionType {
	BlockActionCreate,
	BlockActionTool, // gun and spade
	BlockActionDig,
	BlockActionGrenade
};

// "Hit Packet" and weapon damage query
enum HitType {
	HitTypeTorso,
	HitTypeHead,
	HitTypeArms,
	HitTypeLegs,
	HitTypeBlock, // used for block damage query
	HitTypeMelee 
};

enum HurtType {
	HurtTypeFall = 0,
	HurtTypeWeapon
};

enum KillType {
	KillTypeWeapon = 0,
	KillTypeHeadshot,
	KillTypeMelee,
	KillTypeGrenade,
	KillTypeFall,
	KillTypeTeamChange,
	KillTypeClassChange
};

