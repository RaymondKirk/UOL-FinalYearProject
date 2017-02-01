#include <iostream>
#include "DICOM.h"

DICOM::DICOM(std::string path)
{
    this->isVerbose = false;
    filePath = path;
    this->print("Set filepath to: " + path);
}

DICOM::DICOM(std::string path, bool verbose)
{
    this->isVerbose = verbose;
    this->filePath = path;
    this->print("Set filepath to: " + path);
    captureVideoFrames();
}

DICOM::~DICOM() {
    this->print("Object being deleted (DICOM)");
}

void DICOM::captureVideoFrames()
{
    cv::VideoCapture capture(this->filePath);
    cv::Mat frame;

    if (capture.isOpened())
    {
        do
        {
            capture.read(frame);
            this->frames.push_back(frame.clone());

        } while (!frame.empty());
    }

    capture.release();

    this->frameCount = this->frames.size();

    std::string vectorSize = std::to_string(this->frameCount);
    this->print(vectorSize + " frames saved from: " + this->filePath);
}

cv::Mat DICOM::getFrame(int frameNumber)
{
    return this->frames.at(frameNumber);
}

void DICOM::playFrames(u_int from, u_int to, u_int frameRate)
{
    if (!(from < to && (to < this->frameCount && to > from)))
    {
        this->print("Invalid frame ranges" + from + to);
        return;
    }
    else if (this->frames.empty())
    {
        this->print("Invalid Frame Object");
        return;
    }

    u_int currentFrame = from;
    u_int frameDiff = to - currentFrame;
    u_int fpsTimeout = static_cast<u_int>((1000 / frameRate) + 0.5);

    std::string title{"Playing frames: " + std::to_string(from) + "->" + std::to_string(to)};
    std::string remainingTime{"ETC:" + std::to_string((frameDiff * fpsTimeout) / 100) + "s"};

    this->print(title);
    this->print(remainingTime);

    std::string videoWindow = this->filePath;
    cv::namedWindow(videoWindow, cv::WINDOW_AUTOSIZE);
    
    while (currentFrame < to)
    {
        frameDiff = to - currentFrame;
        remainingTime = "Estimated time to completion: " +
                        std::to_string((frameDiff * fpsTimeout) / 1000) + "s";

        cv::imshow(videoWindow, this->getFrame(currentFrame++));
        cv::setWindowTitle(videoWindow, remainingTime);
        cv::waitKey(50);
    }

    cv::destroyWindow(this->filePath);
}

void DICOM::print(std::string message)
{
    if (this->isVerbose)
        std::cout << message << std::endl;
}