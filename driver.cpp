/* This is main driver of the lp-recognizer
   tool. The tool is used to automatically
   recognize license plates and it uses
   the https://github.com/DoubangoTelecom/ultimateALPR-SDK.

   Copyright (c) 2022 Djordje Todorovic <djolertrk@gmail.com>.
*/

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <fstream>

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
    ;

// Asset manager used on Android to files in "assets" folder.
#if ULTALPR_SDK_OS_ANDROID
#define ASSET_MGR_PARAM() __sdk_android_assetmgr,
#else
#define ASSET_MGR_PARAM()
#endif /* ULTALPR_SDK_OS_ANDROID */

// Support for multithreading.
pthread_t tid;
pthread_t clientthreads[100];

// Port to listen to.
static constexpr unsigned PORT = 8989;

struct AlprInfoServerClient {
  char message[1024];
  int socketFD;
  AlprInfoServerClient(int s, char msg[]) {
    strcpy(message, msg);
    socketFD = s;
  }
};

// Server impl.
void* request_handler(void* param) {
  struct AlprInfoServerClient* msgFromClient =
      (struct AlprInfoServerClient*)param;

  std::string pathFileImage(msgFromClient->message);
  UltAlprSdkResult result;
  // Decode the image.
  AlprFile fileImage;
  if (!alprDecodeFile(pathFileImage, fileImage)) {
    ULTALPR_SDK_PRINT_INFO("Failed to read image file: %s",
                           pathFileImage.c_str());
    std::cout << "\n";
    // Send the result to client.
    const char* errMsgForClient = "Faild to process the img.\n";
    send(msgFromClient->socketFD, errMsgForClient, strlen(errMsgForClient), 0);
    pthread_exit(NULL);
    return NULL;
  }

  // The recognizing logic.
  result = UltAlprSdkEngine::process(
                          fileImage.type, fileImage.uncompressedData,
                          fileImage.width, fileImage.height);
   if(!result.isOK()){
    send(msgFromClient->socketFD, result.phrase() , strlen(result.phrase() ), 0);
    pthread_exit(NULL);
    return NULL;
  }
  ULTALPR_SDK_PRINT_INFO("Processing done.");

  // Print latest result.
  if (result.json()) {
    const std::string& json_ = result.json();
    if (!json_.empty()) {
      printf("%s\n", json_.c_str());
    }
    // Send the result to client.
    send(msgFromClient->socketFD, json_.c_str(), strlen(json_.c_str()), 0);
  }
  printf("\n");
  pthread_exit(NULL);
}

static void printUsage(const std::string& message = "") {
  if (!message.empty()) {
    ULTALPR_SDK_PRINT_ERROR("%s", message.c_str());
  }

  ULTALPR_SDK_PRINT_INFO(
      "\n**********************************************************************"
      "**********\n"
      "lp-recognizer\n"
      "\t[--assets <path-to-assets-folder>] \n"
      "\t[--config <config-file>] \n"
      "\t[--port <port-num>] \n"
      "************************************************************************"
      "********\n");
}

static void internalToolDump(const std::string& message = "") {
  std::cout << "*[lp-recognizer]: " << message << "\n";
}

int main(int argc, char* argv[]) {
  internalToolDump("*** lp-recognizer ***");

  // These variables handle ultAlp library.
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

  // Parsing args.
  std::map<std::string, std::string> args;
  if (!alprParseArgs(argc, argv, args)) {
    printUsage();
    return -1;
  }

  if (args.find("--assets") == args.end()) {
    printUsage("--assets required");
    return -1;
  }
  assetsFolder = args["--assets"];

  // Update JSON config.
  std::string jsonConfig = __jsonConfig;

  if (args.find("--config") != args.end()) {
   std::string configFile = args["--config"];
   std::fstream f(configFile, std::fstream::in);
   getline(f, jsonConfig, '\0');
   jsonConfig.pop_back();
  }

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

  // These variables handle the server implementation.
  int serverSocket, newSocket;
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;

  socklen_t addr_size;

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(PORT);

  // Bind the socket to the
  // address and port number.
  if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) ==
      -1) {
    internalToolDump(
        "Error: Couldn't create a socket with a given address:port.");
    return -1;
  }

  // Init the recognizer.
  ULTALPR_SDK_PRINT_INFO("Starting lp-recognizer...");
  ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::init(
                          ASSET_MGR_PARAM() jsonConfig.c_str(), nullptr))
                         .isOK());

  // Listen on the socket,
  // with 50 max connection
  // requests queued.
  if (listen(serverSocket, 50) == 0)
    internalToolDump("Server is up and running...");
  else {
    internalToolDump("Error with listening on the specified port.");
    // Cleun up the lp-recognizer.
    ULTALPR_SDK_PRINT_INFO("Ending lp-recognizer...");
    ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::deInit()).isOK());
    std::cout << "\n";
    return -1;
  }

  pthread_t tid[60];
  char buffer[1024] = {0};

  int i = 0;
  while (true) {
    addr_size = sizeof(serverStorage);

    // Extract the first
    // connection in the queue
    newSocket =
        accept(serverSocket, (struct sockaddr*)&serverStorage, &addr_size);

    ssize_t size;
    if ((size = recv(newSocket, buffer, 1024, MSG_DONTWAIT)) == -1) {
      char errMsg[200];
      sprintf(errMsg, "%s", strerror(errno));
      internalToolDump(errMsg);
      std::cout << "\n";
      send(newSocket, errMsg, strlen(errMsg), 0);
      sleep(3);
      continue;
    }

    if (size == 0) {
      internalToolDump("Nothing to read for the client. Done.\n");
      const char* errMsgForClient = "Nothing to read\n";
      send(newSocket, errMsgForClient, strlen(errMsgForClient), 0);
      fflush(stdout);
      continue;
    }

    struct AlprInfoServerClient clInfo(newSocket, buffer);

    if (pthread_create(&clientthreads[i++], NULL, request_handler, &clInfo) !=
        0) {
      i--;
      const char* errMsgForClient = "Failed to create thread\n";
      send(newSocket, errMsgForClient, strlen(errMsgForClient), 0);
      continue;
     }

    if (i >= 50) {
      i = 0;
      while (i < 50) {
        // Suspend execution of
        // the calling thread
        // until the target
        // thread terminates.
        pthread_join(clientthreads[i++], NULL);
      }
      i = 0;
    }
  }

  fflush(stdout);

  // Cleun up the lp-recognizer.
  ULTALPR_SDK_PRINT_INFO("Ending lp-recognizer...");
  ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::deInit()).isOK());

  std::cout << "\n";

  return 0;
}
