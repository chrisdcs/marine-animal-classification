// test.cpp : Defines the entry point for the console application.
//

#include"stdafx.h"
#include<iostream>
#include<fstream>
#include<numeric>
#include<math.h>
#include"opencv2\imgproc.hpp"
#include"opencv2\opencv.hpp"
#include"opencv2\highgui\highgui.hpp"
#include"opencv2\imgproc\imgproc.hpp"
#include"opencv2\imgcodecs.hpp"
#include"hungarian.hpp"

using namespace std;
using namespace cv;

double factorial(double n)
{
	return (n == 1 || n == 0) ? 1 : factorial(n - 1);
}

Mat_<double> gamma(double mu[], double k[], int ksize)//function for generating combined gamma kernel
{
	int col = ksize * 2 + 1;//for example, ksize = 50, then the kernel ranges from -50, 50
	int row = col;
	ksize = -ksize;
	int tmp = ksize;
	Mat_<double> NX(row, col);
	Mat_<double> NY(row, col);
	Mat_<double> NZ(row, col);
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			NX(i, j) = pow(ksize, 2);
			NY(j, i) = pow(ksize, 2);
			ksize++;
		}
		ksize = tmp;
	}

	Mat_<double> g1(row, col);
	Mat_<double> g2(row, col);
	Mat_<double> g3(row, col);
	Mat_<double> g4(row, col);
	Mat_<double> g(row, col);

	NZ = NX + NY;
	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			g1(i, j) = pow(mu[0], (k[0] + 1)) / (2 * CV_PI * factorial(k[0])) * pow(NZ(i, j), (k[0] - 1) / 2) * exp(-mu[0] * pow(NZ(i, j), 0.5));
		}
	}
	double s1 = sum(g1)[0];
	g1 = g1 / s1;//normalize the gamma kernel

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			g2(i, j) = pow(mu[1], (k[1] + 1)) / (2 * CV_PI * factorial(k[1])) * pow(NZ(i, j), (k[1] - 1) / 2) * exp(-mu[1] * pow(NZ(i, j), 0.5));
		}
	}
	double s2 = sum(g2)[0];
	g2 = g2 / s2;//normalize the gamma kernel

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			g3(i, j) = pow(mu[2], (k[2] + 1)) / (2 * CV_PI * factorial(k[2])) * pow(NZ(i, j), (k[2] - 1) / 2) * exp(-mu[2] * pow(NZ(i, j), 0.5));
		}
	}
	double s3 = sum(g3)[0];
	g3 = g3 / s3;//normalize the gamma kernel

	for (int i = 0; i < row; i++) {
		for (int j = 0; j < col; j++) {
			g4(i, j) = pow(mu[3], (k[3] + 1)) / (2 * CV_PI * factorial(k[3])) * pow(NZ(i, j), (k[3] - 1) / 2) * exp(-mu[3] * pow(NZ(i, j), 0.5));
		}
	}
	double s4 = sum(g4)[0];
	g4 = g4 / s4;//normalize the gamma kernel

	g = g1 - g2 + g1 - g3 + g1 - g4;
	return g;
}


Mat_<double> transfer(Mat input, Mat img) {
	Mat_<double> result(int(input.total()), 2);
	for (int i = 0; i < input.total(); i++) {
		result(i,0) = input.at<Point>(i).x;
		result(i,1) = input.at<Point>(i).y;
	}
	return result;
}

Mat_<double> dist2(Mat_<double> A, Mat_<double> B)
{
	int n1 = A.rows;
	int d1 = A.cols;
	int n2 = B.rows;
	int d2 = B.cols;

	Mat A1;
	//A.convertTo(A, CV_32F);
	//B.convertTo(B, CV_32F);
	Mat temp_A = A.clone();
	Mat temp_B = B.clone();
	transpose(temp_B, temp_B);
	pow(A, 2, A);
	reduce(A, A, 1, CV_REDUCE_SUM, CV_64F);
	transpose(A, A);
	pow(B, 2, B);
	reduce(B, B, 1, CV_REDUCE_SUM, CV_64F);
	transpose(B, B);
	Mat temp = Mat::ones(n2, 1, CV_64F)*A;
	transpose(temp, temp);
	A1 = temp + Mat::ones(n1, 1, CV_64F)*B - 2 * temp_A*temp_B;
	//cout << Mat::ones(n1, 1, CV_32F)*B;
	return abs(A1);
}

