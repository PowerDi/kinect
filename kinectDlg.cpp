
// kinectDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "kinectDlg.h"
#include "afxdialogex.h"
#include "XnCppWrapper.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <fstream>
#include "kcftracker.hpp"
#include <Windows.h>
#include <queue>
#include <tchar.h>

using namespace cv;


using namespace std;
//kcf参数
#define HOG true
#define FIXEDWINDOW false
#define MULTISCALE true
#define SILENT false
#define LAB false

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CkinectDlg 对话框




CkinectDlg::CkinectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CkinectDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
}

void CkinectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CkinectDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CkinectDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CkinectDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CkinectDlg 消息处理程序

BOOL CkinectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CkinectDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CkinectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CkinectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void GeneratePointCloud( xn::DepthGenerator& rDepthGen,
                         const XnDepthPixel* pDepth,
                         const XnRGB24Pixel* pImage,
                         vector<SColorPoint3D>& vPointCloud )
{
	// 1. number of point is the number of 2D image pixel
	xn::DepthMetaData mDepthMD;
	rDepthGen.GetMetaData( mDepthMD );
	unsigned int uPointNum = mDepthMD.FullXRes() * mDepthMD.FullYRes();
 
	// 2. build the data structure for convert
	XnPoint3D* pDepthPointSet = new XnPoint3D[ uPointNum ];
	unsigned int i, j, idxShift, idx;
	for( j = 0; j < mDepthMD.FullYRes(); ++j )
	{
		idxShift = j * mDepthMD.FullXRes();
		for( i = 0; i < mDepthMD.FullXRes(); ++i )
		{
			idx = idxShift + i;
//			pDepthPointSet[idx].X = i;
//			pDepthPointSet[idx].Y = j;
			pDepthPointSet[idx].Z = pDepth[idx];
		}
	}
 
	//lan 双边滤波
	float *pDataOut=new float[640*480];
	float ws=0.0,wr=0.0;
	float wtotal=0.0;
	for(i=2;i<480-2;i++)
	for(j=2;j<640-2;j++)
	{
		pDataOut[i*640+j]=0.0;
		wtotal=0.0;
		for(int ii=-2;ii<=2;ii++)
		for(int jj=-2;jj<=2;jj++)
		{
			ws=exp(-0.5*(ii*ii+jj*jj)/3/3);
			wr=exp(-0.5*(pDepthPointSet[(i+ii)*640+j+jj].Z-pDepthPointSet[i*640+j].Z)*(pDepthPointSet[(i+ii)*640+j+jj].Z-pDepthPointSet[i*640+j].Z)/34/34);
			wtotal+=ws*wr;
			pDataOut[i*640+j]+=ws*wr*pDepthPointSet[(i+ii)*640+j+jj].Z;
		}
		pDataOut[i*640+j] /=wtotal;
	}
	//lan双边滤波

	//lan转换成世界坐标系？
	for (i=0;i<mDepthMD.FullXRes()*mDepthMD.FullYRes();i++)
		pDepthPointSet[i].Z=pDataOut[i];
	delete []pDataOut;
	for( j = 0; j < mDepthMD.FullYRes(); ++j )
	{
		idxShift = j * mDepthMD.FullXRes();
		for( i = 0; i < mDepthMD.FullXRes(); ++i )
		{
			idx = idxShift + i;
			pDepthPointSet[idx].X =((float)i-320)*pDepthPointSet[idx].Z/525;  //525为焦距
			pDepthPointSet[idx].Y =-((float)j-240)*pDepthPointSet[idx].Z/525;  //参考链接：http://www.cnblogs.com/gaoxiang12/p/3695962.html
		}
	}
	//lan
	// 3. un-project points to real world
//	XnPoint3D* p3DPointSet = new XnPoint3D[ uPointNum ];
//	rDepthGen.ConvertProjectiveToRealWorld( uPointNum, pDepthPointSet, p3DPointSet );
//	delete[] pDepthPointSet;
 
	// 4. build point cloud
	for( i = 0; i < uPointNum; ++ i )
//		vPointCloud.push_back( SColorPoint3D( p3DPointSet[i], pImage[i] ) );
		vPointCloud.push_back( SColorPoint3D( pDepthPointSet[i], pImage[i] ) );
//	delete[] p3DPointSet;
	delete[] pDepthPointSet;
}

