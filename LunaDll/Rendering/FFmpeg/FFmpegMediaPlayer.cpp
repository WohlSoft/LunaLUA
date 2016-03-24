#include "FFmpegMediaPlayer.h"
#include <SDL2/SDL.h>
#include "../../Defines.h"
#include <chrono>
#include <mutex>
#include <Windows.h>

FFmpegThread* FFmpegMediaPlayer::videoOutputThread = new FFmpegThread();
//wrap���
#define UINT32SUB(a,b) ((uint32_t)max(0,(int64_t)a-(int64_t)b))

/*
	����̒�����
*/
void FFmpegMediaPlayerOpQueue::push(FFmpegMediaPlayerOperation*& op) {
	std::lock_guard<std::mutex> lock(mtx1);
	Ops.push_back(op);
}

bool FFmpegMediaPlayerOpQueue::pop(FFmpegMediaPlayerOperation*& op) {
	std::lock_guard<std::mutex> lock(mtx1);
	if (Ops.empty()) return false;
	op = Ops.front();
	Ops.pop_front();
	return true;
}

void FFmpegMediaPlayerOpQueue::clear() {
	
}

void FFmpegMediaPlayerOpQueue::proc() {
	if (wait == NULL) {
		if (!pop(wait))return;
		wait->initOp(wait->data);
	}
	else {
		if (wait->endCond()) {
			
			wait = NULL;
		}
	}
}



/*
	�K���ɕϐ��܂Ƃ߂������������̃N���X�@�Ђǂ��킱��
*/
void FFmpegPlayerStateManager::init() {
	playing = false;
		playTime=0;
		videoPos=0;
		startTime=0;
		plannedStartTime=0;
		pauseTime=0;
		wPlay=false;
		wPause=false;
		wStop=false;
		wSeek=false;
		pauseBooked=false;
		videoRendered=false;
		needAudioPosReset=false;
		needVideoPosReset=false;
		videoMemResetFlag=false;
		stopBooked=false;
		seekCount=0;
		audioMemResetFlag = false;
		audioInitReq = false;
		videoInitReq = false;
		lastAudioDelay = 0;
		videoDelay = 0;
}
void FFmpegPlayerStateManager::play() {
	//�v���C���łȂ��ꍇ�̂ݍĐ���҂�
	if(!playing)wPlay = true;
}

void FFmpegPlayerStateManager::pause() {
	//�v���C���̂݃|�[�Y��҂�
	if(playing)wPause= true;
}

void FFmpegPlayerStateManager::stop() {
	//�v���C���̂ݒ�~��҂�
	if(playing)wStop = true;
}

void FFmpegPlayerStateManager::seek(double sec) {
	//���ł��V�[�N�͏o����
	seekPos = sec;
	wSeek = true;
}
bool FFmpegPlayerStateManager::shouldSeek() {
	//�V�[�N�҂��Ȃ炢�ł��V�[�N���ׂ�
	return wSeek;
}
void FFmpegPlayerStateManager::seekProc() {
	//�V�[�N���̑�����ɖ���
	wSeek = false;
	seekCount++;
}

bool FFmpegPlayerStateManager::audioShouldPlay() {
	//�Đ����Ȃ瓖�R�Đ�(=�p��)���ׂ��ŁA����ɃI�[�f�B�I���Đ��̋N�_�ɂȂ�̂ŁA�Đ��҂��̏ꍇ���Đ�(=�J�n)���ׂ�
	return playing || wPlay;
}

bool FFmpegPlayerStateManager::videoShouldPlay() {
	//�I�[�f�B�I�ƈ���čĐ��̋N�_�ł͂Ȃ��̂ōĐ����ɑ��s����̂�
	return playing;
}
bool FFmpegPlayerStateManager::shouldBegin() {
	//�Đ����łȂ����Đ��҂��Ȃ�Đ��J�n���ׂ�
	return !playing && wPlay;
}

//time:absolute time
//pos:relative time
void FFmpegPlayerStateManager::startProc(double cur, double delay) {
	//�I�[�f�B�I���Đ��̋N�_�Ȃ̂�SDL�̃X���b�h�ŌĂ΂��

	//���Ԃɒ���(���̃X���b�h���Ԃɓ��鎖���l����)

	//1.�J�n����=���݂̎���+�x�ꂩ��O��̍Đ��ʒu����������������
	startTime = cur +delay-playPosition();
	//2."�Đ��̊J�n"���I����̂ōĐ��҂�
	wPlay = false; 
	//3.�Đ�����
	playing = true;
	//�J�n���Ԑݒ�O�ɍĐ��J�n�t���O�𗧂Ă�Ƒ��̃X���b�h���s���ȊJ�n���Ԃ�ǂݎ��̂Œ���
	lastAudioDelay = delay;
}

