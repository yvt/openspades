#include "Player.h"
#ifdef WIN32
#include <windef.h>
#endif

struct MumbleLinkedMemory {
  uint32_t uiVersion;
  uint32_t uiTick;
  float fAvatarPosition[3];
  float fAvatarFront[3];
  float fAvatarTop[3];
  wchar_t name[256];
  float fCameraPosition[3];
  float fCameraFront[3];
  float fCameraTop[3];
  wchar_t identity[256];
  uint32_t context_len;
  unsigned char context[256];
  wchar_t description[2048];
};

class MumbleLink {
  const float metre_per_block;
  MumbleLinkedMemory *mumbleLinkedMemory;
#ifdef _WIN32
  HANDLE obj;
#else
  int fd;
#endif

  void set_mumble_vector3(float mumble_vec[3], const spades::Vector3 &vec);

public:
  MumbleLink();
  ~MumbleLink();

  bool init();
  void setContext(const std::string &context);
  void setIdentity(const std::string &identity);
  void update(spades::client::Player *player);
};
