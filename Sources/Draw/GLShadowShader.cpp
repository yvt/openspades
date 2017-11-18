/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "GLShadowShader.h"
#include "GLAmbientShadowRenderer.h"
#include "GLBasicShadowMapRenderer.h"
#include "GLImage.h"
#include "GLMapShadowRenderer.h"
#include "GLProgramManager.h"
#include "GLRadiosityRenderer.h"
#include "GLRenderer.h"
#include "GLSparseShadowMapRenderer.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Settings.h>

namespace spades {
	namespace draw {
		GLShadowShader::GLShadowShader()
		    : eyeOrigin("eyeOrigin"),
		      eyeFront("eyeFront"),
		      mapShadowTexture("mapShadowTexture"),
		      fogColor("fogColor"),
		      ambientColor("ambientColor"),
		      shadowMapViewMatrix("shadowMapViewMatrix"),
		      shadowMapTexture1("shadowMapTexture1"),
		      shadowMapTexture2("shadowMapTexture2"),
		      shadowMapTexture3("shadowMapTexture3"),
		      shadowMapMatrix1("shadowMapMatrix1"),
		      shadowMapMatrix2("shadowMapMatrix2"),
		      shadowMapMatrix3("shadowMapMatrix3"),
		      ambientShadowTexture("ambientShadowTexture"),
		      radiosityTextureFlat("radiosityTextureFlat"),
		      radiosityTextureX("radiosityTextureX"),
		      radiosityTextureY("radiosityTextureY"),
		      radiosityTextureZ("radiosityTextureZ"),
		      pagetableSize("pagetableSize"),
		      pagetableSizeInv("pagetableSizeInv"),
		      minLod("minLod"),
		      shadowMapSizeInv("shadowMapSizeInv"),
		      ssaoTexture("ssaoTexture"),
		      ssaoTextureUVScale("ssaoTextureUVScale") {}

		std::vector<GLShader *> GLShadowShader::RegisterShader(spades::draw::GLProgramManager *r,
		                                                       GLSettings &settings, bool variance,
		                                                       bool skipSSAO) {
			std::vector<GLShader *> shaders;

			if (skipSSAO) {
				shaders.push_back(r->RegisterShader("Shaders/Shadow/CommonNoSSAO.fs"));
			} else {
				shaders.push_back(r->RegisterShader("Shaders/Shadow/Common.fs"));
			}
			shaders.push_back(r->RegisterShader("Shaders/Shadow/Common.vs"));

			if (variance) {

				shaders.push_back(r->RegisterShader("Shaders/Shadow/MapVariance.fs"));
				shaders.push_back(r->RegisterShader("Shaders/Shadow/MapVariance.vs"));

			} else {

				shaders.push_back(r->RegisterShader("Shaders/Shadow/Map.fs"));
				shaders.push_back(r->RegisterShader("Shaders/Shadow/Map.vs"));
			}

			if (settings.r_modelShadows) {
				if (settings.r_sparseShadowMaps) {
					shaders.push_back(r->RegisterShader("Shaders/Shadow/ModelSparse.fs"));
					shaders.push_back(r->RegisterShader("Shaders/Shadow/ModelSparse.vs"));
				} else {
					shaders.push_back(r->RegisterShader("Shaders/Shadow/Model.fs"));
					shaders.push_back(r->RegisterShader("Shaders/Shadow/Model.vs"));
				}
			} else {
				shaders.push_back(r->RegisterShader("Shaders/Shadow/ModelNull.fs"));
				shaders.push_back(r->RegisterShader("Shaders/Shadow/ModelNull.vs"));
			}

			if (settings.r_radiosity) {
				if ((int)settings.r_radiosity >= 2) {
					shaders.push_back(r->RegisterShader("Shaders/Shadow/MapRadiosity.fs"));
				} else {
					shaders.push_back(r->RegisterShader("Shaders/Shadow/MapRadiosityLow.fs"));
				}
				shaders.push_back(r->RegisterShader("Shaders/Shadow/MapRadiosity.vs"));
			} else {
				shaders.push_back(r->RegisterShader("Shaders/Shadow/MapRadiosityNull.fs"));
				shaders.push_back(r->RegisterShader("Shaders/Shadow/MapRadiosityNull.vs"));
			}

			return shaders;
		}

