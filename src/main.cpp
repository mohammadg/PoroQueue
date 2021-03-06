/*
* Name: PoroQueue
* Description: Auto Queue tool for popular league of legends game.
* Author: Mohammad Ghasembeigi
* Email: me@mohammadg.com
*/

//OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//C++
#include <map>
#include <vector>
#include <string>
#include <iostream>

//Other
#include "definitions.h"
#include "resource.h"

#include <Windows.h>
#include <stdio.h>
#include <thread>
#include <shellapi.h>
#include <vld.h>
#include <tchar.h>
#include <ole2.h>
#include <olectl.h>
#include <process.h>
#include <Tlhelp32.h>
#include <ctime>

//Boost
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>


/* Namespaces */
using namespace std;
using namespace cv;
using boost::property_tree::ptree;
using namespace boost::filesystem;


/* Function Declarations */
// Image Matching
bool hwnd2image(HWND hwnd, LPCSTR fname);
bool saveBitmap(LPCSTR filename, HBITMAP bmp, HPALETTE pal);

double matchTem(HWND handle, Mat *templ); //Match image with provided template and return min value
double matchTwoTem(HWND handle, Mat *firstTem, Mat *secondTem, int *targetMatch); //Same as above but compares two images

                                                                                  //Program flow
void clickMain(void); // Main clicking process function, runs in a seperate thread
void ClickTarget(HWND hwnd, int x, int y, int cols, int rows, int xoffset, int yoffset); // Clicks center of matched image

                                                                                         // Checking and failsafes
void resetCycle(void);
void toggleisActive(void);
void readSettings(void);
void checkImages(void);
void initTemplateNames(void);
void quitWithErrorMessage(const string &error, const string &errorType, unsigned short code, int iconType);

// Helper/Misc functions
bool getLoLRadsDir(HKEY root, const string &regKey, string &location);
bool getLoLExe(const string &LOLDir, string &location);
void TypeInfo(string info);
void breakableSleep(int TIME);
void SetForegroundWindowInternal(HWND hWnd);
void updateTrayWindow(void);
void InitNotifyIconData(const string tip, bool showToolTip);
void updateTrayStatus(const string status, bool showToolTip);
void addLogEntry(unsigned int header, string entry);



/* Global Variables */
int curWindow = NO_WINDOW; //0 = none, 1 = client and 2 = game

                           //Coordinates
int xCord = NULL;
int yCord = NULL;

//Screen resolution
int res_width = 0;
int res_height = 0;
RECT game_res;

//INI Configuration Variables
std::vector<std::string> championList;
size_t championListPos = 0;

std::vector<std::string> ARAMRerollList;
const int ARAMHighlightXPixelDifference = 100;
const int ARAMHighlightYPixelDifference = 50;

std::string TextToCall;
std::string PathToLoLFolder;
int ClientType = NULL;
std::string GameResolution = "Unknown";
int Type = NULL;
int COOPvsAIDifficulty = NULL;
UINT PauseUnpauseHotkey = VK_F1;
UINT ResetHotkey = VK_F2;
bool LoggingEnabled = false;

double DESPERATE_TOLERANCE = 0.25; //For desperate matches
double GOOD_TOLERANCE = 0.1; //For general matching, where you have non distinct images
double STRICT_TOLERANCE = 0.05; //stricter for unique images
double VERY_STRICT_TOLERANCE = 0.01; //need perfect match

                                     //League of Legends literal strings
LPCTSTR clientName = TEXT("PVP.net Client");
LPCTSTR gameName = TEXT("League of Legends (TM) Client");
string gameProcName = "League of Legends.exe";
string winErrorProcName = "WerFault.exe";

HMENU hMenu, hChangeIndex;
UINT WM_TASKBARCREATED = 0;
NOTIFYICONDATA nid;
HWND hwnd;

//Hotkeys for program control
bool isActive = false;
bool breakNextCycle = false;
bool breakSleep = false;
bool terminateProgram = false;

//Template Matching Variables
Mat frame, result;
int match_method = CV_TM_SQDIFF_NORMED; //default for client = CV_TM_SQDIFF_NORMED, default for game = CV_TM_SQDIFF_NORMED

                                        //Template Variables
int currentStageIndex = 0; //indexes image match process
string extensionType = ".jpg";
string image_folder = "imgTemplates\\";
string champion_icon_folder = "champions\\";
string tempPath;
char wchPath[MAX_PATH];


//TemplateNames for each operation mode
std::map<std::string, std::string> templateNames; //contains full mapping of templateShortNames to templateNiceNames
std::vector<std::string> templateMode; //Contains the actual template names for the given game type (queue type)

                                       //Error logging file handler
std::ofstream logFile;
int cycleNumber = 1;


//Callback handler WndProc
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (message == WM_TASKBARCREATED && !IsWindowVisible(hwnd)) {
    updateTrayWindow();
  }

  switch (message) {
  case WM_COMMAND:
  {
    if (lParam == WM_RBUTTONDOWN) { //Show menu
                                    // Get current mouse position.
      POINT curPoint;
      GetCursorPos(&curPoint);

      // Set to top to gain control
      SetForegroundWindow(hwnd);

      // TrackPopupMenu blocks the app until TrackPopupMenu returns
      UINT clicked = TrackPopupMenu(
        hMenu,
        NULL,
        curPoint.x,
        curPoint.y,
        0,
        hwnd,
        NULL
        );
    }

    //Switch params
    switch (LOWORD(wParam))
    {
    case ID_TRAY_EXIT_CONTEXT_MENU_ITEM:
      SendMessage(hwnd, WM_DESTROY, 0, 0);
      PostQuitMessage(0);
      break;
    case ID_TRAY_CHANGE_TO_0:
    case ID_TRAY_CHANGE_TO_1:
    case ID_TRAY_CHANGE_TO_2:
    case ID_TRAY_CHANGE_TO_3:
    case ID_TRAY_CHANGE_TO_4:
    case ID_TRAY_CHANGE_TO_5:
    case ID_TRAY_CHANGE_TO_6:
    case ID_TRAY_CHANGE_TO_7:
    case ID_TRAY_CHANGE_TO_8:
    case ID_TRAY_CHANGE_TO_9:
    case ID_TRAY_CHANGE_TO_10:
    case ID_TRAY_CHANGE_TO_11:
    case ID_TRAY_CHANGE_TO_12:
    case ID_TRAY_CHANGE_TO_13:
    case ID_TRAY_CHANGE_TO_14:
    case ID_TRAY_CHANGE_TO_15:
    case ID_TRAY_CHANGE_TO_16:
      currentStageIndex = LOWORD(wParam) - ID_TRAY_CHANGE_TO_0;
      updateTrayStatus(TIP_PREFIX "Stage Changed (" + templateNames[templateMode[currentStageIndex]] + ")", true);
      break;

    case ID_TRAY_TOGGLE_PAUSE:
      toggleisActive(); //toggle status
      Sleep(500); //sleep for a bit
      break;

    case ID_TRAY_FINISH_LAST_CYCLE:
      isActive = true; //resume program
      breakNextCycle = true;
      break;

    case ID_TRAY_RESET_CYCLE:
    {
      string tip = TIP_PREFIX "Paused (Cycle Reset)";
      updateTrayStatus(tip, true);
      resetCycle(); //reset cycle
      Sleep(500);
    }
    break;

    case ID_TRAY_DONATE:
      ShellExecuteA(NULL, "open", DONATE_URL, NULL, NULL, SW_SHOWNORMAL); //Open donation url
      break;

    }
  }
  break;

  case WM_CREATE:
  {
    MENUITEMINFO separatorBtn = { 0 };
    separatorBtn.cbSize = sizeof(MENUITEMINFO);
    separatorBtn.fMask = MIIM_FTYPE;
    separatorBtn.fType = MFT_SEPARATOR;

    hMenu = CreatePopupMenu();

    if (hMenu) { //Main menu
      hChangeIndex = CreatePopupMenu();
      if (hChangeIndex) { //Change index menu

                          //Get the type selected via config
        std::string typeNiceName;
        if (Type == TYPE_COOP) {
          typeNiceName = "COOP";
          if (COOPvsAIDifficulty == DIFFICULTY_BEGINNER) {
            typeNiceName.append(" (Beginner)");
          }
          else if (COOPvsAIDifficulty == DIFFICULTY_INTERMEDIATE) {
            typeNiceName.append(" (Intermediate)");
          }
        }
        else if (Type == TYPE_NORMAL) {
          typeNiceName = "Normal";
        }

        InsertMenu(hChangeIndex, -1, MF_GRAYED, templateMode.size() + 1, TEXT(("Selected Mode: " + typeNiceName).c_str()));

        for (size_t i = 0; i < templateMode.size(); i++) { //Loop through nice names to get menu items
          InsertMenu(hChangeIndex, -1, MF_STRING, ID_TRAY_CHANGE_TO_0 + i, templateNames[templateMode[i]].c_str());
        }
        AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT_PTR)hChangeIndex, TEXT("Change Stage to:"));
      }

      InsertMenu(hMenu, -1, MF_STRING, ID_TRAY_TOGGLE_PAUSE, TEXT("Pause/Unpause"));
      InsertMenu(hMenu, -1, MF_STRING, ID_TRAY_FINISH_LAST_CYCLE, TEXT("Unpause and Finish Last Cycle"));
      InsertMenu(hMenu, -1, MF_STRING, ID_TRAY_RESET_CYCLE, TEXT("Reset Cycle"));
      InsertMenuItem(hMenu, -1, FALSE, &separatorBtn);
      InsertMenu(hMenu, -1, MF_STRING, ID_TRAY_DONATE, TEXT("Donate"));
      InsertMenuItem(hMenu, -1, FALSE, &separatorBtn);
      InsertMenu(hMenu, -1, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, TEXT("Exit"));
    }

  }
  break;

  case WM_CLOSE: //in our case, close is destory
    SendMessage(hwnd, WM_DESTROY, 0, 0);
    break;

  case WM_DESTROY:
    terminateProgram = true;
    Shell_NotifyIcon(NIM_DELETE, &nid);
    PostQuitMessage(0);
    break;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

