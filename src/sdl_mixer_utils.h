#pragma once

#include <stdexcept>

#include <SDL.h>
#include <SDL_mixer.h>
#include <spdlog/spdlog.h>

#include "sdl_utils.h"
#include "utils.h"

namespace sdl {

inline void VerifyMixerVersion() {
  SDL_version compiled_mixer;
  SDL_MIXER_VERSION(&compiled_mixer);
  const SDL_version* linked_mixer = Mix_Linked_Version();

  spdlog::info("Compiled against SDL_mixer version {}.{}.{}",
               compiled_mixer.major, compiled_mixer.minor,
               compiled_mixer.patch);
  spdlog::info("Linking against SDL_mixer version {}.{}.{}",
               linked_mixer->major, linked_mixer->minor, linked_mixer->patch);
}

struct SDLMixerContext {
  SDLContext sdl_ctx;
  utils::DestructorCallback sdl_mixer_quit;
};

inline SDLMixerContext InitMix() {
  VerifyVersion();
  VerifyMixerVersion();

  auto sdl_ctx = sdl::Init(SDL_INIT_AUDIO);
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
    utils::Throw<std::runtime_error>("Error initializing SDL_mixer: {}",
                                     Mix_GetError());
  }
  auto sdl_mixer_quit = utils::DestructorCallback([] { Mix_Quit(); });
  return SDLMixerContext{std::move(sdl_ctx), std::move(sdl_mixer_quit)};
}

using MixMusicPtr = std::unique_ptr<Mix_Music, decltype(&Mix_FreeMusic)>;

inline MixMusicPtr LoadMusic(const std::filesystem::path& filename) {
  return MixMusicPtr{Mix_LoadMUS(filename.c_str()), &Mix_FreeMusic};
}

inline MixMusicPtr EmptyMixMusicPtr() {
  return MixMusicPtr{nullptr, &Mix_FreeMusic};
}

}  // namespace sdl