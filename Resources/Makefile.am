
Resourcesdir = $(datadir)/openspades/Resources

SHADERS = \
Shaders/Basic.fs \
Shaders/Basic.vs \
Shaders/Basic.program \
Shaders/BasicBlock.fs \
Shaders/BasicBlock.vs \
Shaders/BasicBlock.program \
Shaders/BasicBlockDynamicLit.fs \
Shaders/BasicBlockDynamicLit.vs \
Shaders/BasicBlockDynamicLit.program \
Shaders/BasicImage.fs \
Shaders/BasicImage.vs \
Shaders/BasicImage.program \
Shaders/OptimizedVoxelModel.fs \
Shaders/OptimizedVoxelModel.vs \
Shaders/OptimizedVoxelModel.program \
Shaders/OptimizedVoxelModelDynamicLit.fs \
Shaders/OptimizedVoxelModelDynamicLit.vs \
Shaders/OptimizedVoxelModelDynamicLit.program \
Shaders/OptimizedVoxelModelShadowMap.fs \
Shaders/OptimizedVoxelModelShadowMap.vs \
Shaders/OptimizedVoxelModelShadowMap.program \
Shaders/SoftSprite.fs \
Shaders/SoftSprite.vs \
Shaders/SoftSprite.program \
Shaders/Sprite.fs \
Shaders/Sprite.vs \
Shaders/Sprite.program \
Shaders/LongSprite.fs \
Shaders/LongSprite.vs \
Shaders/LongSprite.program \
Shaders/VoxelModel.fs \
Shaders/VoxelModel.vs \
Shaders/VoxelModel.program \
Shaders/VoxelModelDynamicLit.fs \
Shaders/VoxelModelDynamicLit.vs \
Shaders/VoxelModelDynamicLit.program \
Shaders/VoxelModelShadowMap.fs \
Shaders/VoxelModelShadowMap.vs \
Shaders/VoxelModelShadowMap.program \
Shaders/Water.fs \
Shaders/Water.vs \
Shaders/Water.program \
Shaders/WaterDepth.fs \
Shaders/WaterDepth.vs \
Shaders/WaterDepth.program \
Shaders/LensFlare/Draw.fs \
Shaders/LensFlare/Draw.vs \
Shaders/LensFlare/Draw.program \
Shaders/LensFlare/Scanner.fs \
Shaders/LensFlare/Scanner.vs \
Shaders/LensFlare/Scanner.program \
Shaders/PostFilters/CameraBlur.fs \
Shaders/PostFilters/CameraBlur.vs \
Shaders/PostFilters/CameraBlur.program \
Shaders/PostFilters/GammaMix.fs \
Shaders/PostFilters/GammaMix.vs \
Shaders/PostFilters/GammaMix.program \
Shaders/PostFilters/Gauss1D.fs \
Shaders/PostFilters/Gauss1D.vs \
Shaders/PostFilters/Gauss1D.program \
Shaders/PostFilters/Lens.fs \
Shaders/PostFilters/Lens.vs \
Shaders/PostFilters/Lens.program \
Shaders/PostFilters/FXAA.fs \
Shaders/PostFilters/FXAA.vs \
Shaders/PostFilters/FXAA.program \
Shaders/PostFilters/Fog.fs \
Shaders/PostFilters/Fog.vs \
Shaders/PostFilters/Fog.program \
Shaders/PostFilters/PassThrough.fs \
Shaders/PostFilters/PassThrough.vs \
Shaders/PostFilters/PassThrough.program \
Shaders/Shadow/Common.fs \
Shaders/Shadow/Common.vs \
Shaders/Shadow/Map.fs \
Shaders/Shadow/Map.vs \
Shaders/Shadow/MapSoft.fs \
Shaders/Shadow/MapSoft.vs \
Shaders/Shadow/Model.fs \
Shaders/Shadow/Model.vs \
Shaders/Shadow/ModelNull.fs \
Shaders/Shadow/ModelNull.vs \
Shaders/Shadow/MapRadiosityNull.fs \
Shaders/Shadow/MapRadiosityNull.vs \
Shaders/Shadow/MapRadiosity.fs \
Shaders/Shadow/MapRadiosity.vs \
Shaders/ShadowMap/Common.fs \
Shaders/ShadowMap/Common.vs \
Shaders/ShadowMap/Basic.fs \
Shaders/ShadowMap/Basic.vs \
Shaders/DynamicLight/Common.fs \
Shaders/DynamicLight/Common.vs \
Shaders/DynamicLight/MapNull.fs \
Shaders/DynamicLight/MapNull.vs 

install-data-local:
	echo '-- Installing Shaders'
	$(mkinstalldirs) $(Resourcesdir)
	$(mkinstalldirs) $(Resourcesdir)/Shaders
	$(mkinstalldirs) $(Resourcesdir)/Shaders/PostFilters
	$(mkinstalldirs) $(Resourcesdir)/Shaders/Shadow
	$(mkinstalldirs) $(Resourcesdir)/Shaders/ShadowMap
	$(mkinstalldirs) $(Resourcesdir)/Shaders/DynamicLight
	$(mkinstalldirs) $(Resourcesdir)/Shaders/LensFlare
	for i in $(SHADERS); do \
	$(INSTALL_DATA) $(srcdir)/$$i $(Resourcesdir)/$$i; \
	done

EXTRA_DIST = $(SHADERS)

