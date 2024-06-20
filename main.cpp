#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;
using namespace cv;

class V4L2Capture {
public:
  V4L2Capture(const string &devicePath, int width, int height)
      : devicePath(devicePath), width(width), height(height), fd(-1) {}

  ~V4L2Capture() { close(); }

  bool open() {
    fd = ::open(devicePath.c_str(), O_RDWR);
    if (fd < 0) {
      perror("failed to open device");
      return false;
    }
    return true;
  }

  bool init() {
    return setFormat() && requestBuffers() && queryBuffer() && mapBuffer() &&
           startCapture();
  }

  bool grabFrame(Mat &frame) {
    if (!enqueueBuffer() || !dequeueBuffer()) {
      return false;
    }

    frame =
        imdecode(Mat(1, bufferInfo.bytesused, CV_8UC1, buffer), IMREAD_COLOR);
    return true;
  }

  void close() {
    if (fd >= 0) {
      if (ioctl(fd, VIDIOC_STREAMOFF, &bufferInfo.type) < 0) {
        perror("could not end streaming");
      }

      if (buffer != nullptr) {
        munmap(buffer, bufferInfo.length);
        buffer = nullptr;
      }

      ::close(fd);
      fd = -1;
    }
  }

private:
  bool setFormat() {
    v4l2_format imageFormat = {};
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width = width;
    imageFormat.fmt.pix.height = height;
    imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    imageFormat.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0) {
      perror("device could not set format");
      return false;
    }
    return true;
  }

  bool requestBuffers() {
    v4l2_requestbuffers requestBuffer = {};
    requestBuffer.count = 1;
    requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffer.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0) {
      perror("could not request buffer from device");
      return false;
    }
    return true;
  }

  bool queryBuffer() {
    bufferInfo = {};
    bufferInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferInfo.memory = V4L2_MEMORY_MMAP;
    bufferInfo.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &bufferInfo) < 0) {
      perror("device did not return the buffer information");
      return false;
    }
    return true;
  }

  bool mapBuffer() {
    buffer = (char *)mmap(nullptr, bufferInfo.length, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, bufferInfo.m.offset);
    if (buffer == MAP_FAILED) {
      perror("failed to map device memory");
      return false;
    }
    return true;
  }

  bool startCapture() {
    if (ioctl(fd, VIDIOC_STREAMON, &bufferInfo.type) < 0) {
      perror("could not start streaming");
      return false;
    }
    return true;
  }

  bool enqueueBuffer() {
    if (ioctl(fd, VIDIOC_QBUF, &bufferInfo) < 0) {
      perror("could not queue buffer");
      return false;
    }
    return true;
  }

  bool dequeueBuffer() {
    if (ioctl(fd, VIDIOC_DQBUF, &bufferInfo) < 0) {
      perror("could not dequeue buffer");
      return false;
    }
    return true;
  }

  string devicePath;
  int width, height;
  int fd;
  v4l2_buffer bufferInfo;
  char *buffer = nullptr;
};

int main() {
  V4L2Capture capture("/dev/video0", 1024, 1024);
  if (!capture.open() || !capture.init()) {
    return 1;
  }

  namedWindow("Webcam", WINDOW_AUTOSIZE);

  while (true) {
    Mat frame;
    if (!capture.grabFrame(frame)) {
      break;
    }

    Mat mirroredFrame;
    flip(frame, mirroredFrame, 1);

    imshow("Webcam", mirroredFrame);

    if (waitKey(1) >= 0) {
      break;
    }
  }

  destroyAllWindows();

  return 0;
}