struct CvKinectImage
{
	Mat depth;
	Mat image;
	IplImage* depth_origin;
	Rect select;   //目标框的位置 
	Point origin;
	unsigned short pointData; 
};

//全局变量定义区
bool select_flag=false;
bool Get_rect=false;
int num_of_savepoint = 50; //运动多少个点就进行保存
int g_rectCount=0; //记录画框数量
//extern int* flag_Captured;  //全局变量指示采集数据是否完成 0表示未完成 1表示完成
bool stop_signal=false;
vector<SColorPoint3D> vPointCloud;
HANDLE mutex = CreateMutex(NULL,false,NULL);









void onMouse(int event,int x,int y,int flags,void* param)
{
	vector<CvKinectImage>* kinectImage = (vector<CvKinectImage>*)param;
	//CvKinectImage* kinectImage = (CvKinectImage*)param;
    //Point origin;//不能在这个地方进行定义，因为这是基于消息响应的函数，执行完后origin就释放了，所以达不到效果。
	if (g_rectCount<kinectImage->size())
	{
		if(select_flag)
		{
			kinectImage->at(g_rectCount).select.x=MIN(kinectImage->at(g_rectCount).origin.x,x);//不一定要等鼠标弹起才计算矩形框，而应该在鼠标按下开始到弹起这段时间实时计算所选矩形框
			kinectImage->at(g_rectCount).select.y=MIN(kinectImage->at(g_rectCount).origin.y,y);
			kinectImage->at(g_rectCount).select.width=abs(x-kinectImage->at(g_rectCount).origin.x);//算矩形宽度和高度
			kinectImage->at(g_rectCount).select.height=abs(y-kinectImage->at(g_rectCount).origin.y);
			kinectImage->at(g_rectCount).select&=Rect(0,0,kinectImage->at(g_rectCount).image.cols,kinectImage->at(g_rectCount).image.rows);//保证所选矩形框在视频显示区域之内
			
		}
		if(event==CV_EVENT_LBUTTONDOWN)
		{
			select_flag=true;//鼠标按下的标志赋真值
			kinectImage->at(g_rectCount).origin=Point(x,y);//保存下来单击是捕捉到的点
			kinectImage->at(g_rectCount).select=Rect(x,y,0,0);//这里一定要初始化，宽和高为(0,0)是因为在opencv中Rect矩形框类内的点是包含左上角那个点的，但是不含右下角那个点
		}
		else if(event==CV_EVENT_LBUTTONUP)
		{
			select_flag=false;
			g_rectCount++;
		}
	}
		
	else Get_rect=true;
}
    

int blockSize = 3; 
int apertureSize = 3;
	


vector<CvKinectImage> kinectImage(3); //three points

KCFTracker tracker_p1(HOG, FIXEDWINDOW, MULTISCALE, LAB);
KCFTracker tracker_p2(HOG, FIXEDWINDOW, MULTISCALE, LAB);
KCFTracker tracker_p3(HOG, FIXEDWINDOW, MULTISCALE, LAB);


queue<MyPoint> tracker_1;
queue<MyPoint> tracker_2;
queue<MyPoint> tracker_3;




