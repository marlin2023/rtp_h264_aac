#include <iostream>
#include <stdio.h>

#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpsessionparams.h"
#include "rtpudpv4transmitter.h"

using namespace std;
using namespace jrtplib;

void checkerror(int rtperr)
{
	if(rtperr < 0)
	{
		cout << "Error: " << RTPGetErrorString(rtperr) << endl;
		exit(-1);
	}
}

int main(int argc, char **argv)
{
	RTPSession sess;
	uint16_t localport;
	int status;
	uint8_t *recv_data;
	unsigned int length;
	//cout << "Enter port num:" << endl;
	//cin >> localport ;
	//cout << endl;
	localport = 6666;

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetAcceptOwnPackets(false);
	sessparams.SetOwnTimestampUnit(1.0/10.0);
	transparams.SetPortbase(localport);
	status = sess.Create(sessparams, &transparams);


	//status = sess.Create(localport);
	//status = sess.Create(8000);
	checkerror(status);

	FILE *fp = fopen("ha_desti.aac" ,"wb");
	if(fp == NULL){
		printf("create aac file failed ...\n");
		exit(1);
	}

	uint8_t  adts_header[7];
	while(1)
	{
#ifndef	RTP_SUPPORT_THREAD
		status = sess.Poll();
		checkerror(status);
#endif
		sess.BeginDataAccess();

		if (sess.GotoFirstSourceWithData())
		{
			//cout << "receive data :" << endl;
			do
			{
				RTPPacket *pack;
				while( (pack=sess.GetNextPacket())!=NULL )
				{
					cout << "get packet" << endl;
					//sess.DeletePacket( pack ) ;

					recv_data = pack->GetPayloadData();
					cout << "packet length:" << pack->GetPacketLength() << endl;
					length = pack->GetPayloadLength();
					cout << "payload lenght:" << length << endl;

					int aac_len = length + 7;
					adts_header[0] = 0xFF;
					adts_header[1] = 0xF1;
					/* Object type over first 2 bits */
					adts_header[2] = 0x00 | (6<<2) |(2>>2) ;//     /* rate index over next 4 bits */
					adts_header[3] = ((2 &0x3)<< 6) | (aac_len >> 11);     /* frame size continued over full byte */
					adts_header[4] = (aac_len >> 3) & 0xff;     /* frame size continued first 3 bits */
					adts_header[5] = ((aac_len << 5) & 0xff) | 0x1f;      /* buffer fullness (0x7FF for VBR) continued over 6 first bits + 2 zeros      * number of raw data blocks */
					adts_header[6] = 0xFC;// one raw data blocks .  adts_header[6] |= num_data_block & 0x03; //Set raw Data blocks
					fwrite(adts_header ,1 ,7 ,fp);
					//2:write aac payload information
//					fprintf(fp ,"%s" ,recv_data);
					fwrite(recv_data ,1 ,length ,fp);
					for(int i=0; i<length; i++)
					{
						printf("0x%0x ",*recv_data++);
						if( (i+1)%16==0 )
							printf("\n");
					}
					printf("\n");
					delete pack;
				}
			}while(sess.GotoNextSourceWithData());
		}
		sess.EndDataAccess();
	}
	sess.BYEDestroy(RTPTime(10,0),0,0);

	return 0;
}
