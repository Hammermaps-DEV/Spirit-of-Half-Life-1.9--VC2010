//
// particlemsg.cpp
//
// implementation of CHudParticle class
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "particlemgr.h"
#include "particlesys.h"

#include <string.h>
#include <stdio.h>

extern ParticleSystemManager* g_pParticleSystems;

DECLARE_MESSAGE(m_Particle, Particle)

void CHudParticle::Init(void)
{
	HOOK_MESSAGE(Particle);
	gHUD.AddHudElem(this);

	if (g_pParticleSystems)
	{
		delete g_pParticleSystems;
		g_pParticleSystems = NULL;
	}

	g_pParticleSystems = new ParticleSystemManager();
};

void CHudParticle::VidInit(void)
{
	g_pParticleSystems->ClearSystems();
};

int CHudParticle:: MsgFunc_Particle(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int entindex = READ_WORD();
	char *sz = READ_STRING();

	ParticleSystem *pSystem = new ParticleSystem(entindex, sz);
	g_pParticleSystems->AddSystem(pSystem);

	return 1;
}