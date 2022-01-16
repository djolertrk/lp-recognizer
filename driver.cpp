/* This is main driver of the lp-recognizer
   tool. The tool is used to automatically
   recognize license plates and it uses
   the https://github.com/DoubangoTelecom/ultimateALPR-SDK.

   Copyright (c) 2022 Djordje Todorovic <djolertrk@gmail.com>.
*/

#include <iostream>
#include <string>

#include "alpr_utils.h"
#include "ultimateALPR-SDK-API-PUBLIC.h"

using namespace ultimateAlprSdk;

// Configuration for ANPR deep learning engine.
static const char* __jsonConfig =
    "{"
    "\"debug_level\": \"warn\","
    ""
    "\"gpgpu_enabled\": true,"
    ""
    "\"detect_roi\": [240, 1040, 60, 660],"
    "\"detect_minscore\": 0.1,"
    ""
    "\"recogn_minscore\": 0.3,"
    "\"recogn_score_type\": \"min\""
    "";

// Asset manager used on Android to files in "assets" folder.
#if ULTALPR_SDK_OS_ANDROID
#define ASSET_MGR_PARAM() __sdk_android_assetmgr,
#else
#define ASSET_MGR_PARAM()
#endif /* ULTALPR_SDK_OS_ANDROID */

static void printUsage(const std::string& message = "") {
  if (!message.empty()) {
    ULTALPR_SDK_PRINT_ERROR("%s", message.c_str());
  }

  ULTALPR_SDK_PRINT_INFO(
      "\n**********************************************************************"
      "**********\n"
      "lp-recognizer\n"
      "\t--image <path-to-image-with-to-recognize> \n"
      "\t[--assets <path-to-assets-folder>] \n"
      "************************************************************************"
      "********\n");
}

int main(int argc, char* argv[]) {
  std::cout << "lp-recognizer\n";

  UltAlprSdkResult result;

  std::string assetsFolder, licenseTokenData, licenseTokenFile;
  bool isRectificationEnabled = false;
  bool isCarNoPlateDetectEnabled = false;
  bool isIENVEnabled =
#if defined(__arm__) || defined(__thumb__) || defined(__TARGET_ARCH_ARM) || \
    defined(__TARGET_ARCH_THUMB) || defined(_ARM) || defined(_M_ARM) ||     \
    defined(_M_ARMT) || defined(__arm) || defined(__aarch64__)
      false;
#else  // x86-64
      true;
#endif
  bool isOpenVinoEnabled = true;
  bool isKlassLPCI_Enabled = false;
  bool isKlassVCR_Enabled = false;
  bool isKlassVMMR_Enabled = false;
  bool isKlassVBSR_Enabled = false;
  std::string charset = "latin";
  std::string openvinoDevice = "CPU";
  std::string pathFileImage;

  // Parsing args.
  std::map<std::string, std::string> args;
  if (!alprParseArgs(argc, argv, args)) {
    printUsage();
    return -1;
  }

  if (args.find("--image") == args.end()) {
    printUsage("--image required");
    return -1;
  }
  pathFileImage = args["--image"];

  if (args.find("--assets") == args.end()) {
    printUsage("--assets required");
    return -1;
  }
  assetsFolder = args["--assets"];

  // Update JSON config
  std::string jsonConfig = __jsonConfig;

  //
  if (!assetsFolder.empty()) {
    jsonConfig += std::string(",\"assets_folder\": \"") + assetsFolder +
                  std::string("\"");
  }

  if (!openvinoDevice.empty()) {
    jsonConfig += std::string(",\"openvino_device\": \"") + openvinoDevice +
                  std::string("\"");
  }

  // TODO: Get more parameters.

  // The end of the config.
  jsonConfig += "}";

  // Decode the image.
  AlprFile fileImage;
  if (!alprDecodeFile(pathFileImage, fileImage)) {
    ULTALPR_SDK_PRINT_INFO("Failed to read image file: %s",
                           pathFileImage.c_str());
    return -1;
  }

  // Init the recognizer.
  ULTALPR_SDK_PRINT_INFO("Starting lp-recognizer...");
  ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::init(
                          ASSET_MGR_PARAM() jsonConfig.c_str(), nullptr))
                         .isOK());

  // The recognizing logic.
  ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::process(
                          fileImage.type, fileImage.uncompressedData,
                          fileImage.width, fileImage.height))
                         .isOK());
  ULTALPR_SDK_PRINT_INFO("Processing done.");

  // Print latest result.
  if (result.json()) {
    const std::string& json_ = result.json();
    if (!json_.empty()) {
      printf("%s", json_.c_str());
    }
  }

  // Cleun up the lp-recognizer.
  ULTALPR_SDK_PRINT_INFO("Ending lp-recognizer...");
  ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::deInit()).isOK());

  std::cout << "\n";

  return 0;
}