//Main - Read in settings and initilise tool + thread
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR args, int iCmdShow) {

  //Allow only one instance
  CreateMutex(0, FALSE, TEXT("Local\\$myprogram$")); // try to create a named mutex
  if (GetLastError() == ERROR_ALREADY_EXISTS) // did the mutex already exist?
    quitWithErrorMessage("Program is already running.", "Startup Error", ERR_RUNNING, MB_ICONERROR);

  //Read Configuration Files and set required Global Varis
  readSettings();

  //Change templateNames based on settings
  initTemplateNames();

  //Check images are ready
  checkImages();

  //Set Temp Path
  if (GetTempPath(MAX_PATH, wchPath)) {
    tempPath = wchPath;

    char buffer[L_tmpnam];
    tmpnam(buffer);
    char *pbuffer = buffer;

    tempPath += (pbuffer + extensionType);
  }
  else { //Use local file instead
    char buffer[L_tmpnam];
    tmpnam(buffer);
    char *pbuffer = buffer;

    tempPath = (pbuffer + extensionType);
  }

  //Set resolution
  res_width = GetSystemMetrics(SM_CXSCREEN);
  res_height = GetSystemMetrics(SM_CYSCREEN);

  //Log entry
  if (LoggingEnabled) {
    addLogEntry(HEADER_PREFIX, "User desktop resolution: " + to_string(res_width) + 'x' + to_string(res_height));
    addLogEntry(HEADER_PREFIX, "User game resolution: " + GameResolution + "\n");

    addLogEntry(NORMAL_PREFIX, "Entering Cycle: " + to_string(cycleNumber) + "\n");
  }

  //Ensure focusing can occur
  LockSetForegroundWindow(LSFW_UNLOCK);

  //Start main function in its own seperate thread
  thread clickMain(clickMain);

  // Set Up TaskBar
  TCHAR className[] = TEXT("Poro Icon");

  WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated");
  WNDCLASSEX wnd = { 0 };

  wnd.hInstance = hInstance;
  wnd.lpszClassName = className;
  wnd.lpfnWndProc = WndProc;
  wnd.cbSize = sizeof(WNDCLASSEX);

  wnd.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
  wnd.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
  wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
  wnd.hbrBackground = (HBRUSH)0;

  if (!RegisterClassEx(&wnd)) {
    quitWithErrorMessage("Class registration failed.", "Fatal Error", ERR_CLASS_REG_FAILED, MB_ICONERROR);
  }

  //Make invisible handle
  hwnd = CreateWindowEx(0, className,
    TEXT(""),
    WS_OVERLAPPEDWINDOW,
    0, 0,
    0, 0,
    NULL, NULL,
    hInstance, NULL);


  if (hwnd == NULL) {
    quitWithErrorMessage("Window Creation Failed.", "Fatal Error", ERR_WINDOW_CREATION_FAILED, MB_ICONERROR);
  }

  //Initilise Icon Data ONCE
  string tip = TIP_PREFIX "Paused (Ready)";
  InitNotifyIconData(tip, true);

  // Add the icon to the system tray once
  Shell_NotifyIcon(NIM_ADD, &nid);

  //Logging
  if (LoggingEnabled) {
    addLogEntry(NORMAL_PREFIX, "Tool icon added to system tray.");
  }

  // Hide the main window
  ShowWindow(hwnd, SW_HIDE);

  MSG msg;
  msg.message = WM_ACTIVATE;
  bool justResumed = false;

  while (WM_QUIT != msg.message && !terminateProgram) {
    //Set control hotkeys
    //Press pause/unpause hotkey
    if (GetAsyncKeyState(PauseUnpauseHotkey)) {
      //Logging
      if (LoggingEnabled) {
        addLogEntry(NORMAL_PREFIX, "HOTKEY: Pause/Unpause pressed.");
      }

      toggleisActive(); //toggle status
      justResumed = true;
      Sleep(500);
    }
    //Press hotkey to reset cycle
    if (GetAsyncKeyState(ResetHotkey)) {
      //Logging
      if (LoggingEnabled) {
        addLogEntry(NORMAL_PREFIX, "HOTKEY: Cycle Reset pressed.");
      }

      string tip = TIP_PREFIX "Paused (Cycle Reset)";
      updateTrayStatus(tip, true);

      resetCycle(); //reset cycle
      Sleep(500);
    }

    //Update tray status for current task
    if (!justResumed && isActive) {
      string tip = TIP_PREFIX "Running! Looking for: " + templateNames[templateMode[currentStageIndex]];
      updateTrayStatus(tip, false);
    }
    else if (isActive) {
      justResumed = true;
    }

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else { //Otherwise pause from capturing message, to lower cpu
      Sleep(250);
    }
  }

  //Destory main thread
  try {
    clickMain.detach();
    clickMain.~thread();
  }
  catch (system_error& e) { //catch system error exception
    addLogEntry(NORMAL_PREFIX, "Caught system_error with code: " + to_string(e.code().value()) + " meaning " + e.what());
  }

  //Logging
  if (LoggingEnabled) {
    //Print end sequence to log
    logFile << "\n\n" << std::string(50, '-') << "\n";

    addLogEntry(NORMAL_PREFIX, "Attempted to remove temporary files with status code: " + to_string(remove(tempPath)));  //Remove temp image
    addLogEntry(NORMAL_PREFIX, "Program terminated successfully");
    addLogEntry(NORMAL_PREFIX, "A total of " + to_string(cycleNumber - 1) + " cycles were completed.");

    logFile.close();
  }

  return EXIT_SUCCESS;
}


// Uses the registry to find location of league rads directory
// Key used is Software\Riot Games\RADS\LocalRootFolder
bool getLoLRadsDir(HKEY root, const string &regKey, string &location) {
  bool isFound = false;
  string loc;

  HKEY hKey;
  char val[MAX_PATH];

  RegOpenKeyEx(root,
    regKey.c_str(),
    NULL, KEY_READ, &hKey);

  DWORD size = MAX_PATH;
  DWORD dwType = REG_SZ; //string key

  RegQueryValueEx(
    hKey,
    TEXT("LocalRootFolder"),
    NULL,
    &dwType,
    (LPBYTE)&val,
    &size
    );

  RegCloseKey(hKey);

  loc = val;

  if (loc.size() != 0 && is_directory(loc)) {
    isFound = true;
    location = loc;
  }

  return isFound;
}

// Finds location of league of legends game executable
bool getLoLExe(const string &LOLDir, string &location) {
  bool isFound = false;
  location = LOLDir;
  location += "\\solutions\\lol_game_client_sln\\releases";

  if (is_directory(location)) { //Find Version Folder
    vector<path> v;

    copy(directory_iterator(location), directory_iterator(), back_inserter(v));
    reverse(v.begin(), v.end()); //reverse entries

    bool regexpResult = false;
    boost::regex e(LOL_VER_REGEX); //eg format 0.0.1.13
    boost::smatch m;

    //Loop through all files looking for directory of correct format, first find will be highest version
    for (size_t i = 0; i < v.size(); i++) {
      regexpResult = regex_search(v.at(i).string(), m, e);

      if (regexpResult && is_directory(v.at(i).string())) { //succesful search
        location = v.at(i).string();
        break;
      }
    }

    v.clear();

    //Check if folder was properly founnd
    if (regexpResult) { //Folder found
      location += "\\deploy\\League of Legends.exe";

      //Check if exe is valid
      if (exists(location)) {
        isFound = true;
      }
    }
  }

  return isFound;
}

