#ifndef RPMAIN_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define RPMAIN_H
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <Core/Debug.h>
#include <Core/MemoryStream.h>
#include <Core/Settings.h>
#include <Core/Strings.h>
#include <Core/ServerAddress.h>
#include <enet/enet.h>
#include <math.h>
#include <string>
#include <Client/NetClient.h>
#include "discord.h"

int rpM();
int atualizarRP();

#endif