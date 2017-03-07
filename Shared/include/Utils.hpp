﻿#include <string>
#include <vector>
#include <algorithm>


namespace Util {
	inline float square(float x) {
		return x * x;
	}

	inline float euclideanDistance(int x2, int x1, int y2, int y1) {
		return sqrt((float)(square(x2 - x1) + square(y2 - y1)));
	}

	inline float euclideanDistance(cv::Point a, cv::Point b) {
		return sqrt((float)(square(b.x - a.x) + square(b.y - a.y)));
	}

	void getFactors(std::vector<int> & factors, int number) {
		factors.push_back(1);
		factors.push_back(number);
		for (int i = 2; i * i <= number; ++i)
		{
			if (number % i == 0)
			{
				factors.push_back(i);
				if (i * i != number) {
					factors.push_back(number / i);
				}
			}
		}
	}

	std::vector<int> getBlockSizes(int w, int h) {
		std::vector<int> wF, hF, c;
		getFactors(wF, w);
		getFactors(hF, h);

		std::sort(wF.begin(), wF.end());
		std::sort(hF.begin(), hF.end());

		std::set_intersection(wF.begin(), wF.end(), hF.begin(), hF.end(), std::back_inserter(c));

		return c;
	}

	template<typename T>
	void drawMotionVectors(cv::Mat &canvas, T *& motionVectors, unsigned int wB, unsigned int hB, int blockSize,
		cv::Scalar rectColour = cv::Scalar(255), cv::Scalar lineColour = cv::Scalar(0, 255, 255)) {
		for (size_t i = 0; i < wB; i++)
		{
			for (size_t j = 0; j < hB; j++)
			{
				//Calculate repective position of motion vector
				int idx = i + j * wB;

				//Offset drawn point to represent middle rather than top left of block
				cv::Point offset(blockSize / 2, blockSize / 2);
				cv::Point pos(i * blockSize, j * blockSize);
				cv::Point mVec(motionVectors[idx].x, motionVectors[idx].y);

				//cv::rectangle(canvas, pos, pos + cv::Point(blockSize, blockSize), rectColour);
				cv::arrowedLine(canvas, pos + offset, mVec + offset, lineColour);
			}
		}
	}

	
	template<typename T>
	void visualiseMotionVectors(cv::Mat &canvas, T *& motionVectors, unsigned int wB, unsigned int hB, int blockSize, 
		int thresh = 1, float min_len = 0.0) {

		cv::Mat mask;
		cv::cvtColor(canvas, mask, cv::COLOR_RGB2GRAY);
		threshold(mask, mask, thresh, 255, 0);

		cv::Mat colour_image = canvas.clone(), dst;
		float max_len = euclideanDistance(cv::Point(0, 0), cv::Point(blockSize, blockSize));

		for (size_t i = 0; i < wB; i++)
		{
			for (size_t j = 0; j < hB; j++)
			{
				//Calculate repective position of motion vector
				int idx = i + j * wB;

				//Offset drawn point to represent middle rather than top left of block
				cv::Point offset(blockSize / 2, blockSize / 2);
				cv::Point pos(i * blockSize, j * blockSize);
				cv::Point mVec(motionVectors[idx].x, motionVectors[idx].y);

				float len = (motionVectors[idx].w / max_len);
				if (len >= min_len) {
					float angle = motionVectors[idx].z;
					cv::rectangle(colour_image, pos, pos + cv::Point(blockSize, blockSize), HSVToBGR(angle, len, 1), CV_FILLED);
					//cv::putText(colour_image, std::to_string((int)angle), pos, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255));
					//std::cout << "A:" << angle << ", L:" << len << std::endl;
				}
			}
		}

		for (int i = 0; i < wB * blockSize; i++) {
			for (int j = 0; j < hB * blockSize; j++) {
				if (mask.at<uchar>(j, i) > 0) {
					//Simple alpha blending 50/50
					canvas.at<cv::Vec3b>(j, i) = AlphaBlend(canvas.at<cv::Vec3b>(j, i), colour_image.at<cv::Vec3b>(j, i), 0.4);
				}
			}
		}
		
		//cv::addWeighted(canvas, 0.5, colour_image, 0.5, 0.0, canvas);
	}

	cv::Vec3b AlphaBlend(cv::Vec3b a, cv::Vec3b b, float alpha = 0.5) {
		float beta = 1 - alpha;
		return (alpha * a) + (beta * b);
	}