// Main clicking process function, runs in a seperate thread
// This is the function that performs all clicking
void clickMain(void) {
  //Keep thread alive indefinitely until program is executed
  int idleCount = 0;

  //Set random number seed
  srand((unsigned int)time(NULL));

  while (true) {
    if (!isActive) {
      Sleep(500);
      continue;
    }
    else if (breakSleep) {
      breakSleep = false;
    }

    //Find game/client handles
    HWND clientHandle = FindWindow(NULL, clientName);
    HWND gameHandle = FindWindow(NULL, gameName);
    HWND handle = NULL;

    //Determine if we are in game or in client
    //Set handle to handle of interest (game takes priority)
    if (gameHandle != NULL) { //Check if game is open
      handle = gameHandle;
      curWindow = GAME_WINDOW;
    }
    else if (clientHandle != NULL) {
      //Check if we are in continue step and game still isnt open
      if (templateMode[currentStageIndex] == STAGE_CONTINUE) {

        //Make testcase: Send MAT
        string tmp = image_folder + "send" + extensionType;
        Mat sendMat = imread(tmp, 1);
        bool dodged = false;

        SetWindowPos(handle, 0, (res_width - CLIENT_WIDTH) / 2, (res_height - CLIENT_HEIGHT) / 2, CLIENT_WIDTH, CLIENT_HEIGHT, SWP_NOZORDER);

        //Loop through until game starts
        while (gameHandle == NULL && !dodged) {

          //Set escape for pause
          if (!isActive) {
            Sleep(500);
            gameHandle = FindWindow(NULL, gameName);
            continue;
          }

          if (!IsIconic(clientHandle)) { //if game not minimized, match, otherwise game starting soon
            double mytolerance = matchTem(clientHandle, &sendMat);

            if (mytolerance > STRICT_TOLERANCE) { //failed match
                                                  //Somebody else has dodged

                                                  //Find particular stage
              auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_ACCEPT);
              int accept_pos_index = it - templateMode.begin();
              currentStageIndex = accept_pos_index; //look for accept again

              breakableSleep(500);
              updateTrayStatus(TIP_PREFIX TIP_OTHER_DODGED, true);
              dodged = true;
              continue;
            }
          }

          breakableSleep(1500); //wait 1.5 seconds

          gameHandle = FindWindow(NULL, gameName);
        }

        //If dodged, look for accept
        if (dodged)
          continue;

        //Game has started, wait for load
        updateTrayStatus(TIP_PREFIX "Running! Looking for: " + templateNames[templateMode[currentStageIndex]], true);
        SetForegroundWindowInternal(gameHandle); //bring game to front
        breakableSleep(10000);
        GetWindowRect(gameHandle, &game_res); //set proper game resolution
        breakableSleep(180000); //3 minute timeout
        continue;

      }
      else { //Otherwise, perform normal client steps
        handle = clientHandle;
        curWindow = CLIENT_WINDOW;
        SetWindowPos(handle, 0, (res_width - CLIENT_WIDTH) / 2, (res_height - CLIENT_HEIGHT) / 2, CLIENT_WIDTH, CLIENT_HEIGHT, SWP_NOZORDER);
      }
    }
    //Neither handle is open, LoL not running?
    else {
      if (idleCount % 60 == 0) {
        updateTrayStatus(TIP_PREFIX "Idle (LoL is currently not running)", true);
      }

      idleCount++;
      curWindow = NO_WINDOW;
      Sleep(1000);
      continue;
    }

    Sleep(125); //brif wait for update

                //Logging seperator
    if (LoggingEnabled) {
      logFile << "\n";
    }

    //Load template image into mat
    string tempTemplate = image_folder + templateMode[currentStageIndex] + extensionType;
    Mat tempMat;

    //Logging
    if (LoggingEnabled) {
      addLogEntry(NORMAL_PREFIX, "Looking for: " + templateNames[templateMode[currentStageIndex]]);
    }

    //Use different color based on window
    if (curWindow == GAME_WINDOW) { //Grayscale in game
      tempMat = imread(tempTemplate, CV_LOAD_IMAGE_GRAYSCALE);
    }
    else { //Color in client
      tempMat = imread(tempTemplate, CV_LOAD_IMAGE_COLOR);
    }

    //Variables
    double tolerance = 0;
    unsigned int SLEEPTIMER = 1000; //Reset sleep timer

                                    /*
                                    * Template matching calls.
                                    * We search for particular stages here and perform different actions based on the particular stage
                                    */

    if (templateMode[currentStageIndex] == STAGE_SERVER_DIALOG)
    {
      //Match template
      tolerance = matchTem(handle, &tempMat);

      if (tolerance == MATCH_FAILED || tolerance > STRICT_TOLERANCE) {
        ++currentStageIndex; //move onto next item
        continue;
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_CROSS)
    {
      //Match template
      tolerance = matchTem(handle, &tempMat);

      if (tolerance == MATCH_FAILED || tolerance > STRICT_TOLERANCE) {
        ++currentStageIndex; //move onto next item
        continue;
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_DODGED)
    {
      //Match template
      tolerance = matchTem(handle, &tempMat);

      if (tolerance == MATCH_FAILED || tolerance > STRICT_TOLERANCE) {
        ++currentStageIndex; //move onto finding accept button        
      }
      else {
        //Find particular stage
        auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_SERVER_DIALOG);
        int start_pos_index = it - templateMode.begin();
        currentStageIndex = start_pos_index; //Go back to pressing play, we have to wait

        updateTrayStatus(TIP_PREFIX TIP_DODGED, true);
      }

      continue;
    }
    else if (templateMode[currentStageIndex] == STAGE_ACCEPT)
    {
      //Get Window position information
      RECT windowRect;
      GetWindowRect(handle, &windowRect);

      //Select image to use as 'dodge check image' based on queue
      //Dodge check image is the image that will be visable (detectable) once we are in champion select
      string tmp;
      if (Type == TYPE_COOP || Type == TYPE_NORMAL)
        tmp = image_folder + "search" + extensionType;
      else if (Type == TYPE_ARAM)
        tmp = image_folder + "howling_abyss_map" + extensionType;

      Mat dodgeCheckMat = imread(tmp, CV_LOAD_IMAGE_COLOR);

      //Call matchtwotem
      int targetMatch = MATCH_FAILED;
      tolerance = matchTwoTem(handle, &tempMat, &dodgeCheckMat, &targetMatch);

      //Keep looking for accept while we have not detected the dodgecheckimage
      bool gotoNextStage = false;

      while (isActive) { //Keep looping unless user pauses tool
        if (targetMatch != MATCH_FAILED && tolerance <= VERY_STRICT_TOLERANCE) {
          if (targetMatch == 1) { //accept button found

            if (LoggingEnabled) {
              addLogEntry(NORMAL_PREFIX, "Clicked accept button.");
            }

            ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
          }
          else if (targetMatch == 2) {  //dodge check image found
            //In Coop and Normal queues, continue with search stage
            if (Type == TYPE_COOP || Type == TYPE_NORMAL) {
              tempMat = dodgeCheckMat; //set tempmat to search mat and go on to perform actions

              //Find particular stage
              auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_SEARCH);
              int search_pos_index = it - templateMode.begin();
              currentStageIndex = search_pos_index;

              if (LoggingEnabled) {
                addLogEntry(NORMAL_PREFIX, "Found search (dodge check image) in accept stage. Continuing current loop for search.");
              }
            }
            //In ARAM queue, move onto next stage
            else if (Type == TYPE_ARAM) {
              //Move onto next stage
              ++currentStageIndex;
              gotoNextStage = true;

              if (LoggingEnabled) {
                addLogEntry(NORMAL_PREFIX, "Found howling abyss map (dodge check image) in accept stage. Moving to next stage.");
              }
            }

            break; //leave while loop
          }
        }

        //Rematch pictures
        breakableSleep(1000);
        targetMatch = MATCH_FAILED; //reset target match

        SetCursorPos(windowRect.left, windowRect.top); //move mouse to remove accept button hover (shine)
        tolerance = matchTwoTem(handle, &tempMat, &dodgeCheckMat, &targetMatch);
      }

      //Check for pauses
      //Check to see if we should goto next stage
      if (!isActive || gotoNextStage)
        continue; //go to next loop
    }
    else if (templateMode[currentStageIndex] == STAGE_CHAMPIONS)
    {
      tolerance = matchTem(handle, &tempMat);

      if (tolerance > STRICT_TOLERANCE) { //failed match
                                          //Somebody else has dodged
                                          //Find particular stage
        auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_ACCEPT);
        int accept_pos_index = it - templateMode.begin();
        currentStageIndex = accept_pos_index; //look for accept againn
        breakableSleep(500);

        //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "Send/Champions match failed, another player has dodged. Looking for accept.");
        }

        continue;
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_LOCKIN)
    {
      //Make lockin grey mat
      string tmp = image_folder + "lockin_grey" + extensionType;
      Mat lockingreyMat = imread(tmp, CV_LOAD_IMAGE_COLOR);

      //Call matchtwotem
      int targetMatch = MATCH_FAILED;
      tolerance = matchTwoTem(handle, &tempMat, &lockingreyMat, &targetMatch);

      if (targetMatch == MATCH_FAILED || tolerance > VERY_STRICT_TOLERANCE) {
        //Somebody else has dodged

        //Find particular stage
        auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_ACCEPT);
        int accept_pos_index = it - templateMode.begin();
        currentStageIndex = accept_pos_index; //look for accept again

                                              //Reset champ preference
        championListPos = 0;

        //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "Lockin match failed, another player has dodged. Looking for accept.");
        }

        breakableSleep(500);
        continue;
      }
      else if (targetMatch == 2) { //grey lockin found
                                   //Someone took our champion
                                   //See if we have more preferences
        ++championListPos;

        if (championListPos < championList.size()) {
          //We have more champions we can select
          //Find particular stage
          auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_SEARCH);
          int search_pos_index = it - templateMode.begin();
          currentStageIndex = search_pos_index;
        }
        //We have to dodge and requeue
        else {
          updateTrayStatus(TIP_PREFIX TIP_DODGING, false);

          //Update index to look for play button again
          //Find particular stage
          auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_SERVER_DIALOG);
          int start_pos_index = it - templateMode.begin();
          currentStageIndex = start_pos_index;

          //Reset champ preference
          championListPos = 0;

          //Wait 2 minutes for you to dodge and land on home screen

          //Logging
          if (LoggingEnabled) {
            addLogEntry(NORMAL_PREFIX, "Waiting 2 minutes for you to dodge and land on home screen.");
          }

          //Wait 2 minutes for you to dodge and land on home screen
          SLEEPTIMER = 120000;
          breakableSleep(SLEEPTIMER);
        }

        continue; //go onto next loop
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_ARAM_REROLL)
    {
      //Check to see if we wish to reroll (is a reroll list provided?)
      if (ARAMRerollList.size() == 0) {
        //No reroll champs specified, move on

        //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "No reroll champions provided, moving to next stage.");
        }

        ++currentStageIndex; //move onto next item
        continue;
      }

      //Make reroll grey mat
      string tmp = image_folder + "reroll_grey" + extensionType;
      Mat rerollgreyMat = imread(tmp, CV_LOAD_IMAGE_COLOR);

      //Call matchtwotem
      int targetMatch = MATCH_FAILED;
      tolerance = matchTwoTem(handle, &tempMat, &rerollgreyMat, &targetMatch);

      if (targetMatch == MATCH_FAILED || tolerance > GOOD_TOLERANCE) {
        //Somebody else has dodged

        //Find particular stage
        auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_ACCEPT);
        int accept_pos_index = it - templateMode.begin();
        currentStageIndex = accept_pos_index; //look for accept again

        //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "Finding reroll button failed, another player has dodged. Looking for accept.");
        }

        breakableSleep(500);
        continue;
      }
      else if (targetMatch == 1) { //Blue reroll (rerolls available)

        //We have champions we wish to reroll on
        bool performReroll = false;

        for (auto it = ARAMRerollList.begin(); it != ARAMRerollList.end(); ++it) {

          addLogEntry(NORMAL_PREFIX, "Inside loop for ARAMrerollList with champion: (" + *it + ")");

          //Check to see if champion is present

          //Make champion icon mat
          string tmp = image_folder + champion_icon_folder + *it + extensionType;
          Mat championMat = imread(tmp, CV_LOAD_IMAGE_COLOR);

          tolerance = matchTem(handle, &championMat);

          //Preserve champion mat coords
          int champXCord = xCord;
          int champYCord = yCord;

          addLogEntry(NORMAL_PREFIX, "Champion Icon COORDS: x(" + to_string(champXCord) + "), y(" + to_string(champYCord) + ")");

          if (tolerance > DESPERATE_TOLERANCE) {
            //This champ icon not found, not currently a champion in ARAM lobby
            addLogEntry(NORMAL_PREFIX, "Champion icon not found, champion is not in ARAM lobby.");
            continue;
          }
          else {
            //Found champion icon
            //Check if this is our champion and not someone elses champion

            //Make aram highlight mat
            string tmp = image_folder + "aram_highlight" + extensionType;
            Mat aramHighlightMat = imread(tmp, CV_LOAD_IMAGE_COLOR);
            tolerance = matchTem(handle, &aramHighlightMat);

            if (tolerance > GOOD_TOLERANCE) {
              //This shouldn't ever happen
              //If it does it means we failed to find ARAM highlight, improve image or increase tolerance
              addLogEntry(NORMAL_PREFIX, "This should never happen. Failed to find ARAM highlight, increase tolerance or improve image.");
              continue;
            }
            else {
              //We found our ARAM highlight coords
              //Check to see if our highlight is close enough to matched champion coords
              if (std::abs(champYCord - yCord) <= ARAMHighlightYPixelDifference
                  &&  std::abs(champXCord - xCord) <= ARAMHighlightXPixelDifference) {
                //This is our champion, we should reroll

                addLogEntry(NORMAL_PREFIX, "Champion is ours, we should reroll.");

                //Leave loop knowing we must reroll
                performReroll = true;
                break;
              }
              else {
                //Not our champion continue
                addLogEntry(NORMAL_PREFIX, "Not our champion, continue searching.");
                continue;
              }
            }

          }
        }

        //Check to see if we will reroll or simply proceed to next stage
        if (performReroll) {
          //Match the reroll template so we can click it
          tolerance = matchTem(handle, &tempMat);

          //Logging
          if (LoggingEnabled) {
            addLogEntry(NORMAL_PREFIX, "Champion found in reroll list, rerolling this champion now.");
          }

          //Click reroll button
          ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
          breakableSleep(2000); //wait for animations

          //move onto reroll stage again in case we should reroll again
          //if we have run out of rerolls or no champion is not in reroll list, we will move onto next stage next loop without rerolling
          //Otherwise we will perform another reroll (2 rerolls in a row)
          continue;

        }
        else {
          //We will not reroll, continue to next stage
          //Logging
          if (LoggingEnabled) {
            addLogEntry(NORMAL_PREFIX, "Champion not in reroll list, moving to next stage.");
          }

          ++currentStageIndex; //move onto next item
          continue;
        }

      }
      else if (targetMatch == 2) { //grey reroll found
                                   //No rerolls are available
                                   //Simply play current champion, move onto next stage

        //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "No rerolls available, moving to next stage.");
        }

        ++currentStageIndex; //move onto next stage
        continue;
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_SEND)
    {
      tolerance = matchTem(handle, &tempMat);

      if (tolerance > STRICT_TOLERANCE) { //failed match
                                          //Somebody else has dodged
                                          //Find particular stage
        auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_ACCEPT);
        int accept_pos_index = it - templateMode.begin();
        currentStageIndex = accept_pos_index; //look for accept againn
        breakableSleep(500);

        //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "Send/Champions match failed, another player has dodged. Looking for accept.");
        }

        continue;
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_CONTINUE)
    {
      //mouse mouse to corner of screen so cursor does not hide continue
      SetCursorPos(game_res.left, game_res.top);

      tolerance = matchTem(handle, &tempMat);
      SLEEPTIMER = 10000;

      if (tolerance < STRICT_TOLERANCE) {
        breakableSleep(5000); //sleep just in case we just found button

                              //Double click it
        ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
        ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);

        system("taskkill /F /T /IM \"WerFault.exe\""); //kill the error message that may appear as lol client is trash
        system("taskkill /F /T /IM \"League of Legends.exe\""); //kill the process, it can sometimes crash on game end so this is needed

                                                                //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "Clicked on Continue button and closing League of Legends.exe");
        }

      }
      else {
        //Sleep for 10 seconds
        breakableSleep(SLEEPTIMER);

        continue; //keep searching for continue
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_TITTLELESSDIAG)
    {
      //Get Window position information
      RECT windowRect;
      GetWindowRect(handle, &windowRect);
      SetCursorPos(windowRect.left, windowRect.top); //Move to top corner in case items block home button

      Sleep(500);

      //Match template
      tolerance = matchTem(handle, &tempMat);

      if (tolerance == MATCH_FAILED || tolerance > STRICT_TOLERANCE) {
        ++currentStageIndex; //move onto next item
        continue;
      }
    }
    else {
      //Match single template
      tolerance = matchTem(handle, &tempMat);
    }

    /*
    * Second Pass - Click
    * This is where we perform clicks and other actions based on above
    */
    if (templateMode[currentStageIndex] == STAGE_SERVER_DIALOG ||
      templateMode[currentStageIndex] == STAGE_CROSS ||
      templateMode[currentStageIndex] == STAGE_TITTLELESSDIAG)
    {
      ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, (tempMat.cols / 2) - 6, 0);
    }
    else if (templateMode[currentStageIndex] == STAGE_PLAY)
    {
      ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
    }
    else if (templateMode[currentStageIndex] == STAGE_SOLOQ)
    {
      SLEEPTIMER = 500;  //don't sleep for long, start looking for accept instantly
      ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
    }
    else if (templateMode[currentStageIndex] == STAGE_SEARCH)
    {
      ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
      std::string callText = "^^^" + championList[championListPos] + "$";
      TypeInfo(callText);
      SLEEPTIMER = 100;
    }
    else if (templateMode[currentStageIndex] == STAGE_CHAMPIONS)
    {
      ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, tempMat.rows);
      SLEEPTIMER = 1500;
    }
    else if (templateMode[currentStageIndex] == STAGE_SEND)
    {
      if (TextToCall.length() > 0) {
        ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, -tempMat.cols, 0);
        TypeInfo(TextToCall);
        ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
      }
      SLEEPTIMER = 1000;
    }
    else if (templateMode[currentStageIndex] == STAGE_CONTINUE)
    {
      breakableSleep(10000); //wait 10 seconds for tool to close

      SLEEPTIMER = 5000;

      while (FindWindow(NULL, gameName) != NULL) {
        breakableSleep(SLEEPTIMER); //Wait for proper game closure, check every 5 seconds

                                    //Logging
        if (LoggingEnabled) {
          addLogEntry(NORMAL_PREFIX, "League of Legends.exe still open, waiting for program closure.");
        }
      }
    }
    else if (templateMode[currentStageIndex] == STAGE_HOME)
    {
      ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0);
      SLEEPTIMER = 3000; //wait for honor/other notifications to popup after click
    }
    else
    {
      ClickTarget(handle, xCord, yCord, tempMat.cols, tempMat.rows, 0, 0); //click center of match
    }



    /*
    * Final step is to increment neccessary counters and perform sleep
    */

    //increment template index
    ++currentStageIndex;

    //Sleep based on SLEEPTIMER
    breakableSleep(SLEEPTIMER);

    //Reset Cycle on end
    if (currentStageIndex == templateMode.size()) {

      //Reset champ preference
      championListPos = 0;

      //Find particular stage
      auto it = std::find(templateMode.begin(), templateMode.end(), STAGE_SERVER_DIALOG);
      int start_pos_index = it - templateMode.begin();
      currentStageIndex = start_pos_index;

      //Logging
      if (LoggingEnabled) {
        addLogEntry(NORMAL_PREFIX, "Entering Cycle: " + to_string(cycleNumber) + "\n");
      }

      ++cycleNumber;

      if (breakNextCycle) { //if break cycle true, pause tool
        isActive = false;
        updateTrayStatus(TIP_PREFIX "Paused (Finished Last Cycle)", true);
        breakNextCycle = false;
      }
    }

  }
}

