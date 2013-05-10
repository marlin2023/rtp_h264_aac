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
	destip = inet_addr("127.0.0.1");
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
	sessparams.SetOwnTimestampUnit(1.0 / 44100.0); //must be set

	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(portbase); //portbase = 6000
	status = sess.Create(sessparams, &transparams);
	checkerror(status);

	RTPIPv4Address addr(destip, destport);
	sess.SetDefaultPayloadType(96);
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
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
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

	FILE *fp = fopen("ha_src.aac" ,"wb");
		if(fp == NULL){
			printf("create aac file failed ...\n");
			exit(1);
		}

		uint8_t  adts_header[7];

	int tmp_count = 0;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {

			printf("5___ packet.size = %d ,extrasize = %d__\n" ,packet.size ,pFormatCtx->streams[videoStream]->codec->extradata_size);
			int i = 0;
//			for(i = 0; i < 7; i++){
//				printf("%x " ,pFormatCtx->streams[videoStream]->codec->extradata[i]);
//			}
			printf("/n");
//			tmp_count ++;
//			if(tmp_count == 10) while(1);
			//1:write aac header information

			unsigned int sample_index ,channel ;
			unsigned char temp;
			unsigned int  length;
			length = packet.size + 7;

			sample_index = channel = 0;
			temp = 0;


			sample_index = (pFormatCtx->streams[videoStream]->codec->extradata[0] & 0x07) << 1;
			temp = pFormatCtx->streams[videoStream]->codec->extradata[1] & 0x80;
			sample_index = sample_index + (temp >> 7);
			channel = (pFormatCtx->streams[videoStream]->codec->extradata[1] - temp) >> 3;
//
			adts_header[0] = 0xFF;
			/* Sync point continued over first 4 bits + static 4 bits     * (ID, layer, protection)*/
			adts_header[1] = 0xF1;
			/* Object type over first 2 bits */
			printf("sample_index = %d \n" , sample_index);
//			adts_header[2] = 0x40 | (sample_index << 2) | (channel >> 2);//     /* rate index over next 4 bits */
			adts_header[2] = 0x00 | (6<<2) |(2>>2) ;//     /* rate index over next 4 bits */
			printf("sample_index = %xh \n" , adts_header[2] );
			adts_header[3] = ((channel &0x3)<< 6) | (length >> 11);     /* frame size continued over full byte */
			adts_header[4] = (length >> 3) & 0xff;     /* frame size continued first 3 bits */
			adts_header[5] = ((length << 5) & 0xff) | 0x1f;      /* buffer fullness (0x7FF for VBR) continued over 6 first bits + 2 zeros      * number of raw data blocks */
			adts_header[6] = 0xFC;// one raw data blocks .  adts_header[6] |= num_data_block & 0x03; //Set raw Data blocks.

//
//			adts_header[0] = 0xFF;
//			/* Sync point continued over first 4 bits + static 4 bits     * (ID, layer, protection)*/
//			adts_header[1] = 0xF1;
//			/* Object type over first 2 bits */
//			adts_header[2] = 0x58;//     /* rate index over next 4 bits */
//			adts_header[3] = 0x80;
//			adts_header[4] = 0x05;     /* frame size continued first 3 bits */
//			adts_header[5] = 0x5F;      /* buffer fullness (0x7FF for VBR) continued over 6 first bits + 2 zeros      * number of raw data blocks */
//			adts_header[6] = 0xFC;// one raw data blocks .  adts_header[6] |= num_data_block & 0x03; //Set raw Data blocks.
			fwrite(adts_header ,1 ,7 ,fp);
			//2:write aac payload information
//					fprintf(fp ,"%s" ,recv_data);
			fwrite(packet.data ,1 ,packet.size ,fp);
			// send the packet
			status = sess.SendPacket((void *) packet.data, packet.size, 0, false, 10);
			checkerror(status);
		} //endifif(frameFinished)

		av_free_packet(&packet);
		usleep(500);
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
