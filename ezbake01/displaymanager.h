#ifndef _displaymanager_h
#define _displaymanager_h

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class DisplayManager
{
public:

  DisplayManager(int addr=0x3C);

  bool begin(const char* splash);
  void clear_all();

  void set_title(const char* title);
  
  void reset_clock(int remains);
  void tick_clock();
  int clock() const { return time_remaining_; }

  void update_T_ranges(int lo, int hi, int next);

  void update_current_T(int T, bool in_range=true);

  void update_icon(int id);
  
  void show() { display_.display(); }

private:
  void update_clock();
  void update_rect(int x, int y, const char* msg, int lmargin, GFXcanvas1& canvas, bool invert = false);

  Adafruit_SSD1306 display_;
  int addr_;

  int time_remaining_; 

  GFXcanvas1 title_canvas_;
  GFXcanvas1 clock_canvas_;
  GFXcanvas1 T_range_canvas_;
  GFXcanvas1 T_current_canvas_;
  GFXcanvas1 dir_canvas_;
  

};


#endif //_displaymanager_h