//Use 'hack' to set foreground window of another process
void SetForegroundWindowInternal(HWND hWnd) {
  if (!::IsWindow(hWnd)) return;

  BYTE keyState[256] = { 0 };
  //to unlock SetForegroundWindow we need to imitate Alt pressing
  if (::GetKeyboardState((LPBYTE)&keyState))
  {
    if (!(keyState[VK_MENU] & 0x80))
    {
      ::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
    }
  }

  ::SetForegroundWindow(hWnd);

  if (::GetKeyboardState((LPBYTE)&keyState))
  {
    if (!(keyState[VK_MENU] & 0x80))
    {
      ::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    }
  }
}

//Match image with provided template and click center of it
double matchTem(HWND handle, Mat *templ) {
  //Logging
  if (LoggingEnabled) {
    addLogEntry(NORMAL_PREFIX, "Matching using single template image (matchTem)");
  }

  //Check that handle still exists and return if it doesnt
  if (handle == NULL) {
    if (LoggingEnabled) {
      addLogEntry(NORMAL_PREFIX, "Match failed (NULL handle), returning code: " + to_string(MATCH_FAILED));
    }

    return MATCH_FAILED;
  }

  ShowWindow(handle, SW_SHOWDEFAULT); //maximize handle
  SetForegroundWindowInternal(handle); //set foreground window

                                       //Get Window position information
  RECT windowRect;
  GetWindowRect(handle, &windowRect);

  if (!hwnd2image(handle, tempPath.c_str())) {
    if (LoggingEnabled) {
      addLogEntry(NORMAL_PREFIX, "Match failed (Handle to Image failed), returning code: " + to_string(MATCH_FAILED));
    }

    return MATCH_FAILED;
  }

  if (curWindow == GAME_WINDOW) { //Grayscale in game
    frame = imread(tempPath, CV_LOAD_IMAGE_GRAYSCALE);
  }
  else { //Color in client
    frame = imread(tempPath, CV_LOAD_IMAGE_COLOR);
  }

  /// Create the result matrix
  int result_cols = frame.cols - (*templ).cols + 1;
  int result_rows = frame.rows - (*templ).rows + 1;

  result.create(result_cols, result_rows, CV_32FC1);

  //Match and normalize
  matchTemplate(frame, (*templ), result, match_method);

  /// Localizing the best match with minMaxLoc
  double minVal; double maxVal; Point minLoc; Point maxLoc;
  Point matchLoc;

  minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
  matchLoc = minLoc; //for CV_TM_SQDIFF_NORMED

  if (minVal == MATCH_FAILED) {
    if (LoggingEnabled) {
      addLogEntry(NORMAL_PREFIX, "Match failed (image), returning code: " + to_string(MATCH_FAILED));
    }

    return MATCH_FAILED;
  }

  //Set x and y of best match, factoring in global position
  xCord = matchLoc.x + windowRect.left;
  yCord = matchLoc.y + windowRect.top;

  if (LoggingEnabled) {
    addLogEntry(NORMAL_PREFIX, "Best match at xCord: " + to_string(xCord) + " and yCord: " + to_string(yCord) + " with tolerance: " + to_string(minVal));
  }

  return minVal;
}

