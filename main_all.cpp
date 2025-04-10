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

using namespace std;

class Progress {
private:
    std::mutex mutex_;
    float progress_{0.0f};
    size_t bar_width_{100};
    std::string fill_{"■"}, remainder_{" "};
    std::atomic<bool> started_{false};
public:
    Progress() = default;
    Progress(Progress&& prg)  {}
    // Progress(std::string fill = "■", std::string remainder = " ", std::string status_text = "");
    
    void SetCursorPos(int XPos, int YPos){
    //std::printf("X is %d, Y is %d_____", XPos, YPos);
    std::printf("\033[%d;%dH", YPos+1, XPos+1);
}
    
    void set_progress(float value) {
    std::unique_lock lock{mutex_};  // CTAD (C++17)
    progress_ = value;
}

    void update(float value, int startXPos, int startYPos, int index = 0, size_t size = 1) {
    set_progress(value);
    write_progress(index, startXPos, startYPos, size);
}

    void write_progress(int index, int startXPos, int startYPos, size_t size) {
    std::unique_lock lock{mutex_};
    if (progress_ > 50.0f) return;

    static auto start = std::chrono::system_clock::now();
    if(!started_){
    //Set cursor position to X = 0 and Y = line number
    SetCursorPos(0, startYPos+index);
    // Start bar
    std::cout << index + 1 << "." << "\t" << std::this_thread::get_id() << "\t";
    std::cout << "[";
    started_ = true;
    }
    SetCursorPos(startXPos + progress_, startYPos+index);   
    if(progress_ < 50){
      std::cout << fill_;
      std::cout <<std::flush;
    }else{
      std::cout << "]";
      std::chrono::duration<float> duration = std::chrono::system_clock::now() - start;
      std::cout << "Elapsed time: " << duration.count() << " s";
      SetCursorPos(0, startYPos + size); //THIS IS TO PREVENT OVERWRITING BARS AFTER RETURNING CONTROL TO TERMINAL
    }

}
};


class MultiProgress
{
    std::vector<Progress> bars_;
    std::mutex mutex_;
    std::atomic<bool> started_{false};
    int XPos, YPos;
public:
    MultiProgress() {}
    MultiProgress(std::vector<Progress> bars): bars_(std::move(bars)) {}
    
    void updateCursorPos() {
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
    int getXPos() { return XPos; }
    int getYPos() { return YPos; }
    
    void update(int index, float value) {
    std::unique_lock lock{mutex_};
    //Print header bar
    if(!started_){
        std::cout << "#\tthread id\t";
        updateCursorPos();
        for (size_t i = 0; i < (50 + 2 - 12)/2; ++i) { std::cout << " "; }
        std::cout << "Progress Bar\n";
        started_ = true;
    }
    bars_[index].set_progress(value);
    write_progress(index);
}
    void write_progress(int index) {
    //std::unique_lock lock{mutex_};

    // Write current state of the bar
    bars_[index].write_progress(index, XPos,YPos, bars_.size());
    //std::cout << "\n";

    if (!started_)
        started_ = true;
}

};



int main() {

    std::vector<Progress> barvec;
    barvec.push_back(Progress());
    barvec.push_back(Progress());
    barvec.push_back(Progress());
    // barvec.push_back(Progress("■", " ", ""));

    MultiProgress bars(std::move(barvec));

    // Job for the first bar
    auto job1 = [&bars]() {
        for (size_t i = 0; i <= 50; ++i) {
            bars.update(0,i);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    };

    // Job for the second bar
    auto job2 = [&bars]() {
        for (size_t i = 0; i <= 50; ++i) {
            bars.update(1,i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    // Job for the third bar
    auto job3 = [&bars]() {
        for (size_t i = 0; i <= 50; ++i) {
            bars.update(2,i);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    };

    std::thread first_job(job1);
    std::thread second_job(job2);
    std::thread third_job(job3);

    first_job.join();
    second_job.join();
    third_job.join();
    
    //std::cout << std::endl;
}