DWORD WINAPI CkinectDlg::kcf1proc(LPVOID lpParameter)
{
	bool init_down = false;
	Mat roi_frame;
	Mat origin_image;
	Mat new_image;
	Mat temp_depth; //临时深度图
	Rect result;
	int xMin,yMin,width,height;
	int count_f=0; //record fps
	while (1)
	{
		
		Sleep(30);
		if (stop_signal==true)
		{
			break; //线程结束标志
		}

		if ((g_rectCount==1)&&init_down==false) //必须双重判断保证初始化只进入一次
		{
			xMin=kinectImage.at(0).select.x;
			yMin=kinectImage.at(0).select.y;
			width=kinectImage.at(0).select.width;
			height=kinectImage.at(0).select.height;
			WaitForSingleObject(mutex,INFINITE);
			origin_image = kinectImage.at(0).image;
			temp_depth = kinectImage.at(0).depth;
			ReleaseMutex(mutex);
			tracker_p1.init( Rect(xMin, yMin, width, height), origin_image);

			//rectangle( kinectImage.at(0).image, Point( xMin, yMin ), Point( xMin+width, yMin+height), Scalar( 0, 255, 0 ), 1, 8 );

			insertToQueue(temp_depth,Rect(xMin, yMin, width, height),1); //向缓冲区存储当前点

			if(!roi_frame.empty())
			{
				roi_frame.release();
			}
			roi_frame=kinectImage.at(0).image(Rect(xMin,yMin,width,height));
			init_down=true;	
			
		}

		if ((g_rectCount!=0)&&init_down)
		{
			WaitForSingleObject(mutex,INFINITE);
			new_image = kinectImage.at(0).image;
			temp_depth = kinectImage.at(0).depth;
			ReleaseMutex(mutex);

			result = tracker_p1.update(new_image);
			
			if ((result.x+result.width>kinectImage.at(0).image.cols) || (result.y+result.height>kinectImage.at(0).image.rows)
				||(result.x<0)||(result.y<0))
			{
				AfxMessageBox(_T("运动超出边界,请重开程序"));
				cvDestroyAllWindows();
				exit(-1);
			}

			//rectangle( kinectImage.at(0).image, Point( result.x, result.y ), Point( result.x+result.width, result.y+result.height), Scalar( 0, 255, 0), 1, 8 );

			WaitForSingleObject(mutex,INFINITE);
			kinectImage.at(0).select=result;
			ReleaseMutex(mutex);

			if(!roi_frame.empty())
			{
				roi_frame.release();
			}
			roi_frame=kinectImage.at(0).image(Rect(result.x, result.y,result.width,result.height));

			if (count_f==3) //record pos per 5 frames
			{
				insertToQueue(temp_depth,result,1);  //向缓冲区存储当前点
				count_f=0;
			}
			
			/*if (tracker_1.size()>=200)
			{
				break;
			}*/
			
			count_f++;
		}


	}
	return 0;
}

DWORD WINAPI CkinectDlg::kcf2proc(LPVOID lpParameter)
{

	bool init_down = false;
	Mat roi_frame;
	Mat origin_image;
	Mat new_image;
	Mat temp_depth; //临时深度图
	Rect result;
	int xMin,yMin,width,height;
	int count_f = 0;
	while (1)
	{
		Sleep(30);
		if (stop_signal==true)
		{
			break; //线程结束标志
		}


		if ((g_rectCount==2)&&init_down==false)
		{
			xMin=kinectImage.at(1).select.x;
			yMin=kinectImage.at(1).select.y;
			width=kinectImage.at(1).select.width;
			height=kinectImage.at(1).select.height;
			WaitForSingleObject(mutex,INFINITE);
			origin_image = kinectImage.at(0).image;
			temp_depth = kinectImage.at(0).depth;
			ReleaseMutex(mutex);
			tracker_p2.init( Rect(xMin, yMin, width, height),origin_image );
			//rectangle( kinectImage.at(0).image, Point( xMin, yMin ), Point( xMin+width, yMin+height), Scalar( 0, 255, 0 ), 1, 8 );

			insertToQueue(temp_depth,Rect(xMin, yMin, width, height),2);  //向缓冲区存储当前点

			if(!roi_frame.empty())
			{
				roi_frame.release();
			}
			roi_frame=kinectImage.at(1).image(Rect(xMin,yMin,width,height));
			init_down=true;	
			
		}

		if ((g_rectCount!=0)&&init_down)
		{
			WaitForSingleObject(mutex,INFINITE);
			new_image = kinectImage.at(1).image;
			temp_depth = kinectImage.at(1).depth;
			ReleaseMutex(mutex);

			result = tracker_p2.update(new_image);

			if ((result.x+result.width>kinectImage.at(0).image.cols) || (result.y+result.height>kinectImage.at(0).image.rows)
				||(result.x<0)||(result.y<0))
			{
				AfxMessageBox(_T("运动超出边界,请重开程序"));
				cvDestroyAllWindows();
				exit(-1);
			}
			//rectangle( kinectImage.at(0).image, Point( result.x, result.y ), Point( result.x+result.width, result.y+result.height), Scalar( 0, 255, 0 ), 1, 8 );

			WaitForSingleObject(mutex,INFINITE);
			kinectImage.at(1).select = result;
			ReleaseMutex(mutex);

			if(!roi_frame.empty())
			{
				roi_frame.release();
			}
			roi_frame=kinectImage.at(1).image(Rect(result.x, result.y,result.width,result.height));
			if (count_f==3) //record pos per 5 frames
			{
				insertToQueue(temp_depth,result,2);  //向缓冲区存储当前点
				count_f=0;
			}

			/*if (tracker_2.size()>=200)
			{
				break;
			}*/
			
			count_f++;
		}


	}


	return 0;

}