//Match image with provided template and click center of it
double matchTwoTem(HWND handle, Mat *firstTem, Mat *secondTem, int *targetMatch) {
  //Logging
  if (LoggingEnabled) {
    addLogEntry(NORMAL_PREFIX, "Matching using two template images (matchTwoTem)");
  }

  //Check that handle still exists and return if it doesnt
  if (handle == NULL) {
    if (LoggingEnabled) {
      addLogEntry(NORMAL_PREFIX, "Match failed (NULL handle), returning code: " + to_string(MATCH_FAILED));
    }

    return MATCH_FAILED;
  }

  ShowWindow(handle, SW_SHOWDEFAULT); //maximize handle
  SetForegroundWindowInternal(handle); //bring to foreground

                                       //Get Window position information
  RECT windowRect;
  GetWindowRect(handle, &windowRect);

  if (!hwnd2image(handle, tempPath.c_str())) {
    if (LoggingEnabled) {
      addLogEntry(NORMAL_PREFIX, "Match failed (Handle to Image failed), returning code: " + to_string(MATCH_FAILED));
    }

    return MATCH_FAILED;
  }

  frame = imread(tempPath, CV_LOAD_IMAGE_COLOR);

  //Set up variables for results
  double minResult_1 = 0;
  double minResult_2 = 0;
  double minVal = MATCH_FAILED;

  Mat firstresult, secondresult;
  Point matchLoc;

  //First result
  double minVal_res1; double maxVal_res1; Point minLoc_res1; Point maxLoc_res1;
  Point matchLoc_res1;

  //Second result
  double minVal_res2; double maxVal_res2; Point minLoc_res2; Point maxLoc_res2;
  Point matchLoc_res2;

  //First Tem Results
  int result_cols_1 = frame.cols - (*firstTem).cols + 1;
  int result_rows_1 = frame.rows - (*firstTem).rows + 1;

  firstresult.create(result_cols_1, result_rows_1, CV_32FC1);

  //Match First Template
  matchTemplate(frame, (*firstTem), firstresult, match_method);

  /// Localizing the best match with minMaxLoc
  minMaxLoc(firstresult, &minVal_res1, &maxVal_res1, &minLoc_res1, &maxLoc_res1); //remove Mat() mask
  matchLoc_res1 = minLoc_res1; //for CV_TM_SQDIFF_NORMED

  minResult_1 = minVal_res1; //set min val

                             //Second Tem Results
  int result_cols_2 = frame.cols - (*secondTem).cols + 1;
  int result_rows_2 = frame.rows - (*secondTem).rows + 1;

  secondresult.create(result_cols_2, result_rows_2, CV_32FC1);

  //Match Second Template
  matchTemplate(frame, (*secondTem), secondresult, match_method);

  /// Localizing the best match with minMaxLoc
  minMaxLoc(secondresult, &minVal_res2, &maxVal_res2, &minLoc_res2, &maxLoc_res2);
  matchLoc_res2 = minLoc_res2; //for CV_TM_SQDIFF_NORMED

  minResult_2 = minVal_res2; //set min val in array


                             //Set return variables
  if (minResult_1 == MATCH_FAILED && minResult_2 == MATCH_FAILED) { //no match found
    *targetMatch = MATCH_FAILED;

    if (LoggingEnabled) {
      addLogEntry(NORMAL_PREFIX, "Match failed (image), returning code: " + to_string(MATCH_FAILED));
    }

    return minVal;
  }
  else if (minResult_1 <= minResult_2 && minResult_1 != MATCH_FAILED) { //check first
                                                                        //firstresult.copyTo(result);
    matchLoc = matchLoc_res1;
    minVal = minResult_1;
    *targetMatch = 1;
  }
  else if (minResult_2 <= minResult_1 && minResult_2 != MATCH_FAILED) { //check second
                                                                        //secondresult.copyTo(result);
    matchLoc = matchLoc_res2;
    minVal = minResult_2;
    *targetMatch = 2;
  }

  //Set x and y of best match, factoring in global position
  xCord = matchLoc.x + windowRect.left;
  yCord = matchLoc.y + windowRect.top;

  if (LoggingEnabled) {
    addLogEntry(NORMAL_PREFIX, "Best match for template " + to_string(*targetMatch) + " at xCord: " + to_string(xCord) + " and yCord: " + to_string(yCord) + " with tolerance: " + to_string(minVal));
  }

  return minVal;
}


