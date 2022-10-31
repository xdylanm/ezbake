#include "displaymanager.h"
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans18pt7b.h>

#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)


namespace {
void right_align(int n, char* msg, String const& sval) 
{
  if (sval.length() > n) {
    return;
  }
  int startpos = n-sval.length();           
  for (int i = 0; i < sval.length(); ++i) {
    msg[startpos+i] = sval.charAt(i);
  }
}
}

DisplayManager::DisplayManager(int const addr /*=0x3C*/) 
  : display_(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET), 
    addr_(addr),
    time_remaining_(-1),
    title_canvas_(84,18),
    clock_canvas_(42,18),
    T_range_canvas_(128,28),
    T_current_canvas_(96,18),
    dir_canvas_(20,18)    
{
  title_canvas_.setFont(&FreeSans9pt7b);
  title_canvas_.setTextSize(1);
  title_canvas_.setTextColor(SSD1306_WHITE);

  clock_canvas_.setFont(&FreeSansBold9pt7b);
  clock_canvas_.setTextSize(1);
  clock_canvas_.setTextColor(SSD1306_WHITE);

  T_range_canvas_.setFont(&FreeSansBold9pt7b);
  T_range_canvas_.setTextSize(1);
  T_range_canvas_.setTextColor(SSD1306_WHITE);

  T_current_canvas_.setFont(&FreeSansBold9pt7b);
  T_current_canvas_.setTextSize(1);
  T_current_canvas_.setTextColor(SSD1306_WHITE);  
}

bool DisplayManager::begin(const char* splash)
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display_.begin(SSD1306_SWITCHCAPVCC, addr_)) {
    return false;
  }
  display_.clearDisplay();
  display_.setFont(&FreeSansBold18pt7b);
  display_.setCursor(0,12);
  display_.print(splash);
  display_.display();
  return true;
}

void DisplayManager::clear_all() 
{
  display_.clearDisplay();
}

void DisplayManager::set_title(char const* title) 
{
  update_rect(0,0,title,0,title_canvas_);
}

void DisplayManager::reset_clock(int remains) 
{
  time_remaining_ = remains;
  update_clock();
}

void DisplayManager::tick_clock() 
{
  time_remaining_ -= 1;
  update_clock();
}

void DisplayManager::update_T_ranges(int lo, int hi, int next) 
{
  char msg[] = "   ";
  T_range_canvas_.fillScreen(SSD1306_BLACK);
  T_range_canvas_.setFont(&FreeSansBold9pt7b);
  T_range_canvas_.setCursor(0, T_range_canvas_.height()-4);
  if (lo >= 0 && lo < 1000) {
    String sval_lo (lo);
    right_align(3,msg,sval_lo);
    T_range_canvas_.print(msg);
  } else {
    T_range_canvas_.print("---");    
  }

  T_range_canvas_.setFont(&FreeSansBold12pt7b);
  T_range_canvas_.print(" ");
  if (hi >= 0 && hi < 1000) {
    String sval_hi (hi);
    right_align(3,msg,sval_hi);
    T_range_canvas_.print(msg);
  } else {
    T_range_canvas_.print("---");    
  }
  T_range_canvas_.print(" ");
  T_range_canvas_.setFont(&FreeSans9pt7b);
  if (next >= 0 && next < 1000) {
    String sval_nx (next);
    right_align(3,msg,sval_nx);
    T_range_canvas_.print(msg);
  } else {
    T_range_canvas_.print("---");    
  }

  display_.drawBitmap(0,18,T_range_canvas_.getBuffer(),T_range_canvas_.width(),
    T_range_canvas_.height(),SSD1306_WHITE,SSD1306_BLACK);
}

void DisplayManager::update_current_T(int const T, bool const in_range)
{
  if (T >= 0 && T < 1000) {
    String sval (T);
    update_rect(32,46,sval.c_str(),0,T_current_canvas_, !in_range);  
  } else{
    update_rect(32,46,"---",0,T_current_canvas_);      
  }
}

void DisplayManager::update_icon(int id) 
{
  dir_canvas_.fillScreen(SSD1306_BLACK);
  if (id == 0) {        // ramp
    dir_canvas_.fillTriangle(4,7,9,0,14,7,SSD1306_WHITE);
  } else if (id == 1) { // hold
    dir_canvas_.fillTriangle(4,7,9,0,14,7,SSD1306_WHITE);
    dir_canvas_.fillTriangle(4,9,14,9,9,16,SSD1306_WHITE);
  } else if (id == 2) { // peak
    dir_canvas_.drawTriangle(4,7,9,0,14,7,SSD1306_WHITE);
  } else if (id == 3) { // cool
    dir_canvas_.fillTriangle(4,9,14,9,9,16,SSD1306_WHITE);
  } 
  display_.drawBitmap(0,46,dir_canvas_.getBuffer(),dir_canvas_.width(),dir_canvas_.height(),SSD1306_WHITE,SSD1306_BLACK);
}

void DisplayManager::update_clock() 
{
  bool const is_neg = time_remaining_ < 0;
  int const abs_time_remains = is_neg ? -time_remaining_ : time_remaining_;
  char msg[4];
  msg[3] = '\0';
  for (int i = 0; i < 3; ++i) {
    msg[i] = abs_time_remains > 999 ? '-' : ' ';
  }
  if (abs_time_remains < 1000) {
    String sval (abs_time_remains);
    right_align(3,msg,sval);
  }
  update_rect(86,0,msg,0,clock_canvas_, is_neg);
}

void DisplayManager::update_rect(
    int const x, 
    int const y, 
    const char* msg, 
    int const lmargin,  
    GFXcanvas1& canvas,
    bool const invert)
{
  uint16_t const black = invert ? SSD1306_WHITE : SSD1306_BLACK;
  uint16_t const white = invert ? SSD1306_BLACK : SSD1306_WHITE;
    
  canvas.fillScreen(SSD1306_BLACK);
  canvas.setTextColor(SSD1306_WHITE);
  canvas.setCursor(lmargin, canvas.height()-4);
  canvas.print(msg);

  display_.drawBitmap(x,y,canvas.getBuffer(),canvas.width(),canvas.height(),white,black);
}