//columns of coordinates must be larger than 100!!
//This downsample function take coordinates of a shape equally spaced if # of coordinates are bigger than 100
Mat_<double> downsample(Mat_<double> coordinates)
{
	int nsamp = 100;
	Mat_<uchar> ind(1,nsamp);
	Mat_<double> new_coords(nsamp, 2);
	for (int i = 0; i < nsamp; i++)
	{
		ind(0, i) = int(round(i * coordinates.rows / nsamp));
		new_coords(i, 0) = coordinates(ind(0, i), 0);
		new_coords(i, 1) = coordinates(ind(0, i), 1);
	}
	return new_coords;
}

Mat_<double> theta(Mat_<double> coordinates)
{
	int nsamp = coordinates.rows;
	//coordinates.convertTo(coordinates, CV_32F);
	Mat_<double> c1(nsamp,1), c2(nsamp,1);
	Mat_<double> theta_array(nsamp, nsamp);
	for (int i = 0; i < nsamp; i++) {
		c1(i, 0) = coordinates(i, 0);
		c2(i, 0) = coordinates(i, 1);
	}
	Mat_<double> c1_t; Mat_<double> c2_t;
	transpose(c1, c1_t); transpose(c2, c2_t);
	Mat_<double> x = c2*Mat::ones(1,nsamp, CV_64F) - Mat::ones(nsamp,1, CV_64F) * c2_t;
	Mat_<double> y = c1 * Mat::ones(1, nsamp, CV_64F) - Mat::ones(nsamp, 1, CV_64F) * c1_t;

	
	for (int m = 0; m < nsamp; m++) {
		for (int n = 0; n < nsamp; n++) {
			theta_array(m, n) = atan2(x(m, n), y(m, n));
		}
	}
	transpose(theta_array, theta_array);
	return theta_array;
}

Mat_<double> rem(Mat_<double> theta_array, double num) {
	Mat_<double> residual(theta_array.size());
	for (int i = 0; i < theta_array.rows; i++) {
		for (int j = 0; j < theta_array.cols; j++) {
			if (theta_array(i, j) < num) {
				residual(i, j) = theta_array(i, j);
			}
			else
			{
				residual(i, j) = theta_array(i, j) - num;
			}
		}
	}
	return residual;
}

Mat_<double> quantize_theta(Mat_<double> theta_array_2, int nbins_theta) {
	Mat_<double> quantize(theta_array_2.size());
	for (int i = 0; i < theta_array_2.rows; i++) {
		for (int j = 0; j < theta_array_2.cols; j++) {
			quantize(i, j) = 1 + floor(theta_array_2(i,j) / (2*CV_PI/nbins_theta));
		}
	}
	return quantize;
}

Mat_<double> sc_compute(Mat_<double> nonZeroCoords) {
	int nsamp = nonZeroCoords.rows;
	int nbins_theta = 12;
	Mat_<double> dist_2 = dist2(nonZeroCoords, nonZeroCoords);
	Mat_<double> r_array;
	sqrt(dist_2, r_array);
	Mat_<double> theta_array;
	theta_array = theta(nonZeroCoords);
	Mat_<double> r_bin_edges(1, 5);
	//log scale
	r_bin_edges[0][0] = 0.125; r_bin_edges[0][1] = 0.25; r_bin_edges[0][2] = 0.5; r_bin_edges[0][3] = 1; r_bin_edges[0][4] = 2;
	Mat_<double> r_array_q = Mat::zeros(nsamp, nsamp, CV_8U);
	for (int m = 0; m < 4; m++) {
		for (int i = 0; i < nsamp; i++) {
			for (int j = 0; j < nsamp; j++) {
				r_array_q(i, j) = r_array_q(i, j) + int(r_array(i, j) < r_bin_edges(0, m));
			}
		}
	}
	//flag all points inside outer boundary
	Mat_<uchar> fz(r_array_q.size());
	for (int i = 0; i < nsamp; i++) {
		for (int j = 0; j < nsamp; j++) {
			fz(i, j) = r_array_q(i, j) > 0;
		}
	}
	//put all angles in [0,2*pi) range
	Mat_<double> theta_array_2(theta_array.size());
	theta_array_2 = rem(rem(theta_array, 2 * CV_PI) + 2 * CV_PI, 2 * CV_PI);
	//quantize thetas to a fixed set of angles
	Mat_<double> theta_array_q(theta_array.size());
	theta_array_q = quantize_theta(theta_array_2, nbins_theta);
	//cout << theta_array_q;
	int nbins = nbins_theta * 5;
	Mat_<double> BH = Mat::zeros(theta_array_q.rows, nbins_theta*5, CV_64F);
	for (int n = 0; n < theta_array_q.rows; n++) {
		for (int j = 0; j < theta_array_q.cols; j++) {
			int a = int(nbins_theta*(r_array_q(n, j) - 1) * int(fz(n, j)) + theta_array_q(n, j) * fz(n, j) - 1) * int(fz(n, j));
			BH(n, a) = BH(n, a) + 1;
		}
	}
	return BH;
}