//Set Template names (basically the path our program will take) based on provided settings
//This is also used to set nice names
void initTemplateNames()
{
  templateNames.insert(std::make_pair("server_diag", "Server Notification Dismiss"));
  templateNames.insert(std::make_pair("cross", "Honor Notification Dismiss"));
  templateNames.insert(std::make_pair("play", "Play"));
  templateNames.insert(std::make_pair("coop", "Coop vs AI"));
  templateNames.insert(std::make_pair("classic", "Classic"));
  templateNames.insert(std::make_pair("rift", "Summoners Rift"));
  templateNames.insert(std::make_pair("beginner", "Beginner"));
  templateNames.insert(std::make_pair("intermediate", "Intermediate"));
  templateNames.insert(std::make_pair("server_diag", "Server Notification Dismiss"));
  templateNames.insert(std::make_pair("soloq", "Solo Queue"));
  templateNames.insert(std::make_pair("dodged", "Dodge Check"));
  templateNames.insert(std::make_pair("accept", "Accept"));
  templateNames.insert(std::make_pair("search", "Search"));
  templateNames.insert(std::make_pair("champions", "Select Champion"));
  templateNames.insert(std::make_pair("lockin", "Lockin"));
  templateNames.insert(std::make_pair("send", "Send Chat Message"));
  templateNames.insert(std::make_pair("continue", "Continue (" + GameResolution + ")"));
  templateNames.insert(std::make_pair("titleless_diag", "Dismiss Level 5 Chat Notification"));
  templateNames.insert(std::make_pair("home", "Home"));
  templateNames.insert(std::make_pair("pvp", "PvP"));
  templateNames.insert(std::make_pair("normal_blind", "Normal - Blind Pick"));
  templateNames.insert(std::make_pair("aram", "ARAM"));
  templateNames.insert(std::make_pair("howling_abyss", "Howling Abyss"));
  templateNames.insert(std::make_pair("normal_all_random", "Normal - All Random"));
  templateNames.insert(std::make_pair("reroll", "ARAM Reroll"));

  //Set specific template options based on settings
  if (Type == TYPE_COOP) {
    //Set up template mode
    std::string templateCoop[17] = { "server_diag", "cross", "play", "coop", "classic", "rift", "beginner", "soloq", "dodged", "accept", "search", "champions", "lockin", "send", "continue", "titleless_diag", "home" };

    //1 = beginner, 2 = intermediate
    if (COOPvsAIDifficulty == DIFFICULTY_BEGINNER)
      templateCoop[6] = "beginner";
    else if (COOPvsAIDifficulty == DIFFICULTY_INTERMEDIATE)
      templateCoop[6] = "intermediate";

    templateMode.assign(templateCoop, templateCoop + 17);

  }
  else if (Type == TYPE_NORMAL) {
    //Set up template mode
    std::string templateNormal[17] = { "server_diag", "cross", "play", "pvp", "classic", "rift", "normal_blind", "soloq", "dodged", "accept", "search", "champions", "lockin", "send", "continue", "titleless_diag", "home" };
    templateMode.assign(templateNormal, templateNormal + 17);
  }
  else if (Type == TYPE_ARAM) {
    //Set up template mode
    std::string templateNormal[15] = { "server_diag", "cross", "play", "pvp", "aram", "howling_abyss", "normal_all_random", "soloq", "dodged", "accept", "reroll", "send", "continue", "titleless_diag", "home" };
    templateMode.assign(templateNormal, templateNormal + 15);
  }

}

//Check required images are found
void checkImages(void) {
  std::string missingfile;

  for (size_t i = 0; i < templateMode.size(); i++) {
    std::string imagefile = image_folder + templateMode[i] + extensionType;

    if (!exists(imagefile)) {
      missingfile = templateMode[i] + extensionType;
      break;
    }
  }

  //Check for lockin grey
  std::string imagefile = image_folder + "lockin_grey" + extensionType;
  if (!exists(imagefile))
    missingfile = "lockin_grey" + extensionType;

  //Check for ARAM related images
  imagefile = image_folder + "reroll_grey" + extensionType;
  if (!exists(imagefile))
    missingfile = "reroll_grey" + extensionType;

  imagefile = image_folder + "aram_highlight" + extensionType;
  if (!exists(imagefile))
    missingfile = "aram_highlight" + extensionType;

  imagefile = image_folder + "howling_abyss_map" + extensionType;
  if (!exists(imagefile))
    missingfile = "howling_abyss_map" + extensionType;

  //Check for champion images in reroll list
  for (auto it = ARAMRerollList.begin(); it != ARAMRerollList.end(); ++it) {
    imagefile = image_folder + champion_icon_folder + *it + extensionType;
    if (!exists(imagefile)) {
      missingfile = champion_icon_folder + *it + extensionType;
      break;
    }
  }

  //If required image is missing, terminate
  if (missingfile != "") {
    missingfile = "A required image was not found at: \\" + image_folder + missingfile;

    quitWithErrorMessage(missingfile, "Image Missing", ERR_IMG_MISSING, MB_ICONERROR);
  }
}

//Reset loop cycle
void resetCycle(void) {
  //Set all default state variables
  isActive = false;

  breakSleep = false;

  xCord = NULL;
  yCord = NULL;

  currentStageIndex = 0;
}

//Breakable sleep function
void breakableSleep(int TIME) {
  for (int dw = 0; dw < TIME; dw += 1000) {
    if (breakSleep) {
      breakSleep = false;
      return;
    }
    else {
      Sleep(1000);
    }
  }
}

// Clicks target
void ClickTarget(HWND hwnd, int x, int y, int cols, int rows, int xoffset, int yoffset) {
  //X and Y positions to click
  int xPos = x + (cols / 2) + xoffset;
  int yPos = y + (rows / 2) + yoffset;

  //Randomise click position
  //Generate random number between 0 and 10 then subtract 5
  int x_rand_change = (rand() % 11) - 5;
  int y_rand_change = (rand() % 11) - 5;
  xPos += x_rand_change;
  yPos += y_rand_change;

  POINT win_coords = { xPos, yPos };
  POINT ctrl_coords = { xPos, yPos };

  ScreenToClient(hwnd, &win_coords);
  HWND ctrl_handle = hwnd;
  ScreenToClient(ctrl_handle, &ctrl_coords);

  //Before we click, set foreground window to object we want to click
  SetForegroundWindowInternal(ctrl_handle);

  LPARAM lParam = MAKELPARAM(ctrl_coords.x, ctrl_coords.y);
  SendMessage(ctrl_handle, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
  SendMessage(ctrl_handle, WM_LBUTTONUP, 0, lParam);

}


//Capture screenshot of window from hwnd
bool hwnd2image(HWND hwnd, LPCSTR fname) {
  bool status = false;

  HDC hdcSource = GetDC(NULL);
  HDC hdcMemory = CreateCompatibleDC(hdcSource);

  int capX = GetDeviceCaps(hdcSource, HORZRES);
  int capY = GetDeviceCaps(hdcSource, VERTRES);

  //Get Window position information
  RECT windowRect;
  GetWindowRect(hwnd, &windowRect);

  //Get Client Information
  RECT clientsize;    // get the height and width of the handle
  GetClientRect(hwnd, &clientsize);

  int width = clientsize.right;
  int height = clientsize.bottom;

  int x = windowRect.left;
  int y = windowRect.top;

  HBITMAP hBitmap = CreateCompatibleBitmap(hdcSource, width, height);
  HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcMemory, hBitmap);

  BitBlt(hdcMemory, 0, 0, width, height, hdcSource, x, y, SRCCOPY);
  hBitmap = (HBITMAP)SelectObject(hdcMemory, hBitmapOld);

  //Free hdc sources
  DeleteDC(hdcSource);
  DeleteDC(hdcMemory);

  HPALETTE hpal = NULL;

  //Attempt to save the bitmap
  if (saveBitmap(fname, hBitmap, hpal))
    status = true;

  //Free object data
  DeleteObject(hBitmap);
  DeleteObject(hBitmapOld);
  DeleteObject(hpal);

  return status;
}

