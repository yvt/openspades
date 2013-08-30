SUBDIRS = Resources

Resourcesdir = $(datadir)/openspades/Resources

bin_PROGRAMS = openspades

AM_CPPFLAGS = -I$(srcdir)/Sources/ENet/include

AM_CPPFLAGS += $(GL_CFLAGS)
AM_CPPFLAGS += $(GLEW_CFLAGS)
AM_CPPFLAGS += $(SDL_CFLAGS)
AM_CPPFLAGS += $(FLTK_CFLAGS)
AM_CPPFLAGS += $(ZLIB_CFLAGS)
# AM_CPPFLAGS += $(PNG_CFLAGS)

AM_CPPFLAGS += -DNDEBUG=1
AM_CPPFLAGS += $(RESOURCES_CFLAGS)

openspades_LDADD = $(GL_LIBS)
openspades_LDADD += $(GLEW_LIBS)
openspades_LDADD += $(SDL_LIBS)
openspades_LDADD += $(FLTK_LIBS)
openspades_LDADD += $(ZLIB_LIBS)
# openspades_LDADD += $(PNG_LIBS)
openspades_LDADD += $(WIN32_LIBS)
openspades_LDADD += $(DL_LIBS)

# pkg-config doesn't seem to work...
# openspades_LDADD += -ljpeg

