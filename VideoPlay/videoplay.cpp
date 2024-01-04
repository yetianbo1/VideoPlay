// vlc应用简单例子
#include <iostream>
#include "vlc.h"
#include <conio.h>     
#include <Windows.h>
std::string Unicode2Utf8(const std::wstring& strIn)
{
	std::string str;
	int length = ::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), NULL, 0, NULL, NULL);
	str.resize(length + 1);
	::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), (LPSTR)str.c_str(), length, NULL, NULL);
	return str;
}

int main()
{
	int argc = 1;
	char* argv[2];
	argv[0] = (char*)"--ignore-config";  //忽略配置
	std::string path = Unicode2Utf8(L"file:///D:\\cpp_study\\VideoPlay_vlc\\VideoPlay\\股市讨论.mp4");  //vlc默认unicode
	//创建实例
	libvlc_instance_t* vlc_ins = libvlc_new(argc, argv);
	//加载媒体
	libvlc_media_t* media = libvlc_media_new_location(vlc_ins, path.c_str());
	//创建播放器
	libvlc_media_player_t* player = libvlc_media_player_new_from_media(media);
	do {
		//开始播放
		int ret = libvlc_media_player_play(player);
		if (ret == -1) {
			printf("error found!\r\n");
			break;
		}
		int vol = -1;
		//只有media解析加载完成，才会有下面的参数
		while (vol == -1) {
			Sleep(10);
			//获取播放音量
			vol = libvlc_audio_get_volume(player);
		}
		printf("volume is %d\r\n", vol);
		//设置播放的音量
		libvlc_audio_set_volume(player, 10);
		//获取播放长度
		libvlc_time_t tm = libvlc_media_player_get_length(player);
		printf("%02d:%02d:%02d.%03d\r\n", int(tm / 3600000), int(tm / 60000) % 60, int(tm / 1000) % 60, int(tm) % 1000);
		//获取播放媒体的宽/高
		int width = libvlc_video_get_width(player);
		int height = libvlc_video_get_height(player);
		printf("width=%d height=%d\r\n", width, height);
		while (!_kbhit()) {       // conio.h
			// 获取播放位置
			printf("%f%%\r", 100.0 * libvlc_media_player_get_position(player));
			Sleep(500);
		}
		//暂停播放
		getchar();
		libvlc_media_player_pause(player);
		//恢复播放
		getchar();
		libvlc_media_player_play(player);
		//停止播放
		getchar();
		libvlc_media_player_stop(player);
	} while (0);
	libvlc_media_player_release(player);
	libvlc_media_release(media);
	libvlc_release(vlc_ins);
}
