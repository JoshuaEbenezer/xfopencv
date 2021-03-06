/***************************************************************************
 Copyright (c) 2018, Xilinx, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/

#include "xf_headers.h"
#include "xf_erosion_config.h"


int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat out_img,ocv_ref;
	cv::Mat in_img,diff;


	// reading in the input image
	in_img = cv::imread(argv[1], 0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", argv[1]);
		return 0;
	}

	// create memory for output images
	ocv_ref.create(in_img.rows,in_img.cols,in_img.depth());
	out_img.create(in_img.rows,in_img.cols,in_img.depth());
	diff.create(in_img.rows,in_img.cols,in_img.depth());


	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(in_img.rows,in_img.cols);

	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_img.rows,in_img.cols);


	imgInput.copyTo(in_img.data);


	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	erosion_accel(imgInput, imgOutput);

	#if __SDSCC__
	hw_ctr.stop();

	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	xf::imwrite("hls_out.jpg",imgOutput);

	///////////////// 	Opencv  Reference  ////////////////////////
	cv::Mat element = cv::getStructuringElement( 0,cv::Size(3, 3), cv::Point(-1, -1));
	cv::erode(in_img, ocv_ref, element, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT);
	cv::imwrite("out_ocv.jpg", ocv_ref);

	//////////////////  Compute Absolute Difference ////////////////////

	xf::absDiff(ocv_ref, imgOutput, diff);
	cv::imwrite("out_error.jpg", diff);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=1; i<in_img.rows-1; i++)
	{
		for(int j=1; j<in_img.cols-1; j++)
		{
			unsigned char v = diff.at<unsigned char>(i,j);
			if (v>0)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);
	
	if(err_per > 0.0f)
		return 1;

	return 0;
}
