#include "ZQ_CNN_Net.h"
#if defined(_WIN32)
#include "ZQlib/ZQ_PutTextCN.h"
#endif
#include <vector>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "ZQ_CNN_CompileConfig.h"
#if ZQ_CNN_USE_BLAS_GEMM
#include <openblas/cblas.h>
#pragma comment(lib,"libopenblas.lib")
#elif ZQ_CNN_USE_MKL_GEMM
#include <mkl/mkl.h>
#pragma comment(lib,"mklml.lib")
#else
#pragma comment(lib,"ZQ_GEMM.lib")
#endif

using namespace ZQ;
using namespace std;
using namespace cv;

int main()
{
	int num_threads = 1;

#if ZQ_CNN_USE_BLAS_GEMM
	openblas_set_num_threads(num_threads);
#elif ZQ_CNN_USE_MKL_GEMM
	mkl_set_num_threads(num_threads);
#endif

	ZQ_CNN_Net net1, net2;
	if (!net1.LoadFrom("model/det5-dw48-normal.zqparams", "model/det5-dw48-normal-5000.nchwbin")
		|| !net2.LoadFrom("model/det3.zqparams", "model/det3_bgr.nchwbin"))
		//|| !net2.LoadFrom("model/det4-dw48-1.zqparams", "model/det4-dw48-1-7.nchwbin"))
	{
		cout << "failed to load model\n";
		return EXIT_FAILURE;
	}
	printf("num_MulAdd = %.1f M\n", net1.GetNumOfMulAdd() / (1024.0*1024.0));
	printf("num_MulAdd = %.1f M\n", net2.GetNumOfMulAdd() / (1024.0*1024.0));
	int net1_H, net1_W, net1_C;
	int net2_H, net2_W, net2_C;
	net1.GetInputDim(net1_C, net1_H, net1_W);
	net2.GetInputDim(net2_C, net2_H, net2_W);
	Mat img = imread("data/onet.jpg", 1);
	if (img.empty())
	{
		cout << "failed to load image\n";
		return EXIT_FAILURE;
	}
	if (img.channels() == 1)
		cv::cvtColor(img, img, CV_GRAY2BGR);
	Mat img1, img2;
	cv::resize(img, img1, cv::Size(net1_W, net1_H));
	cv::resize(img, img2, cv::Size(net2_W, net2_H));
	Mat draw_img;
	cv::resize(img, draw_img, cv::Size(960,960));

	ZQ_CNN_Tensor4D_NHW_C_Align128bit input1, input2;
	input1.ConvertFromBGR(img1.data, img1.cols, img1.rows, img1.step[0]);
	input2.ConvertFromBGR(img2.data, img2.cols, img2.rows, img2.step[0]);
	int nIters = 1;
	double t1 = omp_get_wtime();
	for (int i = 0; i < nIters; i++)
		net1.Forward(input1);
	double t2 = omp_get_wtime();
	for (int i = 0; i < nIters; i++)
		net2.Forward(input2);
	double t3 = omp_get_wtime();
	printf("net1 %.3f s / %d = %.3f ms\n", t2 - t1, nIters, 1000 * (t2 - t1) / nIters);
	printf("net2 %.3f s / %d = %.3f ms\n", t3 - t2, nIters, 1000 * (t3 - t2) / nIters);

	const ZQ_CNN_Tensor4D* landmark1 = net1.GetBlobByName("conv6-3");
	const ZQ_CNN_Tensor4D* landmark2 = net2.GetBlobByName("conv6-3");
	if (landmark1 == 0 || landmark2 == 0)
	{
		cout << "failed to get blob conv6-3\n";
		return EXIT_FAILURE;
	}
	const float* landmark1_data = landmark1->GetFirstPixelPtr();
	const float* landmark2_data = landmark2->GetFirstPixelPtr();
	for (int i = 0; i < 106; i++)
	{
		char buf[10];
#if defined(_WIN32)
		sprintf_s(buf,10, "%d", i);
#else
		sprintf(buf, "%d", i);
#endif
		cv::Point pt = cv::Point(960 * landmark1_data[i * 2], 960 * landmark1_data[i * 2 + 1]);
#if defined(_WIN32)
		ZQ_PutTextCN::PutTextCN(draw_img, buf, pt, cv::Scalar(100, 0, 0), 12);
#endif
		cv::circle(draw_img, pt, 2, cv::Scalar(0, 0, 250), 2);
	}
	/*for (int i = 0; i < 5; i++)
	{
		cv::circle(draw_img, cv::Point(960 * landmark2_data[i], 960 * landmark2_data[i + 5]), 2, cv::Scalar(0, 120 + 30 * i, 0), 2);
	}*/

	namedWindow("landmark");

	imshow("landmark", draw_img);
	cv::imwrite("landmark.jpg", draw_img);
	waitKey(0);
	return EXIT_SUCCESS;
}