//�g��Ȃ�
bool FFmpegPlayerStateManager::shouldPause() {
	
	//�|�[�Y���ׂ�
	return playing && pauseTime <= playTime && wPause && pauseBooked;
}
bool FFmpegPlayerStateManager::shouldPauseByVideo() {
	//�|�[�Y�҂����|�[�Y���\�肳��Ă��āA�Đ������|�[�Y(�\��)�ʒu���Đ��ʒu���I�[�o�[���Ă��鎞
	//�|�[�Y���ׂ�
	return playing && pausePosition() <= videoPos && wPause && pauseBooked && !isSeeking();
}
bool FFmpegPlayerStateManager::shouldBookPause() {
	//�Đ������Đ��҂��̎��|�[�Y�̗\�������
	return playing && wPause;
}
void FFmpegPlayerStateManager::pauseBooking(double cur, double remain) {

	//1.�|�[�Y�\�肵���Ƃ������Ƃɂ���(���̃X���b�h�̌���)
	pauseBooked = true;
	//2.�|�[�Y�\�莞���̐ݒ�
	pauseTime = cur + remain;
	
}
void FFmpegPlayerStateManager::stopBooking() {
	//��~�̗\��
	//�擪�ւ̃V�[�N�̎��s
	seek(0);
	//�\��ς݂ɂ���
	stopBooked = true;
}
void FFmpegPlayerStateManager::pauseProc() {
	//1.��ɒ�~��錾���Ă���
	playing = false;
	//2.�|�[�Y���I����̂ŗ\��͏�����
	pauseBooked = false;
	//3.�|�[�Y�҂����I���
	wPause = false;
	//4.�|�[�Y�����܂ōĐ��������Ƃɂ���
	playTime = pauseTime;
}
void FFmpegPlayerStateManager::updateState(double cur) {
	//���ݎ����̍X�V
	playTime = cur;
}

double FFmpegPlayerStateManager::playPosition() {
	//�Đ��ʒu
	return playTime - startTime;
}
double FFmpegPlayerStateManager::pausePosition() {
	//�|�[�Y�ʒu
	return pauseTime - startTime;
}
bool FFmpegPlayerStateManager::isSeeking() {
	//�V�[�N�����ǂ���

	//�V�[�N�҂��܂��͉f���������̈ʒu���Z�b�g�҂��Ȃ�܂��V�[�N��
	return wSeek||needAudioPosReset || needVideoPosReset;
}
bool FFmpegPlayerStateManager::videoShouldKeepUp() {
	//�f����i�߂�ׂ����ǂ���
	//�Đ������A�f���̈ʒu�����݈ʒu���O�ɂ���Ȃ�i�߂�ׂ�
	//�܂��́A�f�������Đ��ʒu�̃��Z�b�g���K�v�ȏꍇ���i�߂�ׂ�
	//(�f�������Đ��ʒu�̃��Z�b�g���K�v 
	// == �V�[�N�ʒu����̃p�P�b�g�ǂݍ��ݍς�&&�����ɂ�郊�Z�b�g�ʒu����ς� 
	// == (�����ƃo�b�t�@�̗ނ�������)���̓ǂݍ��݂ŉf���̃��Z�b�g�ʒu������ł���)
	return ((videoPos <= playPosition()) && playing) || (needVideoPosReset && !needAudioPosReset) ;
}

void FFmpegPlayerStateManager::refreshVideoState(double curFT) {
	//�f���̏�Ԃ̍X�V
	//�����_�����O����Ă��Ȃ���Ԃɂ���
	//�Đ��ʒu�̍X�V
	videoRendered = false;
	videoPos = curFT+videoDelay;
}

bool FFmpegPlayerStateManager::shouldRenderVideo() {
	//�Đ������܂������_�����O����ĂȂ��ĉf���ʒu���Đ��ʒu�������Ă�Ȃ烌���_�����O���ׂ�
	//(���߂ď��������Ƀ����_�����O������)
	return videoPos >= playPosition() && !videoRendered && playing;
}



void FFmpegMediaPlayer::DebugMsgBox(LPCSTR pszFormat, ...)
{
	va_list	argp;
	char pszBuf[256];
	va_start(argp, pszFormat);
	vsprintf(pszBuf, pszFormat, argp);
	va_end(argp);
	MessageBoxA(NULL, pszBuf, "debug info", MB_OK);
}

FFmpegMediaPlayer::~FFmpegMediaPlayer() {
	/* �܂����̃X���b�h�ɂ��鏈����S�Ē�~ */
	if(__outputVideoBuffer)FFmpegMediaPlayer::videoOutputThread->delWork(__outputVideoBuffer);
	if(__queue)FFmpegDecodeQueue::queueThread->delWork(__queue);
	if (postMixCallback)PGE_MusPlayer::removePostMixFunc(postMixCallback);

	/* ����΍폜 */
	if (__outputVideoBuffer)delete __outputVideoBuffer;
	if (__queue)delete __queue;
	if (postMixCallback)delete postMixCallback;

	if (_play)delete _play;
	if (_pause)delete _pause;
	if (_seek)delete _seek;

	/* ���ɂ���CustomAVPacket�̓f�X�g���N�^��free����� */
	if (soundQueue) delete soundQueue;
	if (videoQueue) delete videoQueue;
	if (maskQueue) delete maskQueue;


	/* ����player�̃R���X�g���N�^�œǂݍ��܂ꂽmedia�Ȃ�폜 */
	if (media && loadViaPlayer)delete media;

	//struct�̃f�X�g���N�^�̓X�^�b�N�g���[�X�ɍڂ�Ȃ������肷��
}

