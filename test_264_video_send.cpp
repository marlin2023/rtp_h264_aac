/*
 Here's a small IPv4 example: it asks for a portbase and a destination and
 starts sending packets to that destination.
 */

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

extern "C"{

#ifndef   UINT64_C

#define   UINT64_C(value)__CONCAT(value,ULL)

#endif
}
extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
//#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
//#include <libavutil/avutil.h>


using namespace jrtplib;

//
// This function checks if there was a RTP error. If so, it displays an error
// message and exists.
//

void checkerror(int rtperr) {
	if (rtperr < 0) {
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		exit(-1);
	}
}

//
// The main routine
//

int main(int argc ,char * argv[]) {
	RTPSession sess;
	uint16_t portbase, destport;
	uint32_t destip;
	std::string ipstr;
	int status, i, num;

//        // First, we'll ask for the necessary information
	portbase = 6000;
	destip = inet_addr("192.168.1.16");
	if (destip == INADDR_NONE) {
		std::cerr << "Bad IP address specified" << std::endl;
		return -1;
	}

	// The inet_addr function returns a value in network byte order, but
	// we need the IP address in host byte order, so we use a call to
	// ntohl
	destip = ntohl(destip); // destination ip address
	destport = 6666; // destination port address

	num = 10000000;

	// Now, we'll create a RTP session, set the destination, send some
	// packets and poll for incoming data.

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// In this case, we'll be sending 10 samples each second, so we'll
	// put the timestamp unit to (1.0/10.0)
	sessparams.SetOwnTimestampUnit(1.0 / 10.0); //must be set

	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(portbase); //portbase = 6000
	status = sess.Create(sessparams, &transparams);
	checkerror(status);

	RTPIPv4Address addr(destip, destport);

	status = sess.AddDestination(addr);
	checkerror(status);


	printf("over the jrtplib init function ...\n");
	/* ================   following ,ffmpeg read h.264 data     ================= */
	AVFormatContext * pFormatCtx; //Format I/O context.
	int  videoStream;
	AVCodecContext * pCodecCtx; //

	AVFrame * pFrame;
	AVPacket packet;

	if (argc < 2) {
		printf("Please provide a movie file\n");
		return -1;
	}

	//Register all formats and codecs
	av_register_all();

	pFormatCtx = NULL;
	// Open video file
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) {

		printf("av_open_input_file failed \n");
		exit(1);
	}

	//	if(av_find_stream_info(pFormatCtx) < 0){
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Couldn't find stream information\n");
		exit(1);
	}

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		printf("Didn't find a video stream \n");
		exit(1);
	}

	pFrame = avcodec_alloc_frame();
	if (pFrame == NULL) {
		printf("pFrame allocate failed\n");
		exit(1);
	}

	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {

			printf("5_____\n");
			// send the packet
			status = sess.SendPacket((void *) "1234567890", 10, 0, false, 10);
			checkerror(status);
		} //endifif(frameFinished)

		av_free_packet(&packet);
	}
		//send packet
	for (i = 1; i <= num; i++) {
		printf("\nSending packet %d/%d\n", i, num);

		// send the packet
		status = sess.SendPacket((void *) "1234567890", 10, 0, false, 10);
		checkerror(status);
	}

	sess.BYEDestroy(RTPTime(10, 0), 0, 0);

	// Free the YUV frame
	av_free(pFrame);

	// Close the video file
	avformat_close_input(&pFormatCtx);
	//	av_close_input_file(pFormatCtx);

	return 0;
}
