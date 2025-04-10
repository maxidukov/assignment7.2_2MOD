#include <iostream>
#include <thread>
#include <mutex>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

class Progress {
private:
    mutex mutex_;
    float progress_{0.0f};
    size_t bar_width_{100};
    string fill_{"■"}, remainder_{" "};
    atomic<bool> started_{false};
    atomic<bool> done_{false};
public:
    Progress() = default;
    Progress(Progress&& prg)  {}
    // Progress(std::string fill = "■", std::string remainder = " ", std::string status_text = "");
    
    void set_bar_width(size_t width){
      bar_width_ = width;
    }
    
    void set_cursor_pos(int XPos, int YPos){
    printf("\033[%d;%dH", YPos+1, XPos+1);
}
    
    void set_progress(float value) {
    unique_lock lock{mutex_};  // CTAD (C++17)
    progress_ = value;
}

    void update(float value, int startXPos, int startYPos, int index = 0, size_t size = 1) {
    set_progress(value);
    write_progress(index, startXPos, startYPos, size);
}

    void write_progress(int index, int startXPos, int startYPos, size_t size) {
    try{
    unique_lock lock{mutex_};
    if (progress_ > bar_width_ || done_ ) return;

    static auto start = chrono::system_clock::now();
    if(!started_){
    //Set cursor position to X = 0 and Y = line number
    set_cursor_pos(0, startYPos+index);
    // Start bar
    cout << index + 1 << "." << "\t" << this_thread::get_id() << "\t";
    cout << "[";
    started_ = true;
    }
    set_cursor_pos(startXPos + progress_, startYPos+index);   
    if(progress_ < bar_width_){
      if(rand() < 0.02 * (RAND_MAX+1.0)) {
          done_ = true;
          throw std::runtime_error("An exception occurred in the thread."); //SIMULATE EXCEPTION WITH PROBABILITY 2%
      } 
      cout << fill_;
      cout << flush;
    }else{
      cout << "]";
      chrono::duration<float> duration = chrono::system_clock::now() - start;
      cout << "Elapsed time: " << duration.count() << " s";
      set_cursor_pos(0, startYPos + size); //THIS IS TO PREVENT OVERWRITING BARS AFTER RETURNING CONTROL TO TERMINAL
      done_ = true;
    }
    }catch(...){
      cout << "\033[1;31m Exception thrown!\033[0m";
      set_cursor_pos(0, startYPos + size); //THIS IS TO PREVENT OVERWRITING BARS AFTER RETURNING CONTROL TO TERMINAL
    }

}
};


class MultiProgress
{
    vector<Progress> bars_;
    mutex mutex_;
    atomic<bool> started_{false};
    size_t bar_width_{100};
    int XPos, YPos;
public:
    MultiProgress() {}
    MultiProgress(vector<Progress> bars): bars_(move(bars)) {}
    
    void set_bar_width(size_t width){ 
      bar_width_ = width;
      for( auto& bar : bars_){
        bar.set_bar_width(bar_width_);
      }
    }
    
    void update_cursor_pos() {
    int t = STDOUT_FILENO;
    struct termios sav;
    tcgetattr(t, &sav);
    struct termios opt = sav;
    opt.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(t, TCSANOW, &opt);
    printf("\033[6n");
    fflush(stdout);
    scanf("\033[%d;%dR", &YPos, &XPos);
    tcsetattr(t, TCSANOW, &sav);
}
    size_t get_bar_width() { return bar_width_; }
    int getXPos() { return XPos; }
    int getYPos() { return YPos; }
    
    void update(int index, float value) {
    unique_lock lock{mutex_};
    //Print header bar
    if(!started_){
        cout << "#\tthread id\t";
        update_cursor_pos();
        for (size_t i = 0; i < (bar_width_ + 2 - 12)/2; ++i) { cout << " "; }
        cout << "Progress Bar\n";
        started_ = true;
    }
    bars_[index].set_progress(value);
    write_progress(index);
}
    void write_progress(int index) {

    // Write current state of the bar
    bars_[index].write_progress(index, XPos,YPos, bars_.size());

    if (!started_)
        started_ = true;
}

};



int main() {

    vector<Progress> barvec;
    barvec.push_back(Progress());
    barvec.push_back(Progress());
    barvec.push_back(Progress());
    // barvec.push_back(Progress("■", " ", ""));

    MultiProgress bars(std::move(barvec));
    
    size_t bar_width = 50;
    bars.set_bar_width(bar_width);

    // Job for the first bar
    auto job1 = [&bars]() {
        for (size_t i = 0; i <= bars.get_bar_width(); ++i) {
            bars.update(0,i);
            this_thread::sleep_for(chrono::milliseconds(200));
        }
    };

    // Job for the second bar
    auto job2 = [&bars]() {
        for (size_t i = 0; i <= bars.get_bar_width(); ++i) {
            bars.update(1,i);
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    };

    // Job for the third bar
    auto job3 = [&bars]() {
        for (size_t i = 0; i <= bars.get_bar_width(); ++i) {
            bars.update(2,i);
            this_thread::sleep_for(chrono::milliseconds(300));
        }
    };

    thread first_job(job1);
    thread second_job(job2);
    thread third_job(job3);

    first_job.join();
    second_job.join();
    third_job.join();
}