//Save bitmap file type to file given a HBITMAP
bool saveBitmap(LPCSTR filename, HBITMAP bmp, HPALETTE pal)
{
  bool result = false;
  PICTDESC pd;

  pd.cbSizeofstruct = sizeof(PICTDESC);
  pd.picType = PICTYPE_BITMAP;
  pd.bmp.hbitmap = bmp;
  pd.bmp.hpal = pal;

  LPPICTURE picture;
  HRESULT res = OleCreatePictureIndirect(&pd, IID_IPicture, false,
    reinterpret_cast<void**>(&picture));

  if (!SUCCEEDED(res))
    return false;

  LPSTREAM stream;
  res = CreateStreamOnHGlobal(0, true, &stream);

  if (!SUCCEEDED(res))
  {
    picture->Release();
    return false;
  }

  LONG bytes_streamed;
  res = picture->SaveAsFile(stream, true, &bytes_streamed);

  HANDLE file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, 0,
    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  if (!SUCCEEDED(res) || !file)
  {
    stream->Release();
    picture->Release();
    return false;
  }

  HGLOBAL mem = 0;
  GetHGlobalFromStream(stream, &mem);
  LPVOID data = GlobalLock(mem);

  DWORD bytes_written;

  result = !!WriteFile(file, data, bytes_streamed, &bytes_written, 0);
  result &= (bytes_written == static_cast<DWORD>(bytes_streamed));

  GlobalUnlock(mem);
  CloseHandle(file);

  stream->Release();
  picture->Release();

  return result;
}



//Types string to stdout as physical keyboard strokes
void TypeInfo(const string info) {

  breakableSleep(250);

  INPUT ip;
  ip.type = INPUT_KEYBOARD;
  ip.ki.time = 0;
  ip.ki.wVk = 0;
  ip.ki.dwExtraInfo = 0;

  //Loop through champion name and type it out
  for (size_t i = 0; i < info.length(); ++i) {
    ip.ki.dwFlags = KEYEVENTF_UNICODE;
    ip.ki.wScan = info[i];
    SendInput(1, &ip, sizeof(INPUT));

    //Prepare a keyup event
    ip.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
  }

  breakableSleep(250);
}

//Update tray icon using NIM_MODIFY
void updateTrayStatus(const string status, bool showToolTip) {
  InitNotifyIconData(status, showToolTip);
  updateTrayWindow();
}

//Toggles status of isActive variable
void toggleisActive(void) {
  isActive = !isActive; //switch bool isactive

                        // Show notification of status
  if (isActive) {
    updateTrayStatus(TIP_PREFIX "Running! Looking for: " + templateNames[templateMode[currentStageIndex]], true);
  }
  else {
    breakSleep = true; //leave any sleeps
    updateTrayStatus(TIP_PREFIX "Paused (" + templateNames[templateMode[currentStageIndex]] + ")", true);
  }
}

