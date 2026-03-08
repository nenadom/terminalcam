#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <cmath>
#include <string>
#include <chrono>
#include <cstdio>

// ASCII ramp from AsciiArt.js -- 70 chars, light to dark
const char* RAMP = " .'`^\",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
const int RAMP_LEN = 70;
const double CHAR_ASPECT = 2.2;
const int TARGET_FPS = 15;

// Global state
volatile sig_atomic_t running = 1;
struct termios orig_termios;
bool termios_saved = false;

// Controls state
int brightness_offset = 0;
bool inverted = false;
bool show_fps = true;

void cleanup() {
    // Show cursor
    fputs("\033[?25h", stdout);
    // Clear screen
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
    // Restore terminal settings
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
}

void signal_handler(int) {
    running = 0;
}

void get_terminal_size(int& cols, int& rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        cols = ws.ws_col;
        rows = ws.ws_row - 1; // Reserve 1 row for status bar
    } else {
        cols = 80;
        rows = 23;
    }
}

std::string frame_to_ascii(const cv::Mat& frame, int cols, int rows) {
    cv::Mat flipped;
    cv::flip(frame, flipped, 1); // Mirror horizontally

    int width = flipped.cols;
    int height = flipped.rows;

    // Compute block sizes; charH accounts for terminal character aspect ratio
    double block_w = (double)width / cols;
    double block_h = block_w * CHAR_ASPECT;

    // Recalculate rows to fit the frame with correct aspect
    int actual_rows = (int)(height / block_h);
    if (actual_rows > rows) actual_rows = rows;
    if (actual_rows < 1) actual_rows = 1;

    std::string output;
    output.reserve((cols + 1) * actual_rows);

    for (int r = 0; r < actual_rows; r++) {
        for (int c = 0; c < cols; c++) {
            int x = (int)(c * block_w);
            int y = (int)(r * block_h);
            int bw = (int)block_w;
            int bh = (int)block_h;

            // Clamp to frame boundaries
            if (x + bw > width) bw = width - x;
            if (y + bh > height) bh = height - y;
            if (bw <= 0 || bh <= 0) {
                output += ' ';
                continue;
            }

            cv::Rect roi(x, y, bw, bh);
            cv::Scalar mean_val = cv::mean(flipped(roi));

            // LUMA brightness (BGR order in OpenCV)
            double brightness = 0.299 * mean_val[2] + 0.587 * mean_val[1] + 0.114 * mean_val[0];

            // Apply brightness offset
            brightness += brightness_offset;
            if (brightness < 0) brightness = 0;
            if (brightness > 255) brightness = 255;

            int index = (int)(brightness * (RAMP_LEN - 1) / 255.0);
            if (index < 0) index = 0;
            if (index >= RAMP_LEN) index = RAMP_LEN - 1;

            if (inverted) index = RAMP_LEN - 1 - index;

            output += RAMP[index];
        }
        if (r < actual_rows - 1) output += '\n';
    }

    // Pad remaining rows if frame doesn't fill terminal
    for (int r = actual_rows; r < rows; r++) {
        output += '\n';
    }

    return output;
}

void render_frame(const std::string& ascii, double fps) {
    std::string buf;
    buf.reserve(ascii.size() + 200);

    // Cursor home (no clear = no flicker)
    buf += "\033[H";
    buf += ascii;
    buf += '\n';

    // Status bar
    if (show_fps) {
        char status[256];
        snprintf(status, sizeof(status),
            "\033[7mFPS: %.1f | Brightness: %+d | [q]uit [b/B]rightness [i]nvert [f]ps\033[0m\033[K",
            fps, brightness_offset);
        buf += status;
    } else {
        char status[128];
        snprintf(status, sizeof(status),
            "\033[7m[q]uit [b/B]rightness [i]nvert [f]ps\033[0m\033[K");
        buf += status;
    }

    // Clear any remaining lines below
    buf += "\033[J";

    write(STDOUT_FILENO, buf.c_str(), buf.size());
}

void setup_raw_terminal() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    termios_saved = true;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
    raw.c_cc[VMIN] = 0;              // Non-blocking read
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void handle_input() {
    char ch;
    while (read(STDIN_FILENO, &ch, 1) == 1) {
        switch (ch) {
            case 'q':
            case 'Q':
                running = 0;
                break;
            case 'b':
                brightness_offset -= 10;
                if (brightness_offset < -255) brightness_offset = -255;
                break;
            case 'B':
                brightness_offset += 10;
                if (brightness_offset > 255) brightness_offset = 255;
                break;
            case 'i':
            case 'I':
                inverted = !inverted;
                break;
            case 'f':
            case 'F':
                show_fps = !show_fps;
                break;
        }
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Hide cursor
    fputs("\033[?25l", stdout);
    fflush(stdout);

    // Setup raw terminal for non-blocking keyboard input
    setup_raw_terminal();

    // Open webcam
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        cleanup();
        fprintf(stderr, "Error: Could not open webcam.\n");
        fprintf(stderr, "On macOS, check System Settings > Privacy & Security > Camera.\n");
        return 1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    auto frame_duration = std::chrono::microseconds(1000000 / TARGET_FPS);
    double fps = 0.0;

    while (running) {
        auto frame_start = std::chrono::steady_clock::now();

        // Handle keyboard input
        handle_input();

        cv::Mat frame;
        if (!cap.read(frame) || frame.empty()) continue;

        int cols, rows;
        get_terminal_size(cols, rows);

        std::string ascii = frame_to_ascii(frame, cols, rows);
        render_frame(ascii, fps);

        auto frame_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);

        // Sleep remainder of frame time
        if (elapsed < frame_duration) {
            usleep((frame_duration - elapsed).count());
        }

        auto total = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - frame_start);
        fps = 1000000.0 / total.count();
    }

    cap.release();
    cleanup();

    return 0;
}
