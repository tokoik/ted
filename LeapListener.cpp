#include "LeapListener.h"
#pragma comment (lib, "Leap.lib")

#include "gg.h"
using namespace gg;

#include "SharedMemory.h"

#undef VERBOSE

const std::string fingerNames[] = { "Thumb", "Index", "Middle", "Ring", "Pinky" };
const std::string boneNames[] = { "Metacarpal", "Proximal", "Middle", "Distal" };

const GLfloat scale(0.01f);

void LeapListener::onInit(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Initialized" << std::endl;
#endif
}

void LeapListener::onConnect(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Connected" << std::endl;
#endif
}

void LeapListener::onDisconnect(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  // Note: not dispatched when running in a debugger.
  std::cerr << "Disconnected" << std::endl;
#endif
}

void LeapListener::onExit(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Exited" << std::endl;
#endif
}

void LeapListener::onFrame(const Leap::Controller &controller)
{
  // Get the most recent frame and report some basic information
  const Leap::Frame frame = controller.frame();

#if defined(VERBOSE)
  std::cerr << "Frame id: " << frame.id()
    << ", timestamp: " << frame.timestamp()
    << ", hands: " << frame.hands().count()
    << ", extended fingers: " << frame.fingers().extended().count() << std::endl;
#endif

  for (auto hand : frame.hands())
  {
    // 左手なら 0, 右手なら 1
    const int base(hand.isLeft() ? 0 : 1);

    // 手のひらの位置
    const Leap::Vector handPos(hand.palmPosition() * scale);

    // 手のひらの法線と方向
    const Leap::Vector normal(hand.palmNormal());
    const Leap::Vector direction(hand.direction());

    // 手のひらの法線と方向に直交するベクトル
    const Leap::Vector tangent(direction.cross(normal));

#if defined(VERBOSE)
    // Get hand position
    std::string handType(hand.isLeft() ? "Left hand" : "Right hand");
    std::cerr << std::string(2, ' ') << handType << ", id: " << hand.id()
      << ", palm position: " << hand.palmPosition() << std::endl;

    // Calculate the hand's pitch, roll, and yaw angles
    std::cerr << std::string(2, ' ') << "pitch: " << direction.pitch() * Leap::RAD_TO_DEG << " degrees, "
      << "roll: " << normal.roll() * Leap::RAD_TO_DEG << " degrees, "
      << "yaw: " << direction.yaw() * Leap::RAD_TO_DEG << " degrees" << std::endl;
#endif

    // 手のひらの変換行列を作成する
    const GLfloat mPalm[] =
    {
      -tangent.x, -tangent.z, -tangent.y, 0.0f,
      -direction.x, -direction.z, -direction.y, 0.0f,
      -normal.x, -normal.z, -normal.y, 0.0f,
      -handPos.x, -handPos.z, -handPos.y, 1.0f
    };

    // 変換行列を共有メモリに格納する
    jointMatrix[0 + base] = mPalm;

#if defined(VERBOSE)
    std::cerr << "Hand: " << (base ? "Right" : "Left") << '\n';
    std::cerr << "Position: " << handPos.x << ", " << handPos.y << ", " << handPos.z << '\n';
    std::cerr << "Direction: " << direction.x << ", " << direction.y << ", " << direction.z << '\n';
    std::cerr << "Normal: " << normal.x << ", " << normal.y << ", " << normal.z << '\n';
#endif

#if defined(VERBOSE)
    // Get the Arm bone
    Leap::Arm arm = hand.arm();
    std::cerr << std::string(2, ' ') << "Arm direction: " << arm.direction()
      << " wrist position: " << arm.wristPosition()
      << " elbow position: " << arm.elbowPosition() << std::endl;
#endif

    // 手首の位置と腕の方向ベクトルを求める
    Leap::Vector wristPosition(hand.arm().wristPosition() * scale);
    Leap::Vector armDirection((wristPosition - hand.arm().elbowPosition() * scale).normalized());

    // 腕の方向ベクトルと直交するベクトルを求める
    const Leap::Vector armTangent(armDirection.cross(normal).normalized());

    // 腕の法線ベクトルを求める
    const Leap::Vector armNormal(armDirection.cross(armTangent));

    // 変換行列を作成する
    const GLfloat mWrist[] =
    {
      -armTangent.x, -armTangent.z, -armTangent.y, 0.0f,
      -armNormal.x, -armNormal.z, -armNormal.y, 0.0f,
      -armDirection.x, -armDirection.z, -armDirection.y, 0.0f,
      -wristPosition.x, -wristPosition.z, -wristPosition.y, 1.0f
    };

    // 手首の変換行列を共有メモリに格納する
    jointMatrix[2 + base] = mWrist;

    for (auto finger : hand.fingers())
    {
#if defined(VERBOSE)
      std::cerr << std::string(4, ' ') << fingerNames[finger.type()]
        << " finger, id: " << finger.id()
        << ", length: " << finger.length()
        << "mm, width: " << finger.width() << std::endl;
#endif

      // Get finger bones
      for (int b = 0; b < 4; ++b)
      {
        // 指の種類と骨格を取り出す
        const Leap::Bone::Type boneType(static_cast<Leap::Bone::Type>(b));
        const Leap::Bone bone(finger.bone(boneType));

#if defined(VERBOSE)
        std::cerr << b << std::string(6, ' ') << boneNames[boneType]
          << " bone, start: " << bone.prevJoint()
          << ", end: " << bone.nextJoint()
          << ", direction: " << bone.direction() << std::endl;
#endif

        // 指先の位置と指の方向ベクトルを求める
        const Leap::Vector bonePosition(bone.nextJoint() * scale);
        const Leap::Vector boneDirection((bonePosition - bone.prevJoint() * scale).normalized());

        // 指の手のひらの法線方向のベクトルを求める
        const Leap::Vector boneNormal(boneDirection.cross(tangent).normalized());

        // 指の手のひらの接線方向のベクトルを求める
        const Leap::Vector boneTangent(boneNormal.cross(boneDirection));

        // 変換行列を作成する
        const GLfloat mFinger[] =
        {
          -boneTangent.x, -boneTangent.z, -boneTangent.y, 0.0f,
          -boneNormal.x, -boneNormal.z, -boneNormal.y, 0.0f,
          -boneDirection.x, -boneDirection.z, -boneDirection.y, 0.0f,
          -bonePosition.x, -bonePosition.z, -bonePosition.y, 1.0f
        };

        // 変換行列を共有メモリに格納する
        jointMatrix[4 + finger.type() * 8 + b * 2 + base] = mFinger;
      }
    }
  }

  // 変換行列を共有メモリに格納する
  matrix->store(jointMatrix.data(), begin, static_cast<unsigned int>(jointMatrix.size()));

#if defined(VERBOSE)
  if (!frame.hands().isEmpty()) std::cerr << std::endl;
#endif
}