DWORD WINAPI CkinectDlg::kcf3proc(LPVOID lpParameter)
{

	bool init_down = false;
	Mat roi_frame;
	Rect result;
	Mat origin_image;
	Mat new_image;
	Mat temp_depth;
	int xMin,yMin,width,height;
	int count_f = 0;
	while (1)
	{
		Sleep(30);
		if (stop_signal==true)
		{
			break;
		}


		if ((g_rectCount==3)&&init_down==false)
		{
			xMin=kinectImage.at(2).select.x;
			yMin=kinectImage.at(2).select.y;
			width=kinectImage.at(2).select.width;
			height=kinectImage.at(2).select.height;
			WaitForSingleObject(mutex,INFINITE);
			origin_image = kinectImage.at(0).image;
			temp_depth = kinectImage.at(0).depth;
			ReleaseMutex(mutex);
			tracker_p3.init( Rect(xMin, yMin, width, height), kinectImage.at(0).image);
			//rectangle( kinectImage.at(0).image, Point( xMin, yMin ), Point( xMin+width, yMin+height), Scalar( 0, 255, 0 ), 1, 8 );
			insertToQueue(temp_depth,Rect(xMin, yMin, width, height),3);  //向缓冲区存储当前点
			if(!roi_frame.empty())
			{
				roi_frame.release();
			}
			roi_frame=kinectImage.at(0).image(Rect(xMin,yMin,width,height));
			init_down=true;	
			
		}

		if ((g_rectCount!=0)&&init_down)
		{
			WaitForSingleObject(mutex,INFINITE);
			new_image = kinectImage.at(2).image;
			temp_depth = kinectImage.at(2).depth;
			ReleaseMutex(mutex);

			result = tracker_p3.update(kinectImage.at(2).image);

			if ((result.x+result.width>kinectImage.at(0).image.cols) || (result.y+result.height>kinectImage.at(0).image.rows)
				||(result.x<0)||(result.y<0))
			{
				AfxMessageBox(_T("运动超出边界,请重开程序"));
				cvDestroyAllWindows();
				exit(-1);
			}
			
			//rectangle( kinectImage.at(0).image, Point( result.x, result.y ), Point( result.x+result.width, result.y+result.height), Scalar( 0, 255, 0 ), 1, 8 );

			WaitForSingleObject(mutex,INFINITE);
			kinectImage.at(2).select = result;
			ReleaseMutex(mutex);
			
			if(!roi_frame.empty())
			{
				roi_frame.release();
			}
			roi_frame=kinectImage.at(2).image(Rect(result.x, result.y,result.width,result.height));

			//保存当前运动点
			if (count_f==3) //record pos per 5 frames
			{
				insertToQueue(temp_depth,result,3);
				count_f=0;
			}

			/*if (tracker_3.size()>=200)
			{
				break;
			}*/
			
			count_f++;
		}


	}


	return 0;

}





void CkinectDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	// initialize context
	XnStatus isError = XN_STATUS_OK;
	xn::Context context;
	
	isError = context.Init();
	context.SetGlobalMirror(true);//镜像模式
	// create depth generator
	xn::DepthGenerator depthGen;
	isError = depthGen.Create(context);

	// create image generator
	xn::ImageGenerator imageGen;
	isError = imageGen.Create(context);

	// set map output mode
	XnMapOutputMode mode;
	mode.nXRes = 640;
	mode.nYRes = 480;
	mode.nFPS = 30;
	isError = depthGen.SetMapOutputMode(mode);
	isError = imageGen.SetMapOutputMode(mode);

	// correct viewpoint
	depthGen.GetAlternativeViewPointCap().SetViewPoint(imageGen);

	// start generating image and depth data
	isError = context.StartGeneratingAll();

	// define and initialize a CvKinectImage struct
	

	IplImage* kinectImage_depth = cvCreateImage(cvSize(640,480),IPL_DEPTH_16S,1);
	IplImage* kinectImage_image = cvCreateImage(cvSize(640,480),IPL_DEPTH_8U,3);
	IplImage* kinectImage_depthShow = cvCreateImage(cvSize(640,480),IPL_DEPTH_8U,1);
	IplImage* kinectImage_imageShow = cvCreateImage(cvSize(640,480),IPL_DEPTH_8U,3);
	cvNamedWindow("depth",CV_WINDOW_AUTOSIZE);
	cvNamedWindow("image",CV_WINDOW_AUTOSIZE);
	cvNamedWindow("test",CV_WINDOW_AUTOSIZE);
	// register the callback function //three points
	cvSetMouseCallback("image",onMouse,&kinectImage);
	
	
	// define meta data
	xn::DepthMetaData depthMD;
	xn::ImageMetaData imageMD;

	


	// Create KCFTracker object
	

	// Tracker results
	
	const XnDepthPixel*  pDepthMap;
	const XnRGB24Pixel*  pImageMap;
	HANDLE hThread1;
	HANDLE hThread2;
	HANDLE hThread3;
	HANDLE savePoint_hthread;
	if (context.WaitAndUpdateAll() == XN_STATUS_OK )
	{
		// get meta data
		depthGen.GetMetaData(depthMD);
		imageGen.GetMetaData(imageMD);

		// copy meta data to the related struct members
		memcpy(kinectImage_depth->imageData,depthMD.Data(),640*480*2);
		memcpy(kinectImage_image->imageData,imageMD.Data(),640*480*3);


		// convert scale and color format
		cvConvertScale(kinectImage_depth,kinectImage_depthShow,255.0/4096.0); 
		cvCvtColor(kinectImage_image,kinectImage_imageShow,CV_RGB2BGR);
		
		kinectImage.at(0).image=cvarrToMat(kinectImage_imageShow);
		//flip(cvarrToMat(kinectImage_imageShow),kinectImage.at(0).image,1);
		kinectImage.at(0).depth=cvarrToMat(kinectImage_depthShow);
		//flip(cvarrToMat(kinectImage_depthShow),kinectImage.at(0).depth,1);
		//kinectImage.at(0).depth_origin = kinectImage_depth;
		//kinectImage.at(1).image=kinectImage.at(0).image;
		//kinectImage.at(1).depth=kinectImage.at(0).depth;
		kinectImage.at(1).image=cvarrToMat(kinectImage_imageShow);
		kinectImage.at(1).depth=cvarrToMat(kinectImage_depthShow);
		//kinectImage.at(1).depth_origin = kinectImage_depth;
		//kinectImage.at(2).image=kinectImage.at(0).image;
		//kinectImage.at(2).depth=kinectImage.at(0).depth;
		kinectImage.at(2).image=cvarrToMat(kinectImage_imageShow);
		kinectImage.at(2).depth=cvarrToMat(kinectImage_depthShow);
		//kinectImage.at(2).depth_origin  = kinectImage_depth;
		stop_signal = false; //允许线程
		hThread1 = CreateThread(NULL,0,kcf1proc,NULL,0,NULL);  
		hThread2 = CreateThread(NULL,0,kcf2proc,NULL,0,NULL);
		hThread3 = CreateThread(NULL,0,kcf3proc,NULL,0,NULL);
		savePoint_hthread = CreateThread(NULL,0,savePoint,NULL,0,NULL);
	}

	// record FPS
	
	while(context.WaitAndUpdateAll() == XN_STATUS_OK)
	{
		
		

		// get meta data
		depthGen.GetMetaData(depthMD);
		imageGen.GetMetaData(imageMD);

		// copy meta data to the related struct members
		memcpy(kinectImage_depth->imageData,depthMD.Data(),640*480*2);
		memcpy(kinectImage_image->imageData,imageMD.Data(),640*480*3);
		

		// convert scale and color format
		cvConvertScale(kinectImage_depth,kinectImage_depthShow,255.0/4096.0);
		cvCvtColor(kinectImage_image,kinectImage_imageShow,CV_RGB2BGR);

		WaitForSingleObject(mutex,INFINITE);
		kinectImage.at(0).image=cvarrToMat(kinectImage_imageShow);
		kinectImage.at(0).depth=cvarrToMat(kinectImage_depthShow);
		rectangle(kinectImage.at(0).image,kinectImage.at(0).select,Scalar(0,255,0),1,8);
		//flip(cvarrToMat(kinectImage_imageShow),kinectImage.at(0).image,1);
		//kinectImage.at(0).depth=cvarrToMat(kinectImage_depthShow);
		//flip(cvarrToMat(kinectImage_depthShow),kinectImage.at(0).depth,1);

		kinectImage.at(1).image=cvarrToMat(kinectImage_imageShow);
		kinectImage.at(1).depth=cvarrToMat(kinectImage_depthShow);
		rectangle(kinectImage.at(0).image,kinectImage.at(1).select,Scalar(0,255,0),1,8);
		//kinectImage.at(1).image=kinectImage.at(0).image;
		//kinectImage.at(1).depth=kinectImage.at(0).depth;
		kinectImage.at(2).image=cvarrToMat(kinectImage_imageShow);
		kinectImage.at(2).depth=cvarrToMat(kinectImage_depthShow);
		rectangle(kinectImage.at(0).image,kinectImage.at(2).select,Scalar(0,255,0),1,8);
		//kinectImage.at(2).image=kinectImage.at(0).image;
		//kinectImage.at(2).depth=kinectImage.at(0).depth;
		ReleaseMutex(mutex);
		// get depth data and put it to the image
		/*kinectImage.pointData = CV_IMAGE_ELEM(kinectImage_depth,unsigned short,kinectImage.point.y,kinectImage.point.x);
		sprintf(dataString,"%d %d %d",kinectImage.point.x,kinectImage.point.y,kinectImage.pointData);
		putText(kinectImage.depth,dataString,kinectImage.point,cv::FONT_HERSHEY_COMPLEX,1,CV_RGB(255,255,255));*/

			
			
			
			//myShiTomasi_function(roi_frame,kinectImage.image,xMin,yMin);
			//imshow("roi",roi_frame);
			//cvWaitKey(0);
	    pDepthMap = depthGen.GetDepthMap();
	    pImageMap = imageGen.GetRGB24ImageMap();

		

		// show image
		
		

		imshow("depth",kinectImage.at(0).depth);
		imshow("image",kinectImage.at(0).image);
		/*if ((tracker_1.size()>=200)&&(tracker_2.size()>=200)&&(tracker_3.size()>=200))
		{
			break;
		}*/
		if(cvWaitKey(10) == 27) break;
	}

	

	// 9a. get the depth map
	//const XnDepthPixel*  pDepthMap = depthGen.GetDepthMap();
 
	// 9b. get the image map
	//const XnRGB24Pixel*  pImageMap = imageGen.GetRGB24ImageMap();
 
	// 10 generate point cloud
	//vPointCloud.clear();
	//GeneratePointCloud( depthGen, pDepthMap, pImageMap, vPointCloud );
	
	// set memory free
	cvDestroyWindow("depth");
	while (1)
		if(cvWaitKey(10) == 27) break;
	cvDestroyWindow("image");

	while (1)
		if(cvWaitKey(10) == 27) break;
	cvDestroyWindow("test");


	stop_signal=true;

	/*if (!tracker_1.empty())
	{
		int saveRes = savePoint(tracker_1.size());
		if (saveRes != 0)
		{
			exit(-1);
		}
	}*/
	
	
	context.Shutdown();
	

	//重置全局变量

    select_flag=false;
	Get_rect=false;
	g_rectCount=0; //记录画框数量
	tracker_1 = queue<MyPoint>();
	tracker_2 = queue<MyPoint>();
	tracker_3 = queue<MyPoint>();
	kinectImage.clear();
	kinectImage = vector<CvKinectImage>(3);

}



