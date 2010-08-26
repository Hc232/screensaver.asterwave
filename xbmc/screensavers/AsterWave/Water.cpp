/*
 * Silverwave Screensaver for XBox Media Center
 * Copyright (c) 2004 Team XBMC
 *
  * Ver 1.0 2007-02-12 by Asteron  http://asteron.projects.googlepages.com/home
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2  of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "../../addons/include/xbmc_scr_dll.h"
#include "../../addons/include/xbmc_addon_cpp_dll.h"
#include "Water.h"
#include "Effect.h"
#include "XmlDocument.h"
#include "Util.h"
#include <string>
#include <io.h>

#define DEFAULT_ADDON_ID "screensaver.asterwave"
#define DEFAULT_XBMC_ADDONS_PATH "special://xbmc/addons/"
#define DEFAULT_HOME_ADDONS_PATH "special://home/addons/"
#define PRESETS_DIR "/resources/presets/"
#define PRESETS_MASK PRESETS_DIR "/*.xml"

WaterSettings world;
AnimationEffect * effects[7];
  
LPDIRECT3DDEVICE9 g_pd3dDevice;
LPDIRECT3DTEXTURE9  g_Texture = NULL;

std::string g_strPresetsDir;
DllSetting g_settingPreset(DllSetting::SPIN, "preset", "30000");

int  m_iWidth;
int m_iHeight;
float m_pixelRatio;
D3DXVECTOR3 g_lightDir;
float g_shininess = 0.4f;

static std::vector<DllSetting> m_vecSettings;
static StructSetting ** m_structSettings;
static int m_iVisElements;

void SetDefaults()
{
  //world.effectType = rand()%world.effectCount;
  world.effectType = 0;
  world.isSingleEffect = true;
  world.frame = 0;
  world.nextEffectTime = 0;
  world.isWireframe = false;
  world.isTextureMode = false;
}

void CreateSettings()
{
  m_vecSettings.clear();
  m_iVisElements = 0;
  g_settingPreset.entry.clear();
  g_settingPreset.current = 0;

  std::string strMask(g_strPresetsDir);
  strMask.append(PRESETS_MASK);

  struct _finddata_t c_file;
  long hFile = _findfirst(strMask.c_str(), &c_file);
  if (hFile == -1L) 
  {
    g_strPresetsDir.assign(DEFAULT_HOME_ADDONS_PATH DEFAULT_ADDON_ID);
    strMask.assign(g_strPresetsDir)
      .append(PRESETS_MASK);
    hFile = _findfirst(strMask.c_str(), &c_file);
  }
  if (hFile == -1L) 
  {
    g_strPresetsDir.assign(DEFAULT_XBMC_ADDONS_PATH DEFAULT_ADDON_ID);
    strMask.assign(g_strPresetsDir)
      .append(PRESETS_MASK);
    hFile = _findfirst(strMask.c_str(), &c_file);
  }
  if (hFile != -1L)
	{
    std::string strPreset;
    strPreset.assign(c_file.name, 0, strlen(c_file.name) - 4);
    g_settingPreset.AddEntry(strPreset.c_str());

		while (_findnext(hFile, &c_file) == 0)
		{
      strPreset.assign(c_file.name, 0, strlen(c_file.name) - 4);
      g_settingPreset.AddEntry(strPreset.c_str());
		}
		_findclose(hFile);
	}

  m_vecSettings.push_back(g_settingPreset);
}

// Load settings from the [preset].xml configuration file
void LoadSettingsXML()
{
  XmlNode node, childNode;
  CXmlDocument doc;
  
  float xmin = -10.0f, 
    xmax = 10.0f, 
    ymin = -10.0f, 
    ymax = 10.0f, 
    height = 0.0f, 
    elasticity = 0.5f, 
    viscosity = 0.05f, 
    tension = 1.0f, 
    blendability = 0.04f;

  int xdivs = 50; 
  int ydivs = 50;
  int divs = 50;

  SetDefaults();

  g_lightDir = D3DXVECTOR3(0.0f,0.6f,-0.8f);

  std::string strXMLFile(g_strPresetsDir);
  strXMLFile.append(PRESETS_DIR)
    .append(g_settingPreset.entry.at(g_settingPreset.current))
    .append(".xml");

#ifdef _DEBUG
  OutputDebugString("AsterWave: Loading XML: ");
  OutputDebugString(strXMLFile.c_str());  
  OutputDebugString("\n");
#endif

  // Load the config file
  if (doc.Load(strXMLFile.c_str()) >= 0)
  {
    node = doc.GetNextNode(XML_ROOT_NODE);
    while(node > 0)
    {
      if (strcmpi(doc.GetNodeTag(node),"screensaver"))
      {
        node = doc.GetNextNode(node);
        continue;
      }
      if (childNode = doc.GetChildNode(node,"wireframe")){
        world.isWireframe = strcmp(doc.GetNodeText(childNode),"true") == 0;
      }
      if (childNode = doc.GetChildNode(node,"elasticity")){
        elasticity = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"viscosity")){
        viscosity = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"tension")){
        tension = (float)atof(doc.GetNodeText(childNode));
      }      
      if (childNode = doc.GetChildNode(node,"blendability")){
        tension = (float)atof(doc.GetNodeText(childNode));
      }      
      if (childNode = doc.GetChildNode(node,"xmin")){
        xmin = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"xmax")){
        xmax = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"ymin")){
        ymin = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"ymax")){
        ymax = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"lightx")){
        g_lightDir.x = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"lighty")){
        g_lightDir.y = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"lightz")){
        g_lightDir.z = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"shininess")){
        g_shininess = (float)atof(doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"effect")){
        world.effectType = atoi(doc.GetNodeText(childNode));
        world.isSingleEffect = true;
      }
      if (childNode = doc.GetChildNode(node,"texture")){
        strcpy(world.szTexturePath, doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"texturefolder")){
        strcpy(world.szTextureSearchPath, doc.GetNodeText(childNode));
      }
      if (childNode = doc.GetChildNode(node,"texturemode")){
        world.isTextureMode = strcmp(doc.GetNodeText(childNode),"true") == 0;
      }
      if (childNode = doc.GetChildNode(node,"nexttexture")){
        world.nextTextureTime = (int)(30*(float)atof(doc.GetNodeText(childNode)));
      }
      if (childNode = doc.GetChildNode(node,"quality")){
        divs = atoi(doc.GetNodeText(childNode));
      }
      node = doc.GetNextNode(node);
    }
    doc.Close();
  }
  float scaleRatio = (xmax-xmin)/(ymax-ymin);
  int totalPoints = (int)(divs * divs * scaleRatio);
  //world.scaleX = 0.5f;
  xdivs = (int)sqrt(totalPoints * scaleRatio / world.scaleX);
  ydivs = totalPoints / xdivs;
  //xdivs = 144; ydivs= 90;
  world.waterField = new WaterField(xmin, xmax, ymin, ymax, xdivs, ydivs, height, elasticity, viscosity, tension, blendability, world.isTextureMode);
}

void SetLight()
{
  // Fill in a light structure defining our light
  D3DLIGHT9 light;
  memset(&light, 0, sizeof(D3DLIGHT9));
  light.Type    = D3DLIGHT_POINT;
  light.Ambient = (D3DXCOLOR)D3DCOLOR_RGBA(0,0,0,255);
  light.Diffuse = (D3DXCOLOR)D3DCOLOR_RGBA(255,255,255,255);
  light.Specular = (D3DXCOLOR)D3DCOLOR_RGBA(150,150,150,255);
  light.Range   = 300.0f;
  light.Position = D3DXVECTOR3(0,-5,5);
  light.Attenuation0 = 0.5f;
  light.Attenuation1 = 0.02f;
  light.Attenuation2 = 0.0f;

  // Create a direction for our light - it must be normalized 
  light.Direction = g_lightDir;

  // Tell the device about the light and turn it on
  g_pd3dDevice->SetLight(0, &light);
  g_pd3dDevice->LightEnable(0, TRUE); 
  g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
}

////////////////////////////////////////////////////////////////////////////////

void LoadEffects()
{
  effects[0] = new EffectBoil();
  effects[1] = new EffectTwist();
  effects[2] = new EffectBullet();
  effects[3] = new EffectRain();
  effects[4] = new EffectSwirl();
  effects[5] = new EffectXBMCLogo();
  effects[6] = NULL;
  //effects[i] = new EffectText(),

  int i = 0;
  while(effects[i] != NULL)
    effects[i++]->init(&world);
  world.effectCount = i;
}

void SetMaterial()
{
  D3DMATERIAL9 mat;

  // Set the RGBA for diffuse reflection.
  if (world.isTextureMode)
  {
    mat.Diffuse.r = 1.0f;
    mat.Diffuse.g = 1.0f;
    mat.Diffuse.b = 1.0f;
    mat.Diffuse.a = 1.0f;
  }
  else
  {
    mat.Diffuse.r = 0.5f;
    mat.Diffuse.g = 0.5f;
    mat.Diffuse.b = 0.5f;
    mat.Diffuse.a = 1.0f;
  }

  // Set the RGBA for ambient reflection.
  mat.Ambient.r = 0.5f;
  mat.Ambient.g = 0.5f;
  mat.Ambient.b = 0.5f;
  mat.Ambient.a = 1.0f;

  // Set the color and sharpness of specular highlights.
  mat.Specular.r = g_shininess;
  mat.Specular.g = g_shininess;
  mat.Specular.b = g_shininess;
  mat.Specular.a = 1.0f;
  mat.Power = 100.0f;

  // Set the RGBA for emissive color.
  mat.Emissive.r = 0.0f;
  mat.Emissive.g = 0.0f;
  mat.Emissive.b = 0.0f;
  mat.Emissive.a = 0.0f;

  g_pd3dDevice->SetMaterial(&mat);
}

static HRESULT CreateTextureFromFile(std::string & filename, LPDIRECT3DTEXTURE9 * ppTexture) 
{
  HRESULT res = -1;

	FILE *f = fopen(filename.c_str(), "rb");
  if (f != NULL)
  {
	  fseek(f, 0, SEEK_END);
	  int len = ftell(f);
	  fseek(f, 0, SEEK_SET);
    unsigned char* pBuffer = new unsigned char[len];
	  fread(pBuffer, len, 1, f);
	  fclose(f);

    res = D3DXCreateTextureFromFileInMemory(g_pd3dDevice, pBuffer, len, ppTexture);
    delete [] pBuffer;
  }

  return res;
}

void LoadTexture()
{
  int numTextures = 0;
  std::string foundTexture;  
  if (strlen(world.szTexturePath) > 0) {
    // single texture
    foundTexture.assign(g_strPresetsDir)
      .append("/")
      .append(world.szTexturePath);
    numTextures++;
  } else {
    // random texture
    std::string strPath(g_strPresetsDir);
    strPath.append("/")
      .append(world.szTextureSearchPath)
      .append("/");

    std::string strSearchPath(strPath);
    strSearchPath.append("*");

    struct _finddata_t c_file;
    long hFile = _findfirst(strSearchPath.c_str(), &c_file);
    if (hFile != -1L)
	  {
      while (_findnext(hFile, &c_file) == 0)
		  {
        int len = (int)strlen(c_file.name);
        if (len < 4 || (strcmpi(c_file.name + len - 4, ".txt") == 0))
          continue;
      
        if (rand() % (numTextures+1) == 0) // after n textures each has 1/n prob
        {
          foundTexture.assign(strPath);
          foundTexture.append(c_file.name);
        }
        numTextures++;        
		  }
		  _findclose(hFile);
	  }
  }

  if (g_Texture != NULL && numTextures > 0)
  {
    g_Texture->Release();
    g_Texture = NULL;
  }
    
  if (numTextures > 0)
    CreateTextureFromFile(foundTexture, &g_Texture);
}



void SetCamera()
{
  D3DXMATRIX m_World;
  D3DXMatrixIdentity(&m_World);
  g_pd3dDevice->SetTransform(D3DTS_WORLD, &m_World);

  D3DXMATRIX m_View;
  D3DXVECTOR3 from = D3DXVECTOR3(0,14,14);
  D3DXVECTOR3 to   = D3DXVECTOR3(0,3,0);
  D3DXVECTOR3 up   = D3DXVECTOR3(0,-0.707f,0.707f);

  D3DXMatrixLookAtLH(&m_View, &from, &to, &up);
  g_pd3dDevice->SetTransform(D3DTS_VIEW, &m_View);

  float aspectRatio = 720.0f / 480.0f;
  D3DXMATRIX m_Projection; 
  D3DXMatrixPerspectiveFovLH(&m_Projection, D3DX_PI/4, aspectRatio, 1.0, 1000.0);
  g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &m_Projection);
}

void SetupRenderState()
{
  SetLight();
  SetCamera();
  SetMaterial();

  g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
  g_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, g_shininess > 0);
  if(world.isWireframe)
    g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
  else
    g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

  if (world.isTextureMode)
  {
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP,	 D3DTOP_MODULATE);
	  g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
	  g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);

	  g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	  g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	  g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP);
	  g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP);
	  
    g_pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP,	 D3DTOP_DISABLE);
	  g_pd3dDevice->SetTextureStageState(1, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);
    
    g_pd3dDevice->SetTexture(0, g_Texture);
	  g_pd3dDevice->SetTexture(1, NULL);
  }
}


extern "C"
{

// XBMC has loaded us into memory,
// we should set our core values
// here and load any settings we
// may have from our config file
ADDON_STATUS Create(void* hdl, void* props)
{
  if (!props)
    return STATUS_UNKNOWN;
 
  SCR_PROPS* scrprops = (SCR_PROPS*) props;

  g_strPresetsDir.assign(scrprops->presets);

  g_pd3dDevice = (LPDIRECT3DDEVICE9) scrprops->device;
  m_iWidth =  scrprops->width;
  m_iHeight = scrprops->height;
  m_pixelRatio = scrprops->pixelRatio;

  CreateSettings();

  /*
  char buff[100];
  for(int j = 0; j < 15; j++)
  {
    buff[j] = (int)ratio + '0';
    ratio = (ratio - (int)ratio)*10;
  }
  buff[15] = 0;
  ((EffectText*)(effects[6]))->drawString(buff,0.6f,0.6f,1.3f,0.06f,-7.0f,7.8f);*/
  //world.widescreen = (float)iWidth/ (float)iHeight > 1.7f;

  return STATUS_NEED_SETTINGS;
}