void LeapListener::onFocusGained(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Focus Gained" << std::endl;
#endif
}

void LeapListener::onFocusLost(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Focus Lost" << std::endl;
#endif
}

void LeapListener::onDeviceChange(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Device Changed" << std::endl;
  const Leap::DeviceList devices = controller.devices();

  for (int i = 0; i < devices.count(); ++i)
  {
    std::cerr << "id: " << devices[i].toString() << std::endl;
    std::cerr << "  isStreaming: " << (devices[i].isStreaming() ? "true" : "false") << std::endl;
    std::cerr << "  isSmudged:" << (devices[i].isSmudged() ? "true" : "false") << std::endl;
    std::cerr << "  isLightingBad:" << (devices[i].isLightingBad() ? "true" : "false") << std::endl;
  }
#endif
}

void LeapListener::onServiceConnect(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Service Connected" << std::endl;
#endif
}

void LeapListener::onServiceDisconnect(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Service Disconnected" << std::endl;
#endif
}

void LeapListener::onServiceChange(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Service Changed" << std::endl;
#endif
}

void LeapListener::onDeviceFailure(const Leap::Controller &controller)
{
#if defined(VERBOSE)
  std::cerr << "Device Error" << std::endl;
  const Leap::FailedDeviceList devices = controller.failedDevices();

  for (Leap::FailedDeviceList::const_iterator dl = devices.begin(); dl != devices.end(); ++dl)
  {
    const Leap::FailedDevice device = *dl;
    std::cerr << "  PNP ID:" << device.pnpId();
    std::cerr << "    Failure type:" << device.failure();
  }
#endif
}

void LeapListener::onLogMessage(const Leap::Controller &controller,
  Leap::MessageSeverity severity, int64_t timestamp, const char *msg)
{
#if defined(VERBOSE)
  switch (severity)
  {
  case Leap::MESSAGE_CRITICAL:
    std::cerr << "[Critical]";
    break;
  case Leap::MESSAGE_WARNING:
    std::cerr << "[Warning]";
    break;
  case Leap::MESSAGE_INFORMATION:
    std::cerr << "[Info]";
    break;
  case Leap::MESSAGE_UNKNOWN:
    std::cerr << "[Unknown]";
  }
  std::cerr << "[" << timestamp << "] ";
  std::cerr << msg << std::endl;
#endif
}

// コンストラクタ
LeapListener::LeapListener(SharedMemory *matrix)
  : matrix(matrix)
  , begin(matrix->getUsed())
{
  for (auto &m : jointMatrix) matrix->push(m = ggIdentity());
}

// デストラクタ
LeapListener::~LeapListener()
{
}