void CkinectDlg::insertToQueue(Mat temp_depth, Rect target,int port)
{
	
	float data[3];
	data[0]=target.x+(float(target.width)/2);
	data[1]=target.y+(float(target.height)/2);
	Mat realScaleDepth;
	//data[2]=kinectImage.at(0).depth.at<uchar>((int)data[1],(int)data[0]); //全局变量 取当前深度图中的Z值 效率慢
	


	uchar* row_points = temp_depth.ptr<uchar>((int)data[1]);
	data[2] = row_points[(int)data[0]];

	data[2] = data[2]*4096.0/255.0; //回复真实深度

	//data[2]=(cvGet2D(kinectImage.at(0).depth_origin,30,30)).val[0];
	MyPoint dataPoint(data[0],data[1],data[2]);
	switch(port)
	{
	case 1:
		{

			if (tracker_1.size()<num_of_savepoint)
			{
				tracker_1.push(dataPoint);
			}
			
			break;
		}
	case 2:
		{
			if (tracker_2.size()<num_of_savepoint)
			{
				tracker_2.push(dataPoint);
			}
			
			break;
		}
	case 3:
		{
			if (tracker_3.size()<num_of_savepoint)
			{
				tracker_3.push(dataPoint);
			}
			
			break;
		}
	
	}

}