//��ɃI�u�W�F�N�g�̐����𔺂�Ȃ�������
void FFmpegMediaPlayer::init() {
	loadViaPlayer = false;
	outBuffer = NULL;
	_videoPlayable = _audioPlayable = false;
	_SDLSoundFormat = AUDIO_S16SYS;
	av_init_packet(&pkt);
	av_init_packet(&mPkt);
	av_init_packet(&tmpMPkt);
	media = NULL;
	_play = _pause = _seek = NULL;
	soundQueue = videoQueue = maskQueue = NULL;
	__queue = __outputVideoBuffer = NULL;
	postMixCallback = NULL;
	loop = true;
	loadCompleted = false;
	_volume = 100;
	waitEnd = false;

	//treat it as out-of-screen initially
	onScreen = false;
	offScreenProcessed = true;
	lastOnScreenTime = -1;

	OScrMode = STOP;
	collisionMap = NULL;
	maskMode = 0;
	mPktConsumed = true;
}
void FFmpegMediaPlayer::setVideoDelay(double d) {
	sman.videoDelay = d;
}
//�f���Ɋւ��鏉����
bool FFmpegMediaPlayer::initVideo(FFmpegMedia* m, FFmpegVideoDecodeSetting* vs) {
	if (!m->isVideoAvailable())return false;
	/* make size aligned */
	vs->width = vs->width >0 ? vs->width : m->vidCodecCtx->width;
	vs->height = vs->height >0 ? vs->height : m->vidCodecCtx->height;
	avcodec_align_dimensions2(
		m->vidCodecCtx,
		&vs->width,
		&vs->height,
		aL);

	/* use short variable name */
	AVPixelFormat pixFmt = vs->pixFmt;
	int _w = vs->width;
	int _h = vs->height;

	/* init resampling settings */
	FFVDC.swsCtx = sws_getContext(
		m->width(),				//src info
		m->height(),
		m->vidCodecCtx->pix_fmt,
		_w,	//dest info
		_h,
		pixFmt,
		decodeSetting.video.resampling_mode,
		NULL,
		NULL,
		NULL);
	if (FFVDC.swsCtx == NULL)return false;
	/* malloc decode buffer */
	FFVDC.vBuffer = (uint8_t*)av_malloc(avpicture_get_size(pixFmt, _w, _h)*sizeof(uint8_t));
	if (FFVDC.vBuffer == NULL)return false;
	/* alloc frame buffer */
	FFVDC.vidDestFrame = av_frame_alloc();
	FFVDC.vidSrcFrame = av_frame_alloc();
	//FFVDC.booked = av_frame_alloc();
	if (FFVDC.vidDestFrame == NULL || FFVDC.vidSrcFrame == NULL)return false;
	/* assign buffer to frame */
	avpicture_fill((AVPicture*)FFVDC.vidDestFrame, FFVDC.vBuffer, pixFmt, _w, _h);

	//�f�R�[�h�ƃo�b�t�@�ւ̏������݂�����֐�
	__outputVideoBuffer = new FFmpegThreadFunc((std::function<void(FFmpegThreadFuncController*)>)[=](FFmpegThreadFuncController* ctrl) {
		if (waitEnd)return;
		/* �Đ����ׂ���Ԃ��ǂ����m�F */
		if (!sman.videoShouldPlay())return;

		/* player�̏�Ԃ̍X�V */
		sman.updateState(SDL_GetTicks());

		/*
			�V�[�N���ɍs���鏈��
			�I�[�f�B�I����startTime�̐ݒ肪�I�������ɍs����悤�ɂ���B
		*/
		bool posReset = sman.needVideoPosReset && !sman.needAudioPosReset;

		//�V�[�N�������s�������𖞂������ŏ��̃��[�v�̂ݍs������
		if ((posReset || sman.videoInitReq)&& !sman.videoMemResetFlag) {
			/*
				���̃p�P�b�g�̃^�C���X�^���v�łǂ��ɃV�[�N���ꂽ�����m�F���邽�߁A�V�[�N��̈ʒu�̃p�P�b�g���m���Ɏ擾����B

				(�V�[�N�������s�������𖞂��������A�L���[�ɂ��鎟�̃p�P�b�g�̓V�[�N��̈ʒu�̂��̂ɂȂ��Ă���̂ŁA
				�����Ńo�b�t�@���N���A���Ċm���ɓǂݍ��݂ɍs���΂悢�B)
			*/
			
			if (media->isMaskAvailable()) {
				avcodec_flush_buffers(media->maskCodecCtx);
				av_free_packet(&MaskVDC.vPkt);
				av_init_packet(&MaskVDC.vPkt);
				av_frame_free(&MaskVDC.vidSrcFrame);
				MaskVDC.vidSrcFrame = av_frame_alloc();
			}
			avcodec_flush_buffers(media->vidCodecCtx);
			av_free_packet(&FFVDC.vPkt);
			av_init_packet(&FFVDC.vPkt);
			av_frame_free(&FFVDC.vidSrcFrame);
			FFVDC.vidSrcFrame = av_frame_alloc();
			
			//���̏����������͈�񂫂�
			sman.videoMemResetFlag = true;
			if (sman.videoInitReq)sman.videoInitReq = false;
		}

		//���݂̍Đ������ɔ�ׂē���̎���(�^�C���X�^���v)���x��Ă���Ȃ�i�߂�
		while (sman.videoShouldKeepUp()) {

			//�O�����瑀�삵�����Ƃ�
			if (ctrl->quit || ctrl->pause)return;

			/*
				�ʃX���b�h�ŁAposReset�Ƃ����̊ԂŃV�[�N�������s��ꂽ���͈�U������
			*/
			if (!posReset && sman.needVideoPosReset)return;

			/*
				�|�[�Y���畜�A�����Ƃ��̓f�R�[�h����booked����ǂ݂���
				(����:booked�̓ǂݍ��݂Ɏg����vPkt���L���[����̓ǂݍ��݂Ɏg�����肵�ď㏑�������booked�̃t���[����������)
			*/
			//�g���t���[���̃|�C���^��srcPtr�ɓ���Ă���
			if (FFVDC.booked) {
				FFVDC.srcPtr = FFVDC.booked;
				FFVDC.got_picture = 1;
			}
			else {
				/* �L���[����̓ǂݍ��� */
				if (!videoQueue->pop(FFVDC.vPkt)) {
					//onQueueEnd
					if (videoQueue->dataSize() == 0 && loadCompleted && loop) {
						seek(0);
					}
					return;
				}

				/* �f�R�[�h */
				
				FFVDC.srcPtr = FFVDC.vidSrcFrame;
				/* vPkt�͂����g��Ȃ��̂�free */
				av_free_packet(&FFVDC.vPkt);
			}
			/* 
				�摜�𓾂��ꍇ�ɂ̂݃o�b�t�@�ɏ������ށB
				P�t���[����B�t���[����ǂݍ��񂾏ꍇ�̓f�R�[�_�����̃o�b�t�@�ɒ~�����Ă����ɂ͏o�͂���Ȃ��̂Œ��ӁB
			*/
			if (FFVDC.got_picture) {
				/* �Đ��ʒu�̎擾 */
				double sec = av_q2d(media->video->time_base)*av_frame_get_best_effort_timestamp(FFVDC.srcPtr);
				sman.refreshVideoState(1000 * sec);
				/* �V�[�N���̏��� �����ŃV�[�N�������I������ */
				if (posReset) {
					posReset = false;
					sman.needVideoPosReset = false;
					sman.startTime = sman.plannedStartTime;
					return;
				}
				/* 
					�|�[�Y����ׂ���Ԃ��������̏��� 
					�|�[�Y�͉f�����N�_�Ƃ���B
				*/
				if (sman.shouldPauseByVideo()) {
					//booked��null�Ȃ͂�
					FFVDC.booked = av_frame_alloc();
					//booked�ɓ���Ă����Ď������o��
					av_frame_copy(FFVDC.booked, FFVDC.vidSrcFrame);
					sman.pauseProc();
					return;
				}
				/* �����_�����O���ׂ���Ԃ��������ݐ悪����ꍇ�̂ݏ����o��*/
				if (sman.shouldRenderVideo() && outBuffer != NULL) {
					//���T���v�����O
					sws_scale(
						FFVDC.swsCtx,
						FFVDC.srcPtr->data,
						FFVDC.srcPtr->linesize,
						0,
						FFVDC.srcPtr->height,
						FFVDC.vidDestFrame->data,
						FFVDC.vidDestFrame->linesize
						);
					//�r�b�g�}�b�v���R�s�[
					for (int j = 0; j < decodeSetting.video.height; ++j) {
						memcpy(
							(uint32_t*)outBuffer + j*decodeSetting.video.width,
							((uint8_t*)FFVDC.vidDestFrame->data[0]) + j*FFVDC.vidDestFrame->linesize[0],
							FFVDC.vidDestFrame->linesize[0]
							);
						/* collision map test */
						
					}
						//double tmptime;
					if (media->isMaskAvailable()) {
						while (maskQueue->pop(MaskVDC.vPkt)) {
							avcodec_decode_video2(media->maskCodecCtx, MaskVDC.vidSrcFrame, &MaskVDC.got_picture, &MaskVDC.vPkt);
							av_free_packet(&MaskVDC.vPkt);
							if (MaskVDC.got_picture) {
								if (av_q2d(media->mask->time_base)*av_frame_get_best_effort_timestamp(MaskVDC.vidSrcFrame) >= sec) {
									sws_scale(
										MaskVDC.swsCtx,
										MaskVDC.vidSrcFrame->data,
										MaskVDC.vidSrcFrame->linesize,
										0,
										MaskVDC.vidSrcFrame->height,
										MaskVDC.vidDestFrame->data,
										MaskVDC.vidDestFrame->linesize
										);
									for (int j = 0; j < decodeSetting.video.height; ++j) {
										memcpy(
											(uint32_t*)collisionMap + j*decodeSetting.video.width,
											((uint8_t*)MaskVDC.vidDestFrame->data[0]) + j*MaskVDC.vidDestFrame->linesize[0],
											MaskVDC.vidDestFrame->linesize[0]
											);
										/* collision map test */

									}
									/*
									for (int j = 0; j < decodeSetting.video.height; ++j) {
										for (int k = 0; k < FFVDC.vidDestFrame->linesize[0] / 4; k++) {
											collisionMap[j*MaskVDC.vidDestFrame->linesize[0] / 4 + k] = *(((uint8_t*)MaskVDC.vidDestFrame->data[0]) + j*MaskVDC.vidDestFrame->linesize[0] + k * 4);//[B]GRA
										}
									}
									*/
									break;
								}
							}
						}
					}
					
				}
			}
			//booked�Ƀt���[�����R�s�[�����ꍇ�͂����܂ŗ��Ȃ�(�����R�[�h����������Ȃ炱���܂ŗ������Ȃ��悤��)
			if (FFVDC.booked) {
				av_frame_free(&FFVDC.booked);
				FFVDC.booked = NULL;
			}

		}
	});
	collisionMap = (uint8_t*)malloc(avpicture_get_size(pixFmt, _w, _h)*sizeof(uint8_t));


	/* mask init */
	if (!m->isMaskAvailable())return true;
	MaskVDC.swsCtx = sws_getContext(
		m->maskCodecCtx->width,				//src info
		m->maskCodecCtx->height,
		m->vidCodecCtx->pix_fmt,
		_w,	//dest info
		_h,
		pixFmt,
		decodeSetting.video.resampling_mode,
		NULL,
		NULL,
		NULL);

	if (MaskVDC.swsCtx == NULL)return true;
	MaskVDC.vBuffer = (uint8_t*)av_malloc(avpicture_get_size(pixFmt, _w, _h)*sizeof(uint8_t));
	if (MaskVDC.vBuffer == NULL)return true;
	/* alloc frame buffer */
	MaskVDC.vidDestFrame = av_frame_alloc();
	MaskVDC.vidSrcFrame = av_frame_alloc();
	//FFVDC.booked = av_frame_alloc();
	if (MaskVDC.vidDestFrame == NULL || MaskVDC.vidSrcFrame == NULL)return true;
	/* assign buffer to frame */
	avpicture_fill((AVPicture*)MaskVDC.vidDestFrame, MaskVDC.vBuffer, pixFmt, _w, _h);


	return true;
}