Mat_<double> circshift(Mat_<double> BH) {
	int r = 5; int th = 12;
	Mat_<double> BH2(BH.size());
	for (int i = 0; i < BH.rows; i++) {
		for (int j = 0; j < BH.cols; j++) {
			if (j > 5) {
				BH2(i, j) = BH(i, j - 6);
			}
			else
			{
				BH2(i, j) = BH( 54 + j );
			}
		}
	}
	return BH2;
}

Mat_<double> fliplr(Mat_<double> BH) {
	Mat_<double> BH3(BH.size());
	for (int i = 0; i < BH.rows; i++) {
		for (int j = 0; j < BH.cols; j++) {
			BH3(i, j) = BH(i, int(floor(j / 12) * 12 + (12 - j % 12) - 1));
		}
	}
	return BH3;
}

void imadjust(const Mat1b& src, Mat1b& dst, int tol = 1, Vec2i in = Vec2i(0, 255), Vec2i out = Vec2i(0, 255)) {
	// src : input image
	// dst : output image
	// tol : tolerance, from 0 to 100
	// in : src image bounds
	// out: dst image bounds

	dst = src.clone();

	tol = max(0, min(100, tol));

	if (tol > 0)
	{
		// Compute in and out limits

		// Histogram
		vector<int> hist(256, 0);
		for (int r = 0; r < src.rows; ++r) {
			for (int c = 0; c < src.cols; ++c) {
				hist[src(r, c)]++;
			}
		}

		// Cumulative Histogram
		vector<int> cum = hist;
		for (int i = 1; i < hist.size(); ++i) {
			cum[i] = cum[i - 1] + hist[i];
		}

		// Compute bounds
		int total = src.rows * src.cols;
		int low_bound = total * tol / 100;
		int upp_bound = total * (100 - tol) / 100;
		in[0] = distance(cum.begin(), lower_bound(cum.begin(), cum.end(), low_bound));
		in[1] = distance(cum.begin(), lower_bound(cum.begin(), cum.end(), upp_bound));
	}

	// Sterching
	float scale = float(out[1] - out[0]) / float(in[1] - in[0]);
	for (int r = 0; r < dst.rows; ++r)
	{
		for (int c = 0; c < dst.cols; ++c)
		{
			int vs = max(int(src.at<uchar>(r, c)) - in[0], 0);
			int vd = min(int(vs * scale + 0.5f) + out[0], out[1]);
			dst.at<uchar>(r, c) = saturate_cast<uchar>(vd);
		}
	}

}
//Computing SC cost matrix: Size of BH1 and BH2 should be the same
Mat_<double> histcost(Mat_<double> BH1, Mat_<double> BH2) {
	Mat_<double> costmat(BH1.rows,BH2.rows);
	BH1 = BH1 / 100; BH2 = BH2 / 100;
	Mat_<double> temp; transpose(BH1 - BH2, temp);
	double a = 0;
	for (int i = 0; i < BH1.rows; i++) {
		for (int j = 0; j < BH2.rows; j++) {
			for (int k = 0; k < BH1.cols; k++) {
				a = a + pow(BH1(i, k) - BH2(j, k),2)/(BH1(i, k) + BH2(j, k) + 2.2204e-16);
			}
			costmat(i,j) = 0.5 * a;
			a = 0;
		}
	}
	return costmat;
}

