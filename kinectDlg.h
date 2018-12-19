
// kinectDlg.h : ͷ�ļ�
//

#pragma once


#include "afxdialogex.h"
#include "XnCppWrapper.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <fstream>
#include "kcftracker.hpp"
#include <Windows.h>
#include "kinect.h"
using namespace cv;

using std::vector;


struct MyPoint{
	float x;
	float y;
	float z;
	MyPoint(float x1=0,float y1=0,float z1=0):x(x1),y(y1),z(z1){}
	void SetData(float data[3])
	{
		x=data[0];
		y=data[1];
		z=data[2];
	}

	MyPoint& operator=(const MyPoint& a) //����queue���ƹ��캯����queue(const queue&),�����˳������캯��
	{
		this->x = a.x;
		this->y = a.y;
		this->z = a.z;
		return *this;
	}

	MyPoint& operator=(MyPoint& a)
	{
		this->x = a.x;
		this->y = a.y;
		this->z = a.z;
		return *this;
	}

};

struct SColorPoint3D
{
	float  X;
	float  Y;
	float  Z;
	int  R;
	int  G;
	int  B;

	SColorPoint3D( XnPoint3D pos, XnRGB24Pixel color )
	{
		X = pos.X;
		Y = pos.Y;
		Z = pos.Z;
		R = color.nRed;
		G = color.nGreen;
		B = color.nBlue;
	}
};

// CkinectDlg �Ի���
class CkinectDlg : public CDialogEx
{
// ����
public:
	CkinectDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_KINECT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	void GeneratePointCloud( xn::DepthGenerator& ,
		const XnDepthPixel* ,
		const XnRGB24Pixel* ,
		vector<SColorPoint3D>&  );
	void myShiTomasi_function(const Mat , Mat  ,int ,int /*bool**ImageMap*/);
	static DWORD WINAPI kcf1proc(LPVOID );
	static DWORD WINAPI kcf2proc(LPVOID );
	static DWORD WINAPI kcf3proc(LPVOID );
	static void insertToQueue(Mat, Rect, int);
	static DWORD WINAPI savePoint(LPVOID );


	afx_msg void OnBnClickedButton2();
};