// XBMC tells us we should get ready
// to start rendering. This function
// is called once when the screensaver
// is activated by XBMC.
void Start()
{
  memset(&world, 0, sizeof(WaterSettings));

  world.scaleX = 1.0f;
  if ((m_pixelRatio * m_iWidth / m_iHeight) > 1.5)
    world.scaleX = 1/1.333f;//0.91158f/ratio; widescreen mode
  // Load the settings
  LoadSettingsXML();
  LoadEffects();
  
  if (world.isTextureMode)
  {
    LoadTexture();
    world.effectCount--; //get rid of logo effect
  }

  if (!world.isSingleEffect)
  {
    world.effectType = rand() % world.effectCount;
  }
  else
  {
    effects[world.effectType]->reset();
  }


  world.frame = 0;
  world.nextEffectTime = 0;
  return;
}

// XBMC tells us to render a frame of our screensaver.
void Render()
{
  g_pd3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0);
  SetupRenderState();

  world.frame++;

  if (world.isTextureMode && 
      world.nextTextureTime > 0 && 
     (world.frame % world.nextTextureTime) == 0)
    LoadTexture();

  if (!world.isSingleEffect && world.frame > world.nextEffectTime)
  {
    if ((rand() % 3) == 0)
      incrementColor();
    //static limit = 0;if (limit++>3)
    world.effectType += 1;//+rand() % (ANIM_MAX-1);
    world.effectType %= world.effectCount;
    effects[world.effectType]->reset();
    world.nextEffectTime = world.frame + effects[world.effectType]->minDuration() + 
      rand() % (effects[world.effectType]->maxDuration() - effects[world.effectType]->minDuration());
  }
  effects[world.effectType]->apply();
  world.waterField->Step();
  world.waterField->Render();
  return;
}