	//Based on https://link.springer.com/book/10.1007/b138805
	cv::Scalar HSVToBGR(float h, float s, float v) //H[0, 360], S[0, 1], V[0, 1]
	{
		cv::Scalar out;
		double P, Q, T, fr;

		(h == 360.) ? (h = 0.) : (h /= 60.);
		fr = h - floor(h);

		P = v*(1. - s);
		Q = v*(1. - s*fr);
		T = v*(1. - s*(1. - fr));

		if (0. <= h && h < 1.)
			out = cv::Scalar(P, T, v, s);
		else if (1. <= h && h < 2.)
			out = cv::Scalar(P, v, Q, s);
		else if (2. <= h && h < 3.)
			out = cv::Scalar(P, v, T, s);
		else if (3. <= h && h < 4.)
			out = cv::Scalar(P, Q, v, s);
		else if (4. <= h && h < 5.)
			out = cv::Scalar(v, P, T, s);
		else if (5. <= h && h < 6.)
			out = cv::Scalar(v, P, Q, s);
		else
			out = cv::Scalar(0, 0, 0, 0);

		out *= 255;

		return out;
	}

	//TODO create custom struct for storing values or combine both targets
	//Has to be defined here because OPENCL isnt supported in both targets
	void drawMotionVectorsVec4(cv::Mat &canvas, cv::Vec4f *& motionVectors, unsigned int wB, unsigned int hB, int blockSize,
		cv::Scalar rectColour = cv::Scalar(255), cv::Scalar lineColour = cv::Scalar(0, 255, 255)) {
		for (size_t i = 0; i < wB; i++)
		{
			for (size_t j = 0; j < hB; j++)
			{
				//Calculate repective position of motion vector
				int idx = i + j * wB;

				//Offset drawn point to represent middle rather than top left of block
				cv::Point offset(blockSize / 2, blockSize / 2);
				cv::Point pos(i * blockSize, j * blockSize);
				cv::Point mVec(motionVectors[idx][0], motionVectors[idx][1]);

				//cv::rectangle(canvas, pos, pos + cv::Point(blockSize, blockSize), rectColour);
				cv::arrowedLine(canvas, pos + offset, mVec + offset, lineColour);
			}
		}
	}

	//Has to be defined here because OPENCL isnt supported in both targets
	void visualiseMotionVectorsVec4(cv::Mat &canvas, cv::Vec4f *& motionVectors, unsigned int wB, unsigned int hB, int blockSize,
		int thresh = 1, float min_len = 0.0) {

		cv::Mat mask;
		cv::cvtColor(canvas, mask, cv::COLOR_RGB2GRAY);
		threshold(mask, mask, thresh, 255, 0);

		cv::Mat colour_image = canvas.clone(), dst;
		float max_len = euclideanDistance(cv::Point(0, 0), cv::Point(blockSize, blockSize));

		for (size_t i = 0; i < wB; i++)
		{
			for (size_t j = 0; j < hB; j++)
			{
				//Calculate repective position of motion vector
				int idx = i + j * wB;

				//Offset drawn point to represent middle rather than top left of block
				cv::Point offset(blockSize / 2, blockSize / 2);
				cv::Point pos(i * blockSize, j * blockSize);
				cv::Point mVec(motionVectors[idx][0], motionVectors[idx][1]);

				float len = (motionVectors[idx][3] / max_len);
				if (len >= min_len) {
					float angle = motionVectors[idx][2];
					cv::rectangle(colour_image, pos, pos + cv::Point(blockSize, blockSize), HSVToBGR(angle, len, 1), CV_FILLED);
					//cv::putText(colour_image, std::to_string((int)angle), pos, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.4, cv::Scalar(255, 255, 255));
					//std::cout << "A:" << angle << ", L:" << len << std::endl;
				}
			}
		}

		for (int i = 0; i < wB * blockSize; i++) {
			for (int j = 0; j < hB * blockSize; j++) {
				if (mask.at<uchar>(j, i) > 0) {
					//Simple alpha blending 50/50
					canvas.at<cv::Vec3b>(j, i) = AlphaBlend(canvas.at<cv::Vec3b>(j, i), colour_image.at<cv::Vec3b>(j, i), 0.4);
				}
			}
		}

		//cv::addWeighted(canvas, 0.5, colour_image, 0.5, 0.0, canvas);
	}

	void drawText(cv::Mat& canvas, std::string f, std::string bS, std::string processed_fps, std::string rendered_fps, cv::Scalar colour = cv::Scalar(255, 255, 255)) {
		std::string content("Frame " + f + ", Block Size: " + bS + ", Processed FPS: " + processed_fps + ", Rendered FPS: " + rendered_fps);
		cv::putText(canvas, content, cv::Point(0, canvas.size().height - 1), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.6, colour);
	}

	void MouseCallback(int event, int x, int y, int flags, void* userdata)
	{
		using namespace cv;
		using namespace std;

		if (event == EVENT_LBUTTONDOWN)
		{
			cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
		}
		else if (event == EVENT_RBUTTONDOWN)
		{
			cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
		}
		else if (event == EVENT_MBUTTONDOWN)
		{
			cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
		}
		else if (event == EVENT_MOUSEMOVE)
		{
			cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

		}
	}
}