//Read in config and return error if fails
void readSettings(void) {
  //Variables
  boost::property_tree::ptree pt;
  std::string errorMsg;
  short errorCode = 0;

  if (!exists(SETTINGS_FILE)) {
    errorMsg = "Configuration file was not found in the current directory or cannot be accessed.";
    errorCode = ERR_CONF_MISSING;
  }
  else {
    //Read ini file and get required values
    read_ini(SETTINGS_FILE, pt);

    //Read champion name
    std::string champNameString = pt.get<std::string>("CHAMPION.Name");
    boost::split(championList, champNameString, boost::is_any_of(","), boost::token_compress_on);

    //Read ARAM Reroll List
    std::string rerollString = pt.get<std::string>("QUEUE.ARAMRerollList");
    if (!rerollString.empty())
      boost::split(ARAMRerollList, rerollString, boost::is_any_of(","), boost::token_compress_on);

    TextToCall = pt.get<std::string>("QUEUE.TextToCall");
    Type = atoi(pt.get<std::string>("QUEUE.Type").c_str());
    COOPvsAIDifficulty = atoi(pt.get<std::string>("QUEUE.COOPvsAIDifficulty").c_str());

    DESPERATE_TOLERANCE = atof(pt.get<std::string>("TOLERANCE.DesperateTolerance").c_str());
    GOOD_TOLERANCE = atof(pt.get<std::string>("TOLERANCE.GoodTolerance").c_str());
    STRICT_TOLERANCE = atof(pt.get<std::string>("TOLERANCE.StrictTolerance").c_str());
    VERY_STRICT_TOLERANCE = atof(pt.get<std::string>("TOLERANCE.VeryStrictTolerance").c_str());

    PauseUnpauseHotkey = strtol(pt.get<std::string>("SETTINGS.PauseUnpauseHotkey").c_str(), NULL, 0);
    ResetHotkey = strtol(pt.get<std::string>("SETTINGS.ResetHotkey").c_str(), NULL, 0);

    //Check if logging should be enabled and create log directory/file
    LoggingEnabled = (atoi(pt.get<std::string>("SETTINGS.LoggingEnabled").c_str()) != 0);

    //Make log file and log dir if doesn't exist
    if (!is_directory(LOG_FOLDER)) {
      if (!create_directory(LOG_FOLDER)) { //if directory making failed

                                           //Convert to proper types
        std::string errorText = "Failed to create log file. Logging has been disabled\n\nWarning Code: " + to_string(ERR_CREATE_LOG_FILE);
        std::string errorTypeText = NAME " - Logging Error";

        MessageBox(NULL,
          errorText.c_str(),
          errorTypeText.c_str(),
          MB_ICONWARNING | MB_OK);

        LoggingEnabled = false;
      }
    }

    //Create log file if required
    if (LoggingEnabled) {
      //Make temp file for logging
      char buffer[L_tmpnam];
      tmpnam(buffer);
      std::string tmp = buffer;
      tmp = tmp.substr(1, tmp.length() - 2); //Remove initial '/' and final '1'

      time_t now = time(0);
      tm *ltm = localtime(&now);

      //Make components
      std::string month = to_string(ltm->tm_mon + 1);
      std::string day = to_string(ltm->tm_mday);

      if (ltm->tm_mon < 10)
        month.insert(0, "0");

      if (ltm->tm_mday < 10)
        day.insert(0, "0");


      std::string date = to_string(1900 + ltm->tm_year) + '-' + month + '-' + day;

      //Make full log file name
      std::string logFileName = LOG_FOLDER "\\" + date + "_" + tmp + ".txt";

      //Open file
      logFile.open(logFileName);

      // Add inital entries
      addLogEntry(HEADER_PREFIX, "PoroQueue Log File");
      addLogEntry(HEADER_PREFIX, "Version: " VERSION);
      string dt = ctime(&now);
      dt = dt.substr(0, dt.length() - 1); //remove new line added by ctime
      addLogEntry(HEADER_PREFIX, "Log file created at: " + dt);
    }

    //Get client type
    ClientType = atoi(pt.get<std::string>("GAME.ClientType").c_str());

    //Find path to lol rads folder
    PathToLoLFolder = pt.get<std::string>("GAME.PathToLoLFolder");

    bool PathToLoLFolderFound = false;

    if (PathToLoLFolder == "AUTODETECT") {
      std::string dir;
      std::string exeloc;

      //Try looking in multiple registry locations for RADS dir
      std::vector<std::string> HKCR = boost::assign::list_of
        ("VirtualStore\\MACHINE\\SOFTWARE\\Wow6432Node\\Riot Games\\RADS")
        ("VirtualStore\\MACHINE\\SOFTWARE\\RIOT GAMES\\RADS")
        ;

      std::vector<std::string> HKCU = boost::assign::list_of
        ("SOFTWARE\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\Wow6432Node\\Riot Games\\RADS")
        ("SOFTWARE\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\RIOT GAMES\\RADS")
        ("SOFTWARE\\Riot Games\\RADS")
        ;

      std::vector<std::string> HKLM = boost::assign::list_of
        ("SOFTWARE\\Wow6432Node\\Riot Games\\RADS")
        ("SOFTWARE\\Riot Games\\RADS")
        ;

      bool isFound = false;

      //HKEY_CLASSES_ROOT
      for (std::vector<std::string>::iterator it = HKCU.begin(); it != HKCU.end() && !isFound; ++it) {
        isFound = getLoLRadsDir(HKEY_CLASSES_ROOT, *it, dir);

        if (isFound && LoggingEnabled)
          addLogEntry(HEADER_PREFIX, "RADS registry entry found at: [HKEY_CLASSES_ROOT] " + *it);
      }

      //HKEY_CURRENT_USER
      for (std::vector<std::string>::iterator it = HKCU.begin(); it != HKCU.end() && !isFound; ++it) {
        isFound = getLoLRadsDir(HKEY_CURRENT_USER, *it, dir);

        if (isFound && LoggingEnabled)
          addLogEntry(HEADER_PREFIX, "RADS registry entry found at: [HKEY_CURRENT_USER] " + *it);
      }

      //HKEY_LOCAL_MACHINE
      for (std::vector<std::string>::iterator it = HKCU.begin(); it != HKCU.end() && !isFound; ++it) {
        isFound = getLoLRadsDir(HKEY_LOCAL_MACHINE, *it, dir);

        if (isFound && LoggingEnabled)
          addLogEntry(HEADER_PREFIX, "RADS registry entry found at: [HKEY_LOCAL_MACHINE] " + *it);
      }

      //Check if we managed to auto detect required lol rads dir
      if (isFound && is_directory(dir)) {
        dir = dir.substr(0, dir.find("RADS")); //remove RADS at end

        if (is_directory(dir)) {
          PathToLoLFolder = dir;
          PathToLoLFolderFound = true;
        }
      }

      //Check default LoL installation path
      if (!isFound && is_directory("C:\\Riot Games\\League of Legends")) {
        PathToLoLFolder = "C:\\Riot Games\\League of Legends";
        PathToLoLFolderFound = true;
      }

    }
    else if (is_directory(PathToLoLFolder)) { //Dont show error if folder exists
      PathToLoLFolderFound = true;
    }

    //Try to autodetect in game resolution
    if (PathToLoLFolderFound && (ClientType == CLIENT_TYPE_NORMAL || ClientType == CLIENT_TYPE_GARENA)) { //if path is found
      std::string lolDir = PathToLoLFolder;
      std::string res;

      //If end of path doesn't include '\', add it
      if (lolDir[lolDir.length() - 1] != '\\') { //Check for ending '\'
        lolDir.push_back('\\');
      }

      //Set dir to game.cfg manually
      if (ClientType == CLIENT_TYPE_NORMAL) {
        lolDir += "Config\\game.cfg";
      }
      else if (ClientType == CLIENT_TYPE_GARENA) {
        lolDir += "Game\\DATA\\CFG\\defaults\\GAME.CFG";
      }

      if (exists(lolDir)) { //if found
        boost::property_tree::ptree cfg;
        read_ini(lolDir, cfg);

        //Try reading entry for height and width
        try {
          res = cfg.get<std::string>("General.Width") + "x" + cfg.get<std::string>("General.Height");
          GameResolution = res;
        }
        catch (exception&) {}

      }
    }


    //Error Checking configuration file
    if (champNameString == "" || !(Type >= 1 && Type <= 3) ||
      (Type == 1 && (COOPvsAIDifficulty != 1 && COOPvsAIDifficulty != 2))) {

      errorMsg = "Configuration file is missing required entries or has invalid values.";
      errorCode = ERR_CONF_INVALID;
    }
    else if (
      (DESPERATE_TOLERANCE < 0.01 || DESPERATE_TOLERANCE > 1.0) ||
      (GOOD_TOLERANCE < 0.01 || GOOD_TOLERANCE > 1.0) ||
      (STRICT_TOLERANCE < 0.01 || STRICT_TOLERANCE > 0.1) ||
      (VERY_STRICT_TOLERANCE >= STRICT_TOLERANCE || VERY_STRICT_TOLERANCE < 0.001 || VERY_STRICT_TOLERANCE > 0.05))
    {
      errorCode = ERR_CONF_INVALID;
      errorMsg = "A variable in the [TOLERANCE] section is outside of its permittable range or is invalid.";
    }
    else if (!PathToLoLFolderFound) {
      errorCode = ERR_CONF_INVALID;
      if (PathToLoLFolder == "AUTODETECT")
        errorMsg = "Program failed to auto detect path to league of legends folder. Please set this path manually.";
      else
        errorMsg = "The league of legends folder was not found at the specified path.";
    }
    else if (ClientType != CLIENT_TYPE_NORMAL && ClientType != CLIENT_TYPE_GARENA) {
      errorCode = ERR_CONF_INVALID;
      errorMsg = "Invalid ClientType was provided.";

    }
    else if (PauseUnpauseHotkey == NULL || ResetHotkey == NULL) {
      errorCode = ERR_CONF_INVALID;
      errorMsg = "Entered hotkey is invalid or was not succesfully translated. Please try setting other hotkey values.";
    }

  }

  //If errors encountered in read, terminate
  if (errorCode) {
    quitWithErrorMessage(errorMsg, "Configuration Error", errorCode, MB_ICONERROR);
  }
}

// Initilise NOTIFYICONDATA for tray
void InitNotifyIconData(const string tip, bool showToolTip) {
  memset(&nid, 0, sizeof(NOTIFYICONDATA));
  nid.cbSize = sizeof(NOTIFYICONDATA);
  nid.hWnd = hwnd;
  nid.uVersion = NOTIFYICON_VERSION;
  nid.uID = ID_TRAY_APP_ICON;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
  nid.uCallbackMessage = WM_COMMAND;
  nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON2));
  nid.dwInfoFlags = NIIF_USER;

  //Set tooltip using tchar
  _tcscpy(nid.szTip, tip.c_str());

  //Show ballon on pause or unpause
  std::string ballonTitle = NAME;
  std::string ballonMsg;

  //Check for status
  if (showToolTip) {
    ballonMsg = tip.substr(tip.find("STATUS:"), tip.size());

    //Set baloon if applicable
    if (ballonMsg.length() != 0) {
      _tcscpy(nid.szInfoTitle, ballonTitle.c_str());
      _tcscpy(nid.szInfo, ballonMsg.c_str());
    }
  }

  //Logging
  if (LoggingEnabled)
    addLogEntry(NORMAL_PREFIX, ballonMsg);
}

// Hide a dummy window because Windows HAS to have a HWND attached to a taskbar instance
void updateTrayWindow(void) {
  // Add the icon to the system tray
  Shell_NotifyIcon(NIM_MODIFY, &nid);

  // Hide the main window
  ShowWindow(hwnd, SW_HIDE);
}

//Terminate program with given errors
void quitWithErrorMessage(const string &error, const string &errorType, unsigned short code, int iconType) {
  //Convert to proper types
  std::string errorText = error + "\n\nExit Code: " + to_string(code);
  std::string errorTypeText = NAME " - " + errorType;

  //Output termination message box
  MessageBox(NULL,
    errorText.c_str(),
    errorTypeText.c_str(),
    iconType | MB_OK);

  //Log entry
  if (LoggingEnabled) {
    addLogEntry(NORMAL_PREFIX, "FATAL ERROR: " + errorText);
    logFile.close(); //Close log file
  }

  //Kill
  exit(code);
}

//Write a line to the log file
void addLogEntry(unsigned int HEADER_TYPE, string entry)
{
  //Append new line to entry
  std::string output = entry + "\n";

  //Append time prefix to normal log entries
  if (HEADER_TYPE == NORMAL_PREFIX) {
    time_t timeNow = time(0);
    tm *ltm = localtime(&timeNow);

    //Make components
    std::string hour = to_string(ltm->tm_hour);
    std::string min = to_string(ltm->tm_min);
    std::string sec = to_string(ltm->tm_sec);

    if (ltm->tm_hour < 10)
      hour.insert(0, "0");

    if (ltm->tm_min < 10)
      min.insert(0, "0");

    if (ltm->tm_sec < 10)
      sec.insert(0, "0");

    output.insert(0, "[" + hour + ':' + min + ':' + sec + "] ");
  }

  //Write to log
  logFile << output;
}
