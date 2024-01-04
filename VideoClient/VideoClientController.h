#pragma once
#include "EVlc.h"
#include "VideoClientDlg.h"
enum EVlcCommand {
	EVLC_PLAY,
	EVLC_PAUSE,
	EVLC_STOP,
	EVLC_GET_VOLUME,
	EVLC_GET_POSITION,
	EVLC_GET_LENGTH
};
class VideoClientController
{
public:
	VideoClientController();
	~VideoClientController();
	int Init(CWnd*& pWnd);
	int Invoke();
	//如果strUrl中包含中文符号或字符，则使用utf-8编码
	int SetMedia(const std::string& strUrl);
	//返回-1.0表示错误
	float VideoCtrl(EVlcCommand cmd);
	void SetPosition(float pos);
	int SetWnd(HWND hWnd);
	int SetVolume(int volume);
	VlcSize GetMediaInfo();
	std::string Unicode2Utf8(const std::wstring& strIn);
protected:
	EVlc m_vlc;
	CVideoClientDlg m_dlg;
};