Mat_<double> find_first_fifty(Mat_<double> costmat, int k) {
	int N1 = costmat.rows; int N2 = costmat.cols;
	Mat_<double> cvecAlt = Mat::zeros(1, N1,CV_64F);
	Mat_<double> Val = Mat::zeros(N1, 1, CV_64F);
	Mat_<double> rowInd = Mat::zeros(1, k, CV_64F);
	Mat_<double> colInd = Mat::zeros(1, k, CV_64F);
	Mat_<double> Val_sorted = Mat::zeros(1, k, CV_64F);
	for (int i = 0; i < k; i++) {

	}
	return Val_sorted;
}

//This function find the location of min value of Val_cost matrix
int minLoc(Mat_<double> Val_cost) {
	int ind = 0; double min_val = Val_cost[0][0];
	for (int i = 1; i < 4; i++) {
		if (Val_cost[i][0] < min_val) {
			min_val = Val_cost[i][0];
			ind = i;
		}
	}
	return ind;
}

vector<vector <double>> array_to_matrix(vector<double> m, int rows, int cols) {
	vector<vector <double>> r;
	r.resize(rows, vector<double>(cols, 0));
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			r[i][j] = m[i*cols + j];
		}
	}
	return r;
}

int main()
{
	//gamma kernel parameter mu: decay speed
	double mu[4] = { 1.0,1.0,1.0,1.0 };
	//gamma kernel parameter k: controll where kernel is centered
	double k[4] = { 1.0,20.0,30.0,40.0 };
	//kernel size m
	int m = 50;
	double f;

	//declare vaiables
	Mat im, out_im, gaussx, gaussy, gauss, saliency;
	Point anchor;
	double delta;
	int ddepth;
	ifstream P1;
	double t;
	int maxvalue = 1;
	double alpha = 2;

	//Initialize arguments for kernel convolution
	anchor = Point(-1, -1);
	delta = 0;
	ddepth = -1;

	//Gamma kernel
	Mat_<double> g;
	g = gamma(mu, k, m);

	//Generate Gaussian 2D Kernel
	gaussx = getGaussianKernel(26, 0.2 * 26, CV_64F);
	gaussy = getGaussianKernel(26, 0.2 * 26, CV_64F);
	gauss = gaussx * gaussy.t();

	//load p1: Give more weights for center area
	P1.open("p1.txt");
	Mat p1 = Mat::zeros(128, 171, CV_64F);
	int cnt = 0;
	while (P1 >> f) {
		int tmprow = cnt / 171;
		int tmpcol = cnt % 171;
		p1.at<double>(tmprow, tmpcol) = f;//give P1 value to Mat p1
		cnt++;
	}
	P1.close();

	/***********************************************************Load Template Data*************************************************************/
	int num_barra = 2;
	int num_turtle = 4;

	vector<Mat_<double>> barra_temp(num_barra);
	vector<Mat_<double>> turtle_temp(num_turtle);

	//Mat barra_templ = Mat::zeros(num_barra*2, 100, CV_64F);
	//Mat turtle_templ = Mat::zeros(num_turtle*4, 100, CV_64F);

	Mat_<double> temp1(2,100); Mat_<double> temp2(2, 100);

	ifstream barra;
	ifstream turtle;
	barra.open("barra.txt");
	turtle.open("turtle.txt");

	int tmprow; int tmpcol; cnt = 0;

	while (barra >> f) {
		if (cnt < 200) {
			tmprow = int(floor(cnt / 100));
			tmpcol = cnt % 100;
			temp1(tmprow, tmpcol) = f;//give barra value to Mat barra_templ
			cnt++;
		}
		if (cnt < 400 && cnt >= 200)
		{
			tmprow = int(floor(cnt / 100)) - 2;
			tmpcol = cnt % 100;
			temp2(tmprow, tmpcol) = f;//give barra value to Mat barra_templ
			cnt++;
		}
	}
	Mat_<double> tempr;
	transpose(temp1, tempr); barra_temp[0] = tempr.clone(); transpose(temp2, tempr); barra_temp[1] = tempr.clone();
	
	//cout << barra_temp[1];
	Mat_<double> temp3(2, 100); Mat_<double> temp4(2, 100);
	cnt = 0;
	while (turtle >> f) {
		if (cnt < 200) {
			tmprow = int(floor(cnt / 100));
			tmpcol = cnt % 100;
			temp1(tmprow, tmpcol) = f;//give barra value to Mat barra_templ
			//barra_temp[0](tmprow, tmpcol) = f;
			cnt++;
		}
		if (cnt < 400 && cnt >= 200)
		{
			tmprow = int(floor(cnt / 100)) - 2;
			tmpcol = cnt % 100;
			temp2(tmprow, tmpcol) = f;//give barra value to Mat barra_templ
			cnt++;
		}
		if (cnt < 600 && cnt >= 400)
		{
			tmprow = int(floor(cnt / 100)) - 4;
			tmpcol = cnt % 100;
			temp3(tmprow, tmpcol) = f;//give barra value to Mat barra_templ
			cnt++;
		}
		if (cnt < 800 && cnt >= 600)
		{
			tmprow = int(floor(cnt / 100)) - 6;
			tmpcol = cnt % 100;
			temp4(tmprow, tmpcol) = f;//give barra value to Mat barra_templ
			cnt++;
		}
		transpose(temp1, tempr); turtle_temp[0] = tempr.clone(); transpose(temp2, tempr); turtle_temp[1] = tempr.clone(); 
		transpose(temp3, tempr); turtle_temp[2] = tempr.clone(); transpose(temp3, tempr); turtle_temp[3] = tempr.clone();
	}
	vector<Mat_<double>> BH_temp_barra(num_barra);vector<Mat_<double>> BH_temp_turtle(num_turtle);

	for (int i = 0; i < num_barra; i++) {
		BH_temp_barra[i] = sc_compute(barra_temp[i]);
	}
	for (int j = 0; j < num_turtle; j++) {
		BH_temp_turtle[j] = sc_compute(turtle_temp[j]);
	}

	/*******************************************Below is used for testing 2D Gamma Kernel on videos********************************************/
	string filename = "tx3.avi";
	VideoCapture capture(filename);
	Mat1b frame;//used for saving every new frame in video sequence
	Mat temp;//temp is for drawing bounding box

	for (; ; )
	{
		capture >> frame;
		if (frame.empty())
			break;
		//RGB2Gray
		Mat1b gray(frame.size(), CV_64FC1);
		cvtColor(frame, gray, CV_BGR2GRAY);
		
		imadjust(gray, gray);

		//Apply Gamma Kernel
		Mat out1_im;
		filter2D(gray, out1_im, ddepth, g, anchor, delta, BORDER_CONSTANT);
		out1_im = abs(out1_im);
		//Apply Gaussian Kernel
		Mat saliency1;
		filter2D(out1_im, saliency1, ddepth, gauss, anchor, delta, BORDER_CONSTANT);

		//Multiply p1: Center Prior (More details are available in paper:
		//Predicting visual attention using gamma kernels, IEEE, ICCASP, R. Burt)
		//or this center prior can be excluded and this code still works
		resize(p1, p1, saliency1.size(), 0.5, 0.5, INTER_LINEAR);
		saliency1.convertTo(saliency1, CV_64F);
		Mat sal_map1 = Mat::zeros(saliency1.size().height, saliency1.size().width, CV_64F);
		for (int i = 0; i < saliency1.size().height; i++) {
			for (int j = 0; j < saliency1.size().width; j++) {
				sal_map1.at<double>(i, j) = pow(saliency1.at<double>(i, j) * p1.at<double>(i, j), alpha);
			}
		}
		//Normalize Saliency
		double min1, max1;
		minMaxLoc(sal_map1, &min1, &max1);
		sal_map1 = (sal_map1 - min1) / (max1 - min1);

		//Get Threshold Map
		Mat th1;
		Scalar tempVal = mean(sal_map1);
		t = tempVal.val[0]*3;
		threshold(sal_map1, th1, t, maxvalue, THRESH_BINARY);
		imshow("threshold", th1);
		th1.convertTo(th1, CV_8U);
		
		//Drawing bouding box
		//Contours
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(th1, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));
		vector<vector<Point>> contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());

		for (size_t i = 0; i < contours.size(); i++)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}

		//Copy frames to temp to draw bounding box
		frame.copyTo(temp);

		Point pt1, pt2;
		for (size_t i = 0; i < contours.size(); i++)
		{
			int extension = 15;
			Scalar color = Scalar(100, 200, 30);
			pt1.x = boundRect[i].x - extension;
			pt1.y = boundRect[i].y - extension;
			pt2.x = boundRect[i].x + boundRect[i].width + extension;
			pt2.y = boundRect[i].y + boundRect[i].height + extension;
			rectangle(temp, pt1, pt2, color, 1, 8, 0);
		}

		//GrabCut Algorithm For segmentation: Pretty Slow
		Mat mask(frame.size(),CV_8UC1);
		Mat bgModel, fgdModel; Mat color; Mat srcBlur;
		cvtColor(gray, color, cv::COLOR_GRAY2BGR);
		blur(color, srcBlur, Size(3, 3));
		grabCut(srcBlur,
				mask,//Segmentation Result Mask
				boundRect[0],
				bgModel, fgdModel,
				3,
				GC_INIT_WITH_RECT);
		compare(mask, GC_PR_FGD, mask, CMP_EQ);

		//Image Closing by morphology
		int morph_size = 4;
		Mat element_m = getStructuringElement(MORPH_ELLIPSE, Size(2 * morph_size + 1, 2 * morph_size + 1),
			Point(morph_size, morph_size));
		morphologyEx(mask, mask, 3, element_m);
		
		vector<vector<Point>> mask_coordinate;
		vector<Vec4i> hierarchy1;

		Mat canny_output; 
		int thresh = 1;
		
		Canny(mask, canny_output, thresh, thresh * 2, 3); 
		imshow("c", canny_output);
		

		Mat nonZeroCoordinates;
		//may need to flip y coordinates
		findNonZero(canny_output, nonZeroCoordinates);
		//nonZeroCoordinates.convertTo(nonZeroCoordinates, CV_64FC2);
		Mat_<double> nonZeroCoords = transfer(nonZeroCoordinates, frame);
		normalize(nonZeroCoords,nonZeroCoords,0,1,NORM_MINMAX);
		//cout << nonZeroCoords;
		if (nonZeroCoords.rows >= 100) {
			nonZeroCoords = abs(downsample(nonZeroCoords));
		
			/*******************************************************Compute Shape Context************************************************************/
			vector<Mat_<double>> BH_querry(4);
			BH_querry[0] = sc_compute(nonZeroCoords);
			BH_querry[1] = circshift(BH_querry[0]);
			BH_querry[2] = fliplr(BH_querry[0]);
			BH_querry[3] = circshift(BH_querry[2]);
			
			vector<Mat_<double>> costmat_alt(4);
			Mat_<double> Val_cost(4,1);
			for (int i = 0; i < 4; i++) {
				costmat_alt[i] = histcost(BH_temp_barra[0],BH_querry[i]);//histcost function is slow!!!!!
				Val_cost[i][0] = sum(costmat_alt[i])[0];
				//cout << Val_cost[i][0] << '\t';
			}
			//double array[] = costmat_alt[0].data;
			int ind = minLoc(Val_cost);
			Mat_<double> costmat = costmat_alt[ind] / 200;
			vector<double> costmat_feed;
			//transfer costmat into a vector for hungarian/LAPJV assignment algorithm
			costmat_feed.assign(costmat.begin(), costmat.end());
			vector<vector<double>> cost_mat;
			cost_mat = array_to_matrix(costmat_feed, 100, 100);
			for (int i = 0; i < 100; i++) {
				for (int j = 0; j < 100; j++) {
					cout << cost_mat[i][j] << " ";
				}
				cout << ";"<<'\n';
			}
			//Hungarian hungarian(cost_mat, 100, 100, HUNGARIAN_MODE_MINIMIZE_COST);
			//hungarian.print_assignment();
			//hungarian.print_cost();
		}
		else
		{
			printf("Segmentation Failure Or Nothing Detected");
		}
		imshow("adjusted", srcBlur);
		imshow("saliency",sal_map1);
		imshow("mask", mask);
		imshow("Bounding Box", temp);
		//cout << nonZeroCoordinates;
		//cout << canny_output.size();
		waitKey(0);
		
	}
	return 0;
}