openspades_SOURCES = \
Sources/Audio/ALDevice.cpp \
Sources/Audio/ALDevice.h \
Sources/Audio/ALFuncs.cpp \
Sources/Audio/ALFuncs.h \
Sources/Audio/AL/al.h \
Sources/Audio/AL/alc.h \
Sources/Audio/AL/alctypes.h \
Sources/Audio/AL/altypes.h \
Sources/Audio/AL/alut.h \
Sources/Audio/AL/efx-creative.h \
Sources/Audio/AL/efx-presets.h \
Sources/Audio/AL/efx.h \
Sources/Audio/AL/MacOSX_OALExtensions.h \
Sources/Client/AsyncRenderer.cpp \
Sources/Client/AsyncRenderer.h \
Sources/Client/CenterMessageView.cpp \
Sources/Client/CenterMessageView.h \
Sources/Client/ChatWindow.cpp \
Sources/Client/ChatWindow.h \
Sources/Client/Client.cpp \
Sources/Client/Client.h \
Sources/Client/Corpse.cpp \
Sources/Client/Corpse.h \
Sources/Client/CTFGameMode.cpp \
Sources/Client/CTFGameMode.h \
Sources/Client/FallingBlock.cpp \
Sources/Client/FallingBlock.h \
Sources/Client/FontData.h \
Sources/Client/GameMap.cpp \
Sources/Client/GameMap.h \
Sources/Client/GameMapWrapper.cpp \
Sources/Client/GameMapWrapper.h \
Sources/Client/Grenade.cpp \
Sources/Client/Grenade.h \
Sources/Client/GunCasing.cpp \
Sources/Client/GunCasing.h \
Sources/Client/HurtRingView.cpp \
Sources/Client/HurtRingView.h \
Sources/Client/IAudioChunk.cpp \
Sources/Client/IAudioChunk.h \
Sources/Client/IAudioDevice.cpp \
Sources/Client/IAudioDevice.h \
Sources/Client/IFont.cpp \
Sources/Client/IFont.h \
Sources/Client/IGameMapListener.cpp \
Sources/Client/IGameMapListener.h \
Sources/Client/IGameMode.cpp \
Sources/Client/IGameMode.h \
Sources/Client/IImage.cpp \
Sources/Client/IImage.h \
Sources/Client/IInputInterface.cpp \
Sources/Client/IInputInterface.h \
Sources/Client/ILocalEntity.cpp \
Sources/Client/ILocalEntity.h \
Sources/Client/IModel.cpp \
Sources/Client/IModel.h \
Sources/Client/IRenderer.cpp \
Sources/Client/IRenderer.h \
Sources/Client/IWorldListener.cpp \
Sources/Client/IWorldListener.h \
Sources/Client/LimboView.cpp \
Sources/Client/LimboView.h \
Sources/Client/MapView.cpp \
Sources/Client/MapView.h \
Sources/Client/NetClient.cpp \
Sources/Client/NetClient.h \
Sources/Client/PaletteView.cpp \
Sources/Client/PaletteView.h \
Sources/Client/ParticleSpriteEntity.cpp \
Sources/Client/ParticleSpriteEntity.h \
Sources/Client/Player.cpp \
Sources/Client/Player.h \
Sources/Client/PhysicsConstants.h \
Sources/Client/Quake3Font.cpp \
Sources/Client/Quake3Font.h \
Sources/Client/SceneDefinition.cpp \
Sources/Client/SceneDefinition.h \
Sources/Client/ScoreboardView.cpp \
Sources/Client/ScoreboardView.h \
Sources/Client/SmokeSpriteEntity.cpp \
Sources/Client/SmokeSpriteEntity.h \
Sources/Client/TCGameMode.cpp \
Sources/Client/TCGameMode.h \
Sources/Client/TCProgressView.cpp \
Sources/Client/TCProgressView.h \
Sources/Client/Weapon.cpp \
Sources/Client/Weapon.h \
Sources/Client/World.cpp \
Sources/Client/World.h \
Sources/Core/AutoLocker.cpp \
Sources/Core/AutoLocker.h \
Sources/Core/Bitmap.cpp \
Sources/Core/Bitmap.h \
Sources/Core/BitmapAtlasGenerator.cpp \
Sources/Core/BitmapAtlasGenerator.h \
Sources/Core/ConcurrentDispatch.cpp \
Sources/Core/ConcurrentDispatch.h \
Sources/Core/DynamicMemoryStream.cpp \
Sources/Core/DynamicMemoryStream.h \
Sources/Core/Debug.cpp \
Sources/Core/Debug.h \
Sources/Core/DeflateStream.cpp \
Sources/Core/DeflateStream.h \
Sources/Core/Deque.cpp \
Sources/Core/Deque.h \
Sources/Core/DirectoryFileSystem.cpp \
Sources/Core/DirectoryFileSystem.h \
Sources/Core/DynamicLibrary.cpp \
Sources/Core/DynamicLibrary.h \
Sources/Core/Exception.cpp \
Sources/Core/Exception.h \
Sources/Core/FileManager.cpp \
Sources/Core/FileManager.h \
Sources/Core/FltkImageReader.cpp \
Sources/Core/IAudioStream.cpp \
Sources/Core/IAudioStream.h \
Sources/Core/IBitmapCodec.cpp \
Sources/Core/IBitmapCodec.h \
Sources/Core/IFileSystem.cpp \
Sources/Core/IFileSystem.h \
Sources/Core/ILockable.cpp \
Sources/Core/ILockable.h \
Sources/Core/IRunnable.cpp \
Sources/Core/IRunnable.h \
Sources/Core/IStream.cpp \
Sources/Core/IStream.h \
Sources/Core/Math.cpp \
Sources/Core/Math.h \
Sources/Core/MemoryStream.cpp \
Sources/Core/MemoryStream.h \
Sources/Core/Mutex.cpp \
Sources/Core/Mutex.h \
Sources/Core/Semaphore.cpp \
Sources/Core/Semaphore.h \
Sources/Core/Settings.cpp \
Sources/Core/Settings.h \
Sources/Core/StdStream.cpp \
Sources/Core/StdStream.h \
Sources/Core/Stopwatch.cpp \
Sources/Core/Stopwatch.h \
Sources/Core/TargaReader.cpp \
Sources/Core/TargaWriter.cpp \
Sources/Core/Thread.cpp \
Sources/Core/Thread.h \
Sources/Core/ThreadLocalStorage.cpp \
Sources/Core/ThreadLocalStorage.h \
Sources/Core/VoxelModel.cpp \
Sources/Core/VoxelModel.h \
Sources/Core/WavAudioStream.cpp \
Sources/Core/WavAudioStream.h \
Sources/Core/ZipFileSystem.cpp \
Sources/Core/ZipFileSystem.h \
Sources/Draw/GLAmbientShadowRenderer.cpp \
Sources/Draw/GLAmbientShadowRenderer.h \
Sources/Draw/GLBasicShadowMapRenderer.cpp \
Sources/Draw/GLBasicShadowMapRenderer.h \
Sources/Draw/GLBloomFilter.cpp \
Sources/Draw/GLBloomFilter.h \
Sources/Draw/GLCameraBlurFilter.cpp \
Sources/Draw/GLCameraBlurFilter.h \
Sources/Draw/GLDynamicLight.cpp \
Sources/Draw/GLDynamicLight.h \
Sources/Draw/GLDynamicLightShader.cpp \
Sources/Draw/GLDynamicLightShader.h \
Sources/Draw/GLFlatMapRenderer.cpp \
Sources/Draw/GLFlatMapRenderer.h \
Sources/Draw/GLFramebufferManager.cpp \
Sources/Draw/GLFramebufferManager.h \
Sources/Draw/GLFXAAFilter.cpp \
Sources/Draw/GLFXAAFilter.h \
Sources/Draw/GLFogFilter.cpp \
Sources/Draw/GLFogFilter.h \
Sources/Draw/GLImage.cpp \
Sources/Draw/GLImage.h \
Sources/Draw/GLImageManager.cpp \
Sources/Draw/GLImageManager.h \
Sources/Draw/GLImageRenderer.cpp \
Sources/Draw/GLImageRenderer.h \
Sources/Draw/GLLensFilter.cpp \
Sources/Draw/GLLensFilter.h \
Sources/Draw/GLLensFlareFilter.cpp \
Sources/Draw/GLLensFlareFilter.h \
Sources/Draw/GLMapChunk.cpp \
Sources/Draw/GLMapChunk.h \
Sources/Draw/GLMapRenderer.cpp \
Sources/Draw/GLMapRenderer.h \
Sources/Draw/GLMapShadowRenderer.cpp \
Sources/Draw/GLMapShadowRenderer.h \
Sources/Draw/GLModel.cpp \
Sources/Draw/GLModel.h \
Sources/Draw/GLModelManager.cpp \
Sources/Draw/GLModelManager.h \
Sources/Draw/GLModelRenderer.cpp \
Sources/Draw/GLModelRenderer.h \
Sources/Draw/GLOptimizedVoxelModel.cpp \
Sources/Draw/GLOptimizedVoxelModel.h \
Sources/Draw/GLProfiler.cpp \
Sources/Draw/GLProfiler.h \
Sources/Draw/GLProgram.cpp \
Sources/Draw/GLProgram.h \
Sources/Draw/GLProgramAttribute.cpp \
Sources/Draw/GLProgramAttribute.h \
Sources/Draw/GLProgramManager.cpp \
Sources/Draw/GLProgramManager.h \
Sources/Draw/GLProgramUniform.cpp \
Sources/Draw/GLProgramUniform.h \
Sources/Draw/GLQuadRenderer.cpp \
Sources/Draw/GLQuadRenderer.h \
Sources/Draw/GLRadiosityRenderer.cpp \
Sources/Draw/GLRadiosityRenderer.h \
Sources/Draw/GLRenderer.cpp \
Sources/Draw/GLRenderer.h \
Sources/Draw/GLShader.cpp \
Sources/Draw/GLShader.h \
Sources/Draw/GLShadowMapShader.cpp \
Sources/Draw/GLShadowMapShader.h \
Sources/Draw/GLShadowShader.cpp \
Sources/Draw/GLShadowShader.h \
Sources/Draw/GLSoftSpriteRenderer.cpp \
Sources/Draw/GLSoftSpriteRenderer.h \
Sources/Draw/GLSpriteRenderer.cpp \
Sources/Draw/GLSpriteRenderer.h \
Sources/Draw/GLLongSpriteRenderer.cpp \
Sources/Draw/GLLongSpriteRenderer.h \
Sources/Draw/GLVoxelModel.cpp \
Sources/Draw/GLVoxelModel.h \
Sources/Draw/GLWaterRenderer.cpp \
Sources/Draw/GLWaterRenderer.h \
Sources/Draw/IGLDevice.cpp \
Sources/Draw/IGLDevice.h \
Sources/Draw/IGLSpriteRenderer.cpp \
Sources/Draw/IGLSpriteRenderer.h \
Sources/Draw/IGLShadowMapRenderer.cpp \
Sources/Draw/IGLShadowMapRenderer.h \
Sources/Gui/Main.cpp \
Sources/Gui/DetailConfigWindow.cpp \
Sources/Gui/DetailConfigWindowHandler.cpp \
Sources/Gui/DetailConfigWindow.h \
Sources/Gui/DetailConfigWindow.fl \
Sources/Gui/DetailConfigTable.cpp \
Sources/Gui/DetailConfigTable.h \
Sources/Gui/MainWindow.cpp \
Sources/Gui/MainWindowHandler.cpp \
Sources/Gui/MainWindow.h \
Sources/Gui/MainWindow.fl \
Sources/Gui/SDLGLDevice.cpp \
Sources/Gui/SDLGLDevice.h \
Sources/Gui/SDLAsyncRunner.cpp \
Sources/Gui/SDLAsyncRunner.h \
Sources/Gui/SDLRunner.cpp \
Sources/Gui/SDLRunner.h \
Sources/ENet/callbacks.c \
Sources/ENet/compress.c \
Sources/ENet/host.c \
Sources/ENet/list.c \
Sources/ENet/packet.c \
Sources/ENet/peer.c \
Sources/ENet/protocol.c \
Sources/ENet/unix.c \
Sources/ENet/win32.c \
Sources/ENet/include/enet/callbacks.h \
Sources/ENet/include/enet/enet.h \
Sources/ENet/include/enet/list.h \
Sources/ENet/include/enet/protocol.h \
Sources/ENet/include/enet/time.h \
Sources/ENet/include/enet/types.h \
Sources/ENet/include/enet/unix.h \
Sources/ENet/include/enet/utility.h \
Sources/ENet/include/enet/win32.h \
Sources/poly2tri/common/shapes.cc \
Sources/poly2tri/common/shapes.h \
Sources/poly2tri/common/utils.h \
Sources/poly2tri/poly2tri.h \
Sources/poly2tri/sweep/advancing_front.cc \
Sources/poly2tri/sweep/advancing_front.h \
Sources/poly2tri/sweep/cdt.cc \
Sources/poly2tri/sweep/cdt.h \
Sources/poly2tri/sweep/sweep_context.cc \
Sources/poly2tri/sweep/sweep_context.h \
Sources/poly2tri/sweep/sweep.cc \
Sources/poly2tri/sweep/sweep.h \
Sources/kiss_fft130/kiss_fft.c \
Sources/kiss_fft130/kiss_fft.h \
Sources/kiss_fft130/_kiss_fft_guts.h \
Sources/binpack2d/binpack2d.hpp \
Sources/unzip/unzip.c \
Sources/unzip/unzip.h \
Sources/unzip/ioapi.c \
Sources/unzip/ioapi.h \
Sources/Imports/SDL.h \
Sources/Imports/OpenGL.h


EXTRA_DIST = \
Sources/ENet/LICENSE \
Sources/ENet/README \
Sources/kiss_fft130/CHANGELOG \
Sources/kiss_fft130/COPYING \
Sources/kiss_fft130/README \
Sources/kiss_fft130/README.simd \
Sources/Gui/AboutText.html \
autogen.sh