		int GLShadowShader::operator()(GLRenderer *renderer, spades::draw::GLProgram *program,
		                               int texStage) {
			mapShadowTexture(program);
			fogColor(program);
			eyeOrigin(program);
			eyeFront(program);

			const client::SceneDefinition &def = renderer->GetSceneDef();

			eyeOrigin.SetValue(def.viewOrigin.x, def.viewOrigin.y, def.viewOrigin.z);
			eyeFront.SetValue(def.viewAxis[2].x, def.viewAxis[2].y, def.viewAxis[2].z);

			Vector3 fc = renderer->GetFogColorForSolidPass();
			fc *= fc; // linearize
			fogColor.SetValue(fc.x, fc.y, fc.z);

			IGLDevice *dev = program->GetDevice();
			dev->ActiveTexture(texStage);
			if (renderer->mapShadowRenderer) {
				dev->BindTexture(IGLDevice::Texture2D, renderer->mapShadowRenderer->GetTexture());
			} else {
				// TODO: do this case properly
				GLImage *img = (GLImage *)renderer->RegisterImage("Gfx/White.tga");
				img->Bind(IGLDevice::Texture2D);
			}
			mapShadowTexture.SetValue(texStage);
			texStage++;

			auto &settings = renderer->GetSettings();

			if (settings.r_modelShadows) {

				if (settings.r_sparseShadowMaps) {
					shadowMapTexture1(program);
					shadowMapTexture2(program);
					shadowMapMatrix1(program);
					pagetableSize(program);
					pagetableSizeInv(program);
					minLod(program);
					shadowMapSizeInv(program);

					GLSparseShadowMapRenderer *s =
					  static_cast<GLSparseShadowMapRenderer *>(renderer->GetShadowMapRenderer());
					SPAssert(s != NULL);

					pagetableSize.SetValue((float)GLSparseShadowMapRenderer::Tiles);
					pagetableSizeInv.SetValue(1.f / (float)GLSparseShadowMapRenderer::Tiles);
					minLod.SetValue((float)(1 << s->minLod));
					shadowMapSizeInv.SetValue(1.f / (float)s->textureSize);

					dev->ActiveTexture(texStage);
					dev->BindTexture(IGLDevice::Texture2D, s->texture);
					shadowMapTexture1.SetValue(texStage);
					shadowMapMatrix1.SetValue(s->matrix);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareMode,
					                  IGLDevice::CompareRefToTexture);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareFunc,
					                  IGLDevice::Less);

					texStage++;

					dev->ActiveTexture(texStage);
					dev->BindTexture(IGLDevice::Texture2D, s->pagetableTexture);
					shadowMapTexture2.SetValue(texStage);

					texStage++;

				} else {

					shadowMapTexture1(program);
					shadowMapTexture2(program);
					shadowMapTexture3(program);
					shadowMapMatrix1(program);
					shadowMapMatrix2(program);
					shadowMapMatrix3(program);
					shadowMapViewMatrix(program);

					GLBasicShadowMapRenderer *s =
					  static_cast<GLBasicShadowMapRenderer *>(renderer->GetShadowMapRenderer());
					SPAssert(s != NULL);

					dev->ActiveTexture(texStage);
					dev->BindTexture(IGLDevice::Texture2D, s->texture[0]);
					shadowMapTexture1.SetValue(texStage);
					shadowMapMatrix1.SetValue(s->matrices[0]);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareMode,
					                  IGLDevice::CompareRefToTexture);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareFunc,
					                  IGLDevice::Less);

					texStage++;

					dev->ActiveTexture(texStage);
					dev->BindTexture(IGLDevice::Texture2D, s->texture[1]);
					shadowMapTexture2.SetValue(texStage);
					shadowMapMatrix2.SetValue(s->matrices[1]);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareMode,
					                  IGLDevice::CompareRefToTexture);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareFunc,
					                  IGLDevice::Less);

					texStage++;

					dev->ActiveTexture(texStage);
					dev->BindTexture(IGLDevice::Texture2D, s->texture[2]);
					shadowMapTexture3.SetValue(texStage);
					shadowMapMatrix3.SetValue(s->matrices[2]);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareMode,
					                  IGLDevice::CompareRefToTexture);
					dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareFunc,
					                  IGLDevice::Less);

					texStage++;

					shadowMapViewMatrix.SetValue(renderer->GetViewMatrix());
				}
			}

			if (settings.r_radiosity) {

				Vector3 ac = renderer->GetFogColor();
				ac *= ac; // linearize
				ambientColor(program);
				ac *= 0.5f;
				const float minimumLuminance = 0.35f;
				float luminance = (ac.x + ac.y + ac.z) / 3.0f;
				if (luminance < minimumLuminance) {
					// we want things still visible even if sky is pitch black
					Vector3 offset = (ac + 0.003f) / (luminance + 0.003f);
					ac += offset * (minimumLuminance - luminance);
				}
				ambientColor.SetValue(ac.x, ac.y, ac.z);

				ambientShadowTexture(program);
				radiosityTextureFlat(program);
				radiosityTextureX(program);
				radiosityTextureY(program);
				radiosityTextureZ(program);

				// TODO: deal with the case that renderer->ambientShadowRenderer is null
				SPAssert(renderer->ambientShadowRenderer != NULL);

				dev->ActiveTexture(texStage);
				dev->BindTexture(IGLDevice::Texture3D,
				                 renderer->ambientShadowRenderer->GetTexture());
				ambientShadowTexture.SetValue(texStage);
				texStage++;

				dev->ActiveTexture(texStage);
				dev->BindTexture(IGLDevice::Texture3D,
				                 renderer->radiosityRenderer->GetTextureFlat());
				radiosityTextureFlat.SetValue(texStage);
				texStage++;

				dev->ActiveTexture(texStage);
				dev->BindTexture(IGLDevice::Texture3D, renderer->radiosityRenderer->GetTextureX());
				radiosityTextureX.SetValue(texStage);
				texStage++;

				dev->ActiveTexture(texStage);
				dev->BindTexture(IGLDevice::Texture3D, renderer->radiosityRenderer->GetTextureY());
				radiosityTextureY.SetValue(texStage);
				texStage++;

				dev->ActiveTexture(texStage);
				dev->BindTexture(IGLDevice::Texture3D, renderer->radiosityRenderer->GetTextureZ());
				radiosityTextureZ.SetValue(texStage);
				texStage++;
			}

			if (settings.r_ssao) {
				ssaoTexture(program);
				ssaoTextureUVScale(program);

				if (ssaoTexture.IsActive()) {
					dev->ActiveTexture(texStage);
					dev->BindTexture(IGLDevice::Texture2D, renderer->ssaoBufferTexture);
					ssaoTexture.SetValue(texStage);
					texStage++;

					ssaoTextureUVScale.SetValue(1.0f / renderer->ScreenWidth(),
					                            1.0f / renderer->ScreenHeight());
				}
			}

			dev->ActiveTexture(texStage);

			return texStage;
		}
	}
}