// XBMC tells us to stop the screensaver
// we should free any memory and release
// any resources we have created.
void Stop()
{
  delete world.waterField;
  world.waterField = NULL;
  for (int i = 0; effects[i] != NULL; i++) {
    delete effects[i];
    effects[i] = NULL;
  }
  if (g_Texture != NULL)
  {
    g_Texture->Release();
    g_Texture = NULL;
  }
  return;
}

void GetInfo(SCR_INFO* pInfo)
{
  // not used, but can be used to pass info
  // back to XBMC if required in the future
  return;
}

void Destroy()
{
  return;
}

ADDON_STATUS GetStatus()
{
  return STATUS_OK;
}

bool HasSettings()
{
  return true;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
unsigned int GetSettings(StructSetting*** sSet)
{
  if(m_vecSettings.empty())
    CreateSettings();

  m_iVisElements = DllUtils::VecToStruct(m_vecSettings, &m_structSettings);
  *sSet = m_structSettings;
  return m_iVisElements;
}

void FreeSettings()
{
  DllUtils::FreeStruct(m_iVisElements, &m_structSettings);
}

ADDON_STATUS SetSetting(const char *strSetting, const void* value)
{
  if (!strSetting || !value)
    return STATUS_UNKNOWN;

  if (strcmp(strSetting, "preset")==0)
  {
    int iValue = *(int *) value;
    if ((iValue >= 0) && 
        (iValue < (int) g_settingPreset.entry.size()))
    {
      g_settingPreset.current = iValue;
      return STATUS_OK;
    }
  }
  return STATUS_UNKNOWN;
}


void Remove()
{
}

} // extern "C"