DWORD WINAPI CkinectDlg::savePoint(LPVOID lpParameter)
{
	queue<MyPoint> temp_1;
	queue<MyPoint> temp_2;
	queue<MyPoint> temp_3;
	MyPoint t1,t2,t3;
	while (1)
	{
		Sleep(1000);
		if (stop_signal==true)
		{
			break;
		} 
		else if((tracker_1.size()==num_of_savepoint)&&(tracker_2.size()==num_of_savepoint)&&(tracker_3.size()==num_of_savepoint))
		{
			try
			{
				WaitForSingleObject(mutex,INFINITE); //互斥后对象变成了const对象？
				temp_1=tracker_1;
				temp_2=tracker_2;
				temp_3=tracker_3;
				ReleaseMutex(mutex);
				FILE *fp = fopen("pointData.txt","w");
				if (!fp)
				{
					AfxMessageBox(_T("文件名错误"));
					cvDestroyAllWindows();
					exit(-1);
				}
				for (int i=0;i<num_of_savepoint;i++)
				{
					t1 = temp_1.front(); //先取元素再弹出数据,pop不是用来取数据的
					temp_1.pop();
					fprintf(fp,"%f %f %f\n",t1.x,t1.y,t1.z);
					t2 = temp_2.front();
					temp_2.pop();
					fprintf(fp,"%f %f %f\n",t2.x,t2.y,t2.z);
					t3 = temp_3.front(); 
					temp_3.pop();
					fprintf(fp,"%f %f %f\n",t3.x,t3.y,t3.z);
				}

				fclose(fp);
				tracker_1=queue<MyPoint>();
				tracker_2=queue<MyPoint>();
				tracker_3=queue<MyPoint>();
				//*flag_Captured = *flag_Captured + 1; //数据保存完毕
				AfxMessageBox(_T("动作捕获完成"));
				
			}
			catch (CException* e)
			{
				AfxMessageBox(_T("运动保存失败"));
				exit(-1);
			}
			
		}
		
		
	}
	
	return 0;
}

void CkinectDlg::OnBnClickedButton2() //计算法向量的代码
{
	// TODO: 在此添加控件通知处理程序代码
	
	ifstream in("pointData.txt");

	if (!in)
	{
		AfxMessageBox(_T("不存在点文件"));
		exit(-1);
	}

	queue<MyPoint> pt1;
	queue<MyPoint> pt2;
	queue<MyPoint> pt3;

	MyPoint temp_pt;
	string line;

	while (in.peek()!=EOF) //peek是判断下一个字符，但却不移动文件指针
	{
		getline(in,line);
		sscanf(line.c_str(),"%f%f%f",&temp_pt.x,&temp_pt.y,&temp_pt.z); //如果是逗号，可以用%[^,]表示取非,的字符
		pt1.push(temp_pt);

		getline(in,line);
		sscanf(line.c_str(),"%f%f%f",&temp_pt.x,&temp_pt.y,&temp_pt.z); //如果是逗号，可以用%[^,]表示取非,的字符
		pt2.push(temp_pt);

		getline(in,line);
		sscanf(line.c_str(),"%f%f%f",&temp_pt.x,&temp_pt.y,&temp_pt.z); //如果是逗号，可以用%[^,]表示取非,的字符
		pt3.push(temp_pt);

	}
	
	//计算每三个点的法向量，留意顺序




}