//dirty

bool FFmpegMediaPlayer::initAudio(FFmpegMedia* m, FFmpegAudioDecodeSetting* as) {
	if (!m->isAudioAvailable())return false;
	FFADC.swrCtx = swr_alloc();
	if (FFADC.swrCtx == NULL)return false;
	int o_chLayout = m->audCodecCtx->channel_layout;
	int o_sRate = m->audCodecCtx->sample_rate;
	AVSampleFormat o_sFmt = m->audCodecCtx->sample_fmt;
	av_opt_set_int(FFADC.swrCtx, "in_channel_layout", o_chLayout? o_chLayout :AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_int(FFADC.swrCtx, "in_sample_rate", o_sRate? o_sRate:44100, 0);
	av_opt_set_sample_fmt(FFADC.swrCtx, "in_sample_fmt", o_sFmt == AV_SAMPLE_FMT_NONE?AV_SAMPLE_FMT_S16:o_sFmt, 0);
	av_opt_set_int(FFADC.swrCtx, "out_channel_layout", as->channelLayout, 0);
	av_opt_set_int(FFADC.swrCtx, "out_sample_rate", as->sample_rate, 0);
	av_opt_set_sample_fmt(FFADC.swrCtx, "out_sample_fmt", as->sample_format, 0);
	if (swr_init(FFADC.swrCtx) < 0)return false;
	as->channel_num = av_get_channel_layout_nb_channels(as->channelLayout);
	FFADC.audFrame = av_frame_alloc();
	if (FFADC.audFrame == NULL)return false;
	FFADC.audBuf = (uint8_t*)malloc(FFMP_AUDIO_BUFFER_SIZE*sizeof(uint8_t));
	if (FFADC.audBuf == NULL)return false;
	postMixCallback = new PGE_PostMixFunc((std::function<void(void *udata, uint8_t *stream, int len)>)[=](void *udata, uint8_t *stream, int len) {
		if (waitEnd)return;
		if (!sman.audioShouldPlay())return;
		int len1, audio_size;
		double time_stamp = 0;
		bool first_send = true;
		//Check reset request before popping packet
		bool posReset = sman.needAudioPosReset;
		if ((posReset || sman.audioInitReq) && !sman.audioMemResetFlag) {
			avcodec_flush_buffers(media->audCodecCtx);
			av_free_packet(&FFADC.aPkt);
			av_init_packet(&FFADC.aPkt);
			//this cause popping
			FFADC.audPktData = NULL;
			FFADC.audPktSize = 0;
			sman.audioMemResetFlag = true;
			if (sman.audioInitReq)sman.audioInitReq = false;
		}
		while (len > 0) {

			if (FFADC.audBufOffset >= FFADC.audBufSize) {
				audio_size = decodeAudioFrame(FFADC.audBuf, FFMP_AUDIO_BUFFER_SIZE, &time_stamp);
				if (audio_size < 0) {
					FFADC.audBufSize = 1024;
					memset(FFADC.audBuf, 0, FFADC.audBufSize);
				}
				else {
					FFADC.audBufSize = audio_size;
				}
				FFADC.audBufOffset = 0;
			}
			len1 = FFADC.audBufSize - FFADC.audBufOffset;
			if (len1 > len)len1 = len;
			SDL_MixAudioFormat(stream, (uint8_t*)FFADC.audBuf + FFADC.audBufOffset, _SDLSoundFormat, len1, needMute()?0:_volume);
			len -= len1;
			stream += len1;
			FFADC.audBufOffset += len1;
			if (sman.shouldBegin()) {
				sman.startProc(SDL_GetTicks(), -15.6);// 2*1000 * len / ((double)PGE_MusPlayer::sampleRate()*PGE_MusPlayer::channels()));
			}
			if (sman.shouldBookPause()) {

				sman.pauseBooking(SDL_GetTicks(), 2*1000 * len / ((double)PGE_MusPlayer::sampleRate()*PGE_MusPlayer::channels()));
			}
			if (first_send && posReset) {
				posReset = false;
				//time_stamp may become huge negative value
				sman.plannedStartTime = (double)SDL_GetTicks() - max(0, time_stamp) + 2*1000 * len / ((double)PGE_MusPlayer::sampleRate()*PGE_MusPlayer::channels()) -15;
				sman.needAudioPosReset = false;
				first_send = false;
				
			}
		}
	});
	
	return setSDLAudioDevice(as);
}
void FFmpegMediaPlayer::coreInit() {
	soundQueue = new FFmpegDecodeQueue(44100 * 2); //0.5sec
	videoQueue = new FFmpegDecodeQueue(800 * 600 * 4*8); //whole raw screen*8
	maskQueue = new FFmpegDecodeQueue(800 * 600 * 4 * 8); //whole raw screen*8
	_play = new FFmpegMediaPlayerOperation();
	_pause = new FFmpegMediaPlayerOperation();
	_seek = new FFmpegMediaPlayerOperation();
	opq = new FFmpegMediaPlayerOpQueue();
	_play->initOp = [=](void* data) {
		sman.play();
	};
	_play->endCond = [=]() {
		return sman.playing && !sman.wPlay;
	};

	_pause->initOp = [=](void* data) {
		sman.pause();
	};

	_pause->endCond = [=]() {
		return !sman.wPause && !sman.playing && !sman.pauseBooked;
	};

	_seek->initOp = [=](void* data) {
		sman.seek(*(double*)data);
	};
	_seek->endCond = [=]() {
		return !sman.isSeeking();
	};
	_seek->data = &__seekVal;

	__queue = new FFmpegThreadFunc((std::function<void(FFmpegThreadFuncController*)>)[=](FFmpegThreadFuncController* ctrl) {
		
		if (waitEnd)return;
		if (shouldEnd()) {
			waitEnd = true;
		}

		//use better condition of unboost
		
		if (!sman.isSeeking() && videoOutputThread->boost) {
			videoOutputThread->boost = false;
		}
		if (!sman.isSeeking() && FFmpegDecodeQueue::queueThread->boost) {
			FFmpegDecodeQueue::queueThread->boost = false;
		}
		
		if (SDL_GetTicks() - lastOnScreenTime > 100) {
			onScreen = false;
		}
		if (!onScreen && !offScreenProcessed) {
			switch (OScrMode) {
			case CONTINUE:
				break;
			case PAUSE:
				pause();
				break;
			case STOP:
				stop();
				break;
			default:
				break;
			}
			offScreenProcessed = true;
		}

		if (onScreen && offScreenProcessed) {
			play();
			offScreenProcessed = false;
		}
		int end = 0,mend=0;
		opq->proc();

		if (sman.shouldSeek()) {
			seek_internal(sman.seekPos);
			sman.seekProc();
		}			
			//av_init_packet(&pkt);
		
		int vqc = 0, aqc = 0,mqc=0;
			while (!ctrl->quit && vqc<MAX_QUEUE_ONCE && aqc<MAX_QUEUE_ONCE && soundQueue->dataSize() < soundQueue->MAX_SIZE && videoQueue->dataSize() < videoQueue->MAX_SIZE) {
				
				end = av_read_frame(media->fmtCtx, &pkt);
				if (end < 0) {
					loadCompleted = (end == AVERROR_EOF);
					break;
				}
				if (pkt.stream_index == media->audStreamIdx) {
					soundQueue->push(pkt);
					aqc++;
				}
				else if (pkt.stream_index == media->vidStreamIdx) {
					videoQueue->push(pkt);
					vqc++;
				}
				else {
					av_free_packet(&pkt);
				}
				
			}

			
			if (media->isMaskAvailable()) {
				while (!ctrl->quit && mqc<MAX_QUEUE_ONCE&& maskQueue->dataSize() < maskQueue->MAX_SIZE) {

					mend = av_read_frame(media->mFmtCtx, &mPkt);
					if (mend < 0) {
						//loadCompleted = (end == AVERROR_EOF);
						break;
					}
					if (mPkt.stream_index == media->maskStreamIdx) {
						maskQueue->push(mPkt);
						mqc++;
					}
					else {
						av_free_packet(&mPkt);
					}

				}
			}
	});
}
FFmpegMediaPlayer::FFmpegMediaPlayer() {
	init();
}

FFmpegMediaPlayer::FFmpegMediaPlayer(std::wstring filePath, FFmpegDecodeSetting dSet) :FFmpegMediaPlayer() {
	if (!isSettingValid(dSet))return;
	media = new FFmpegMedia(filePath);
	loadViaPlayer = true;
	decodeSetting = dSet;
	_videoPlayable = initVideo(media, &decodeSetting.video);
	_audioPlayable = initAudio(media, &decodeSetting.audio);
	
	if (!isAudioPlayable() && !isVideoPlayable())return;
	coreInit();
	//startTime = std::chrono::system_clock::now();
	if (isVideoPlayable())FFmpegMediaPlayer::videoOutputThread->addWork(__outputVideoBuffer);
	
	FFmpegDecodeQueue::queueThread->addWork(__queue);
	//play();
}
void FFmpegMediaPlayer::play() {
	opq->push(_play);
}
void FFmpegMediaPlayer::pause() {
	opq->push(_pause);
}
void FFmpegMediaPlayer::seek(double sec) {
	
	if (sec <= 0) {
		soundQueue->clear();
		videoQueue->clear();
		if (media->isMaskAvailable())maskQueue->clear();
		av_seek_frame(media->fmtCtx, media->vidStreamIdx, 0, AVSEEK_FLAG_BACKWARD);
		if (media->isMaskAvailable())av_seek_frame(media->mFmtCtx, media->maskStreamIdx, 0, AVSEEK_FLAG_BACKWARD);

		sman.init();
		sman.wPlay = true;
		sman.videoMemResetFlag = false;
		sman.audioMemResetFlag = false;
		sman.videoInitReq = true;
		sman.audioInitReq = true;
	}
	else {
	
		__seekVal = sec;
		opq->push(_seek);
	}
}
void FFmpegMediaPlayer::stop() {
	seek(0);
	sman.wPlay = false;
	//pause();
}
void FFmpegMediaPlayer::setVolume(int vol) {
	_volume = min(127, max(0, vol));
}
bool FFmpegMediaPlayer::needMute() {
	return sman.isSeeking() || (offScreenProcessed && (OScrMode == PAUSE || OScrMode == STOP));
}
bool FFmpegMediaPlayer::shouldEnd() {
	return GM_EPISODE_MODE || GM_LEVEL_MODE;
}
void FFmpegMediaPlayer::setOffScreenMode(OffScreenMode mode) {
	OScrMode = mode;
}
void FFmpegMediaPlayer::setOnScreen(bool _onScreen) {
	//dbgboxA(_onScreen ? "ooon" : "nooon");
	onScreen = _onScreen;
	if(onScreen)lastOnScreenTime = SDL_GetTicks();
}
FFmpegDecodeSetting FFmpegMediaPlayer::getAppliedSetting() {
	return decodeSetting;
}

//Do this on queue thread
void FFmpegMediaPlayer::seek_internal(double sec) {
	
	//videoOutputThread->boost = true;
	FFmpegDecodeQueue::queueThread->boost = true;
	
	/*
	std::lock_guard<std::mutex>lock1(soundQueue->mtx1);
	std::lock_guard<std::mutex>lock2(videoQueue->mtx1);
	*/
	soundQueue->clear();
	videoQueue->clear();
	if (media->isMaskAvailable())maskQueue->clear();
	av_seek_frame(media->fmtCtx, media->vidStreamIdx, (int64_t)round(sec / av_q2d(media->video->time_base)), AVSEEK_FLAG_BACKWARD);
	//EnterCriticalSection(&crSectionA); EnterCriticalSection(&crSectionV); EnterCriticalSection(&crSectionM);

	if (media->isMaskAvailable())av_seek_frame(media->mFmtCtx, media->maskStreamIdx, (int64_t)round(sec / av_q2d(media->mask->time_base)), AVSEEK_FLAG_BACKWARD);
	
	if (FFVDC.booked) {
		av_frame_free(&FFVDC.booked);
		FFVDC.booked = NULL;
	}
	//LeaveCriticalSection(&crSectionA); LeaveCriticalSection(&crSectionV); LeaveCriticalSection(&crSectionM);
	sman.videoMemResetFlag = false;
	sman.audioMemResetFlag = false;
	sman.needAudioPosReset = true;
	sman.needVideoPosReset = true;
}
void FFmpegMediaPlayer::setVideoBufferDest(void* dest) {
	outBuffer = (uint8_t*)dest;
}
int FFmpegMediaPlayer::decodeAudioFrame(uint8_t* buffer, int buffer_size,double* head_time) {
	int audDestBufSize = 0, ret = 0;
	bool first_decode = true;
	while (1) {
		while (FFADC.audPktSize > 0) {
			int len1 = avcodec_decode_audio4(media->audCodecCtx, FFADC.audFrame, &FFADC.got_sound, &FFADC.aPkt);
			if (first_decode) {
				*head_time = 1000 * av_q2d(media->audio->time_base)*av_frame_get_best_effort_timestamp(FFADC.audFrame);
				first_decode = false;
			}
			if (len1 < 0) {
				FFADC.audPktSize = 0;
				break;
			}
			FFADC.audPktData += len1;
			FFADC.audPktSize -= len1;
			audDestBufSize = 0;
			if (FFADC.got_sound) {
				audDestBufSize = resampleAudio(FFADC.audFrame,&FFADC.audDestData);
				/* seg fault point*/
				if (audDestBufSize > 0) {
					memcpy(buffer, FFADC.audDestData[0], audDestBufSize);
				}
				av_freep(&FFADC.audDestData[0]);
				av_freep(&FFADC.audDestData);
			}
			if (audDestBufSize <= 0) {
				continue;
			}

			return audDestBufSize;
		}
		if (FFADC.aPkt.data)av_free_packet(&FFADC.aPkt);

		if (!soundQueue->pop(FFADC.aPkt))return -1;
		FFADC.audPktData = FFADC.aPkt.data;
		FFADC.audPktSize = FFADC.aPkt.size;

	}
}
int FFmpegMediaPlayer::resampleAudio(AVFrame* decodedFrame,uint8_t ***dest_data) {
	int ret = 0;
	FFADC.audDestMaxSamples = FFADC.audDestSamples = (int)av_rescale_rnd(
		decodedFrame->nb_samples,
		decodeSetting.audio.sample_rate,
		media->audCodecCtx->sample_rate,
		AV_ROUND_UP);

	ret = av_samples_alloc_array_and_samples(
		dest_data,
		&FFADC.audDestLSize,
		decodeSetting.audio.channel_num,
		FFADC.audDestSamples,
		decodeSetting.audio.sample_format,
		0);

	if (ret < 0)return -1;
	FFADC.audDestSamples = (int)av_rescale_rnd(
		swr_get_delay(FFADC.swrCtx, media->audCodecCtx->sample_rate) + decodedFrame->nb_samples,
		decodeSetting.audio.sample_rate,
		media->audCodecCtx->sample_rate,
		AV_ROUND_UP);

	if (FFADC.audDestSamples > FFADC.audDestMaxSamples) {
		//av_freep((&(*dest_data)[0]));
		//av_freep(dest_data);
		ret = av_samples_alloc(
			*dest_data,
			&FFADC.audDestLSize,
			decodeSetting.audio.channel_num,
			FFADC.audDestSamples,
			decodeSetting.audio.sample_format,
			1);
		if (ret < 0)return -1;
		FFADC.audDestMaxSamples = FFADC.audDestSamples;
	}
	ret = swr_convert(
		FFADC.swrCtx,
		*dest_data,
		FFADC.audDestSamples,
		(const uint8_t**)decodedFrame->data,
		decodedFrame->nb_samples
		);
	if (ret < 0)return -1;

	return av_samples_get_buffer_size(
		&FFADC.audDestLSize,
		decodeSetting.audio.channel_num,
		ret,
		decodeSetting.audio.sample_format,
		1);
}

bool FFmpegMediaPlayer::setSDLAudioDevice(FFmpegAudioDecodeSetting *as) {
	switch (as->sample_format) {
	case AV_SAMPLE_FMT_NONE:
		_SDLSoundFormat = AUDIO_S16SYS; //most ordinary one?
		break;
	case AV_SAMPLE_FMT_U8:
		_SDLSoundFormat = AUDIO_U8;
		break;
	case AV_SAMPLE_FMT_S16:
		_SDLSoundFormat = AUDIO_S16SYS;
		break;
	case AV_SAMPLE_FMT_S32:
		_SDLSoundFormat = AUDIO_S32SYS;
		break;
	case AV_SAMPLE_FMT_FLT:
		_SDLSoundFormat = AUDIO_F32SYS;
		break;
	default:
		// SDL cannot deal the formats that not listed above.
		return false;
		break;
	}
	
	//!!!clean this on delete
	//////////////////////////////////////////////////////////
	PGE_MusPlayer::addPostMixFunc(postMixCallback);
	/////////////////////////////////////////////////////////
	return true;
}
bool FFmpegMediaPlayer::isAudioPlayable() const {
	return _audioPlayable;
}
bool FFmpegMediaPlayer::isVideoPlayable() const {
	return _videoPlayable;
}

bool FFmpegMediaPlayer::isSettingValid(FFmpegDecodeSetting s) {
	return s.audio.sample_rate > 0;
}